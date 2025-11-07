# Task: Implement button-driven factory reset & LED coordination

## Requirements Reference
- docs/Requirements/Project_document.md:5.3 (RGB status LED behaviour)
- docs/Requirements/feature addition/Display_UI_Requirements.md (factory reset workflow)
- docs/Requirements/feature addition/RGB_LED_Behavior.md

## Current Shortcoming
- No button handling for UP+DOWN 30 s factory reset.
- LED controller not informed of UI countdown or reset state; registers 31/32 are not reset via UI.
- LED behaviour during countdown (all off) not implemented.

## Implementation Notes
1. Detect simultaneous UP+DOWN long press, start 30 s countdown, provide UI feedback.
2. During countdown, set LED off, block other inputs, allow cancel if buttons released.
3. On completion, reset Modbus registers (31/32) and relevant preferences, reboot as required.
4. Update LED controller and register bank to reflect defaults immediately.
