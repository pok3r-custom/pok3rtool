
import logging
import struct
import binascii
import time

import usb.core
from tqdm import tqdm

from .device import Device, find_hid_devices

log = logging.getLogger(__name__)

PID_BOOT_BIT = 0x1000

known_devices = {
    (0x04d9, 0x0141): "Vortex POK3R",
    (0x04d9, 0x0112): "KBP V60",
    (0x04d9, 0x0129): "KBP V80",
}

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x02

# Erase flash pages
CMD_ERASE = 0

CMD_FLASH = 1
# Verify flash against provided data (words)
CMD_FLASH_VERIFY = 0
# Write flash with provided data (words)
CMD_FLASH_WRITE = 1
# Read flash at given address
CMD_FLASH_READ = 2
# Check if flash (words) at given address and size are erased
CMD_FLASH_ERASE_CHECK = 3

# CRC-16 of given address and size
# No restrictions on address or size
CMD_CRC = 2

# Return some info about the flash on the device
CMD_GET_INFO = 3

CMD_RESET = 4
# Reboot to the other firmware (if valid)
CMD_RESET_SWITCH = 0
# Reboot to the bootloader
CMD_RESET_BOOT = 1

# Release the DP pull-up and wait until watchdog reset
CMD_DISCONNECT = 5

RESP_SUCCESS = 0x4f

VERSION_SIZE = 0x400
# HT32F1655 is 128K
FLASH_SIZE = 0x20000

# POK3R firmware XOR encryption/decryption key
# Found at 0x2188 in Pok3r flash
xor_key = [
    0x55aa55aa,
    0xaa55aa55,
    0x000000ff,
    0x0000ff00,
    0x00ff0000,
    0xff000000,
    0x00000000,
    0xffffffff,
    0x0f0f0f0f,
    0xf0f0f0f0,
    0xaaaaaaaa,
    0x55555555,
    0x00000000,
]

# This array was painstakingly translated from a switch with a lot of shifts in the firmware.
# I noticed after the fact that it was identical to the array that Sprite used in his hack,
# but the groups of offsets were in a rotated order. Oh well.
swap_key = [
    0,1,2,3,
    1,2,3,0,
    2,1,3,0,
    3,2,1,0,
    3,1,0,2,
    1,2,0,3,
    2,3,1,0,
    0,2,1,3,
]


def decode_firmware_packet(encoded: bytes, num: int) -> bytes:
    assert len(encoded) == 4 * 13, "data must be 52 bytes"
    data = bytearray(encoded)

    # XOR encryption
    for i in range(13):
        p = i * 4
        eword = data[p] | (data[p+1] << 8) | (data[p+2] << 16) | (data[p+3] << 24)
        dword = eword ^ xor_key[i]
        data[p] = dword & 0xFF
        data[p+1] = (dword >> 8) & 0xFF
        data[p+2] = (dword >> 16) & 0xFF
        data[p+3] = (dword >> 24) & 0xFF

    # Swap encryption
    f = (num & 7) << 2
    for i in range(0, len(data), 4):
        a = data[i + swap_key[f + 0]]
        b = data[i + swap_key[f + 1]]
        c = data[i + swap_key[f + 2]]
        d = data[i + swap_key[f + 3]]

        data[i + 0] = a
        data[i + 1] = b
        data[i + 2] = c
        data[i + 3] = d

    return bytes(data)


def encode_firmware_packet(decoded: bytes, num: int) -> bytes:
    assert len(decoded) == 4 * 13, "data must be 52 bytes"
    data = bytearray(decoded)

    # Swap encryption
    f = (num & 7) << 2
    for i in range(0, len(data), 4):
        a = data[i + 0]
        b = data[i + 1]
        c = data[i + 2]
        d = data[i + 3]

        data[i + swap_key[f + 0]] = a
        data[i + swap_key[f + 1]] = b
        data[i + swap_key[f + 2]] = c
        data[i + swap_key[f + 3]] = d

    # XOR encryption
    for i in range(13):
        p = i * 4
        eword = data[p] | (data[p+1] << 8) | (data[p+2] << 16) | (data[p+3] << 24)
        dword = eword ^ xor_key[i]
        data[p] = dword & 0xFF
        data[p+1] = (dword >> 8) & 0xFF
        data[p+2] = (dword >> 16) & 0xFF
        data[p+3] = (dword >> 24) & 0xFF

    return bytes(data)


