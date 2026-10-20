/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only ownership admission experiment, not production authorization.
 * EP147 parser connection +0x20 -> transport; transport +8 -> socket object;
 * socket object +0 holds the fd used by native recv. Verify the reverse TCP
 * tuple against live mod-owned outbound sockets, not the peer's claimed ID.
 * Bounded message tickets are consumed at dispatch or native destruction.
 * Socket pins/generations are rechecked at dispatch. Source generations and
 * native cancellation ordering still need further tests.
 */
static fn8 probe_parse_original;
static fn8 probe_message_destroy_original;
static fn8 probe_handshake_original;
static pthread_mutex_t probe_ticket_mutex=PTHREAD_MUTEX_INITIALIZER;
struct probe_ticket { uint64_t message; struct link_socket_proof proof; };
static struct probe_ticket probe_tickets[256];
static __thread struct probe_ticket probe_dispatch_ticket;
static struct probe_ticket probe_ticket_take(uint64_t message) {
    struct probe_ticket ticket={.proof={.server=-1,.server_pin=-1}};
    if (!message) return ticket;
    pthread_mutex_lock(&probe_ticket_mutex);
    for (unsigned i=0;i<256;i++) if (probe_tickets[i].message==message) {
        ticket=probe_tickets[i];
        probe_tickets[i]=(struct probe_ticket){0};
        break;
    }
    pthread_mutex_unlock(&probe_ticket_mutex);
    return ticket;
}
static void probe_ticket_discard(uint64_t message) {
    struct probe_ticket ticket=probe_ticket_take(message);
    link_socket_release(&ticket.proof);
}
static void probe_ticket_put(uint64_t message,struct link_socket_proof proof) {
    pthread_mutex_lock(&probe_ticket_mutex);
    for (unsigned i=0;i<256;i++) if (!probe_tickets[i].message) {
        probe_tickets[i]=(struct probe_ticket){message,proof};
        pthread_mutex_unlock(&probe_ticket_mutex);
        return;
    }
    pthread_mutex_unlock(&probe_ticket_mutex);
    link_socket_release(&proof);
    lf_log("ownership probe: ticket capacity exhausted; private request will be denied");
}
static int probe_message_allowed(uint64_t message,int player) {
    return probe_dispatch_ticket.message==message && probe_dispatch_ticket.proof.player==player &&
        link_socket_valid(&probe_sockets,&probe_dispatch_ticket.proof);
}
static uint64_t probe_dispatch(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    struct probe_ticket previous=probe_dispatch_ticket;
    uint64_t message=*(uint64_t *)a;
    probe_dispatch_ticket=probe_ticket_take(message);
    uint64_t result=o_dispense(a,b,c,d,e,f,g,h);
    link_socket_release(&probe_dispatch_ticket.proof);
    probe_dispatch_ticket=previous;
    return result;
}
static uint64_t probe_message_destroy(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    probe_ticket_discard(a);
    return probe_message_destroy_original(a,b,c,d,e,f,g,h);
}
/* Exact EP147 detector calls this handshake before its connected listener.
 * A negative result takes the native destruction path, before registration can
 * replace an existing communicator with the same requester number. The setup
 * reply has already been sent; rejection is observed as a subsequent close.
 * Ownership comes from the pinned socket registry, not diagnostic traced[].
 */
static uint64_t probe_handshake(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t result=probe_handshake_original(a,b,c,d,e,f,g,h);
    if (!lf_runtime_ready() || (int32_t)result<0 || !b) return result;
    unsigned requester=*(unsigned *)(b+8);
    if (!probe_requester_reserved(requester)) return result;
    uintptr_t transport=*(uintptr_t *)(b+0x20);
    uintptr_t object=transport ? *(uintptr_t *)(transport+8):0;
    int fd=object ? *(int *)object:-1;
    struct link_socket_proof proof=link_socket_find(&probe_sockets,fd);
    int owned=link_socket_valid(&probe_sockets,&proof) &&
        requester==probe_requester_for_player(proof.player);
    link_socket_release(&proof);
    lf_log("ownership probe: handshake requester=%u server_fd=%d owned=%d",requester,fd,owned);
    return owned ? result:UINT32_MAX;
}
static uint64_t probe_parse(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t message=probe_parse_original(a,b,c,d,e,f,g,h);
    static unsigned notes;
    if (lf_runtime_ready() && message) {
        uint64_t transport=*(uint64_t *)(a+0x20);
        uint64_t socket_object=transport ? *(uint64_t *)(transport+8) : 0;
        int fd=socket_object ? *(int *)socket_object : -1;
        struct link_socket_proof proof=link_socket_find(&probe_sockets,fd);
        int player=proof.player;
        probe_ticket_discard(message);
        uint32_t dmst=((uint32_t (*)(uint64_t,int))0xf05680)(message,0);
        unsigned slot=(dmst>>8)&255;
        if (slot>=0xe0 && slot<0xe0+MAX_FAKES) {
            if (player) {
                probe_ticket_put(message,proof);
                proof=(struct link_socket_proof){.server=-1,.server_pin=-1};
            }
            else lf_log("ownership probe: unowned private request command=%#x slot=%#x connection=%u",
                        *(unsigned short *)message,slot,*(unsigned *)(a+8));
        }
        link_socket_release(&proof);
        if (__atomic_fetch_add(&notes,1,__ATOMIC_RELAXED)<40) lf_log("ownership probe: message=%#lx connection=%u server_fd=%d player=%d command=%#x",
               message,*(unsigned *)(a+8),fd,player,*(unsigned short *)message);
    }
    return message;
}
static void probe_library_ownership_install(void) {
    probe_parse_original=make_hook(0xef05c8,probe_parse);
    probe_message_destroy_original=make_hook(0xf00de0,probe_message_destroy);
    probe_handshake_original=make_hook(0xeee540,probe_handshake);
}
