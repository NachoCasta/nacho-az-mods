/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit standalone modes; no implicit activation or deployment. */
#define _GNU_SOURCE
#include "public-dhcp-child.h"
#include "public-broker-probe-child.h"
#ifdef LF_PUBLIC_BROKER_SERVICE_EMULATOR
#include "public-broker-service-emulator.h"
#endif
#if defined(LF_PUBLIC_BROKER_ALLOCATE_EMULATOR) || defined(LF_PUBLIC_BROKER_CANDIDATE) || defined(LF_PUBLIC_BROKER_CANDIDATE_EMULATOR)
#include "public-broker-allocate-emulator.h"
#endif
int main(int argc,char **argv) {
#ifdef LF_PUBLIC_BROKER_CANDIDATE_EMULATOR
    if (argc>1 && !strcmp(argv[1],"--broker-candidate-emulator")) return hub_public_broker_candidate_emulator(argc,argv);
#endif
#ifdef LF_PUBLIC_BROKER_CANDIDATE
    if (argc>1 && !strcmp(argv[1],"--broker-share")) return hub_public_broker_candidate(argc,argv);
#endif
#ifdef LF_PUBLIC_BROKER_ALLOCATE_EMULATOR
    if (argc>1 && !strcmp(argv[1],"--broker-allocate-emulator")) return hub_public_broker_allocate_emulator(argc,argv);
#endif
#ifdef LF_PUBLIC_BROKER_SERVICE_EMULATOR
    if (argc>1 && !strcmp(argv[1],"--broker-service-emulator")) return hub_public_broker_service_emulator(argc,argv);
#endif
    if (argc>1 && !strcmp(argv[1],"--broker-probe")) return hub_public_broker_probe_child(argc,argv);
    return hub_public_dhcp_child(argc,argv);
}
