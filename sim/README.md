# Eagle Landing AGC Cycle-Steal Throttle – Core Loop Skeleton

Lightweight C11 simulation of the Apollo Guidance Computer instruction cycle with an explicit **cycle-steal throttle**. The goal is to model (and prevent) the executive-overflow conditions that produced the famous 1201 / 1202 alarms during the Apollo 11 powered descent.

## Historical Motivation

On 20 July 1969 the Lunar Module computer (Luminary) experienced repeated executive overflows. The root cause was a flood of **cycle steals** from the Rendezvous Radar interface: phase differences between 800 Hz references made a stationary antenna appear to jitter, generating thousands of counter updates per second and consuming ~13 % of the AGC’s duty cycle. The priority-driven executive (designed by the MIT Instrumentation Laboratory team) correctly shed lower-priority work and recovered, allowing the landing to continue.

This project provides a minimal, deterministic skeleton that:

- Emulates a basic fetch-decode-execute loop
- Accepts cycle-steal requests from simulated radar peripherals
- Applies a **token-bucket + priority-mask throttle** (the “BIT5 / PRIORITY mask” idea)
- Raises 1201/1202-style alarms only when the throttle is overwhelmed

## Building & Running

```bash
make          # builds agc_sim
make run      # builds and executes the demo scenario
```

Requirements: any C11 compiler (gcc/clang). No external libraries.

## Architecture (current skeleton)

```
include/agc.h     – public types and API
src/agc.c         – core loop, throttle logic, cycle-steal handling
src/main.c        – demo driver that recreates a short descent with RR bursts
```

### Key structures

- `agc_regs_t` – simplified register file (A, L, Q, Z, banks, counters)
- `agc_memory_t` – static core memory array
- `cycle_steal_req_t` – request from a peripheral (source, priority, address, delta)
- `throttle_t` – token bucket + priority mask + soft/hard load limits
- `agc_state_t` – complete simulation state

### Core API

```c
void agc_init(agc_state_t *agc);
void agc_step(agc_state_t *agc);                    /* one instruction cycle */
bool agc_request_cycle_steal(agc_state_t *agc, const cycle_steal_req_t *req);
void agc_run(agc_state_t *agc, uint32_t max_cycles);
void agc_dump_status(const agc_state_t *agc);
```

## Demo Scenario

The `main` driver runs four phases:

1. Quiet descent (normal instruction cycles)
2. Heavy Rendezvous Radar burst at priority 5 (allowed by default mask)
3. Low-priority spam (priority 3) that the mask should reject
4. Resume guidance cycles

At the end it reports whether any 1201/1202 alarms were raised.

## Design Constraints (intentionally strict)

- Pure C11
- No `malloc` / `free` in the core (everything static)
- Deterministic timing model
- Explicit interrupt / steal hooks so the throttle can be reasoned about

## Next Steps

- Expand the instruction set beyond the current “noop + increment”
- Model real core-set / VAC allocation so the alarms become accurate
- Add Landing Radar vs Rendezvous Radar distinction with different priorities
- Python/FastAPI telemetry front-end (SSE) as sketched in the original brainstorm
- Formal worst-case analysis of the token-bucket parameters

## License

This is educational / heritage software. Use and modify freely.
