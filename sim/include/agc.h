/**
 * agc.h - Minimal AGC core definitions for cycle-steal throttle simulation
 *
 * Inspired by the Apollo Guidance Computer (Luminary series).
 * Strict C11, static allocation only, deterministic.
 */

#ifndef AGC_H
#define AGC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Word size & memory model (simplified)                             */
/* ------------------------------------------------------------------ */

/* AGC used 15-bit words + 1 parity bit. We model the 15-bit data. */
typedef uint16_t agc_word_t;          /* lower 15 bits significant */

#define AGC_WORD_MASK     0x7FFF      /* 15 bits */
#define AGC_CORE_SIZE     2048        /* simplified erasable + fixed */
#define AGC_FIXED_BASE    1024        /* fixed memory starts here */

/* ------------------------------------------------------------------ */
/*  Registers (subset of real AGC)                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    agc_word_t A;     /* Accumulator */
    agc_word_t L;     /* Lower accumulator / Q for multiply */
    agc_word_t Q;     /* Return address / multiply high */
    agc_word_t Z;     /* Program counter (next instruction) */
    agc_word_t BB;    /* Bank register (simplified) */
    agc_word_t FB;    /* Fixed bank */
    agc_word_t EB;    /* Erasable bank */

    /* Interrupt / executive state */
    bool       interrupt_enabled;
    uint8_t    current_priority;   /* 0 = highest in this model */
    uint32_t   cycle_count;        /* total instruction cycles executed */
    uint32_t   steal_count;        /* total cycle steals accepted */
    uint32_t   steal_dropped;      /* cycle steals rejected by throttle */
} agc_regs_t;

/* ------------------------------------------------------------------ */
/*  Core memory                                                       */
/* ------------------------------------------------------------------ */

typedef struct {
    agc_word_t mem[AGC_CORE_SIZE];
} agc_memory_t;

/* ------------------------------------------------------------------ */
/*  Cycle-steal request (from radar peripherals)                      */
/* ------------------------------------------------------------------ */

typedef enum {
    STEAL_NONE = 0,
    STEAL_RENDEZVOUS_RADAR,   /* high rate, historically problematic */
    STEAL_LANDING_RADAR,      /* needed for descent guidance */
    STEAL_OTHER
} steal_source_t;

typedef struct {
    steal_source_t source;
    uint8_t        priority;  /* 0 = highest priority, higher number = lower */
    uint16_t       address;   /* memory location being updated (counter) */
    int16_t        delta;     /* +1 or -1 typically for angle counters */
} cycle_steal_req_t;

/* ------------------------------------------------------------------ */
/*  Throttle state (token-bucket + priority mask)                     */
/* ------------------------------------------------------------------ */

typedef struct {
    /* Token bucket for rate limiting cycle steals */
    uint32_t tokens;          /* current tokens available */
    uint32_t max_tokens;      /* bucket capacity */
    uint32_t refill_rate;     /* tokens added per main-loop iteration */
    uint32_t last_refill;     /* cycle_count at last refill */

    /* Priority mask: bits that are allowed to steal when CPU is hot */
    uint8_t  priority_mask;   /* BIT5 style mask – bit N set = priority N allowed */

    /* Saturation thresholds (percentage of recent cycles that were steals) */
    uint8_t  soft_limit;      /* start throttling */
    uint8_t  hard_limit;      /* drop almost everything except highest prio */

    /* Statistics window */
    uint32_t window_cycles;
    uint32_t window_steals;
} throttle_t;

/* ------------------------------------------------------------------ */
/*  Full AGC simulation state                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    agc_regs_t    regs;
    agc_memory_t  memory;
    throttle_t    throttle;

    /* Simple alarm flags */
    bool alarm_1201;  /* no vacant areas / core sets */
    bool alarm_1202;  /* no core sets (executive overflow) */

    /* Running flag */
    bool running;
} agc_state_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

/** Initialize the entire AGC state (static, no malloc). */
void agc_init(agc_state_t *agc);

/** Single instruction cycle: fetch → decode → execute + possible steal. */
void agc_step(agc_state_t *agc);

/** Request a cycle steal from a peripheral (radar etc.).
 *  Returns true if the steal was accepted, false if throttled/dropped. */
bool agc_request_cycle_steal(agc_state_t *agc, const cycle_steal_req_t *req);

/** Run N cycles (or until halted / alarm). */
void agc_run(agc_state_t *agc, uint32_t max_cycles);

/** Simple diagnostic dump to stdout. */
void agc_dump_status(const agc_state_t *agc);

#ifdef __cplusplus
}
#endif

#endif /* AGC_H */
