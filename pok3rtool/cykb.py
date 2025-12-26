
import logging
import struct
import time
import zlib

import usb.core
from tqdm import tqdm

from .device import Device, find_hid_devices

log = logging.getLogger(__name__)

VID_HOLTEK = 0x4d9
PID_BOOT_BIT = 0x1000

known_devices = {
    0x0167: "Vortex POK3R RGB",
    0x0207: "Vortex POK3R RGB2",
    0x0175: "Vortex Core",
    0x0192: "Vortex Race 3",
    0x0216: "Vortex ViBE",
    0x0282: "Vortex Cypher",
    0x0304: "Vortex Tab 60",
    0x0344: "Vortex Tab 75",
    0x0346: "Vortex Tab 90",
    0x0163: "Tex Yoda II",
    0x0143: "Mistel Barocco MD600",
    0x0200: "Mistel Freeboard MD200",  # same as Vortex 22-key Tester
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
# ?
CMD_READ_400 = 0
# ?
CMD_READ_3c00 = 1
# Get firmware mode
CMD_READ_MODE = 2
# Read version string
CMD_READ_VER = 0x20

CMD_FW = 0x1d
# Erase flash
CMD_FW_ERASE = 0
# Sum flash
CMD_FW_SUM = 1
# CRC flash
CMD_FW_CRC = 2

CMD_ADDR = 0x1e
CMD_ADDR_GET = 0
CMD_ADDR_SET = 1

# Write flash
CMD_WRITE = 0x1f
RESP_WRITE_ADDR = 2

ADDR_VERSION = 0x3000

version_magic = [
    0x00800004, 0x00010300, 0x00000041, 0xefffffff,
    0x00000001, 0x00000000, 0x016704d9, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0x001c5aa5,
]

# POK3R RGB XOR encryption/decryption key
# See fw_xor_decode.c for the way this key was obtained.
xor_key = [
    0xe7c29474,
    0x79084b10,
    0x53d54b0d,
    0xfc1e8f32,
    0x48e81a9b,
    0x773c808e,
    0xb7483552,
    0xd9cb8c76,
    0x2a8c8bc6,
    0x0967ada8,
    0xd4520f5c,
    0xd0c3279d,
    0xeac091c5,
]


def xor_encode_decode(encoded: bytes):
    assert len(encoded) % 4 == 0, "data size must be a multiple of 4"
    data = bytearray(encoded)
    for i in range(len(data) // 4):
        p = i * 4
        eword = data[p] | (data[p+1] << 8) | (data[p+2] << 16) | (data[p+3] << 24)
        dword = eword ^ xor_key[i % len(xor_key)]
        data[p] = dword & 0xFF
        data[p+1] = (dword >> 8) & 0xFF
        data[p+2] = (dword >> 16) & 0xFF
        data[p+3] = (dword >> 24) & 0xFF
    return bytes(data)


def decode_firmware(encoded: bytes) -> bytes:
    return xor_encode_decode(encoded)


def encode_firmware(decoded: bytes) -> bytes:
    return xor_encode_decode(decoded)


def dump_info_section(data: bytes):
    if data[:4] == b"\xff\xff\xff\xff":
        ver = "CLEARED"
    else:
        vlen = min(int.from_bytes(data[:4], "little"), 60)
        ver = data[4:vlen].decode("utf-16le").rstrip("\0")
    log.info(f"Version: {ver}")

    a = int.from_bytes(data[0x78:][:4], "little")
    b = int.from_bytes(data[0x7c:][:4], "little")
    c = int.from_bytes(data[0x80:][:4], "little")
    d = int.from_bytes(data[0x84:][:4], "little")
    e = int.from_bytes(data[0x88:][:4], "little")
    f = int.from_bytes(data[0x8c:][:4], "little")
    ivid = int.from_bytes(data[0x90:][:2], "little")
    ipid = int.from_bytes(data[0x92:][:2], "little")
    h = int.from_bytes(data[0xb0:][:4], "little")

    log.info(f"a: {a:#x}")
    log.info(f"b: {b:#x}")
    log.info(f"c: {c:#x}")
    log.info(f"d: {d:#x}")
    log.info(f"e: {e:#x}")
    log.info(f"f: {f:#x}")
    log.info(f"VID/PID: {ivid:#x}/{ipid:#x}")
    log.info(f"h: {h:#x}")


class CYKB_Device(Device):
    def is_bootloader(self):
        return self.dev.idProduct & PID_BOOT_BIT == PID_BOOT_BIT

    def version(self):
        _, vstr = self.read_version()
        return vstr or "CLEARED"

    def send_cmd(self, cmd: int, subcmd: int = 0, data: bytes = b""):
        pkt = struct.pack("<BBH", cmd, subcmd, 0) + data
        pkt = pkt.ljust(64, b"\0")
        self.send(pkt)

    def recv_resp(self, cmd: int, subcmd: int = 0) -> bytes:
        resp = self.recv(64)
        rcmd, rsubcmd, rcrc = struct.unpack_from("<BBH", resp)
        if rcmd != cmd or rsubcmd != subcmd:
            raise RuntimeError(f"Error response {rcmd:02x} {rsubcmd:02x}, expected {cmd:02x} {subcmd:02x}")
        if rcrc != 0:
            raise RuntimeError(f"Expected CRC 0")
        return resp[4:]

    def reboot(self, mode: int):
        log.debug(f"REBOOT {mode}")

        if self.is_bootloader():
            new_pid = self.dev.idProduct & ~PID_BOOT_BIT
        else:
            new_pid = self.dev.idProduct | PID_BOOT_BIT

        vid = self.dev.idVendor
        match_pids = {
            self.dev.idProduct: None,
            new_pid: None
        }

        log.info("Reboot...")
        self.send_cmd(CMD_RESET, mode)

        self.close()

        for i in range(3):
            time.sleep(1)

            new_devs = list(find_hid_devices(
                self.__class__,
                vid=vid,
                known_devices=match_pids,
                usage_page=UPDATE_USAGE_PAGE,
                usage=UPDATE_USAGE
            ))

            if len(new_devs) > 1:
                raise RuntimeError("Too many matching devices")
            elif not len(new_devs):
                log.warning("Waiting for device...")
            else:
                break
        else:
            raise RuntimeError("No matching device found")

        # kind of hacky
        name, new_dev = new_devs[0]
        self.replace(new_dev)

        if self.dev.idProduct != new_pid:
            raise RuntimeError("Reboot failed")

    def erase_flash(self, start: int, size: int):
        log.debug(f"ERASE {start:#x} {size} bytes")
        self.send_cmd(CMD_FW, CMD_FW_ERASE, struct.pack("<II", start, size))

        while True:
            try:
                self.recv_resp(CMD_FW, CMD_FW_ERASE)
                break
            except usb.core.USBTimeoutError:
                log.warning("Waiting for erase...")
                continue

    def write_flash(self, addr: int, data: bytes, *, progress=False):
        log.debug(f"WRITE {addr:#x} {len(data)} bytes")
        self.send_cmd(CMD_ADDR, CMD_ADDR_SET, struct.pack("<I", addr))
        self.recv_resp(CMD_ADDR, CMD_ADDR_SET)
        block_size = 52
        gen = range(0, len(data), block_size)
        if progress:
            gen = tqdm(gen)
        for p in gen:
            a = addr + p
            wdata = data[p:p+block_size]

            self.send_cmd(CMD_WRITE, len(wdata), wdata)
            resp = self.recv_resp(CMD_ADDR, RESP_WRITE_ADDR)
            waddr, = struct.unpack_from("<I", resp)
            if waddr != a + len(wdata):
                raise RuntimeError(f"Write response unexpected: {waddr} !+ {a + len(wdata)}")

    def crc_flash(self, addr: int, size: int):
        log.debug(f"CRC {addr:#x} {size} bytes")

        self.send_cmd(CMD_FW, CMD_FW_SUM, struct.pack("<II", addr, size))
        resp = self.recv_resp(CMD_FW, CMD_FW_SUM)
        csum, = struct.unpack_from("<I", resp)

        self.send_cmd(CMD_FW, CMD_FW_CRC, struct.pack("<II", addr, size))
        resp = self.recv_resp(CMD_FW, CMD_FW_CRC)
        crc, = struct.unpack_from("<I", resp)

        return csum, crc

    def read_version(self):
        # read the version page
        # we can read 0x3fc because (0x3c * 18) > 0x400
        ver = bytes()
        for i in range(17):
            self.send_cmd(CMD_READ, CMD_READ_VER + i)
            ver += self.recv_resp(CMD_READ, CMD_READ_VER + i)

        log.debug(f"ver: {ver.hex()}")

        vlen, = struct.unpack_from("<I", ver)
        if vlen == 0xffff_ffff:
            return vlen, None

        vstr = ver[4:][:vlen].rstrip(b"\xff").decode("utf-8").rstrip("\0")
        return vlen, vstr

    def write_version(self, version: str):
        vstr = version.encode("utf-8")
        vlen = len(vstr)
        vstr += b"\0"
        vdata = vlen.to_bytes(4, "little") + vstr.ljust(4 * ((len(vstr) + 3) // 4), b"\0")
        assert len(vdata) <= 0x3c * 2, "version too long"

        # this magic version data needs to be present or the bootloader will jump to the app
        vdata = vdata.ljust(0x3c * 2, b"\xff")
        for v in version_magic:
            vdata += v.to_bytes(4, "little")

        self.erase_flash(0, len(vdata))

        self.write_flash(0, vdata)

    def read_info(self):
        self.send_cmd(CMD_READ, CMD_READ_400)
        info1 = self.recv_resp(CMD_READ, CMD_READ_400)
        info1 = info1[:0x34]
        log.debug(f"info 400: {info1.hex()}")

        a, b, vid, pid, c, d, e, f, g, h, i, j, k, model, m = struct.unpack("<IIHHHHIIIIHHH10sI", info1)
        model = model.rstrip(b"\0").decode("ascii")

        log.debug(f"a: {a:#x}")
        log.debug(f"b: {b:#x}")
        log.debug(f"VID/PID: {vid:#x}/{pid:#x}")
        log.debug(f"c: {c:#x}")
        log.debug(f"d: {d:#x}")
        log.debug(f"e: {e:#x}")
        log.debug(f"f: {f:#x}")
        log.debug(f"g: {g:#x}")
        log.debug(f"h: {h:#x}")
        log.debug(f"i: {i:#x}")
        log.debug(f"j: {j:#x}")
        log.debug(f"k: {k:#x}")
        log.debug(f"Model: {model}")
        log.debug(f"m: {m:#x}")

        self.send_cmd(CMD_READ, CMD_READ_3c00)
        info2 = self.recv_resp(CMD_READ, CMD_READ_3c00)
        info2 = info2[:4]
        log.debug(f"info 3c00: {info2.hex()}")

        n, = struct.unpack("<I", info2)

        log.debug(f"n: {n:#x}")

        return info1, info2

    def flash(self, version: str, fw_data: bytes, *, progress=False):
        if not self.is_bootloader():
            self.reboot(CMD_RESET_BL)

        fw_offset = 0x400

        # erase version info
        self.erase_flash(0, 128)

        # pad to multiple of 4
        # fw_data = fw_data.ljust(4 * ((len(fw_data) + 3) // 4), b"\xff")

        crc0 = zlib.crc32(fw_data, 0)
        enc_fw_data = encode_firmware(fw_data)
        crc1 = zlib.crc32(enc_fw_data, 0)

        log.info("Erase...")
        self.erase_flash(fw_offset, len(enc_fw_data))

        log.info("Write...")
        self.write_flash(fw_offset, enc_fw_data, progress=progress)

        log.info("Verify...")
        csum, crc = self.crc_flash(fw_offset, len(fw_data))
        if crc != crc1:
            raise RuntimeError(f"CRC check failed: {crc:08x} / {csum:08x} != {crc0:08x} / {crc1:08x}")

        self.write_version(version)

        self.reboot(CMD_RESET_FW)

    def leak_flash(self):
        crc_map = {}
        # for i in range(0, 256):
        #     crc = binascii.crc_hqx(bytes([i]), 0)
        #     assert crc not in crc_map
        #     crc_map[crc] = i

        crc = self.crc_flash(0x400, 0x1000)

        data = bytearray(0x3000)
        # for addr in tqdm(range(0x400, 0x400 + len(data))):
        #     crc = self.crc_flash(addr, 4)
        #     assert crc in crc_map
        #     data[addr] = crc_map[crc]
        return data


def get_devices():
    bl_known_devices = {
        **known_devices,
        **{pid | PID_BOOT_BIT: f"{name} (bootloader)" for pid, name in known_devices.items()}
    }

    yield from find_hid_devices(
        CYKB_Device,
        vid=VID_HOLTEK,
        known_devices=bl_known_devices,
        usage_page=UPDATE_USAGE_PAGE,
        usage=UPDATE_USAGE
    )
