# AtmosphereUSBLoader — Phase 2

A read-only, platform-neutral prototype for USB mass-storage integration. It is
**not yet a Switch USB driver**, does not mount FAT/exFAT, and is not linked into
Atmosphère's production build.

## Modules

- `include/usb_loader/block_device.hpp`: validated read-only sector interface.
- `include/usb_loader/scsi_disk.hpp`: SCSI READ CAPACITY(10) and READ(10), behind
  a transport-agnostic `ScsiDataIn` callback.
- `tests/`: fake-disk and fake-SCSI host tests.

The callback must complete one full SCSI data-in operation, including transport
status and exact transfer-length validation. Passing raw USB bulk transactions
as this callback is incorrect. USB Mass Storage Bulk-Only Transport (CBW,
CSW, endpoints, resets, stall recovery, and sense data) is not implemented.

The current SCSI reader supports devices reporting capacity via READ CAPACITY(10)
(up to 2^32 sectors) and transfers of at most 65535 sectors per READ(10). A
READ CAPACITY(16)/READ(16) implementation and request chunking are future work.

## Test on a workstation

```sh
cmake -S usb_loader -B build-usb-loader -DCMAKE_BUILD_TYPE=Debug
cmake --build build-usb-loader --parallel
ctest --test-dir build-usb-loader --output-on-failure
```

## Integration boundary and safety

This is not thread-safe; callers must synchronize disconnect and in-flight
I/O. Hardware backend integration must validate transfers, handle hot-unplug,
check device readiness, and avoid reusing stale callbacks. No write commands
are exposed. Upstream Atmosphère system components remain unchanged.

## Next milestones

1. Implement and fake-test USB Mass Storage Bulk-Only Transport (BOT).
2. Identify usable USB host APIs in the Switch system-module context.
3. Add host-controller/device-enumeration integration and safe hot-unplug.
4. Add partition/filesystem support and read-only filesystem integration.
5. Validate on target hardware, then evaluate opt-in writes if needed.
