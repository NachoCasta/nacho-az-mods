/* fakeplayer: fakecdj2.py ported to C, to run INSIDE the emulator guest.
 *
 * usage: [FP_NAME=<volume name>] [FP_FW=<x.yy>] fakeplayer <my_ip> <deck_ip> <seconds> [player=3]
 *
 * my_ip is a secondary address on the guest's eth0 (ip addr add 10.0.2.50/24 dev eth0),
 * so EP147 sees a remote player without any real network. EP147 needs Pro DJ Link Mode on
 * and lists only players 3 and 4 (it is players 1 and 2 itself).
 * - keep-alive 0x06 every 1.5 s to <deck_ip>:50000 and the broadcast address
 * - CDJ status 0x0a every 200 ms to <deck_ip>:50002, built from the deck's own status
 *   if we manage to receive one (template), else from scratch; firmware at 0x7c = FP_FW
 * - answers the media slot query 0x05 with a media slot response 0x06 named FP_NAME
 * - logs every packet it receives on 50000/50001/50002/50004 (first sighting with hex)
 *
 * Build (static, the deck's Linaro 6.3.1):
 *   aarch64-linux-gnu-gcc -O2 -static -std=gnu11 -Wall -o fakeplayer fakeplayer.c
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static const char MAGIC[] = "Qspt1WmJOL";
static const char NAME[] = "CDJ-3000";
static double t0;

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

#define LOG(...) do { printf("%6.1fs ", now() - t0); printf(__VA_ARGS__); putchar('\n'); fflush(stdout); } while (0)

static void hexdump(const uint8_t *d, int n, int limit) {
    for (int i = 0; i < n && i < limit; i += 16) {
        printf("      %04x:", i);
        for (int j = i; j < i + 16 && j < n; j++) printf(" %02x", d[j]);
        putchar('\n');
    }
    fflush(stdout);
}

static int udp_sock(const char *ip, int port) {
    int s = socket(AF_INET, SOCK_DGRAM, 0), one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof one);
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof one);
    struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(port) };
    inet_pton(AF_INET, ip, &a.sin_addr);
    if (bind(s, (void *)&a, sizeof a) < 0) {
        LOG("bind %s:%d: %s", ip, port, strerror(errno));
        close(s);
        return -1;
    }
    return s;
}

static void send_to(int s, const char *ip, int port, const void *p, int n) {
    struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(port) };
    inet_pton(AF_INET, ip, &a.sin_addr);
    if (sendto(s, p, n, 0, (void *)&a, sizeof a) < 0)
        LOG("sendto %s:%d: %s", ip, port, strerror(errno));
}

/* Media slot response 0x06 for slot sr (dysentery media page): D at 0x24..0x27, S at 0x28..0x2b. */
static void send_media(int tx, const char *deck_ip, int player, const struct in_addr *me, int sr) {
    uint8_t r[0x24 + 0x9c] = { 0 };
    (void)me;
    memcpy(r, MAGIC, 10);
    r[0x0a] = 0x06;
    memcpy(r + 0x0b, NAME, strlen(NAME));
    r[0x1f] = 0x01; r[0x21] = player; r[0x22] = 0x00; r[0x23] = 0x9c;
    r[0x27] = player;
    r[0x2b] = sr;
    const char *base = getenv("FP_NAME") ? getenv("FP_NAME") : "HUB USB", *dt = "2026-10-16";
    char vn[40];
    if (sr == 3) snprintf(vn, sizeof vn, "%s", base);
    else snprintf(vn, sizeof vn, "%s S%d", base, sr); /* one name per slot */
    for (int j = 0; vn[j]; j++) r[0x2c + 2 * j + 1] = vn[j];
    for (int j = 0; dt[j]; j++) r[0x6c + 2 * j + 1] = dt[j];
    r[0xa6] = 0; r[0xa7] = 100;   /* tracks */
    r[0xaa] = 0x01;               /* rekordbox */
    r[0xae] = 0; r[0xaf] = 5;     /* playlists */
    uint64_t tot = 128ULL * 1000000000ULL, fr = 64ULL * 1000000000ULL;
    for (int j = 0; j < 8; j++) { r[0xb0 + j] = tot >> (56 - 8 * j); r[0xb8 + j] = fr >> (56 - 8 * j); }
    send_to(tx, deck_ip, 50002, r, sizeof r);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s <my_ip> <deck_ip> <seconds> [player]\n", argv[0]);
        return 2;
    }
    const char *my_ip = argv[1], *deck_ip = argv[2];
    double secs = atof(argv[3]);
    int player = argc > 4 ? atoi(argv[4]) : 3;
    char bcast[32];
    snprintf(bcast, sizeof bcast, "%s", my_ip);
    strcpy(strrchr(bcast, '.'), ".255");
    t0 = now();

    uint8_t mac[6] = { 0x02, 0x0a, 0x2d, 0x00, 0x00, (uint8_t)player };
    struct in_addr me;
    inet_pton(AF_INET, my_ip, &me);

    uint8_t ka[54] = { 0 };
    memcpy(ka, MAGIC, 10);
    ka[0x0a] = 0x06;
    memcpy(ka + 0x0c, NAME, strlen(NAME));
    ka[0x20] = 0x01; ka[0x21] = 0x03; ka[0x22] = 0x00; ka[0x23] = 0x36;
    ka[0x24] = player;
    ka[0x25] = 0x01;
    memcpy(ka + 0x26, mac, 6);
    memcpy(ka + 0x2c, &me, 4);
    ka[0x30] = 0x01;
    ka[0x34] = 0x01;

    /* From-scratch CDJ-3000 status (dysentery vcdj layout), replaced by a deck template. */
    uint8_t st[2048] = { 0 };
    int st_len = 0x200, have_template = 0;
    memcpy(st, MAGIC, 10);
    st[0x0a] = 0x0a;
    memcpy(st + 0x0b, NAME, strlen(NAME));
    st[0x1f] = 0x01; st[0x20] = 0x06;
    st[0x22] = (0x200 - 0x24) >> 8; st[0x23] = (0x200 - 0x24) & 0xff;
    memcpy(st + 0x7c, "3.30", 4);
