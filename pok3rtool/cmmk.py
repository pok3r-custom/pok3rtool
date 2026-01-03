
import logging
import struct
import time

from tqdm import tqdm

from .device import find_hid_devices
from .cykb import CYKB_Device

log = logging.getLogger(__name__)

known_devices = {
    (0x2516, 0x003c): "MasterKeys Pro S RGB",
}

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x02

CMD_RESET = 0x11
# Reset to bootloaer
CMD_RESET_BL = 0
# Reset to application (if valid)
CMD_RESET_FW = 1
# Disconnect USB
CMD_RESET_DIS = 2

CMD_READ = 0x12
# Some info at 0x400 in flash
CMD_READ_BL_INFO = 0
# Some info at 0x5000 in flash
CMD_READ_FW_INFO = 1
# Read version string
CMD_READ_VER = 0x20

CMD_FW = 0x1d
# Erase flash
CMD_FW_ERASE = 0
# Sum flash
CMD_FW_SUM = 1

CMD_ADDR = 0x1e
CMD_ADDR_GET = 0
CMD_ADDR_SET = 1

# Write flash
CMD_WRITE = 0x1f
RESP_WRITE_ADDR = 2


def checksum(data: bytes, value: int = 0) -> int:
    assert len(data) % 4 == 0, "data size must be a multiple of 4"
    # this "checksum" makes no sense, it only sums a quarter of the data you ask it to
    count = (len(data) + 3) // 4
    for i in range(count):
        value += data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24)
    return value & 0xffff_ffff


