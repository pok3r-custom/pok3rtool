
import logging

import hid

log = logging.getLogger(__name__)

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x01

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


class POK3R_Device:
    def __init__(self, dev: hid.Device, name: str | None = None):
        self.dev = dev
        self.name = name

    def __str__(self):
        return f"{self.name}: {self.version}"

    @property
    def version(self):
        ver = self.read_version()
        vlen = min(int.from_bytes(ver[:4], byteorder="little"), 60)
        return ver[4:4+vlen].decode("UTF8").split("\x00")[0]

    def read(self, addr, size):
        pkt = bytes([CMD_FLASH, CMD_FLASH_READ] + [0] * 2) + addr.to_bytes(4, byteorder="little") + (addr + 64).to_bytes(4, byteorder="little")
        crc = crc16(pkt)
        pkt = pkt[:2] + crc.to_bytes(2, byteorder="little", signed=False) + pkt[4:]
        self.dev.write(pkt)
        data = self.dev.read(64)
        return data

    def read_version(self):
        ver = self.read(ADDR_VERSION, 64)
        log.debug(f"ver: {ver.hex()}")
        return ver


def get_devices(vid: int | None = None, pid: int | None = None):
    # clown-ass hidapi wrapper
    for dev_params in hid.enumerate(0, 0):
        if ((vid is not None and vid == dev_params["vendor_id"]) or
                dev_params["vendor_id"] == VID_HOLTEK):
            if ((pid is not None and pid == dev_params["product_id"]) or
                    dev_params["product_id"] in known_devices.keys() or
                    dev_params["product_id"] in [ p | PID_BOOT_BIT for p in known_devices.keys() ]):
                if dev_params["usage_page"] == UPDATE_USAGE_PAGE and dev_params["usage"] == UPDATE_USAGE:
                    log.debug(dev_params)
                    dev = hid.Device(path=dev_params["path"])
                    name=known_devices[dev_params["product_id"]]
                    yield POK3R_Device(dev, name)
