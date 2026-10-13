/*
 * AtmosphereUSBLoader - transport-independent SCSI transparent command layer.
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once
#include <usb_loader/block_device.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace usb_loader {

// One complete SCSI command data-in transaction, including transport status
// and exact received-length validation. NOT an individual USB bulk transfer.
using ScsiDataIn = Status (*)(void *context, const std::uint8_t *cdb,
                             std::size_t cdb_length, void *buffer,
                             std::size_t buffer_length);

class ScsiDisk {
public:
    Status Initialize(ScsiDataIn command, void *context) {
        Disconnect();
        if (!command) return Status::InvalidArgument;
        std::uint8_t capacity_cdb[10] = {0x25}; // READ CAPACITY(10)
        std::uint8_t reply[8] = {};
        Status status = command(context, capacity_cdb, sizeof(capacity_cdb), reply, sizeof(reply));
        if (status != Status::Ok) return status;
        const std::uint32_t last_lba = ReadBe32(reply);
        const std::uint32_t sector_size = ReadBe32(reply + 4);
        // 0xffffffff indicates that READ CAPACITY(16) is required.
        if (last_lba == UINT32_MAX) return Status::InvalidGeometry;
        const Geometry geometry{sector_size, static_cast<std::uint64_t>(last_lba) + 1};
        status = device_.Attach(geometry, &ReadThunk, this);
        if (status != Status::Ok) return status;
        command_ = command;
        context_ = context;
        return Status::Ok;
    }

    void Disconnect() {
        device_.Disconnect();
        command_ = nullptr;
        context_ = nullptr;
    }

    const ReadOnlyBlockDevice &BlockDevice() const { return device_; }

private:
    static std::uint32_t ReadBe32(const std::uint8_t *p) {
        return (static_cast<std::uint32_t>(p[0]) << 24) |
               (static_cast<std::uint32_t>(p[1]) << 16) |
               (static_cast<std::uint32_t>(p[2]) << 8) |
               static_cast<std::uint32_t>(p[3]);
    }

    static Status ReadThunk(void *context, std::uint64_t lba,
                            std::uint32_t count, void *buffer) {
        return static_cast<ScsiDisk *>(context)->ReadSectors(lba, count, buffer);
    }

    Status ReadSectors(std::uint64_t lba, std::uint32_t count, void *buffer) const {
        if (!command_) return Status::NotConnected;
        if (count == 0 || count > UINT16_MAX || lba > UINT32_MAX ||
            count - 1 > UINT32_MAX - lba) return Status::InvalidArgument;
        std::uint8_t cdb[10] = {0x28}; // READ(10)
        cdb[2] = static_cast<std::uint8_t>(lba >> 24);
        cdb[3] = static_cast<std::uint8_t>(lba >> 16);
        cdb[4] = static_cast<std::uint8_t>(lba >> 8);
        cdb[5] = static_cast<std::uint8_t>(lba);
        cdb[7] = static_cast<std::uint8_t>(count >> 8);
        cdb[8] = static_cast<std::uint8_t>(count);
        const auto size = static_cast<std::size_t>(count) * device_.GetGeometry().sector_size;
        return command_(context_, cdb, sizeof(cdb), buffer, size);
    }

    ScsiDataIn command_ = nullptr;
    void *context_ = nullptr;
    ReadOnlyBlockDevice device_;
};

} // namespace usb_loader
