
import logging
import struct
import time
import zlib

import usb.core
from tqdm import tqdm

from .device import Device, find_hid_devices

log = logging.getLogger(__name__)

PID_BOOT_BIT = 0x1000

known_devices = {
    (0x04d9, 0x0167): "Vortex POK3R RGB",
    (0x04d9, 0x0207): "Vortex POK3R RGB2",
    (0x04d9, 0x0175): "Vortex Core",
    (0x04d9, 0x0192): "Vortex Race 3",
    (0x04d9, 0x0216): "Vortex ViBE",
    (0x04d9, 0x0282): "Vortex Cypher",
    (0x04d9, 0x0304): "Vortex Tab 60",
    (0x04d9, 0x0344): "Vortex Tab 75",
    (0x04d9, 0x0346): "Vortex Tab 90",
    (0x04d9, 0x0163): "Tex Yoda II",
    (0x04d9, 0x0143): "Mistel Barocco MD600",
    (0x04d9, 0x0200): "Mistel Freeboard MD200",  # same as Vortex 22-key Tester
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


class CYKB_Device(Device):
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

    def is_bootloader(self):
        self.send_cmd(CMD_READ, CMD_READ_MODE)
        resp = self.recv_resp(CMD_READ, CMD_READ_MODE)
        return resp[0] == 0

    def reboot(self, bootloader: bool = False):
        log.debug(f"REBOOT {bootloader}")
        mode = CMD_RESET_BL if bootloader else CMD_RESET_FW

        _, _, vid, bpid = self.read_info()

        if bootloader:
            new_pid = bpid
        else:
            new_pid = bpid & ~PID_BOOT_BIT

        match_ids = {(self.dev.idVendor, self.dev.idProduct): None}
        match_ids[(vid, new_pid)] = None

        log.info("Reboot...")
        self.send_cmd(CMD_RESET, mode)

        self.close()

        for i in range(3):
            time.sleep(1)

            new_devs = list(find_hid_devices(
                self.__class__,
                known_devices=match_ids,
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

        if (self.dev.idVendor, self.dev.idProduct) != (vid, new_pid):
            raise RuntimeError("Reboot failed")

    def erase_flash(self, start: int, size: int):
        log.debug(f"ERASE {start:#x} {size} bytes")
        self.send_cmd(CMD_FW, CMD_FW_ERASE, struct.pack("<II", start, size))

        while True:
            time.sleep(0.5)
            try:
                self.recv_resp(CMD_FW, CMD_FW_ERASE)
                break
            except usb.core.USBTimeoutError:
                log.warning("Waiting for erase...")
                continue

    def read_flash(self, addr: int, size: int, *, progress=False):
        """
        Arbitrary address read is only available via patched firmware
        """
        log.debug(f"READ {addr:#x} {size} bytes")
        data = bytes()
        block_size = 60
        gen = range(0, size, block_size)
        if progress:
            gen = tqdm(gen)
        for p in gen:
            a = addr + p
            self.send_cmd(CMD_READ, 0xff, struct.pack("<I", a))
            resp = self.recv_resp(CMD_READ, 0xff)
            data += resp[:60]
        return data

    def write_flash(self, addr: int, data: bytes, *, progress=False):
        log.debug(f"WRITE {addr:#x} {len(data)} bytes")
        self.send_cmd(CMD_ADDR, CMD_ADDR_SET, struct.pack("<I", addr))
        self.recv_resp(CMD_ADDR, CMD_ADDR_SET)

        self.send_cmd(CMD_ADDR, CMD_ADDR_GET, struct.pack("<I", 0xffff_ffff))
        resp = self.recv_resp(CMD_ADDR, CMD_ADDR_GET)
        raddr, = struct.unpack_from("<I", resp)
        if raddr != addr:
            raise RuntimeError(f"Set addresss response unexpected: {raddr} != {addr}")

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
                raise RuntimeError(f"Write response unexpected: {waddr} != {a + len(wdata)}")

    def crc_flash(self, addr: int, size: int):
        log.debug(f"CRC {addr:#x} {size} bytes")

        self.send_cmd(CMD_FW, CMD_FW_SUM, struct.pack("<II", addr, size))
        time.sleep(0.1)
        resp = self.recv_resp(CMD_FW, CMD_FW_SUM)
        csum, = struct.unpack_from("<I", resp)

        self.send_cmd(CMD_FW, CMD_FW_CRC, struct.pack("<II", addr, size))
        time.sleep(0.1)
        resp = self.recv_resp(CMD_FW, CMD_FW_CRC)
        crc, = struct.unpack_from("<I", resp)

        return csum, crc

    def read_info(self):
        self.send_cmd(CMD_READ, CMD_READ_400)
        info1 = self.recv_resp(CMD_READ, CMD_READ_400)
        info1 = info1[:0x34]
        log.debug(f"bootloader info: {info1.hex()}")

        a, b, vid, pid, c, d, e, f, g, h, fw_offset, fw_pages, page_size, model, m = struct.unpack("<IIHHHHIIIIHHH10sI", info1)
        model = model.rstrip(b"\0").decode("ascii")

        log.debug(f"a: {a:#x}")
        log.debug(f"b: {b:#x}")
        log.debug(f"BOOT VID/PID: {vid:#x}/{pid:#x}")
        log.debug(f"c: {c:#x}")
        log.debug(f"d: {d:#x}")
        log.debug(f"e: {e:#x}")
        log.debug(f"f: {f:#x}")
        log.debug(f"g: {g:#x}")
        log.debug(f"h: {h:#x}")
        log.debug(f"firmware offset?: {fw_offset:#x}")
        log.debug(f"firmware pages: {fw_pages:#x}")
        log.debug(f"page size?: {page_size:#x}")
        log.debug(f"Model: {model}")
        log.debug(f"m: {m:#x}")

        # i'm not sure which is which, so make sure they're the same
        assert fw_offset == page_size

        fw_size = fw_pages * page_size

        log.debug(f"Firmware Size: {fw_size:#x}")

        self.send_cmd(CMD_READ, CMD_READ_3c00)
        info2 = self.recv_resp(CMD_READ, CMD_READ_3c00)
        info2 = info2[:4]
        log.debug(f"app info: {info2.hex()}")

        n, o = struct.unpack("<HH", info2)

        log.debug(f"n: {n:#x}")
        log.debug(f"o: {o:#x}")

        return fw_offset, fw_size, vid, pid

    def read_version(self):
        fw_offset, _, _, _ = self.read_info()

        # read the version page
        vdata = bytes()
        for i in range(fw_offset // 60):
            self.send_cmd(CMD_READ, CMD_READ_VER + i)
            vdata += self.recv_resp(CMD_READ, CMD_READ_VER + i)
        log.debug(f"ver: {vdata.hex()}")

        vmagic, = struct.unpack_from("<I", vdata[0xB0:])
        magic = vmagic & 0x3ffff
        num_vals = magic >> 18

        log.debug(f"magic: {magic:#x}")

        a, v3, v2, v1, v0, c, d, e, f, vid, pid = struct.unpack_from("<IBBBBIIIIHH", vdata[0x78:])

        log.debug(f"a: {a:#x}")
        log.debug(f"version: {v0}.{v1}.{v2}.{v3}")
        log.debug(f"c: {c:#x}")
        log.debug(f"d: {d:#x}")
        log.debug(f"e: {e:#x}")
        log.debug(f"f: {f:#x}")
        log.debug(f"VID/PID: {vid:#x}/{pid:#x}")

        vlen, = struct.unpack_from("<I", vdata)
        if vlen == 0xffff_ffff:
            return vlen, None

        vstr = vdata[4:][:vlen].rstrip(b"\xff").decode("utf-8").rstrip("\0")
        return vlen, vstr

    def version(self):
        _, vstr = self.read_version()
        return vstr or "CLEARED"

    def write_version(self, version: str):
        vstr = version.encode("utf-8")
        vlen = len(vstr)
        vstr += b"\0"
        vdata = vlen.to_bytes(4, "little") + vstr.ljust(4 * ((len(vstr) + 3) // 4), b"\0")
        assert len(vdata) <= 0x78, "version too long"

        _, _, bvid, bpid = self.read_info()

        ver = (1,0,0)

        # the bootloader checks that this list of words is non-FF
        # i don't know what else, if anything, uses these values
        vvalues = [
            0x800004,
            (ver[0] << 16) | (ver[1] << 8) | ver[2],
            0x41,
            0xefffffff,
            0x1,
            0x0,
            bvid | ((bpid & ~PID_BOOT_BIT) << 16)
        ]

        vdata = vdata.ljust(0x78, b"\xff")
        for v in vvalues:
            vdata += v.to_bytes(4, "little")

        # this magic value needs to be present
        magic = 0x5aa5 | (len(vvalues) << 18)
        vdata = vdata.ljust(0xB0, b"\xff")
        vdata += magic.to_bytes(4, "little")

        self.erase_flash(0, len(vdata))

        self.write_flash(0, vdata)

    @staticmethod
    def decode_firmware(encoded: bytes) -> bytes:
        return xor_encode_decode(encoded)

    @staticmethod
    def encode_firmware(decoded: bytes) -> bytes:
        return xor_encode_decode(decoded)

    def flash(self, version: str, fw_data: bytes, *, progress=False):
        enc_fw_data = self.encode_firmware(fw_data)
        # the CRC command returns the CRC of the encrypted data
        crc1 = zlib.crc32(enc_fw_data, 0)

        if not self.is_bootloader():
            self.reboot(True)

        fw_offset, max_fw_size, _, _ = self.read_info()

        assert len(fw_data) <= max_fw_size, "firmware too large"

        log.info("Erase...")
        self.erase_flash(0, max_fw_size)

        log.info("Write...")
        self.write_flash(fw_offset, enc_fw_data, progress=progress)

        _, crc = self.crc_flash(fw_offset, len(fw_data))
        if crc != crc1:
            raise RuntimeError(f"CRC check failed: {crc:08x} != {crc1:08x}")

        self.write_version(version)

        self.reboot()

    def dump(self):
        log.info("Dumping flash with patched command...")

        # A fairly simple patch to the firmware introduces a new subcommand to the READ command
        # that will let us read arbitrary memory addresses
        # See:
        # https://github.com/pok3r-custom/pok3r_re_firmware/blob/master/disassemble/pok3r_rgb/v130/patch_v130.txt
        # https://github.com/pok3r-custom/pok3r_re_firmware/blob/master/disassemble/pok3r_rgb/v130/firmware_v130.txt#L4710

        # HT32F1654 is 64K
        flash_size = 0x10000
        # HT32F52352 is 128K
        # flash_size = 0x20000

        # read up until the last 60-byte chunk that would overflow the flash and fault the firmware
        read_size = 60 * (flash_size // 60)
        data = self.read_flash(0, read_size, progress=True)
        # read the last chunk
        read_addr = flash_size - 60
        data = data[:read_addr] + self.read_flash(read_addr, 60)

        return data


def get_devices():
    bl_known_devices = {
        **known_devices,
        **{(vid, pid | PID_BOOT_BIT): f"{name} (bootloader)" for (vid, pid), name in known_devices.items()}
    }

    yield from find_hid_devices(
        CYKB_Device,
        known_devices=bl_known_devices,
        usage_page=UPDATE_USAGE_PAGE,
        usage=UPDATE_USAGE
    )
