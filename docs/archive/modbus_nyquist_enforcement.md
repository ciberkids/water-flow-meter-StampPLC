# Task: Enforce Nyquist validation workflow for sensor configuration

## Requirements Reference
- docs/Requirements/Project_document.md:5.1 (Sensor Configuration Workflow, step 5)
- docs/Requirements/Implementation_Alignment_Report.md

## Current Shortcoming
- `ModbusManager::applyHoldingWrite` commits configuration values (Q, F, Adjust) even when they violate Nyquist limits.
- No rejection or rollback occurs; undersampling flag is updated only after invalid data has been accepted.
- No support for explicit override path described in the requirement.

## Implementation Notes
1. Before writing new configuration values, compute Nyquist condition with existing `pollingRate_kHz` and candidate value.
2. If invalid and no override flag, reject write (return appropriate Modbus error) and leave previous value intact.
3. Provide mechanism (e.g., dedicated register or bit) allowing master to acknowledge override.
4. Ensure undersampling flag (reg 30) is only raised when values are accepted under override or when actual runtime data fails validations.
5. Update documentation and tests accordingly.
