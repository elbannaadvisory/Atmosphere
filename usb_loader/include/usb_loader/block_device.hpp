/*
 * AtmosphereUSBLoader prototype
 * Copyright (c) 2026 AtmosphereUSBLoader contributors
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Platform-neutral read-only block-device contract.
 * No USB host or Horizon OS integration is implemented here.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace usb_loader {

enum class Status {
    Ok,
    InvalidArgument,
    InvalidGeometry,
    NotConnected,
    OutOfBounds,
    IoError,
};

// The callback reads 'count' sectors starting at 'lba' into 'destination'.
// It must write exactly count * sector_size bytes on success.
using ReadSectors = Status (*)(void *context, std::uint64_t lba,
                               std::uint32_t count, void *destination);

struct Geometry {
    std::uint32_t sector_size;
    std::uint64_t sector_count;
};

class ReadOnlyBlockDevice {
public:
    ReadOnlyBlockDevice() = default;

    Status Attach(Geometry geometry, ReadSectors reader, void *context) {
        Disconnect();
        // Limit to common power-of-two sector sizes; avoid pathological buffers.
        if (reader == nullptr || geometry.sector_count == 0 ||
            geometry.sector_size < 512 || geometry.sector_size > 4096 ||
            (geometry.sector_size & (geometry.sector_size - 1)) != 0) {
            return Status::InvalidGeometry;
        }
        geometry_ = geometry;
        reader_ = reader;
        context_ = context;
        connected_ = true;
        return Status::Ok;
    }

    void Disconnect() {
        connected_ = false;
        reader_ = nullptr;
        context_ = nullptr;
        geometry_ = {0, 0};
    }

    bool IsConnected() const { return connected_; }
    Geometry GetGeometry() const { return geometry_; }

    Status Read(std::uint64_t lba, std::uint32_t count,
                void *destination, std::size_t destination_size) const {
        if (!connected_) return Status::NotConnected;
        if (count == 0 || destination == nullptr) return Status::InvalidArgument;
        // Subtraction avoids overflow in lba + count.
        if (lba >= geometry_.sector_count ||
            static_cast<std::uint64_t>(count) > geometry_.sector_count - lba) {
            return Status::OutOfBounds;
        }
        if (static_cast<std::uint64_t>(count) >
            std::numeric_limits<std::size_t>::max() / geometry_.sector_size) {
            return Status::InvalidArgument;
        }
        const std::size_t required = static_cast<std::size_t>(count) * geometry_.sector_size;
        if (destination_size < required) return Status::InvalidArgument;
        return reader_(context_, lba, count, destination);
    }

private:
    Geometry geometry_{0, 0};
    ReadSectors reader_ = nullptr;
    void *context_ = nullptr;
    bool connected_ = false;
};

} // namespace usb_loader
