/* SPDX-License-Identifier: GPL-3.0-only */
/* Research-only observation at the normal (non-sret) caller of DB lookup.
 * The lookup itself returns via x8; intercepting it as fn8 would corrupt ABI. */
static fn8 o_db_cache_query;
static uint64_t h_db_cache_query(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    char name[32]={0};
    pthread_getname_np(pthread_self(),name,sizeof name);
    if (!strncmp(name,"HubLib",6) && a) {
        uint64_t provider=*(uint64_t *)(a+0x10);
        if (getenv("LINKFAKE_PRIVATE_TAG_CACHE")) provider=hub_tag_cache_this(provider,99,(uint64_t)__builtin_return_address(0));
        uint16_t wanted=((uint16_t (*)(uint64_t))0x186af10)(a+8);
        uint16_t ids[32]; unsigned count=0; int found=0;
        if (provider) {
            ((void (*)(uint64_t))0x24a3970)(provider+8);
            uint64_t begin=*(uint64_t *)(provider+0xa0),end=*(uint64_t *)(provider+0xa8);
            for (uint64_t item=begin;item && item<end && count<32;item+=16) {
                uint64_t db=*(uint64_t *)item;
                if (!db) continue;
                ((void (*)(uint64_t))0x24a3970)(db);
                uint16_t id=*(uint16_t *)(db+0x98);
                ((void (*)(uint64_t))0x24a33b0)(db);
                ids[count++]=id;
                if (id==wanted) found=1;
            }
            ((void (*)(uint64_t))0x24a33b0)(provider+8);
        }
        static __thread unsigned samples;
        if (!found || samples++<20) {
            char list[256]; unsigned used=0;
            list[0]=0;
            for (unsigned i=0;i<count && used<sizeof list-8;i++)
                used+=snprintf(list+used,sizeof list-used," %04x",ids[i]);
            lf_log("DB cache query thread=%s caller=%p object=%#lx key=%#lx wanted=%#x item=%u provider=%#lx count=%u found=%d ids=%s",name,__builtin_return_address(0),a,*(uint64_t *)(a+8),wanted,(unsigned)b,provider,count,found,list);
        }
    }
    return o_db_cache_query(a,b,c,d,e,f,g,h);
}
