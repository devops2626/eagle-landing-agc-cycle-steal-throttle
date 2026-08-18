/**
 * main.c - Demo driver for the AGC cycle-steal throttle skeleton
 *
 * Simulates a short powered-descent segment with heavy Rendezvous Radar
 * cycle-steal traffic, demonstrating that the throttle can keep the
 * executive from raising 1201/1202 alarms.
 */

#include "agc.h"
#include <stdio.h>
#include <stdlib.h>

static void simulate_radar_burst(agc_state_t *agc, int count, uint8_t prio)
{
    cycle_steal_req_t req = {
        .source   = STEAL_RENDEZVOUS_RADAR,
        .priority = prio,
        .address  = 0x0100,   /* pretend angle counter */
        .delta    = 1
    };

    for (int i = 0; i < count; i++) {
        agc_request_cycle_steal(agc, &req);
        /* Interleave a few normal instruction cycles */
        if ((i & 3) == 0)
            agc_step(agc);
    }
}

int main(void)
{
    agc_state_t agc;
    agc_init(&agc);

    printf("Eagle Landing AGC Cycle-Steal Throttle – Core Loop Skeleton\n");
    printf("===========================================================\n\n");

    printf("Initial state:\n");
    agc_dump_status(&agc);
    printf("\n");

    /* Phase 1: quiet descent – mostly normal cycles */
    printf("--- Phase 1: quiet descent (200 cycles) ---\n");
    agc_run(&agc, 200);
    agc_dump_status(&agc);
    printf("\n");

    /* Phase 2: heavy Rendezvous Radar activity (the historical problem)
       Priority 5 is allowed by the default mask (BIT5). */
    printf("--- Phase 2: RR cycle-steal burst (priority 5) ---\n");
    simulate_radar_burst(&agc, 120, 5);
    agc_dump_status(&agc);
    printf("\n");

    /* Phase 3: even heavier + lower priority that should be masked */
    printf("--- Phase 3: low-priority spam (priority 3, should be throttled) ---\n");
    simulate_radar_burst(&agc, 80, 3);
    agc_dump_status(&agc);
    printf("\n");

    /* Phase 4: continue normal guidance cycles */
    printf("--- Phase 4: resume guidance (150 cycles) ---\n");
    agc_run(&agc, 150);
    agc_dump_status(&agc);
    printf("\n");

    if (agc.alarm_1201 || agc.alarm_1202) {
        printf("RESULT: Executive overflow alarm raised.\n");
        return 1;
    }

    printf("RESULT: No 1201/1202 alarms. Throttle held the line.\n");
    printf("\"Houston, Tranquility Base here. The Eagle has landed.\"\n");
    return 0;
}
