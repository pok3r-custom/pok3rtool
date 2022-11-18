/* Simple Raw HID functions with LibUSB-1.0 backend
 *
 * Based on http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * Copryight (c) 2022 Hansem Ro <hansemro@outlook.com>
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
#include <libusb.h>

#include "hid.h"

#define INTERFACE_CLASS_HID     3
#define INTERFACE_SUBCLASS_NONE 0
#define INTERFACE_PROTOCOL_NONE 0

#define TAG_USAGE_PAGE      0x4
#define TAG_USAGE           0x8


#define printf(...)  // comment this out for lots of info

struct hid_struct {
    libusb_device_handle *usb;
    int open;
    int iface;
    int ep_in;
    int ep_out;
};

struct __attribute__((__packed__)) hid_descriptor_extra {
    uint8_t     bDescriptorType;
    uint16_t    wDescriptorLength;
};

struct __attribute((__packed__)) hid_descriptor {
    uint8_t                     bLength;
    uint8_t                     bDescriptorType;
    uint16_t                    bcdHID;
    uint8_t                     bCountryCode;
    uint8_t                     bNumDescriptors;
    struct hid_descriptor_extra extra[1];
};

static int count = 0;

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
    int transferred;
    int rc;

    if (!hid || !hid->open) return -1;
    printf("recv ep %d\n", hid->ep_in);
    rc = libusb_interrupt_transfer(hid->usb, LIBUSB_ENDPOINT_IN | hid->ep_in, buf, len, &transferred, timeout);
    if (rc == LIBUSB_ERROR_TIMEOUT) return 0;
    if (rc < 0) return -1;
    return transferred;
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
    int transferred;
    if (!hid || !hid->open) return -1;
    printf("send ep %d\n", hid->ep_out);
    if (hid->ep_out) {
        int rc = libusb_interrupt_transfer(hid->usb, hid->ep_out, (unsigned char *)buf, len, &transferred, timeout);
        if (rc < 0) return -1;
        return transferred;
    } else {
        return libusb_control_transfer(hid->usb, 0x21, LIBUSB_REQUEST_SET_CONFIGURATION, 0, hid->iface, (void *)buf, len, timeout);
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
    int rc;
    int opencount = 0;
    uint8_t buf[1024];
    struct rawhid_detail detail;
    memset(&detail, 0, sizeof(struct rawhid_detail));

    printf("rawhid_open_filter\n");
    if (libusb_init(NULL) < 0) {
        fprintf(stderr, "Initializing USB failed\n");
        return 0;
    }
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_NONE);

    struct libusb_device **list = NULL;
    int dev_count = libusb_get_device_list(NULL, &list);
    if (dev_count < 0) {
        fprintf(stderr, "Failed to get list of USB devices\n");
        return 0;
    }
    // loop over devices
    printf("looping over devices...\n");
    for (int i = 0; i < dev_count; i++) {
        struct libusb_device_descriptor descriptor;
        rc = libusb_get_device_descriptor(list[i], &descriptor);
        if (rc < 0) {
            fprintf(stderr, "Failed to get device descriptor (%d: %s)\n", rc, libusb_strerror(rc));
            if (list != NULL)
                libusb_free_device_list(list, 1);
            return 0;
        }

        // call user callback with device info
        detail.step = RAWHID_STEP_DEV;
        detail.bus = libusb_get_bus_number(list[i]);
        detail.device = libusb_get_device_address(list[i]);
        detail.vid = descriptor.idVendor;
        detail.pid = descriptor.idProduct;
        if (!cb(user, &detail)) {
            // if false, do not open/inspect device
            printf("callback dev false\n");
            continue;
        }

        struct libusb_config_descriptor *config = NULL;
        if (libusb_get_active_config_descriptor(list[i], &config)) continue;

        if (config->bNumInterfaces < 1) {
            libusb_free_config_descriptor(config);
            continue;
        }

        printf("device %d.%d: vid=%04X, pid=%04X, wTotalLength: %d, with %d iface\n", detail.bus, detail.device, descriptor.idVendor, descriptor.idProduct, config->wTotalLength, config->bNumInterfaces);

        const struct libusb_interface *iface = config->interface;
        struct libusb_device_handle *handle = NULL;
        int claimed = 0;
        // loop over interfaces
        printf("Looping over interfaces...\n");
        for (int j = 0; j < config->bNumInterfaces && iface; j++, iface++) {
            const struct libusb_interface_descriptor *desc = iface->altsetting;
            if (!desc) continue;

            // Skip non-HID interfaces
            if (desc->bInterfaceClass != LIBUSB_CLASS_HID) continue;

            const int ifnum = desc->bInterfaceNumber;
            printf("\tiface %d type %d, %d, %d\n", ifnum, desc->bInterfaceClass, desc->bInterfaceSubClass, desc->bInterfaceProtocol);
            // loop over endpoints
            printf("\t\tendpoints: %d\n", desc->bNumEndpoints);
            const struct libusb_endpoint_descriptor *ep = desc->endpoint;
            int ep_in = 0, ep_out = 0, epin_size = 0, epout_size = 0;
            for (int n = 0; n < desc->bNumEndpoints; n++, ep++) {
                int epnum = ep->bEndpointAddress & 0x7F;
                if (ep->bEndpointAddress & 0x80) {
                    printf("\t\t\tIN endpoint %d (%d)\n", epnum, ep->wMaxPacketSize);
                    if (!ep_in){
                        ep_in = epnum;
                        epin_size = ep->wMaxPacketSize;
                    }
                } else {
                    printf("\t\t\tOUT endpoint %d (%d)\n", epnum, ep->wMaxPacketSize);
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

            int len = 1024;
            const struct hid_descriptor *hid_desc;
            const struct hid_descriptor_extra *hid_desc_extra;

            // Find HID Report descriptor length from interface descriptor
            if (desc->extra_length >= (int)sizeof(struct hid_descriptor)) {
                hid_desc = (const struct hid_descriptor *)desc->extra;
                if (hid_desc->bDescriptorType == LIBUSB_DT_HID) {
                    // For each extra HID descriptor entry
                    for (hid_desc_extra = hid_desc->extra;
                         hid_desc_extra <
                            hid_desc->extra + hid_desc->bNumDescriptors &&
                         (uint8_t *)hid_desc_extra <
                            (uint8_t *)hid_desc + hid_desc->bLength &&
                         (unsigned char *)hid_desc_extra <
                            desc->extra +
                            desc->extra_length;
                         hid_desc_extra++) {
                        // If this is a report descriptor entry
                        if (hid_desc_extra->bDescriptorType ==
                                LIBUSB_DT_REPORT)
                        {
                            len = hid_desc_extra->wDescriptorLength;
                            printf("\t\tHID Report Descriptor length=%d\n", len);
                            break;
                        }
                    }
                }
            }

            if (!ep_in) continue;
            // open device if not already open
            if (!handle) {
                rc = libusb_open(list[i], &handle);
                if (rc != 0) {
                    printf("\t\tunable to open device (%d: %s)\n", rc, libusb_strerror(rc));
                    break;
                }
            }

            // unbind kernel drivers from interface
            if (libusb_kernel_driver_active(handle, ifnum) != 0) {
                printf("\t\tin use by a kernel driver\n");
                if (libusb_detach_kernel_driver(handle, ifnum) < 0) {
                    printf("\t\tunable to detach from kernel\n");
                    continue;
                }
            }
            if (libusb_claim_interface(handle, ifnum) < 0) {
                printf("\t\tunable claim interface %d\n", ifnum);
                continue;
            }

            // hid report descriptor request
            rc = libusb_control_transfer(handle,
                    LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE,
                    LIBUSB_REQUEST_GET_DESCRIPTOR,
                    (LIBUSB_DT_REPORT << 8),
                    ifnum,
                    (unsigned char *)buf,
                    len,
                    5000);
            if (rc < 2) {
                libusb_release_interface(handle, ifnum);
                continue;
            }

            uint8_t *p = buf;
            uint32_t val = 0, parsed_usage_page = 0, parsed_usage = 0;
            int tag;
            while ((tag = hid_parse_item(&val, &p, buf + rc)) >= 0) {
                printf("\t\t\ttag: 0x%X, val 0x%X\n", tag, val);
                if (tag == TAG_USAGE_PAGE) parsed_usage_page = val;
                if (tag == TAG_USAGE) parsed_usage = val;
                if (parsed_usage_page && parsed_usage) break;
            }
            if ((!parsed_usage_page) || (!parsed_usage)) {
                libusb_release_interface(handle, ifnum);
                continue;
            }

            // call user callback with hid info
            detail.step = RAWHID_STEP_REPORT;
            detail.report_desc = buf;
            detail.rdesc_len = len;
            detail.usage_page = parsed_usage_page;
            detail.usage = parsed_usage;
            if (!cb(user, &detail)) {
                libusb_release_interface(handle, ifnum);
                printf("callback report false\n");
                continue;
            }

            hid_t *hid = (struct hid_struct *)malloc(sizeof(struct hid_struct));
            if (!hid) {
                libusb_release_interface(handle, ifnum);
                continue;
            }

            // call user callback with open hid_t
            hid->usb = handle;
            hid->iface = ifnum;
            hid->ep_in = ep_in;
            hid->ep_out = ep_out;
            hid->open = 1;

            detail.step = RAWHID_STEP_OPEN;
            detail.hid = hid;
            if (!cb(user, &detail)) {
                printf("callback open false\n");
                continue;
            }

            opencount++;
            claimed++;
            count++;
        }

        // close device if opened and not needed
        if (handle && !claimed) {
            libusb_close(handle);
        }

        libusb_free_config_descriptor(config);
    }
    libusb_free_device_list(list, 1);
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

    libusb_release_interface(hid->usb, hid->iface);
    libusb_close(hid->usb);

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
