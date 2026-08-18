/**
 * agc.c - Core instruction loop skeleton + cycle-steal throttle
 *
 * Lightweight AGC emulator focused on the cycle-steal / executive
 * overload problem that produced the Apollo 11 1201/1202 alarms.
 *
 * Strict C11, static allocation, deterministic.
 */

#include "agc.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                  */
/* ------------------------------------------------------------------ */

static inline agc_word_t mask15(agc_word_t w)
{
    return w & AGC_WORD_MASK;
}

/* Very crude "instruction" decode for skeleton purposes.
 * Real AGC had a rich instruction set; here we just advance Z
 * and occasionally touch A to keep the loop alive. */
static void execute_instruction(agc_state_t *agc, agc_word_t instr)
{
    agc_regs_t *r = &agc->regs;

    /* Skeleton: treat every word as a simple "noop + increment A" */
    r->A = mask15(r->A + 1);

    /* Advance program counter (wrap for demo) */
    r->Z = mask15(r->Z + 1);
    if (r->Z >= AGC_CORE_SIZE)
        r->Z = 0;

    (void)instr; /* silence unused for skeleton */
}

/* ------------------------------------------------------------------ */
/*  Throttle logic                                                    */
/* ------------------------------------------------------------------ */

static void throttle_refill(agc_state_t *agc)
{
    throttle_t *t = &agc->throttle;
    uint32_t now = agc->regs.cycle_count;

    if (now - t->last_refill >= 1) {
        uint32_t elapsed = now - t->last_refill;
        uint32_t add = elapsed * t->refill_rate;
        t->tokens += add;
        if (t->tokens > t->max_tokens)
            t->tokens = t->max_tokens;
        t->last_refill = now;
    }
}

/* Decide whether a cycle-steal request is allowed under current load. */
static bool throttle_allow(agc_state_t *agc, const cycle_steal_req_t *req)
{
    throttle_t *t = &agc->throttle;

    throttle_refill(agc);

    /* Priority mask (BIT5 style): if the bit for this priority is clear,
       the request is rejected when we are under pressure. */
    bool mask_ok = (t->priority_mask & (1u << (req->priority & 7))) != 0;

    /* Simple load metric: steals in recent window */
    uint32_t load_pct = 0;
    if (t->window_cycles > 0)
        load_pct = (t->window_steals * 100) / t->window_cycles;

    if (load_pct >= t->hard_limit) {
        /* Hard limit: only highest priority (0) and only if mask allows */
        if (req->priority != 0 || !mask_ok)
            return false;
    } else if (load_pct >= t->soft_limit) {
        /* Soft limit: respect mask and require a token */
        if (!mask_ok || t->tokens == 0)
            return false;
    }

    /* Consume a token if we are rate-limiting */
    if (t->tokens > 0)
        t->tokens--;

    return true;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void agc_init(agc_state_t *agc)
{
    memset(agc, 0, sizeof(*agc));

    /* Registers start at a plausible state */
    agc->regs.Z = 0;
    agc->regs.A = 0;
    agc->regs.interrupt_enabled = true;
    agc->regs.current_priority = 5; /* medium */

    /* Throttle defaults – tuned so that unrestricted RR can still
       push us over the edge, but the mask/token bucket can save us. */
    agc->throttle.max_tokens   = 32;
    agc->throttle.tokens       = 32;
    agc->throttle.refill_rate  = 4;   /* tokens per cycle (aggressive for demo) */
    agc->throttle.last_refill  = 0;
    agc->throttle.priority_mask = 0x21; /* bits 0 and 5 set (BIT5 + highest) */
    agc->throttle.soft_limit   = 25;  /* % */
    agc->throttle.hard_limit   = 40;  /* % */
    agc->throttle.window_cycles = 0;
    agc->throttle.window_steals = 0;

    agc->running = true;
    agc->alarm_1201 = false;
    agc->alarm_1202 = false;

    /* Seed a tiny "program" in memory so the loop has something to fetch */
    for (size_t i = 0; i < 64; i++)
        agc->memory.mem[i] = (agc_word_t)(0x1000 + i); /* dummy opcodes */
}

bool agc_request_cycle_steal(agc_state_t *agc, const cycle_steal_req_t *req)
{
    if (!agc->running || !agc->regs.interrupt_enabled)
        return false;

    if (!throttle_allow(agc, req)) {
        agc->regs.steal_dropped++;
        return false;
    }

    /* Accept the steal: update the target counter and burn a cycle */
    if (req->address < AGC_CORE_SIZE) {
        agc_word_t *loc = &agc->memory.mem[req->address];
        *loc = mask15((agc_word_t)((int16_t)*loc + req->delta));
    }

    agc->regs.steal_count++;
    agc->throttle.window_steals++;

    /* Simulate the cost: a cycle steal consumes an instruction slot */
    agc->regs.cycle_count++;

    /* Crude executive overflow detection for the skeleton.
       In the real AGC this was "no free core sets / VAC areas".
       Here we trip an alarm if the recent steal rate is extreme. */
    if (agc->throttle.window_cycles > 50) {
        uint32_t pct = (agc->throttle.window_steals * 100) /
                       agc->throttle.window_cycles;
        if (pct > 55) {
            agc->alarm_1202 = true;
            /* In a fuller model we would shed tasks; for now just flag */
        }
    }

    return true;
}

void agc_step(agc_state_t *agc)
{
    if (!agc->running)
        return;

    /* Sliding window for load calculation */
    agc->throttle.window_cycles++;
    if (agc->throttle.window_cycles > 200) {
        /* decay the window */
        agc->throttle.window_cycles /= 2;
        agc->throttle.window_steals /= 2;
    }

    /* Fetch */
    agc_word_t instr = 0;
    if (agc->regs.Z < AGC_CORE_SIZE)
        instr = agc->memory.mem[agc->regs.Z];

    /* Decode + Execute */
    execute_instruction(agc, instr);

    agc->regs.cycle_count++;

    /* In a fuller implementation the Executive would schedule jobs here.
       For the skeleton we just keep the lights on. */
}

void agc_run(agc_state_t *agc, uint32_t max_cycles)
{
    uint32_t start = agc->regs.cycle_count;
    while (agc->running &&
           (agc->regs.cycle_count - start) < max_cycles &&
           !agc->alarm_1201 && !agc->alarm_1202) {
        agc_step(agc);
    }
}

void agc_dump_status(const agc_state_t *agc)
{
    const agc_regs_t *r = &agc->regs;
    const throttle_t *t = &agc->throttle;

    printf("=== AGC Status ===\n");
    printf("  Z (PC)          : %04o\n", r->Z);
    printf("  A               : %04o\n", r->A);
    printf("  Cycles          : %u\n", r->cycle_count);
    printf("  Steals accepted : %u\n", r->steal_count);
    printf("  Steals dropped  : %u\n", r->steal_dropped);
    printf("  Tokens left     : %u / %u\n", t->tokens, t->max_tokens);
    printf("  Priority mask   : 0x%02X\n", t->priority_mask);
    printf("  Window load     : %u steals / %u cycles\n",
           t->window_steals, t->window_cycles);
    printf("  Alarm 1201      : %s\n", agc->alarm_1201 ? "YES" : "no");
    printf("  Alarm 1202      : %s\n", agc->alarm_1202 ? "YES" : "no");
    printf("  Running         : %s\n", agc->running ? "yes" : "halted");
}
