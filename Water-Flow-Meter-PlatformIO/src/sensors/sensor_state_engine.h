#pragma once

#include <cstddef>

#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "sensors/sensor_topology.h"

namespace plc {

class SensorStateEngine {
 public:
  struct Dependencies {
    SensorData* sensors = nullptr;
    SensorCharacteristics* configs = nullptr;
    std::size_t sensorCount = 0;
    RegisterBank* registerBank = nullptr;
    ModbusManager* modbusManager = nullptr;
    double* totalSessionLitersCache = nullptr;
    double* aggregateFlowLpmCache = nullptr;
    bool* allSensorsReadyCache = nullptr;
    uint16_t* undersamplingFlags = nullptr;

    /**
     * The cascade topology (`N-e` T1), or null. **Null means every channel is a root**, which is the
     * parallel installation and today's behaviour — so a caller with no opinion about topology gets
     * R2.2's reduction for free, and every existing host test keeps asserting the same numbers.
     */
    const SensorTopology* topology = nullptr;

    /**
     * The DELIVERED pair — R2.1's sum over channels whose EFFECTIVE parent is 0 — beside the gross pair
     * above, never instead of it (R2.3).
     *
     * Two accumulators rather than one netted value because three consumers want the gross figure and
     * would misbehave on a netted one (§5.3): the red LED's litre step, the blue LED's liveness, and
     * P0's flow dots. The last two are the sharp ones — under a netted aggregate a root reading zero
     * while its child meters real flow gives zero, so the panel and the LED both report "no water"
     * during exactly the condition this feature exists to surface (R2.4).
     */
    double* deliveredSessionLitersCache = nullptr;
    double* deliveredFlowLpmCache = nullptr;

    /**
     * Bits 0-7: an effective root whose branch volume nobody can state, because the root itself is
     * in service with no valid calibration (R2.5, §7 Q1's decision).
     *
     * When any bit is set the delivered pair is published as **NaN** rather than as a smaller number.
     * Zero is a legal reading and blanking would be indistinguishable from no flow, so a device that
     * cannot support a total says so instead of stating one it cannot stand behind.
     *
     * Shaped like `REG_UNDERSAMPLING_FLAGS` — bits, not a count — because a consumer wants to name the
     * branch in an alert. And deliberately NOT `uncalibratedFlags` from the diagnostics payload: that
     * one names every uncalibrated channel, this one names only uncalibrated EFFECTIVE ROOTS, and on a
     * parallel topology the two sets are identical — so reusing it would look correct until the first
     * cascade was declared and no test on the default topology could tell them apart.
     */
    uint16_t* unknownBranchesCache = nullptr;
  };

  explicit SensorStateEngine(const Dependencies& deps);

  void update(float elapsedSeconds);
  void refreshDiagnostics();

 private:
  Dependencies deps_;
};

}  // namespace plc
