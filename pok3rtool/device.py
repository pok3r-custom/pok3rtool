
import logging
import warnings

import usb
import hid_parser

log = logging.getLogger(__name__)

INTERFACE_CLASS_HID = 3

USB_DIR_IN = 0x80
USB_TYPE_STANDARD = 0x00
USB_RECIP_IFACE = 0x01
GET_DESCRIPTOR = 0x06

DESC_TYPE_REPORT = 0x22


def get_hid_report_descriptor(dev: usb.core.Device, intf_num: int, length: int = 4096) -> bytes:
    data = dev.ctrl_transfer(
        USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_IFACE,
        GET_DESCRIPTOR,
        (DESC_TYPE_REPORT << 8) | 0,
        intf_num,
        length,
        )
    return bytes(data)


def find_devices(cls,
                 vid: int | None = None,
                 pid: int | None = None,
                 known_devices: dict | None = None,
                 usage_page: int | None = None,
                 usage: int | None = None
                 ):
    known_devices = known_devices or {}
    for dev in usb.core.find(find_all=True):
        # log.debug(f"device {repr(dev)}")
        if vid is None or vid == dev.idVendor:
            if (pid is not None and pid == dev.idProduct) or dev.idProduct in known_devices:
                cfg = dev[0]

                # query each HID interface to find the update interface
                for intf in usb.util.find_descriptor(cfg, find_all=True, bInterfaceClass=INTERFACE_CLASS_HID):
                    # log.debug(f"interface {repr(intf)}")

                    # Ensure configuration is set
                    if dev.get_active_configuration() is None:
                        dev.set_configuration()

                    if dev.is_kernel_driver_active(intf.bInterfaceNumber):
                        dev.detach_kernel_driver(intf.bInterfaceNumber)

                    try:
                        usb.util.claim_interface(dev, intf.bInterfaceNumber)
                    except:
                        log.error("failed to claim interface")
                        continue

                    matched_interface = None
                    try:
                        try:
                            hid_desc = get_hid_report_descriptor(dev, intf.bInterfaceNumber)
                        except:
                            log.exception(f"failed to read descriptor")
                        else:
                            # log.debug(f"descriptor {hid_desc.hex()}")
                            with warnings.catch_warnings():
                                # suppress warnings from python-hid-parser
                                warnings.simplefilter("ignore")
                                rdesc = hid_parser.ReportDescriptor(hid_desc)
                                try:
                                    for item in rdesc.get_input_items():
                                        if isinstance(item, hid_parser.VariableItem):
                                            # log.debug(f"item {item}")
                                            if usage_page is None or item.usage.page == usage_page:
                                                if usage is None or item.usage.usage == usage:
                                                    matched_interface = intf
                                                    break
                                except:
                                    log.warning(f"failed to parse report descriptor")
                    finally:
                        usb.util.release_interface(dev, intf.bInterfaceNumber)

                    if matched_interface is not None:
                        name = None
                        if dev.idProduct in known_devices:
                            name = known_devices[dev.idProduct]
                        log.debug(f"matched {repr(dev)} {repr(matched_interface)} -> {name}")
                        yield name, cls(dev, matched_interface)
                        break


class Device:
    def __init__(self, dev: usb.Device, intf: usb.Interface):
        self.dev: usb.Device = dev
        self.intf: usb.Interface = intf
        self.ep_in: usb.Endpoint = None
        self.ep_out: usb.Endpoint = None

    def __enter__(self):
        try:
            if self.dev.is_kernel_driver_active(self.intf.bInterfaceNumber):
                self.dev.detach_kernel_driver(self.intf.bInterfaceNumber)
        except:
            log.error("failed to detach kernel driver")
            raise

        try:
            usb.util.claim_interface(self.dev, self.intf.bInterfaceNumber)
        except:
            log.error("failed to claim interface")
            raise

        self.ep_in = usb.util.find_descriptor(
            self.intf,
            custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN and usb.util.endpoint_type(e.bmAttributes) == usb.util.ENDPOINT_TYPE_INTR,
        )

        self.ep_out = usb.util.find_descriptor(
            self.intf,
            custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT and usb.util.endpoint_type(e.bmAttributes) == usb.util.ENDPOINT_TYPE_INTR,
        )

        if not self.ep_in:
            raise RuntimeError("Could not find ep_in")
        if not self.ep_out:
            raise RuntimeError("Could not find ep_out")

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        usb.util.release_interface(self.dev, self.intf.bInterfaceNumber)

    def send(self, pkt):
        # self.dev.write(self.ep_out.bEndpointAddress, pkt, 100)
        self.ep_out.write(pkt, 500)

    def recv(self, size):
        # return self.dev.read(self.ep_in.bEndpointAddress, size, 100)
        return bytes(self.ep_in.read(size, 500))
