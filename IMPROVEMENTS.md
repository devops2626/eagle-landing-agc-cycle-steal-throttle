# Proposed / Implemented Improvements

## Radar Cycle-Stealing Throttle (BIT5 / PRIORITY mask)

### Problem
Historically, the Rendezvous Radar interface could generate a continuous stream of cycle steals (PINC/MINC on angle counters) due to phase differences between 800 Hz references. This consumed approximately 13% of AGC duty cycle and, under already high load in P63/P64, exhausted core sets / VAC areas → 1201/1202 alarms.

### Solution Concept
Engage a hardware/software throttle or priority mask (referenced here as BIT5/PRIORITY) that:
- Limits the effective rate at which radar cycle steals can interrupt or allocate Executive resources during critical landing phases.
- Ensures Landing Radar (altitude/velocity) updates continue to flow to guidance without allowing the RR interface to starve the scheduler.
- Keeps high-priority guidance, autopilot, and display tasks able to run to completion.

### Observed Result in this Run
- CPU Load: Nominal
- Zero 1201 / 1202 alarms
- Smooth Landing Radar altitude stream
- Clean guidance flags
- Successful contact light and engine stop at ~10 ft

### Notes for Further Development
- Exact bit definition and location in the Executive / interrupt handling code should be documented against Luminary 116 listings.
- Worst-case analysis: maximum expected cycle-steal rate under the throttle still leaves margin for SERVICER, DAP, and landing guidance jobs.
- Compatibility with abort modes (where RR data may become critical).

## Other Clean-ups Observed
- E/M Abort logic isolated outside P64 critical path.
- IMU monitor (V53) active with acceptable gyro drift.
- Noun 69 checklist available as contingency.
