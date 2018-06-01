/* Simple Raw HID functions for Linux - for use with Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 *
 *  rawhid_open - open 1 or more devices
 *  rawhid_recv - receive a packet
 *  rawhid_send - send a packet
 *  rawhid_close - close a device
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Version 1.0: Initial Release
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <usb.h>

#include "hid.h"

#define INTERFACE_CLASS_HID     3
#define INTERFACE_SUBCLASS_NONE 0
#define INTERFACE_PROTOCOL_NONE 0

#define DESCRIPTOR_HID      0x2100
#define DESCRIPTOR_REPORT   0x2200
#define DESCRIPTOR_PHYSICAL 0x2300

#define TAG_USAGE_PAGE      0x4
#define TAG_USAGE           0x8

// On Linux there are several options to access HID devices.
//
// libusb 0.1 - the only way that works well on all distributions
// libusb 1.0 - someday will become standard on most distributions
// hidraw driver - relatively new, not supported on many distributions (yet)
// hiddev driver - old, ubuntu, fedora, others dropping support
// usbfs - low level usb API: http://www.kernel.org/doc/htmldocs/usb.html#usbfs
//
// This code uses libusb 0.1, which is well supported on all linux distributions
// and very stable and widely used by many programs.  libusb 0.1 only provides a
// simple synchronous interface, basically the same as this code needs.  However,
// if you want non-blocking I/O, libusb 0.1 simply does not provide that.  There
// is also no kernel-level buffering, so performance is poor.
//
// UPDATE: As of November 2011, hidraw support seems to be working well in all
// major linux distributions.  Embedded and "small" distros, used on ARM boards,
// routers and embedded hardware stil seem to omit the hidraw driver.
//
// The hidraw driver is a great solution.  However, it has only been supported
// by relatively recent (in 2009) kernels.  Here is a quick survey of the status
// of hidraw support in various distributions as of Sept 2009:
//
// Fedora 11: works, kernel 2.6.29.4
// Mandiva 2009.1: works, kernel 2.6.29.1
// Ubuntu 9.10-alpha6: works, kernel 2.6.31
// Ubuntu 9.04: sysfs attrs chain broken (hidraw root only), 2.6.28 kernel
// openSUSE 11.1: sysfs attrs chain broken (hidraw root only), 2.6.27.7 kernel
// Debian Live, Lenny 5.0.2: sysfs attrs chain broken (hidraw root only), 2.6.26
// SimplyMEPIS 8.0.10: sysfs attrs chain broken (hidraw root only), 2.6.27
// Mint 7: sysfs attrs chain broken (hidraw root only), 2.6.28 kernel
// Gentoo 2008.0-r1: sysfs attrs chain broken (hidraw root only), 2.6.24 kernel
// Centos 5: no hidraw or hiddev devices, 2.6.18 kernel
// Slitaz 2.0: no hidraw devices (has hiddev), 2.6.25.5 kernel
// Puppy 4.3: no hidraw devices (has hiddev), 2.6.30.5 kernel
// Damn Small 4.4.10: (would not boot)
// Gentoo 10.0-test20090926: (would not boot)
// PCLinuxOS 2009.2: (would not boot)
// Slackware: (no live cd available?  www.slackware-live.org dead)

#define printf(...)  // comment this out for lots of info

struct hid_struct {
    usb_dev_handle *usb;
    int open;
    int iface;
    int ep_in;
    int ep_out;
    int ref_id;
};

static int count = 0;
static int handle_refs[128];

// private functions, not intended to be used from outside this file
static void hid_close(hid_t *hid);
static int hid_parse_item(uint32_t *val, uint8_t **data, const uint8_t *end);

//  rawhid_recv - receive a packet
//    Inputs:
//	num = device to receive from
//	buf = buffer to receive packet
//	len = buffer's size
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes received, or -1 on error
//
int rawhid_recv(hid_t *hid, void *buf, int len, int timeout)
{
    int r;

    if (!hid || !hid->open) return -1;
    printf("recv ep %d\n", hid->ep_in);
    r = usb_interrupt_read(hid->usb, hid->ep_in, buf, len, timeout);
    //if (r >= 0) return r;
    if (r == -110) return 0;  // timeout
    return r;
}

//  rawhid_send - send a packet
//    Inputs:
//	num = device to transmit to
//	buf = buffer containing packet to send
//	len = number of bytes to transmit
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes sent, or -1 on error
//
int rawhid_send(hid_t *hid, const void *buf, int len, int timeout)
{
    if (!hid || !hid->open) return -1;
    printf("send ep %d\n", hid->ep_out);
    if (hid->ep_out) {
        return usb_interrupt_write(hid->usb, hid->ep_out, buf, len, timeout);
    } else {
        return usb_control_msg(hid->usb, 0x21, USB_REQ_SET_CONFIGURATION, 0, hid->iface, (void *)buf, len, timeout);
    }
}

struct hid_match {
    int max;
    int vid;
    int pid;
    int usage_page;
    int usage;
    int count;
    hid_t **hids;
};

static int filter_cb(void *user, struct rawhid_detail *detail)
{
    struct hid_match *match = (struct hid_match *)user;

    switch(detail->step){
        case RAWHID_STEP_DEV:
            return (match->count < match->max &&
                    detail->vid == match->vid &&
                    detail->pid  == match->pid);
        case RAWHID_STEP_IFACE:
            return (detail->ifclass == INTERFACE_CLASS_HID &&
                    detail->subclass == INTERFACE_SUBCLASS_NONE &&
                    detail->protocol == INTERFACE_PROTOCOL_NONE);
        case RAWHID_STEP_REPORT:
            return (detail->usage_page == match->usage_page &&
                    detail->usage == match->usage);
        case RAWHID_STEP_OPEN:
            printf("filter open %x %x %x %x\n", detail->vid, detail->pid, detail->usage_page, detail->usage);
            match->hids[match->count] = detail->hid;
            match->count++;
            return 1;
    }
    return 0;
}

//  rawhid_open - open a device
//
//    Inputs:
//	max = maximum number of devices to open
//	vid = Vendor ID, or -1 if any
//	pid = Product ID, or -1 if any
//	usage_page = top level usage page, or -1 if any
//	usage = top level usage number, or -1 if any
//    Output:
//	device handle
//
hid_t *rawhid_open(int vid, int pid, int usage_page, int usage)
{
    hid_t *hid;

    struct hid_match match;
    match.max = 1;
    match.vid = vid;
    match.pid = pid;
    match.usage_page = usage_page;
    match.usage = usage;
    match.count = 0;
    match.hids = &hid;

    if (rawhid_openall_filter(filter_cb, &match))
        return hid;
    return NULL;
}

int rawhid_openall(hid_t **hids, int max, int vid, int pid, int usage_page, int usage)
{
    struct hid_match match;
    match.max = max;
    match.vid = vid;
    match.pid = pid;
    match.usage_page = usage_page;
    match.usage = usage;
    match.count = 0;
    match.hids = hids;

    return rawhid_openall_filter(filter_cb, &match);
}

int rawhid_openall_filter(rawhid_filter_cb cb, void *user)
{
    int opencount = 0;
    uint8_t buf[1024];
    struct rawhid_detail detail;
    memset(&detail, 0, sizeof(struct rawhid_detail));

    printf("rawhid_open_filter\n");
    usb_init();
    usb_find_busses();
    usb_find_devices();
    // loop over buses
    for (struct usb_bus *bus = usb_get_busses(); bus; bus = bus->next) {
        // loop over devices
        for (struct usb_device *dev = bus->devices; dev; dev = dev->next) {

            // call user callback with device info
            detail.step = RAWHID_STEP_DEV;
            detail.bus = bus->location;
            detail.device = dev->devnum;
            detail.vid = dev->descriptor.idVendor;
            detail.pid = dev->descriptor.idProduct;
            if (!cb(user, &detail)) {
                // if false, do not open/inspect device
                printf("callback dev false\n");
                continue;
            }

            if (!dev->config) continue;
            if (dev->config->bNumInterfaces < 1) continue;
            printf("device: vid=%04X, pic=%04X, with %d iface\n", dev->descriptor.idVendor, dev->descriptor.idProduct, dev->config->bNumInterfaces);
            struct usb_interface *iface = dev->config->interface;
            usb_dev_handle *hand = NULL;
            int claimed = 0;
            int ref_id = -1;
            // loop over interfaces
            for (int i = 0; i < dev->config->bNumInterfaces && iface; i++, iface++) {
                struct usb_interface_descriptor *desc = iface->altsetting;
                if (!desc) continue;
                const int ifnum = desc->bInterfaceNumber;
                printf("  iface %d type %d, %d, %d\n", ifnum, desc->bInterfaceClass, desc->bInterfaceSubClass, desc->bInterfaceProtocol);

                printf("    endpoints: %d\n", desc->bNumEndpoints);
                struct usb_endpoint_descriptor *ep = desc->endpoint;
                int ep_in = 0, ep_out = 0, epin_size = 0, epout_size = 0;
                // loop over endpoints
                for (int n = 0; n < desc->bNumEndpoints; n++, ep++) {
                    int epnum = ep->bEndpointAddress & 0x7F;
                    if (ep->bEndpointAddress & 0x80) {
                        printf("      IN endpoint %d (%d)\n", epnum, ep->wMaxPacketSize);
                        if (!ep_in){
                            ep_in = epnum;
                            epin_size = ep->wMaxPacketSize;
                        }
                    } else {
                        printf("      OUT endpoint %d (%d)\n", epnum, ep->wMaxPacketSize);
                        if (!ep_out){
                            ep_out = epnum;
                            epout_size = ep->wMaxPacketSize;
                        }
                    }
                }

                // call user callback with interface info
                detail.step = RAWHID_STEP_IFACE;
                detail.ifnum = ifnum;
                detail.ifclass = desc->bInterfaceClass;
                detail.subclass = desc->bInterfaceSubClass;
                detail.protocol = desc->bInterfaceProtocol;
                detail.ep_in = ep_in;
                detail.ep_out = ep_out;
                detail.epin_size = epin_size;
                detail.epout_size = epout_size;
                if (!cb(user, &detail)) {
                    printf("callback iface false\n");
                    continue;
                }

                if (!ep_in) continue;
                // open device if not already open
                if (!hand) {
                    hand = usb_open(dev);
                    if (!hand) {
                        printf("  unable to open device: %s\n", usb_strerror());
                        break;
                    }
                }
                // unbind kernel drivers from interface
                if (usb_get_driver_np(hand, ifnum, (char *)buf, sizeof(buf)) >= 0) {
                    printf("  in use by driver \"%s\"\n", buf);
                    if (usb_detach_kernel_driver_np(hand, ifnum) < 0) {
                        printf("  unable to detach from kernel\n");
                        continue;
                    }
                }
                if (usb_claim_interface(hand, ifnum) < 0) {
                    printf("  unable claim interface %d\n", ifnum);
                    continue;
                }
                // hid report descriptor request
                int len = usb_control_msg(hand, USB_ENDPOINT_IN | USB_RECIP_INTERFACE, USB_REQ_GET_DESCRIPTOR, DESCRIPTOR_REPORT, ifnum, (char *)buf, sizeof(buf), 250);
                printf("    hid descriptor, len=%d\n", len);
                if (len < 2) {
                    usb_release_interface(hand, ifnum);
                    continue;
                }
                uint8_t *p = buf;
                uint32_t val = 0, parsed_usage_page = 0, parsed_usage = 0;
                int tag;
                while ((tag = hid_parse_item(&val, &p, buf + len)) >= 0) {
                    printf("      tag: %X, val %X\n", tag, val);
                    if (tag == TAG_USAGE_PAGE) parsed_usage_page = val;
                    if (tag == TAG_USAGE) parsed_usage = val;
                    if (parsed_usage_page && parsed_usage) break;
                }
                if ((!parsed_usage_page) || (!parsed_usage)) {
                    usb_release_interface(hand, ifnum);
                    continue;
                }

                // call user callback with hid info
                detail.step = RAWHID_STEP_REPORT;
                detail.report_desc = buf;
                detail.rdesc_len = len;
                detail.usage_page = parsed_usage_page;
                detail.usage = parsed_usage;
                if (!cb(user, &detail)) {
                    usb_release_interface(hand, ifnum);
                    printf("callback report false\n");
                    continue;
                }

                hid_t *hid = (struct hid_struct *)malloc(sizeof(struct hid_struct));
                if (!hid) {
                    usb_release_interface(hand, ifnum);
                    continue;
                }

                if (ref_id < 0) {
                    ref_id = count;
                    handle_refs[ref_id] = 0;
                }

                // call user callback with open hid_t
                hid->usb = hand;
                hid->iface = ifnum;
                hid->ep_in = ep_in;
                hid->ep_out = ep_out;
                hid->open = 1;
                hid->ref_id = ref_id;

                detail.step = RAWHID_STEP_OPEN;
                detail.hid = hid;
                if (!cb(user, &detail)) {
                    usb_release_interface(hand, ifnum);
                    printf("callback open false\n");
                    continue;
                }

                opencount++;
                claimed++;
                count++;
                handle_refs[ref_id]++;
            }
            // close device if opened and not needed
            if (hand && !claimed) {
                usb_close(hand);
            }
        }
    }
    return opencount;
}

//  rawhid_close - close a device
//
//    Inputs:
//	num = device to close
//    Output
//	(nothing)
//
void rawhid_close(hid_t *hid)
{
    if (!hid || !hid->open) return;
    hid_close(hid);
    free(hid);
}

static void hid_close(hid_t *hid)
{
    hid_t *p;

    usb_release_interface(hid->usb, hid->iface);

    if (hid->ref_id >= 0) {
        handle_refs[hid->ref_id]--;
        if (handle_refs[hid->ref_id] == 0) {
            usb_close(hid->usb);
            hid->ref_id = -1;
        }
    }
    count--;
    hid->usb = NULL;
}

// Chuck Robey wrote a real HID report parser
// (chuckr@telenix.org) chuckr@chuckr.org
// http://people.freebsd.org/~chuckr/code/python/uhidParser-0.2.tbz
// this tiny thing only needs to extract the top-level usage page
// and usage, and even then is may not be truly correct, but it does
// work with the Teensy Raw HID example.
static int hid_parse_item(uint32_t *val, uint8_t **data, const uint8_t *end)
{
    const uint8_t *p = *data;
    uint8_t tag;
    int table[4] = {0, 1, 2, 4};
    int len;

    if (p >= end) return -1;
    if (p[0] == 0xFE) {
        // long item, HID 1.11, 6.2.2.3, page 27
        if (p + 5 >= end || p + p[1] >= end) return -1;
        tag = p[2];
        *val = 0;
        len = p[1] + 5;
    } else {
        // short item, HID 1.11, 6.2.2.2, page 26
        tag = p[0] & 0xFC;
        len = table[p[0] & 0x03];
        if (p + len + 1 >= end) return -1;
        switch (p[0] & 0x03) {
          case 3: *val = p[1] | (p[2] << 8) | (p[3] << 16) | (p[4] << 24); break;
          case 2: *val = p[1] | (p[2] << 8); break;
          case 1: *val = p[1]; break;
          case 0: *val = 0; break;
        }
    }
    *data += len + 1;
    return tag;
}