#define PATCH_STATUS() do { \
        memcpy(st + 0x0b, NAME, strlen(NAME)); memset(st + 0x0b + strlen(NAME), 0, 0x14 - strlen(NAME)); \
        st[0x21] = player; st[0x24] = player; st[0x27] = 0x00; memset(st + 0x28, 0, 3); \
        st[0x6a] = 0x04; st[0x6f] = 0x00; st[0x73] = getenv("FP_SD") ? 0x00 : 0x04; st[0x75] = 0x01; \
        st[0xb7] = getenv("FP_MP") ? strtol(getenv("FP_MP"), NULL, 0) : 0x01; \
        memcpy(st + 0x7c, getenv("FP_FW") ? getenv("FP_FW") : "3.30", 4); \
    } while (0)
    PATCH_STATUS();

    int tx = udp_sock(my_ip, 0);
    int rx[4] = { udp_sock(my_ip, 50000), udp_sock(my_ip, 50001), udp_sock(my_ip, 50002), udp_sock(my_ip, 50004) };
    const int rxport[4] = { 50000, 50001, 50002, 50004 };
    if (tx < 0) return 1;

    struct { int port, kind, len, n; } seen[64];
    int nseen = 0;
    double next_ka = 0, next_st = 0, end = t0 + secs;
    while (now() < end) {
        double t = now();
        if (t >= next_ka) {
            send_to(tx, deck_ip, 50000, ka, sizeof ka);
            send_to(tx, bcast, 50000, ka, sizeof ka);
            next_ka = t + 1.5;
        }
        if (t >= next_st) {
            /* FP_SD_AT=<s>: report the SD card inserted only after <s> seconds (a change event) */
            if (getenv("FP_SD_AT")) st[0x73] = (t - t0 >= atof(getenv("FP_SD_AT"))) ? 0x00 : 0x04;
            send_to(tx, deck_ip, 50002, st, st_len);
            next_st = t + 0.2;
        }
        fd_set fs;
        FD_ZERO(&fs);
        int mx = -1;
        for (int i = 0; i < 4; i++)
            if (rx[i] >= 0) { FD_SET(rx[i], &fs); if (rx[i] > mx) mx = rx[i]; }
        struct timeval tv = { 0, 50000 };
        if (mx < 0) { select(0, NULL, NULL, NULL, &tv); continue; }
        if (select(mx + 1, &fs, NULL, NULL, &tv) <= 0) continue;
        for (int i = 0; i < 4; i++) {
            if (rx[i] < 0 || !FD_ISSET(rx[i], &fs)) continue;
            uint8_t d[4096];
            struct sockaddr_in src;
            socklen_t sl = sizeof src;
            int n = recvfrom(rx[i], d, sizeof d, 0, (void *)&src, &sl);
            if (n <= 0) continue;
            char sip[32];
            inet_ntop(AF_INET, &src.sin_addr, sip, sizeof sip);
            if (strcmp(sip, deck_ip)) continue;
            int kind = (n > 10 && !memcmp(d, MAGIC, 10)) ? d[10] : -1, k;
            for (k = 0; k < nseen; k++)
                if (seen[k].port == rxport[i] && seen[k].kind == kind && seen[k].len == n) break;
            if (k == nseen && nseen < 64) seen[nseen++] = (typeof(seen[0])){ rxport[i], kind, n, 0 };
            if (k < 64) seen[k].n++;
            if (kind == 0x0a && !have_template && n <= (int)sizeof st && d[0x21] != player) {
                memcpy(st, d, n);
                st_len = n;
                have_template = 1;
                PATCH_STATUS();
                LOG("got deck status template (player %d, %d B); Ul=%#x L=%#x Mp=%#x",
                    d[0x21], n, d[0x6f], d[0x75], d[0xb7]);
            }
            if (rxport[i] == 50002 && kind == 0x05 && n >= 0x30) {
                int sr = d[n - 1];
                send_media(tx, deck_ip, player, &me, sr);
                LOG("media query (Dr=%d Sr=%d) -> answered", d[n - 5], sr);
                /* FP_SLOTS="2,7,8": also push unsolicited responses for these slots */
                const char *ex = getenv("FP_SLOTS");
                for (; ex && *ex; ex = strchr(ex, ',') ? strchr(ex, ',') + 1 : "")
                    if (atoi(ex) != sr) send_media(tx, deck_ip, player, &me, atoi(ex));
            }
            if (k < 64 && seen[k].n == 1) {
                LOG("UDP :%d type=%#x len=%d from :%d", rxport[i], kind, n, ntohs(src.sin_port));
                hexdump(d, n, 0x80);
            }
        }
    }
    LOG("summary:");
    for (int k = 0; k < nseen; k++)
        printf("  :%d type=%#x len=%d x%d\n", seen[k].port, seen[k].kind, seen[k].len, seen[k].n);
    return 0;
}
