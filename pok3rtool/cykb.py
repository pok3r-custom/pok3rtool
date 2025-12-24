
import logging

from .device import Device, find_devices

log = logging.getLogger(__name__)

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x02

UPDATE_REPORT_SIZE = 64

CMD_RESET = 0x11
CMD_READ = 0x12
CMD_READ_400 = 0
CMD_READ_3c00 = 1
CMD_READ_MODE = 2
CMD_READ_VER1 = 0x20
CMD_READ_VER2 = 0x22
CMD_FW = 0x1d
CMD_ADDR = 0x1e
CMD_WRITE = 0x1f

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

# POK3R RGB XOR encryption/decryption key
# Somone somewhere thought a random XOR key was any better than the one they
# used in the POK3R firmware. Yeah, good one.
# See fw_xor_decode.c for the hilarious way this key was obtained.
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
    data = bytearray(len(encoded))
    for i in range(len(data) // 4):
        p = i * 4
        eword = encoded[p] | (encoded[p+1] << 8) | (encoded[p+2] << 16) | (encoded[p+3] << 24)
        dword = eword ^ xor_key[i % len(xor_key)]
        data[p] = dword & 0xFF
        data[p+1] = (dword >> 8) & 0xFF
        data[p+2] = (dword >> 16) & 0xFF
        data[p+3] = (dword >> 24) & 0xFF
    return bytes(data)


def decode_firmware(encoded: bytes) -> bytes:
    return xor_encode_decode(encoded)


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
    def version(self):
        v1, v2 = self.read_version()
        vlen = min(int.from_bytes(v1[4:8], byteorder="little"), 52)
        return v1[8:8+vlen].decode("UTF16").split("\x00")[0]

    def read_version(self):
        pkt = bytes([CMD_READ, CMD_READ_VER1] + [0] * 62)
        self.send(pkt)
        ver1 = self.recv(64)
        log.debug(f"ver1: {ver1.hex()}")

        pkt = bytes([CMD_READ, CMD_READ_VER2] + [0] * 62)
        self.send(pkt)
        ver2 = self.recv(64)
        log.debug(f"ver2: {ver2.hex()}")

        return ver1, ver2

    def read_info(self):
        pkt = bytes([CMD_READ, CMD_READ_400] + [0] * 62)
        self.send(pkt)
        info1 = self.recv(64)
        log.debug(f"info1: {info1.hex()}")

        pkt = bytes([CMD_READ, CMD_READ_3c00] + [0] * 62)
        self.send(pkt)
        info2 = self.recv(64)
        log.debug(f"info2: {info2.hex()}")

        return info1, info2


def get_devices():
    bl_known_devices = {}
    bl_known_devices |= known_devices
    bl_known_devices |= {pid | PID_BOOT_BIT: f"{name} (bootloader)" for pid, name in known_devices.items()}

    yield from find_devices(CYKB_Device, vid=VID_HOLTEK, known_devices=bl_known_devices, usage_page=UPDATE_USAGE_PAGE, usage=UPDATE_USAGE)
