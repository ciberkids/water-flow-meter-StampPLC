# Task: Implement button input handling for UI state machine

## Requirements Reference
- docs/Requirements/feature addition/Display_UI_Requirements.md (sections 3-5)
- docs/Requirements/feature addition/RGB_LED_Behavior.md (factory reset interaction)
- docs/Requirements/Project_document.md (UI/LED behaviour summary)

## Current Shortcoming
- Firmware never polls BtnA/BtnB/BtnC; no navigation, no countdown triggers, no long-press detection.
- Factory reset combo (UP+DOWN 30 s) is not triggered because inputs are ignored.
- UI renderer operates with static output and lacks state transitions driven by user input.

## Implementation Notes
1. Add a button polling module (debounced reads of M5.BtnA/B/C) running on the logic task loop.
2. Detect short/long press events and simultaneous button combos; expose them to the UI state machine.
3. Maintain Idle, Info, and Configuration states, with page navigation on short presses and countdown initiations on long presses.
4. Integrate with LED controller and Modbus reset logic for factory reset workflow.
5. Provide callbacks for UI renderer to update countdown overlays, legends, and prompts in response to button events.
