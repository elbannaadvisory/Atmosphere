#include <usb_loader/scsi_disk.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace usb_loader;

struct FakeScsi {
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(512 * 4, 0x5A);
    bool fail = false;
    bool large = false;
    unsigned reads = 0;
};

static Status Command(void *context, const std::uint8_t *cdb,
                      std::size_t cdb_len, void *out, std::size_t size) {
    auto &disk = *static_cast<FakeScsi *>(context);
    if (disk.fail) return Status::IoError;
    if (cdb_len != 10) return Status::IoError;
    if (cdb[0] == 0x25 && size == 8) {
        std::uint8_t reply[8] = {0, 0, 0, disk.large ? std::uint8_t(0xff) : std::uint8_t(3), 0, 0, 2, 0};
        if (disk.large) reply[0] = reply[1] = reply[2] = 0xff;
        std::memcpy(out, reply, sizeof(reply));
        return Status::Ok;
    }
    if (cdb[0] == 0x28) {
        const std::uint32_t lba = (std::uint32_t(cdb[2]) << 24) |
                                  (std::uint32_t(cdb[3]) << 16) |
                                  (std::uint32_t(cdb[4]) << 8) | cdb[5];
        const std::uint32_t count = (std::uint32_t(cdb[7]) << 8) | cdb[8];
        if (lba + count > 4 || size != count * 512) return Status::IoError;
        std::memcpy(out, disk.bytes.data() + lba * 512, size);
        ++disk.reads;
        return Status::Ok;
    }
    return Status::IoError;
}

int main() {
    FakeScsi fake;
    ScsiDisk disk;
    std::vector<std::uint8_t> buffer(1024);
    assert(disk.Initialize(nullptr, nullptr) == Status::InvalidArgument);
    assert(disk.Initialize(Command, &fake) == Status::Ok);
    assert(disk.BlockDevice().GetGeometry().sector_size == 512);
    assert(disk.BlockDevice().GetGeometry().sector_count == 4);
    assert(disk.BlockDevice().Read(1, 2, buffer.data(), buffer.size()) == Status::Ok);
    assert(fake.reads == 1);
    for (auto x : buffer) assert(x == 0x5A);
    assert(disk.BlockDevice().Read(3, 2, buffer.data(), buffer.size()) == Status::OutOfBounds);
    assert(disk.BlockDevice().Read(0, 1, buffer.data(), 511) == Status::InvalidArgument);
    fake.fail = true;
    assert(disk.BlockDevice().Read(0, 1, buffer.data(), buffer.size()) == Status::IoError);
    disk.Disconnect();
    assert(disk.BlockDevice().Read(0, 1, buffer.data(), buffer.size()) == Status::NotConnected);
    fake.fail = false;
    fake.large = true;
    assert(disk.Initialize(Command, &fake) == Status::InvalidGeometry);
    assert(!disk.BlockDevice().IsConnected());
}
