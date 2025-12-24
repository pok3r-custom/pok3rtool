
import logging
import struct
import binascii

from .device import Device, find_devices

log = logging.getLogger(__name__)

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x02

UPDATE_REPORT_SIZE = 64

CMD_ERASE = 0
CMD_FLASH = 1
CMD_FLASH_CHECK = 0
CMD_FLASH_WRITE = 1
CMD_FLASH_READ = 2
CMD_CRC = 2
CMD_UPDATE_STATE = 3
CMD_RESET = 4

ADDR_VERSION = 0x2800
ADDR_APP = 0x2c00

VID_HOLTEK = 0x4d9
PID_BOOT_BIT = 0x1000

known_devices = {
    0x0141: "Vortex POK3R",
    0x0112: "KBP V60",
    0x0129: "KBP V80",
}


def crc16(data: bytes, poly=0x8408):
    '''
    CRC-16-CCITT Algorithm
    '''
    data = bytearray(data)
    crc = 0xFFFF
    for b in data:
        cur_byte = 0xFF & b
        for _ in range(0, 8):
            if (crc & 0x0001) ^ (cur_byte & 0x0001):
                crc = (crc >> 1) ^ poly
            else:
                crc >>= 1
            cur_byte >>= 1
    crc = (~crc & 0xFFFF)
    crc = (crc << 8) | ((crc >> 8) & 0xFF)

    return crc & 0xFFFF

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

    data = bytearray(len(encoded))
    for i in range(13):
        p = i * 4
        eword = encoded[p] | (encoded[p+1] << 8) | (encoded[p+2] << 16) | (encoded[p+3] << 24)
        dword = eword ^ xor_key[i]
        data[p] = dword & 0xFF
        data[p+1] = (dword >> 8) & 0xFF
        data[p+2] = (dword >> 16) & 0xFF
        data[p+3] = (dword >> 24) & 0xFF

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
    data = bytearray(len(encoded))
    for i in range(len(encoded) // 52):
        p = i * 52
        pkt = encoded[p:p + 52]
        if 10 <= i <= 100:
            pkt = decode_firmware_packet(pkt, i)
        data[p:p + 52] = pkt

    return bytes(data)


class POK3R_Device(Device):
    def version(self):
        ver = self.read_version()
        if ver[:4] == b"\xff\xff\xff\xff":
            return "CLEARED"
        vlen = min(int.from_bytes(ver[:4], byteorder="little"), 60)
        return ver[4:4+vlen].decode("UTF8").split("\x00")[0]

    def read(self, addr, size):
        pkt = struct.pack("<BBHII", CMD_FLASH, CMD_FLASH_READ, 0, addr, addr+64)
        pkt = pkt.ljust(64, b"\x00")
        crc = binascii.crc_hqx(pkt, 0)
        pkt = pkt[:2] + crc.to_bytes(2, byteorder="little", signed=False) + pkt[4:]
        self.send(pkt)
        data = self.recv(64)
        return data

    def read_version(self):
        ver = self.read(ADDR_VERSION, 64)
        log.debug(f"ver: {ver.hex()}")
        return ver


def get_devices():
    bl_known_devices = {}
    bl_known_devices |= known_devices
    bl_known_devices |= {pid | PID_BOOT_BIT: f"{name} (bootloader)" for pid, name in known_devices.items()}

    yield from find_devices(POK3R_Device, vid=VID_HOLTEK, known_devices=bl_known_devices, usage_page=UPDATE_USAGE_PAGE, usage=UPDATE_USAGE)