class CMMK_Device(CYKB_Device):
    PID_BOOT_BIT = 0x1

    @staticmethod
    def decode_firmware(encoded: bytes) -> bytes:
        return encoded

    @staticmethod
    def encode_firmware(decoded: bytes) -> bytes:
        return decoded

    def is_bootloader(self):
        return self.dev.idProduct & self.PID_BOOT_BIT == self.PID_BOOT_BIT

    def read_info(self):
        self.send_cmd(CMD_READ, CMD_READ_BL_INFO)
        info1 = self.recv_resp(CMD_READ, CMD_READ_BL_INFO)
        info1 = info1[:0x34]
        log.debug(f"bootloader info: {info1.hex()}")

        a, b, page_size, fw_pages, c, model, bvid, bpid, ver_addr, fw_offset, e, f, g = struct.unpack_from("<IIHHI8sHHHHHHH", info1)
        model = model.rstrip(b"\0").decode("ascii")

        log.debug(f"a: {a:#x}")
        log.debug(f"b: {b:#x}")
        log.debug(f"page size?: {page_size:#x}")
        log.debug(f"firmware pages: {fw_pages:#x}")
        log.debug(f"c: {c:#x}")
        log.debug(f"Model: {model}")
        log.debug(f"BOOT VID/PID: {bvid:#x}/{bpid:#x}")
        log.debug(f"version address: {ver_addr:#x}")
        log.debug(f"firmware offset: {fw_offset:#x}")
        log.debug(f"e: {e:#x}")
        log.debug(f"f: {f:#x}")
        log.debug(f"g: {g:#x}")

        # i'm not sure which is which, so make sure they're the same
        assert fw_offset == page_size

        fw_size = fw_pages * page_size

        log.debug(f"Firmware Size: {fw_size:#x}")

        self.send_cmd(CMD_READ, CMD_READ_FW_INFO)
        info2 = self.recv_resp(CMD_READ, CMD_READ_FW_INFO)
        info2 = info2[:32]
        log.debug(f"app info: {info2.hex()}")

        a, v0, v1, v2, v3, c, d, e, f, g, vid, pid = struct.unpack_from("<IBBBBIIIIIHH", info2)

        log.debug(f"a: {a:#x}")
        log.debug(f"version: 1.{v0}.{v1}.{v2}.{v3}")
        log.debug(f"c: {c:#x}")
        log.debug(f"d: {d:#x}")
        log.debug(f"e: {e:#x}")
        log.debug(f"f: {f:#x}")
        log.debug(f"g: {g:#x}")
        log.debug(f"VID/PID: {vid:#x}/{pid:#x}")

        return fw_offset, fw_size, bvid, bpid

    def read_version(self):
        fw_offset, _, _, _ = self.read_info()

        # read the version page
        vdata = bytes()
        for i in range(fw_offset // 60):
            self.send_cmd(CMD_READ, CMD_READ_VER + i)
            vdata += self.recv_resp(CMD_READ, CMD_READ_VER + i)
        log.debug(f"ver: {vdata.hex()}")

        a, v0, v1, v2, v3, c, d, e, f, g, vid, pid = struct.unpack_from("<IBBBBIIIIIHH", vdata[0x78:])

        log.debug(f"a: {a:#x}")
        log.debug(f"version: 1.{v0}.{v1}.{v2}.{v3}")
        log.debug(f"c: {c:#x}")
        log.debug(f"d: {d:#x}")
        log.debug(f"e: {e:#x}")
        log.debug(f"f: {f:#x}")
        log.debug(f"g: {g:#x}")
        log.debug(f"VID/PID: {vid:#x}/{pid:#x}")

        vlen, = struct.unpack_from("<I", vdata)
        if vlen == 0xffff_ffff:
            return vlen, None

        vstr = vdata[4:][:vlen].rstrip(b"\xff").decode("utf-8").rstrip("\0")
        return vlen, vstr

    def write_version(self, version: str):
        vstr = version.encode("utf-8")
        vlen = len(vstr)
        vstr += b"\0"
        vdata = vlen.to_bytes(4, "little") + vstr.ljust(4 * ((len(vstr) + 3) // 4), b"\0")
        assert len(vdata) <= 0x78, "version too long"

        _, _, bvid, bpid = self.read_info()

        ver = (1,5)

        # these seem to be identical to the "app info" at 0x5000
        # i don't know what, if anything, uses these values
        vvalues = [
            0x35a0003,
            (ver[1] << 8) | ver[0],
            0x19,
            0xffff_ffff,
            0x3,
            0xffff_ffff,
            0x0,
            bvid | ((bpid & ~self.PID_BOOT_BIT) << 16)
        ]

        vdata = vdata.ljust(0x78, b"\xff")
        for v in vvalues:
            vdata += v.to_bytes(4, "little")

        self.erase_flash(0, len(vdata))

        self.write_flash(0, vdata)

    def sum_flash(self, addr: int, size: int):
        log.debug(f"SUM {addr:#x} {size} bytes")

        self.send_cmd(CMD_FW, CMD_FW_SUM, struct.pack("<II", addr, size))
        time.sleep(0.1)
        resp = self.recv_resp(CMD_FW, CMD_FW_SUM)
        csum, = struct.unpack_from("<I", resp)

        return csum

    def verify_flash(self, addr: int, fw_data: bytes, *, progress=False):
        # this is really stupid, each sum command only "sums" 1/4 of the data we ask for,
        # and that length is limited to 0xff
        csize = 60 * 4
        gen = range(0, len(fw_data), csize // 4)
        if progress:
            gen = tqdm(gen)
        for offset in gen:
            vsize = min(csize, len(fw_data) - offset)
            sum0 = checksum(fw_data[offset:offset+vsize], 0)

            fsum = self.sum_flash(addr+offset, vsize)
            if fsum != sum0:
                raise RuntimeError(f"Checksum check failed: {fsum:08x} != {sum0:08x}")


def get_devices():
    bl_known_devices = {
        **known_devices,
        **{(vid, pid | CMMK_Device.PID_BOOT_BIT): f"{name} (bootloader)" for (vid, pid), name in known_devices.items()}
    }

    yield from find_hid_devices(
        CMMK_Device,
        known_devices=bl_known_devices,
        usage_page=UPDATE_USAGE_PAGE,
        usage=UPDATE_USAGE
    )
