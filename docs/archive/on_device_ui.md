# Task: Implement interactive on-device UI per Display_UI_Requirements

## Requirements Reference
- docs/Requirements/feature addition/Display_UI_Requirements.md (sections 3-7)
- docs/Requirements/Project_document.md, section 5.3 & button behaviour references

## Current Shortcoming
- `UiRenderer` only prints static telemetry; no state management, button handling, or countdowns.
- Propeller animation frames and LED legend are not displayed.
- Configuration mode, per-sensor editing, Nyquist warning prompts, and LED settings pages (C5/C6) are absent.

## Implementation Notes
1. Add input polling for BtnA/B/C to manage Idle, Info, and Configuration states.
2. Implement page navigation with circular list, countdown overlays, and helper text.
3. Render propeller animation using SVG frames aligned with flow activity.
4. Create configuration menus for device-level settings (including LED registers) and sensor submenus, with validation and override prompts.
5. Display LED status legend and diagnostic badges for undersampling flags.
6. Enforce 120 s idle timeout and backlight control.
