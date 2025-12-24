
import logging

import hid

log = logging.getLogger(__name__)

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x01

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


class CYKB_Device:
    def __init__(self, dev: hid.Device, name: str | None = None):
        self.dev = dev
        self.name = name

    def __str__(self):
        return f"{self.name}: {self.version}"

    @property
    def version(self):
        v1, v2 = self.read_version()
        vlen = min(int.from_bytes(v1[4:8], byteorder="little"), 52)
        return v1[8:8+vlen].decode("UTF16").split("\x00")[0]

    def read_version(self):
        pkt = bytes([CMD_READ, CMD_READ_VER1] + [0] * 62)
        self.dev.write(pkt)
        ver1 = self.dev.read(64)
        log.debug(f"ver1: {ver1.hex()}")

        pkt = bytes([CMD_READ, CMD_READ_VER2] + [0] * 62)
        self.dev.write(pkt)
        ver2 = self.dev.read(64)
        log.debug(f"ver2: {ver2.hex()}")

        return ver1, ver2

    def read_info(self):
        pkt = bytes([CMD_READ, CMD_READ_400] + [0] * 62)
        self.dev.write(pkt)
        info1 = self.dev.read(64)
        log.debug(f"info1: {info1.hex()}")

        pkt = bytes([CMD_READ, CMD_READ_3c00] + [0] * 62)
        self.dev.write(pkt)
        info2 = self.dev.read(64)
        log.debug(f"info2: {info2.hex()}")

        return info1, info2


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
                    yield CYKB_Device(dev, name)