class POK3R_Device(Device):
    def send_cmd(self, cmd: int, subcmd: int = 0, data: bytes = b""):
        pkt = struct.pack("<BBH", cmd, subcmd, 0) + data
        pkt = pkt.ljust(64, b"\0")
        crc = binascii.crc_hqx(pkt, 0)
        pkt = pkt[:2] + crc.to_bytes(2, byteorder="little") + pkt[4:]
        self.send(pkt)

    def recv_resp(self, size: int = 0) -> bytes:
        resp = self.alt_recv(64)
        data = resp[:size]
        if resp[size] != RESP_SUCCESS:
            raise RuntimeError(f"Error response {resp[0]:02x}")
        return data

    def is_bootloader(self):
        return self.dev.idProduct & PID_BOOT_BIT == PID_BOOT_BIT

    def reboot(self, bootloader: bool = False):
        log.debug(f"REBOOT {bootloader}")
        mode = CMD_RESET_BOOT if bootloader else CMD_RESET_SWITCH

        if bootloader:
            new_pid = self.dev.idProduct | PID_BOOT_BIT
        elif self.is_bootloader():
            new_pid = self.dev.idProduct & ~PID_BOOT_BIT
        else:
            new_pid = self.dev.idProduct | PID_BOOT_BIT

        match_ids = {(self.dev.idVendor, self.dev.idProduct): None}
        vid = self.dev.idVendor
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

    def read_flash(self, addr: int, size: int, *, progress=False):
        log.debug(f"READ {addr:#x} {size} bytes")
        data = bytes()
        block_size = 64
        gen = range(addr, addr+size, block_size)
        if progress:
            gen = tqdm(gen)
        for a in gen:
            self.send_cmd(CMD_FLASH, CMD_FLASH_READ, struct.pack("<II", a, a + block_size))
            data += self.recv(64)[:block_size]
            self.recv_resp()
        return data[:size]

    def erase_flash(self, start: int, end: int):
        log.debug(f"ERASE {start:#x} to {end:#x}")
        self.send_cmd(CMD_ERASE, 0, struct.pack("<II", start, end))

        while True:
            try:
                self.recv_resp()
                break
            except usb.core.USBTimeoutError:
                log.warning("Waiting for erase...")
                continue

        self.send_cmd(CMD_FLASH, CMD_FLASH_ERASE_CHECK, struct.pack("<II", start, end))
        self.recv_resp()

    def write_flash(self, addr: int, data: bytes, *, progress=False):
        log.debug(f"WRITE {addr:#x} {len(data)} bytes")
        block_size = 52
        gen = range(0, len(data), block_size)
        if progress:
            gen = tqdm(gen)
        for p in gen:
            a = addr + p
            wdata = data[p:p+block_size]
            self.send_cmd(CMD_FLASH, CMD_FLASH_WRITE, struct.pack("<II", a, a + len(wdata) - 1) + wdata)
            self.recv_resp()

    def verify_flash(self, addr: int, data: bytes, *, progress=False):
        log.debug(f"VERIFY {addr:#x} {len(data)} bytes")
        block_size = 52
        gen = range(0, len(data), block_size)
        if progress:
            gen = tqdm(gen)
        for p in gen:
            a = addr + p
            wdata = data[p:p+block_size]
            self.send_cmd(CMD_FLASH, CMD_FLASH_VERIFY, struct.pack("<II", a, a + len(wdata) - 1) + wdata)
            self.recv_resp()

    def crc_flash(self, addr: int, size: int):
        log.debug(f"CRC {addr:#x} {size} bytes")
        self.send_cmd(CMD_CRC, 0, struct.pack("<II", addr, size))
        resp = self.recv_resp(2)
        return int.from_bytes(resp, byteorder="little")

    def read_info(self):
        log.debug(f"GET INFO")
        self.send_cmd(CMD_GET_INFO)
        data = self.recv(64)
        self.recv_resp()

        log.debug(f"info: {data.hex()}")
        a, b, fw_addr, page_size, e, f, ver_addr = struct.unpack_from("<HHHHHHI", data)

        # FMC + 0x1a4
        # appears to be the chip model, 0x1655 -> HT32F1655
        log.debug(f"a: {a:#x}")
        # 0x1100
        log.debug(f"b: {b:#x}")
        log.debug(f"firmware address: {fw_addr:#x}")
        # FMC + 0x1ac
        log.debug(f"page size?: {page_size:#x}")
        # (FMC + 0x1a8) - 10
        log.debug(f"e: {e:#x}")
        # (FMC + 0x1a8) - 10
        log.debug(f"f: {f:#x}")
        log.debug(f"version address: {ver_addr:#x}")

        return ver_addr, fw_addr

    def read_version(self):
        ver_addr, _ = self.read_info()
        # read the version page
        vdata = self.read_flash(ver_addr, 0x400)
        log.debug(f"ver: {vdata.hex()}")

        vlen = int.from_bytes(vdata[:4], byteorder="little")
        if vlen == 0xffff_ffff:
            return vlen, None

        vstr = vdata[4:][:vlen].rstrip(b"\xff").decode("utf-8").rstrip("\0")
        return vlen, vstr

    def version(self):
        _, vstr = self.read_version()
        return vstr or "CLEARED"

    def write_version(self, version: str):
        ver_addr, app_addr = self.read_info()

        vstr = version.encode("utf-8")
        vlen = len(vstr)
        vstr += b"\0"
        vdata = vlen.to_bytes(4, "little") + vstr.ljust(4 * ((len(vstr) + 3) // 4), b"\0")
        assert len(vdata) < (app_addr - ver_addr), "version too long"

        self.erase_flash(ver_addr, ver_addr + len(vdata))

        self.write_flash(ver_addr, vdata)

    @staticmethod
    def decode_firmware(encoded: bytes) -> bytes:
        data = bytearray(encoded)
        for i in range(len(data) // 52):
            if 10 <= i <= 100:
                p = i * 52
                pkt = data[p:p + 52]
                pkt = decode_firmware_packet(pkt, i)
                data[p:p + 52] = pkt
        return bytes(data)

    @staticmethod
    def encode_firmware(decoded: bytes) -> bytes:
        data = bytearray(decoded)
        for i in range(len(data) // 52):
            if 10 <= i <= 100:
                p = i * 52
                pkt = data[p:p + 52]
                pkt = encode_firmware_packet(pkt, i)
                data[p:p + 52] = pkt
        return bytes(data)

    def flash(self, version: str, fw_data: bytes, *, progress=False):
        crc0 = binascii.crc_hqx(fw_data, 0)
        enc_fw_data = self.encode_firmware(fw_data)

        if not self.is_bootloader():
            self.reboot(True)

        ver_addr, fw_addr = self.read_info()

        assert len(fw_data) <= (FLASH_SIZE - fw_addr), "firmware too large"

        log.info("Erase...")
        self.erase_flash(ver_addr, fw_addr + len(enc_fw_data))

        log.info("Write...")
        self.write_flash(fw_addr, enc_fw_data, progress=progress)

        log.info("Verify...")
        self.verify_flash(fw_addr, enc_fw_data, progress=progress)

        crc = self.crc_flash(fw_addr, len(fw_data))
        if crc != crc0:
            raise RuntimeError(f"CRC check failed: {crc:04x} != {crc0:04x}")

        self.write_version(version)

        self.reboot()

    def dump(self):
        log.info("Dumping flash with CRC leak...")

        # the CRC command produces a CRC-16 for ANY memory or flash address and size
        # we can use this to leak the value of any one byte per CRC command by
        # pre-computing a lookup of CRC values to byte values
        # this lets us dump the full flash at like ~380 Bps

        crc_map = {}
        for i in range(0, 256):
            crc = binascii.crc_hqx(bytes([i]), 0)
            assert crc not in crc_map
            crc_map[crc] = i

        data = bytearray(FLASH_SIZE)
        for addr in tqdm(range(0, len(data))):
            crc = self.crc_flash(addr, 1)
            assert crc in crc_map
            data[addr] = crc_map[crc]
        return data


def get_devices():
    bl_known_devices = {
        **known_devices,
        **{(vid, pid | PID_BOOT_BIT): f"{name} (bootloader)" for (vid, pid), name in known_devices.items()}
    }

    yield from find_hid_devices(
        POK3R_Device,
        known_devices=bl_known_devices,
        usage_page=UPDATE_USAGE_PAGE,
        usage=UPDATE_USAGE
    )
