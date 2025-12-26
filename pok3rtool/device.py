import errno
import logging
import time
import warnings

import usb
import hid_parser

log = logging.getLogger(__name__)

USBTRACE = logging.DEBUG - 5
logging.addLevelName(USBTRACE, "USBTRACE")

USB_TIMEOUT = 500

INTERFACE_CLASS_HID = 3

USB_DIR_IN = 0x80
USB_TYPE_STANDARD = 0x00
USB_TYPE_CLASS = 0x20
USB_RECIP_IFACE = 0x01

GET_DESCRIPTOR = 0x06

HID_GET_REPORT = 0x01
HID_SET_REPORT = 0x09

DESC_TYPE_HID = 0x21
DESC_TYPE_REPORT = 0x22

REPORT_TYPE_INPUT = 0x01
REPORT_TYPE_OUTPUT = 0x02

def hid_get_descriptor(dev: usb.core.Device, intf_num: int, length: int = 4096) -> bytes:
    desc_type = DESC_TYPE_REPORT
    data = dev.ctrl_transfer(
        USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_IFACE,
        GET_DESCRIPTOR,
        (desc_type << 8) | 0,
        intf_num,
        length,
        USB_TIMEOUT
    )
    return bytes(data)


def hid_get_report(dev: usb.core.Device, intf_num: int, length: int = 4096) -> bytes:
    report_type = REPORT_TYPE_INPUT
    data = dev.ctrl_transfer(
        USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_IFACE,
        HID_GET_REPORT,
        (report_type << 8) | 0,
        intf_num,
        length,
        USB_TIMEOUT
    )
    return bytes(data)


def find_hid_devices(cls,
                     vid: int | None = None,
                     pid: int | None = None,
                     known_devices: dict | None = None,
                     usage_page: int | None = None,
                     usage: int | None = None
                     ):
    known_devices = known_devices or {}
    for dev in usb.core.find(find_all=True):
        log.log(USBTRACE, f"device {repr(dev)}")
        if vid is None or vid == dev.idVendor:
            if (pid is not None and pid == dev.idProduct) or dev.idProduct in known_devices:
                cfg = dev[0]

                # query each HID interface to find the update interface
                for intf in usb.util.find_descriptor(
                        cfg,
                        bInterfaceClass=INTERFACE_CLASS_HID,
                        bInterfaceSubClass=0,
                        bInterfaceProtocol=0,
                        find_all=True,
                ):
                    log.log(USBTRACE, f"interface {repr(intf)}")

                    # Ensure configuration is set
                    if dev.get_active_configuration() is None:
                        dev.set_configuration()

                    try:
                        if dev.is_kernel_driver_active(intf.bInterfaceNumber):
                            dev.detach_kernel_driver(intf.bInterfaceNumber)
                    except Exception as e:
                        log.error(f"failed to detach kernel driver: {e}")
                        continue

                    for i in range(3):
                        try:
                            usb.util.claim_interface(dev, intf.bInterfaceNumber)
                        except OSError as e:
                            if e.errno == errno.EBUSY:
                                log.warning(f"failed to claim interface: {e}")
                                time.sleep(1)
                                continue
                            else:
                                log.error(f"failed to claim interface: {e}")
                        except Exception as e:
                            log.error(f"failed to claim interface: {e}")
                        break
                    else:
                        continue

                    matched_interface = None
                    try:
                        try:
                            hid_desc = hid_get_descriptor(dev, intf.bInterfaceNumber)
                        except:
                            log.exception(f"failed to read descriptor")
                        else:
                            log.log(USBTRACE, f"descriptor {hid_desc.hex()}")
                            with warnings.catch_warnings():
                                # suppress warnings from python-hid-parser
                                warnings.simplefilter("ignore")
                                rdesc = hid_parser.ReportDescriptor(hid_desc)
                                try:
                                    for item in rdesc.get_input_items():
                                        if isinstance(item, hid_parser.VariableItem):
                                            log.log(USBTRACE, f"item {item}")
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
        self.ep_in: usb.Endpoint | None = None
        self.ep_out: usb.Endpoint | None = None

    def open(self):
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

    def close(self):
        self.ep_in = None
        self.ep_out = None
        usb.util.release_interface(self.dev, self.intf.bInterfaceNumber)

    def replace(self, new_dev: usb.Device):
        self.dev = new_dev.dev
        self.intf = new_dev.intf
        self.open()

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def send(self, pkt: bytes):
        """Write to OUT endpoint"""
        log.log(USBTRACE, f"USB TX {pkt.hex()}")
        self.ep_out.write(pkt, USB_TIMEOUT)

    def recv(self, size: int) -> bytes:
        """Read from IN endpoint"""
        pkt = bytes(self.ep_in.read(size, USB_TIMEOUT))
        log.log(USBTRACE, f"USB RX {pkt.hex()}")
        return pkt

    def alt_recv(self, size: int):
        """Read with GET_REPORT request"""
        pkt = hid_get_report(self.dev, self.intf.bInterfaceNumber, size)
        log.log(USBTRACE, f"USB RX {pkt.hex()}")
        return pkt
