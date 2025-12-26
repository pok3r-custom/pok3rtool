
import logging

from .device import find_hid_devices
from .cykb import CYKB_Device
from .pok3r import POK3R_Device

log = logging.getLogger(__name__)

PID_BOOT_BIT = 0x1

known_devices = {
    (0x2516, 0x003c): "MasterKeys Pro S RGB",
}

UPDATE_USAGE_PAGE = 0xff00
UPDATE_USAGE = 0x02


class CMMK_Device(CYKB_Device):
    @staticmethod
    def decode_firmware(encoded: bytes) -> bytes:
        return POK3R_Device.decode_firmware(encoded)

    @staticmethod
    def encode_firmware(decoded: bytes) -> bytes:
        return POK3R_Device.encode_firmware(decoded)


def get_devices():
    bl_known_devices = {
        **known_devices,
        **{(vid, pid | PID_BOOT_BIT): f"{name} (bootloader)" for (vid, pid), name in known_devices.items()}
    }

    yield from find_hid_devices(
        CMMK_Device,
        known_devices=bl_known_devices,
        usage_page=UPDATE_USAGE_PAGE,
        usage=UPDATE_USAGE
    )
