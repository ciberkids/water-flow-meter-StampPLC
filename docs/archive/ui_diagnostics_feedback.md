# Task: Surface diagnostics (undersampling flags, warnings) in the UI

## Requirements Reference
- docs/Requirements/feature addition/Display_UI_Requirements.md:5.6
- docs/Requirements/Project_document.md:5.1 (warning when undersampled)

## Current Shortcoming
- UI does not indicate which sensors are undersampled or out of configuration.
- No visual badges, warnings, or override prompts.

## Implementation Notes
1. Extend UI rendering to show per-sensor warning icons when corresponding bit in reg 30 is set.
2. During configuration edits, display Nyquist warning dialog with UP=edit, DOWN=ignore-and-save flow.
3. Provide summary banner (e.g., red text) when any diagnostic flag is active.
