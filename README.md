# Split Tone v2 — OpenFX port (from DCTL)

This is a straight port of `Split Tone_v2.dctl` into an OpenFX ImageEffect plugin (CPU, float RGBA).

## What matches the DCTL

- `Input Color Space` choice **only selects the middle-gray value** (it does not do any log/linear conversion).
- `Preserve Midgray` creates a symmetric “no-change” zone around the chosen middle gray.
- Shadow/Highlight RGB sliders apply a power curve **per channel** (exact same math as the DCTL).
- `Show Curve` draws the curve overlay & guide lines, matching the DCTL logic.

## Build (high level)

OpenFX plugins are built against the OpenFX API and typically use the “Support” C++ wrappers.

1. Get OpenFX Support sources (Academy Software Foundation OpenFX repo) and build/prepare headers.
2. Configure this project with CMake:

```bash
cmake -S . -B build -DOFX_SUPPORT_ROOT=/path/to/openfx/Support
cmake --build build --config Release
```

Because Support layouts/build products vary across versions, you may need to:
- Add the Support `.cpp` files directly to the `SplitToneV2` target, **or**
- Link against a prebuilt Support library produced in your environment.

## Installing

Each host has its own OFX plugin folder. Common locations:
- **DaVinci Resolve / Fusion:** `OFX/Plugins/`
- **Natron:** `~/.Natron/Plugins/OFX/`
- **Nuke:** supports OFX via plugins path (varies)

You may need to wrap the built library into an OFX bundle depending on platform/host.

## License

No license was provided with the original DCTL; this code is provided as a reference port.
