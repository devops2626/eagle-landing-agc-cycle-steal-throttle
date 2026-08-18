# Eagle Landing: AGC Cycle-Stealing Throttle Achievement

**Mission Time:** T+102:45:40  
**Vehicle:** Lunar Module Eagle (Luminary 116)  
**Target:** Oceanus Procellarum (LAT 3.012° S, LON 23.421° W)

## Achievement Summary

Successful powered descent and touchdown with **zero 1201 or 1202 program alarms**.

The key improvement: **Radar Cycle-Stealing Throttle** with BIT5/PRIORITY mask engaged. This prevented the classic rendezvous-radar-induced executive overflow that historically produced the famous alarms on Apollo 11, while still allowing smooth Landing Radar altitude updates without starving the Executive.

### Historical Context (for comparison)
On the real Apollo 11 flight, spurious cycle steals from the Rendezvous Radar (left in a mode that generated phantom angle updates due to phase differences in the 800 Hz references) consumed ~13% of the AGC duty cycle. Combined with other load, this triggered 1201/1202 executive overflow alarms. The priority-based executive (designed by J. Halcombe Laning and the MIT Instrumentation Lab team, including Margaret Hamilton’s group) correctly shed lower-priority tasks and recovered, allowing the landing to continue.

This simulation/patch applies a throttle/mask on the cycle-steal path (BIT5/PRIORITY) so that radar interface activity cannot monopolize core sets / VAC areas during the critical P63/P64 descent path.

## Implementation Status

**Core loop skeleton is now available** under [`sim/`](sim/):

- Pure C11 instruction-cycle emulator
- Explicit cycle-steal request API
- Token-bucket + priority-mask throttle (BIT5 style)
- Demo that recreates a radar-burst descent segment and shows the throttle preventing 1201/1202 alarms

```bash
cd sim
make run
```

See [`sim/README.md`](sim/README.md) for build instructions and architecture details.

## Telemetry & Descent Log

* PRO executed. Program 63 initialized. Descent Orbit Insertion burn verified.
* Radar Cycle-Stealing Throttle: BIT5/PRIORITY mask engaged. Landing Radar altitude updates streaming smoothly without starving the Executive.
* CPU Load: Nominal. Zero 1201 or 1202 alarms registered. The throttle is buying back those vital microseconds.
* IMU Monitor (Verb 53): Active. Gyro drift tracking within acceptable tolerances. Noun 69 checklist standing by just in case.
* Guidance Flags: Clean. E/M Abort logic safely isolated outside the P64 critical path.

## Air-to-Ground

> "The burn is complete. Velocity is nominal, altitude dropping through 45,000 feet. Radar data lock is solid—zero alarms, CapCom. The cycle-stealing patch is holding the line."

## Touchdown Sequence

```
[ALTITUDE: 1,200 ft] -- Engine thrust vector stable.
[ALTITUDE:   500 ft] -- Pitch-up maneuver executing. Target coordinates in sight.
[ALTITUDE:   100 ft] -- Dust kick-up. Inertial velocity decaying.
[ALTITUDE:    10 ft] -- Contact light on. Engine stop.
```

> "Houston, Tranquility Base here. The Eagle has landed."

## Improvements Implemented

1. **Cycle-Stealing Throttle (BIT5 / PRIORITY mask)**  
   Limits the rate or priority impact of radar counter increments so they cannot push the Executive past its core-set / VAC limits during high-load descent phases.

2. **Clean Guidance Flags**  
   E/M Abort logic kept outside the P64 critical path.

3. **Nominal CPU Load**  
   Continuous Landing Radar updates without executive starvation.

## Future Work / Possible Further Improvements

- Expand the instruction set and model real core-set / VAC allocation
- Formal verification of the throttle mask under worst-case radar interrupt rates
- Landing Radar vs Rendezvous Radar priority differentiation
- Python/FastAPI real-time telemetry front-end (SSE)
- Integration tests with full Luminary 116 image + simulated RR interface jitter

---

*Published for achievements and continuous improvement of AGC guidance software heritage.*
