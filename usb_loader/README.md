# AtmosphereUSBLoader — Phase 1

This directory contains an **experimental, read-only, platform-neutral block-device prototype**.
It is **not** a Nintendo Switch USB driver and is not linked into any Atmosphère build.

## Verified integration context

- `stratosphere/ams_mitm/source/fs_mitm/` contains filesystem MITM interfaces.
- `emummc/` implements an SDMMC driver replacement, not a USB Mass Storage backend.
- Atmosphère's root build uses Makefiles; this prototype has its own optional
  CMake host tests and does not modify upstream build targets.

## Build and test on a PC

```sh
cmake -S usb_loader -B /tmp/atmosphere-usb-loader-build
cmake --build /tmp/atmosphere-usb-loader-build
ctest --test-dir /tmp/atmosphere-usb-loader-build --output-on-failure
```

## Scope and safety

The `ReadOnlyBlockDevice` wrapper validates geometry, reads, and disconnection
state before dispatching to an injected sector reader. The fake-disk tests
exercise successful reads, bounds checks, bad geometry, I/O failure, and unplug
state. No writes, partition parsing, USB protocol, FAT/exFAT, or title mounting
are implemented.

**Important:** The abstraction is not thread-safe. A future USB backend must
serialize device removal with outstanding reads, invalidate in-flight requests,
and handle USB transport errors without accessing stale device contexts.

## Next milestones

1. Identify a supported USB host API for the exact Switch runtime context.
2. Implement USB Mass Storage discovery / transport in a separate backend.
3. Add partition and filesystem handling with safe unmount/reconnect behavior.
4. Explore a filesystem service integration point, initially read-only.
5. Run CI/toolchain builds and hardware tests before using it on a console.

Upstream Atmosphère modules are deliberately unchanged in this phase.
