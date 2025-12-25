
import logging
import struct
import binascii
import time

import usb.core
from tqdm import tqdm

from .device import Device, find_hid_devices

log = logging.getLogger(__name__)

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x02

UPDATE_REPORT_SIZE = 64

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

ADDR_VERSION = 0x2800
ADDR_APP = 0x2c00

VID_HOLTEK = 0x4d9
PID_BOOT_BIT = 0x1000

known_devices = {
    0x0141: "Vortex POK3R",
    0x0112: "KBP V60",
    0x0129: "KBP V80",
}

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
    assert len(encoded) == 4 * 13
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


def decode_firmware(encoded: bytes) -> bytes:
    data = bytearray(encoded)
    for i in range(len(data) // 52):
        if 10 <= i <= 100:
            p = i * 52
            pkt = data[p:p + 52]
            pkt = decode_firmware_packet(pkt, i)
            data[p:p + 52] = pkt
    return bytes(data)


def encode_firmware_packet(encoded: bytes, num: int) -> bytes:
    assert len(encoded) == 4 * 13
    data = bytearray(encoded)

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


def encode_firmware(decoded: bytes) -> bytes:
    data = bytearray(decoded)
    for i in range(len(data) // 52):
        if 10 <= i <= 100:
            p = i * 52
            pkt = data[p:p + 52]
            pkt = encode_firmware_packet(pkt, i)
            data[p:p + 52] = pkt
    return bytes(data)


class POK3R_Device(Device):
    def is_bootloader(self):
        return self.dev.idProduct & PID_BOOT_BIT == PID_BOOT_BIT

    def version(self):
        _, vstr = self.read_version()
        return vstr or "CLEARED"

    def cmd(self, cmd: int, subcmd: int = 0, data: bytes = b""):
        pkt = struct.pack("<BBH", cmd, subcmd, 0) + data
        pkt = pkt.ljust(64, b"\x00")
        crc = binascii.crc_hqx(pkt, 0)
        pkt = pkt[:2] + crc.to_bytes(2, byteorder="little", signed=False) + pkt[4:]
        self.send(pkt)

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
        self.cmd(CMD_RESET, mode)

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


    def read_flash(self, addr: int, size: int):
        log.debug(f"READ {addr:#x} {size} bytes")
        data = bytes()
        for a in range(addr, addr+size, 64):
            self.cmd(CMD_FLASH, CMD_FLASH_READ, struct.pack("<II", a, a + 64))
            data += self.recv(64)
            resp = self.alt_recv(64)
            assert resp[0] == RESP_SUCCESS
        return data[:size]

    def write_flash(self, addr: int, data: bytes, *, progress=False):
        log.debug(f"WRITE {addr:#x} {len(data)} bytes")
        gen = range(0, len(data), 52)
        if progress:
            gen = tqdm(gen)
        for p in gen:
            a = addr + p
            wdata = data[p:p+52]
            self.cmd(CMD_FLASH, CMD_FLASH_WRITE, struct.pack("<II", a, a + len(wdata) - 1) + wdata)
            resp = self.alt_recv(64)
            assert resp[0] == RESP_SUCCESS

    def verify_flash(self, addr: int, data: bytes, *, progress=False):
        log.debug(f"VERIFY {addr:#x} {len(data)} bytes")
        gen = range(0, len(data), 52)
        if progress:
            gen = tqdm(gen)
        for p in gen:
            a = addr + p
            wdata = data[p:p+52]
            self.cmd(CMD_FLASH, CMD_FLASH_VERIFY, struct.pack("<II", a, a + len(wdata) - 1) + wdata)
            resp = self.alt_recv(64)
            assert resp[0] == RESP_SUCCESS

    def erase_flash(self, start: int, end: int):
        log.debug(f"ERASE {start:#x} to {end:#x}")
        self.cmd(CMD_ERASE, 0, struct.pack("<II", start, end))
        while True:
            try:
                resp = self.alt_recv(64)
                break
            except usb.core.USBTimeoutError:
                log.warning("Waiting for Erase...")
                continue
        assert resp[0] == RESP_SUCCESS

        self.cmd(CMD_FLASH, CMD_FLASH_ERASE_CHECK, struct.pack("<II", start, end))
        resp = self.alt_recv(64)
        assert resp[0] == RESP_SUCCESS

    def crc_flash(self, addr: int, size: int):
        log.debug(f"CRC {addr:#x} {size} bytes")
        self.cmd(CMD_CRC, 0, struct.pack("<II", addr, size))
        resp = self.alt_recv(64)
        assert resp[2] == RESP_SUCCESS
        return int.from_bytes(resp[:2], byteorder="little")

    def read_info(self):
        log.debug(f"GET INFO")
        self.cmd(CMD_GET_INFO)
        data = self.recv(64)
        resp = self.alt_recv(64)
        assert resp[0] == RESP_SUCCESS

        log.debug(f"info: {data.hex()}")
        a, b, fw_addr, page_size, e, f, ver_addr = struct.unpack_from("<HHHHHHI", data)
        # FMC + 0x1a4
        log.debug(f"a: {a:#x}")
        # 0x11000000
        log.debug(f"b: {b:#x}")
        log.debug(f"firmware address: {fw_addr:#x}")
        # FMC + 0x1ac
        log.debug(f"page size: {page_size:#x}")
        # (FMC + 0x1a8) - 10
        log.debug(f"e: {e:#x}")
        # (FMC + 0x1a8) - 10
        log.debug(f"f: {f:#x}")
        log.debug(f"version address: {ver_addr:#x}")
        return ver_addr, fw_addr

    def read_version(self):
        ver_addr, _ = self.read_info()
        # read the version string length
        vdata = self.read_flash(ver_addr, 4)

        vlen = int.from_bytes(vdata[:4], byteorder="little")
        if vlen == 0xffff_ffff:
            return vlen, None

        # read the whole version string
        vdata = self.read_flash(ver_addr + 4, vlen)
        vstr = vdata.rstrip(b"\xff").decode("utf-8").rstrip("\0")
        return vlen, vstr

    def write_version(self, version: str):
        ver_addr, _ = self.read_info()

        vstr = version.encode("utf-8")
        vlen = len(vstr)
        vstr += b"\0"
        vdata = vlen.to_bytes(4, "little") + vstr.ljust((len(vstr) * 3) // 4, b"\0")

        self.erase_flash(ver_addr, ver_addr + len(vdata))

        self.write_flash(ver_addr, vdata)

    def flash(self, version: str, fw_data: bytes, *, progress=False):
        if not self.is_bootloader():
            self.reboot(CMD_RESET_BOOT)

        ver_addr, fw_addr = self.read_info()

        # erase version info
        self.erase_flash(ver_addr, ver_addr + 128)

        crc0 = binascii.crc_hqx(fw_data, 0)
        crc02 = binascii.crc_hqx(fw_data, 0xffff)
        enc_fw_data = encode_firmware(fw_data)

        log.info("Erase...")
        self.erase_flash(fw_addr, fw_addr + len(enc_fw_data))

        log.info("Write...")
        self.write_flash(fw_addr, enc_fw_data, progress=progress)

        log.info("Verify...")
        self.verify_flash(fw_addr, enc_fw_data, progress=progress)

        crc1 = self.crc_flash(fw_addr, len(fw_data))
        if crc1 != crc0:
            raise RuntimeError(f"CRC check failed: {crc1:04x} ({crc02:04x}) != {crc0:04x}")

        self.write_version(version)

        self.reboot(CMD_RESET_SWITCH)

    def leak_flash(self):
        crc_map = {}
        for i in range(0, 256):
            crc = binascii.crc_hqx(bytes([i]), 0)
            assert crc not in crc_map
            crc_map[crc] = i

        data = bytearray(0x2800)
        for addr in tqdm(range(0, len(data))):
            crc = self.crc_flash(addr, 1)
            assert crc in crc_map
            data[addr] = crc_map[crc]
        return data


def get_devices():
    bl_known_devices = {}
    bl_known_devices |= known_devices
    bl_known_devices |= {pid | PID_BOOT_BIT: f"{name} (bootloader)" for pid, name in known_devices.items()}

    yield from find_hid_devices(
        POK3R_Device,
        vid=VID_HOLTEK,
        known_devices=bl_known_devices,
        usage_page=UPDATE_USAGE_PAGE,
        usage=UPDATE_USAGE
    )
