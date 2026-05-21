#include "ui_bindings.h"
#include "GeneratedUi.h"
#include "FirmwareValues.h" // User must implement GetFirmwareValue(id)
#include "ScreenManager.h" // User must implement UpdateElementText(id, value)
#include <cstring>
#include <cstdio>

namespace ui_exporter {

void UpdateValues_InfoP0GlobalStatus() {
    // No bound values on this screen
}

void UpdateValues_InfoP1InstantFlow() {
    // No bound values on this screen
}

void UpdateValues_InfoP2CumulativeLiters() {
    // No bound values on this screen
}

void UpdateValues_InfoP3CumulativeM3() {
    // No bound values on this screen
}

void UpdateValues_InfoP4SessionLiters() {
    // No bound values on this screen
}

void UpdateValues_InfoP5SessionM3() {
    // No bound values on this screen
}

void UpdateValues_InfoP6MaxFlow() {
    // No bound values on this screen
}

void UpdateValues_InfoP7EnterConfig() {
    // No bound values on this screen
}

void UpdateValues_CountdownEnterConfig() {
    // No bound values on this screen
}

void UpdateValues_CountdownResetSession() {
    // No bound values on this screen
}

void UpdateValues_CountdownResetAll() {
    // No bound values on this screen
}

void UpdateValues_CountdownFactoryReset() {
    // No bound values on this screen
}

void UpdateValues_CountdownSensorSave() {
    // No bound values on this screen
}

void UpdateValues_CountdownConfigExit() {
    // No bound values on this screen
}

void UpdateValues_NyquistWarning() {
    // No bound values on this screen
}

void UpdateValues_StateIdle() {
    // No bound values on this screen
}

void UpdateValues_ConfigC1ModbusId() {
    // No bound values on this screen
}

void UpdateValues_ConfigC2BaudRate() {
    // No bound values on this screen
}

void UpdateValues_ConfigC3Parity() {
    // No bound values on this screen
}

void UpdateValues_ConfigC4StopBits() {
    // No bound values on this screen
}

void UpdateValues_ConfigC5LedPulseVol() {
    // No bound values on this screen
}

void UpdateValues_ConfigC6LedPulsePeriod() {
    // No bound values on this screen
}

void UpdateValues_ConfigC7SensorSelect() {
    // No bound values on this screen
}

void UpdateValues_ConfigS1Connected() {
    // No bound values on this screen
}

void UpdateValues_ConfigS2Multiplier() {
    // No bound values on this screen
}

void UpdateValues_ConfigS3Adjust() {
    // No bound values on this screen
}

void UpdateValues_ConfigS4MaxFlow() {
    // No bound values on this screen
}

void UpdateScreenValues(const char* screenId) {
    if (strcmp(screenId, "info-p0-global-status") == 0) {
        UpdateValues_InfoP0GlobalStatus();
        return;
    }
    if (strcmp(screenId, "info-p1-instant-flow") == 0) {
        UpdateValues_InfoP1InstantFlow();
        return;
    }
    if (strcmp(screenId, "info-p2-cumulative-liters") == 0) {
        UpdateValues_InfoP2CumulativeLiters();
        return;
    }
    if (strcmp(screenId, "info-p3-cumulative-m3") == 0) {
        UpdateValues_InfoP3CumulativeM3();
        return;
    }
    if (strcmp(screenId, "info-p4-session-liters") == 0) {
        UpdateValues_InfoP4SessionLiters();
        return;
    }
    if (strcmp(screenId, "info-p5-session-m3") == 0) {
        UpdateValues_InfoP5SessionM3();
        return;
    }
    if (strcmp(screenId, "info-p6-max-flow") == 0) {
        UpdateValues_InfoP6MaxFlow();
        return;
    }
    if (strcmp(screenId, "info-p7-enter-config") == 0) {
        UpdateValues_InfoP7EnterConfig();
        return;
    }
    if (strcmp(screenId, "countdown-enter-config") == 0) {
        UpdateValues_CountdownEnterConfig();
        return;
    }
    if (strcmp(screenId, "countdown-reset-session") == 0) {
        UpdateValues_CountdownResetSession();
        return;
    }
    if (strcmp(screenId, "countdown-reset-all") == 0) {
        UpdateValues_CountdownResetAll();
        return;
    }
    if (strcmp(screenId, "countdown-factory-reset") == 0) {
        UpdateValues_CountdownFactoryReset();
        return;
    }
    if (strcmp(screenId, "countdown-sensor-save") == 0) {
        UpdateValues_CountdownSensorSave();
        return;
    }
    if (strcmp(screenId, "countdown-config-exit") == 0) {
        UpdateValues_CountdownConfigExit();
        return;
    }
    if (strcmp(screenId, "nyquist-warning") == 0) {
        UpdateValues_NyquistWarning();
        return;
    }
    if (strcmp(screenId, "state-idle") == 0) {
        UpdateValues_StateIdle();
        return;
    }
    if (strcmp(screenId, "config-c1-modbus-id") == 0) {
        UpdateValues_ConfigC1ModbusId();
        return;
    }
    if (strcmp(screenId, "config-c2-baud-rate") == 0) {
        UpdateValues_ConfigC2BaudRate();
        return;
    }
    if (strcmp(screenId, "config-c3-parity") == 0) {
        UpdateValues_ConfigC3Parity();
        return;
    }
    if (strcmp(screenId, "config-c4-stop-bits") == 0) {
        UpdateValues_ConfigC4StopBits();
        return;
    }
    if (strcmp(screenId, "config-c5-led-pulse-vol") == 0) {
        UpdateValues_ConfigC5LedPulseVol();
        return;
    }
    if (strcmp(screenId, "config-c6-led-pulse-period") == 0) {
        UpdateValues_ConfigC6LedPulsePeriod();
        return;
    }
    if (strcmp(screenId, "config-c7-sensor-select") == 0) {
        UpdateValues_ConfigC7SensorSelect();
        return;
    }
    if (strcmp(screenId, "config-s1-connected") == 0) {
        UpdateValues_ConfigS1Connected();
        return;
    }
    if (strcmp(screenId, "config-s2-multiplier") == 0) {
        UpdateValues_ConfigS2Multiplier();
        return;
    }
    if (strcmp(screenId, "config-s3-adjust") == 0) {
        UpdateValues_ConfigS3Adjust();
        return;
    }
    if (strcmp(screenId, "config-s4-max-flow") == 0) {
        UpdateValues_ConfigS4MaxFlow();
        return;
    }
}

} // namespace ui_exporter