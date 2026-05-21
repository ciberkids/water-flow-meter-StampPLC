#include "GeneratedUi.h"
#include "FirmwareAction.h" // User must implement this
#include "ScreenManager.h" // User must implement this

namespace ui_exporter {

void DispatchAction(const char* actionId) {
    if (actionId == nullptr) return;
    // Firmware implementation hook
    ManifestAction(actionId);
}

void RequestScreenLoad(const char* screenId) {
    if (screenId == nullptr) return;
    ScreenManager::LoadScreen(screenId);
}

void RegisterEvents_InfoP0GlobalStatus() {
    // No events defined for this screen
}

void RegisterEvents_InfoP1InstantFlow() {
    // No events defined for this screen
}

void RegisterEvents_InfoP2CumulativeLiters() {
    // No events defined for this screen
}

void RegisterEvents_InfoP3CumulativeM3() {
    // No events defined for this screen
}

void RegisterEvents_InfoP4SessionLiters() {
    // No events defined for this screen
}

void RegisterEvents_InfoP5SessionM3() {
    // No events defined for this screen
}

void RegisterEvents_InfoP6MaxFlow() {
    // No events defined for this screen
}

void RegisterEvents_InfoP7EnterConfig() {
    // No events defined for this screen
}

void RegisterEvents_CountdownEnterConfig() {
    // No events defined for this screen
}

void RegisterEvents_CountdownResetSession() {
    // No events defined for this screen
}

void RegisterEvents_CountdownResetAll() {
    // No events defined for this screen
}

void RegisterEvents_CountdownFactoryReset() {
    // No events defined for this screen
}

void RegisterEvents_CountdownSensorSave() {
    // No events defined for this screen
}

void RegisterEvents_CountdownConfigExit() {
    // No events defined for this screen
}

void RegisterEvents_NyquistWarning() {
    // No events defined for this screen
}

void RegisterEvents_StateIdle() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC1ModbusId() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC2BaudRate() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC3Parity() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC4StopBits() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC5LedPulseVol() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC6LedPulsePeriod() {
    // No events defined for this screen
}

void RegisterEvents_ConfigC7SensorSelect() {
    // No events defined for this screen
}

void RegisterEvents_ConfigS1Connected() {
    // No events defined for this screen
}

void RegisterEvents_ConfigS2Multiplier() {
    // No events defined for this screen
}

void RegisterEvents_ConfigS3Adjust() {
    // No events defined for this screen
}

void RegisterEvents_ConfigS4MaxFlow() {
    // No events defined for this screen
}

void RegisterAllEvents(const char* screenId) {
    if (strcmp(screenId, "info-p0-global-status") == 0) {
        RegisterEvents_InfoP0GlobalStatus();
        return;
    }
    if (strcmp(screenId, "info-p1-instant-flow") == 0) {
        RegisterEvents_InfoP1InstantFlow();
        return;
    }
    if (strcmp(screenId, "info-p2-cumulative-liters") == 0) {
        RegisterEvents_InfoP2CumulativeLiters();
        return;
    }
    if (strcmp(screenId, "info-p3-cumulative-m3") == 0) {
        RegisterEvents_InfoP3CumulativeM3();
        return;
    }
    if (strcmp(screenId, "info-p4-session-liters") == 0) {
        RegisterEvents_InfoP4SessionLiters();
        return;
    }
    if (strcmp(screenId, "info-p5-session-m3") == 0) {
        RegisterEvents_InfoP5SessionM3();
        return;
    }
    if (strcmp(screenId, "info-p6-max-flow") == 0) {
        RegisterEvents_InfoP6MaxFlow();
        return;
    }
    if (strcmp(screenId, "info-p7-enter-config") == 0) {
        RegisterEvents_InfoP7EnterConfig();
        return;
    }
    if (strcmp(screenId, "countdown-enter-config") == 0) {
        RegisterEvents_CountdownEnterConfig();
        return;
    }
    if (strcmp(screenId, "countdown-reset-session") == 0) {
        RegisterEvents_CountdownResetSession();
        return;
    }
    if (strcmp(screenId, "countdown-reset-all") == 0) {
        RegisterEvents_CountdownResetAll();
        return;
    }
    if (strcmp(screenId, "countdown-factory-reset") == 0) {
        RegisterEvents_CountdownFactoryReset();
        return;
    }
    if (strcmp(screenId, "countdown-sensor-save") == 0) {
        RegisterEvents_CountdownSensorSave();
        return;
    }
    if (strcmp(screenId, "countdown-config-exit") == 0) {
        RegisterEvents_CountdownConfigExit();
        return;
    }
    if (strcmp(screenId, "nyquist-warning") == 0) {
        RegisterEvents_NyquistWarning();
        return;
    }
    if (strcmp(screenId, "state-idle") == 0) {
        RegisterEvents_StateIdle();
        return;
    }
    if (strcmp(screenId, "config-c1-modbus-id") == 0) {
        RegisterEvents_ConfigC1ModbusId();
        return;
    }
    if (strcmp(screenId, "config-c2-baud-rate") == 0) {
        RegisterEvents_ConfigC2BaudRate();
        return;
    }
    if (strcmp(screenId, "config-c3-parity") == 0) {
        RegisterEvents_ConfigC3Parity();
        return;
    }
    if (strcmp(screenId, "config-c4-stop-bits") == 0) {
        RegisterEvents_ConfigC4StopBits();
        return;
    }
    if (strcmp(screenId, "config-c5-led-pulse-vol") == 0) {
        RegisterEvents_ConfigC5LedPulseVol();
        return;
    }
    if (strcmp(screenId, "config-c6-led-pulse-period") == 0) {
        RegisterEvents_ConfigC6LedPulsePeriod();
        return;
    }
    if (strcmp(screenId, "config-c7-sensor-select") == 0) {
        RegisterEvents_ConfigC7SensorSelect();
        return;
    }
    if (strcmp(screenId, "config-s1-connected") == 0) {
        RegisterEvents_ConfigS1Connected();
        return;
    }
    if (strcmp(screenId, "config-s2-multiplier") == 0) {
        RegisterEvents_ConfigS2Multiplier();
        return;
    }
    if (strcmp(screenId, "config-s3-adjust") == 0) {
        RegisterEvents_ConfigS3Adjust();
        return;
    }
    if (strcmp(screenId, "config-s4-max-flow") == 0) {
        RegisterEvents_ConfigS4MaxFlow();
        return;
    }
}

} // namespace ui_exporter