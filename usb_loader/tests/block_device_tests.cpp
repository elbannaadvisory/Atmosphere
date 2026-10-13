#include <usb_loader/block_device.hpp>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

using namespace usb_loader;

struct FakeDisk {
    std::vector<std::uint8_t> bytes;
    std::uint32_t sector_size;
    bool fail = false;
};

static Status ReadFake(void *context, std::uint64_t lba,
                       std::uint32_t count, void *out) {
    auto &disk = *static_cast<FakeDisk *>(context);
    if (disk.fail) return Status::IoError;
    std::memcpy(out, disk.bytes.data() + static_cast<std::size_t>(lba) * disk.sector_size,
                static_cast<std::size_t>(count) * disk.sector_size);
    return Status::Ok;
}

int main() {
    ReadOnlyBlockDevice device;
    std::vector<std::uint8_t> buffer(1024, 0);
    assert(device.Read(0, 1, buffer.data(), buffer.size()) == Status::NotConnected);
    assert(device.Attach({0, 4}, ReadFake, nullptr) == Status::InvalidGeometry);
    assert(device.Attach({512, 0}, ReadFake, nullptr) == Status::InvalidGeometry);
    assert(device.Attach({513, 4}, ReadFake, nullptr) == Status::InvalidGeometry);
    assert(device.Attach({512, 4}, nullptr, nullptr) == Status::InvalidGeometry);

    FakeDisk disk{{}, 512};
    disk.bytes.resize(4 * 512);
    std::fill(disk.bytes.begin() + 512, disk.bytes.begin() + 1024, 0xA5);
    assert(device.Attach({512, 4}, ReadFake, &disk) == Status::Ok);
    assert(device.IsConnected());
    assert(device.Read(1, 1, buffer.data(), buffer.size()) == Status::Ok);
    assert(std::all_of(buffer.begin(), buffer.begin() + 512,
                       [](std::uint8_t x) { return x == 0xA5; }));
    assert(device.Read(3, 2, buffer.data(), buffer.size()) == Status::OutOfBounds);
    assert(device.Read(std::numeric_limits<std::uint64_t>::max(), 1,
                       buffer.data(), buffer.size()) == Status::OutOfBounds);
    assert(device.Read(0, 0, buffer.data(), buffer.size()) == Status::InvalidArgument);
    assert(device.Read(0, 1, nullptr, 512) == Status::InvalidArgument);
    assert(device.Read(0, 2, buffer.data(), 511) == Status::InvalidArgument);
    disk.fail = true;
    assert(device.Read(0, 1, buffer.data(), buffer.size()) == Status::IoError);
    device.Disconnect();
    assert(!device.IsConnected());
    assert(device.Read(0, 1, buffer.data(), buffer.size()) == Status::NotConnected);
    return 0;
}
