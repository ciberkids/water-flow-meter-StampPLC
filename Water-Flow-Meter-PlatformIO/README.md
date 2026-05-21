# Water Flow Meter Firmware

This PlatformIO workspace hosts the embedded side of the StampPLC UI pipeline.

## Layout

```
src/
├── firmware.cpp          # entrypoint + tasks
├── input/                # button input manager
├── led/                  # LED controller + animations
├── modbus/               # Modbus manager, registers, sensor structs
└── ui/
    ├── core/             # UiController, UiRenderer, UiAssets loader
    ├── generated/        # exporter output from web/mockup
    └── theme/            # ThemePalette helpers
```

## Building & Testing

### Local toolchain

```bash
cd Water-Flow-Meter-PlatformIO
pio run            # build firmware (default env)
pio test -d tests/build  # run compile-smoke tests
```

### Docker (CI-friendly)

```bash
cd Water-Flow-Meter-PlatformIO
podman build -t stampplc-fw .
podman run --rm -v $(pwd):/workspace stampplc-fw
```

The Docker image installs PlatformIO and reuses `platformio.ini` to compile the firmware inside a clean container.

## UI Integration

See `docs/Requirements/feature addition/UI_Firmware_Interface.md` for the contract between generated UI assets and the runtime renderer.
