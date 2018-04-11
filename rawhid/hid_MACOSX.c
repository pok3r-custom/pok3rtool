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
#include <unistd.h>
#include <string.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>

#include "hid.h"

#define BUFFER_SIZE 64

#define printf(...) // comment this out to get lots of info printed

//typedef struct hid_struct hid_t;
typedef struct buffer_struct buffer_t;

struct hid_struct {
    IOHIDDeviceRef ref;
    int open;
    uint8_t buffer[BUFFER_SIZE];
    buffer_t *first_buffer;
    buffer_t *last_buffer;
    struct hid_struct *prev;
    struct hid_struct *next;
};

struct buffer_struct {
    struct buffer_struct *next;
    uint32_t len;
    uint8_t buf[BUFFER_SIZE];
};

struct callback_context {
    rawhid_filter_cb cb;
    void *user;
    struct rawhid_detail detail;
    int opencount;
};

static int count = 0;
IOHIDManagerRef hid_manager = NULL;

// private functions, not intended to be used from outside this file
static void hid_close(hid_t *);
static void timeout_callback(CFRunLoopTimerRef, void *);
static void input_callback(void *, IOReturn, void *, IOHIDReportType,
     uint32_t, uint8_t *, CFIndex);

static int hid_init()
{
    // Start the HID Manager
    // http://developer.apple.com/technotes/tn2007/tn2187.html
    if (!hid_manager) {
        hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (hid_manager == NULL || CFGetTypeID(hid_manager) != IOHIDManagerGetTypeID()) {
            if (hid_manager)
                CFRelease(hid_manager);
            return 1;
        }
    }
    return 0;
}

static void input_callback(void *context, IOReturn ret, void *sender,
    IOHIDReportType type, uint32_t id, uint8_t *data, CFIndex len)
{
    buffer_t *n;
    hid_t *hid;

    printf("input_callback\n");
    if (ret != kIOReturnSuccess || len < 1) return;
    hid = context;
    if (!hid || hid->ref != sender) return;
    n = (buffer_t *)malloc(sizeof(buffer_t));
    if (!n) return;
    if (len > BUFFER_SIZE) len = BUFFER_SIZE;
    memcpy(n->buf, data, len);
    n->len = len;
    n->next = NULL;
    if (!hid->first_buffer || !hid->last_buffer) {
        hid->first_buffer = hid->last_buffer = n;
    } else {
        hid->last_buffer->next = n;
        hid->last_buffer = n;
    }
    CFRunLoopStop(CFRunLoopGetCurrent());
}

static void timeout_callback(CFRunLoopTimerRef timer, void *info)
{
    printf("timeout_callback\n");
    *(int *)info = 1;
    CFRunLoopStop(CFRunLoopGetCurrent());
}

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
    buffer_t *b;
    CFRunLoopTimerRef timer=NULL;
    CFRunLoopTimerContext context;
    int ret=0, timeout_occurred=0;

    if (len < 1) return 0;
    if (!hid || !hid->open) return -1;
    if ((b = hid->first_buffer) != NULL) {
        if (len > b->len) len = b->len;
        memcpy(buf, b->buf, len);
        hid->first_buffer = b->next;
        free(b);
        return len;
    }
    memset(&context, 0, sizeof(context));
    context.info = &timeout_occurred;
    timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() +
        (double)timeout / 1000.0, 0, 0, 0, timeout_callback, &context);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopDefaultMode);
    while (1) {
        CFRunLoopRun();
        if ((b = hid->first_buffer) != NULL) {
            if (len > b->len) len = b->len;
            memcpy(buf, b->buf, len);
            hid->first_buffer = b->next;
            free(b);
            ret = len;
            break;
        }
        if (!hid->open) {
            printf("rawhid_recv, device not open\n");
            ret = -1;
            break;
        }
        if (timeout_occurred) break;
    }
    CFRunLoopTimerInvalidate(timer);
    CFRelease(timer);
    return ret;
}

static void output_callback(void *context, IOReturn ret, void *sender,
    IOHIDReportType type, uint32_t id, uint8_t *data, CFIndex len)
{
    printf("output_callback, r=%d\n", ret);
    if (ret == kIOReturnSuccess) {
        *(int *)context = len;
    } else {
        // timeout if not success?
        *(int *)context = 0;
    }
    CFRunLoopStop(CFRunLoopGetCurrent());
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
    int result=-100;

    if (!hid || !hid->open) return -1;
#if 1
    #warning "Send timeout not implemented on MACOSX"
    IOReturn ret = IOHIDDeviceSetReport(hid->ref, kIOHIDReportTypeOutput, 0, buf, len);
    result = (ret == kIOReturnSuccess) ? len : -1;
#endif
#if 0
    // No matter what I tried this never actually sends an output
    // report and output_callback never gets called.  Why??
    // Did I miss something?  This is exactly the same params as
    // the sync call that works.  Is it an Apple bug?
    // (submitted to Apple on 22-sep-2009, problem ID 7245050)
    //
    IOHIDDeviceScheduleWithRunLoop(hid->ref, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    // should already be scheduled with run loop by attach_callback,
    // sadly this doesn't make any difference either way

    // could this be related?
    // http://lists.apple.com/archives/usb/2008/Aug/msg00021.html
    //
    IOHIDDeviceSetReportWithCallback(hid->ref, kIOHIDReportTypeOutput,
        0, buf, len, (double)timeout / 1000.0, output_callback, &result);
    while (1) {
        printf("enter run loop (send)\n");
        CFRunLoopRun();
        printf("leave run loop (send)\n");
        if (result > -100) break;
        if (!hid->open) {
            result = -1;
            break;
        }
    }
#endif
    return result;
}

static void detach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
    hid_t *hid;

    printf("detach callback\n");
//    for (hid = first_hid; hid; hid = hid->next) {
//        if (hid->ref == dev) {
//            hid->open = 0;
//            CFRunLoopStop(CFRunLoopGetCurrent());
//            return;
//        }
//    }
}

static Boolean IOHIDDevice_GetLongProperty(IOHIDDeviceRef inDeviceRef, CFStringRef inKey, long *outValue)
{
    Boolean result = FALSE;

    CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty(inDeviceRef, inKey);
    if (tCFTypeRef) {
        // if this is a number
        if (CFNumberGetTypeID() == CFGetTypeID(tCFTypeRef)) {
            // get its value
            result = CFNumberGetValue((CFNumberRef)tCFTypeRef, kCFNumberSInt32Type, outValue);
        }
    }
    return result;
}

static Boolean IOHIDDevice_GetDataProperty(IOHIDDeviceRef inDeviceRef, CFStringRef inKey, uint8_t *data, long *size)
{
    Boolean result = FALSE;

    CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty(inDeviceRef, inKey);
    if (tCFTypeRef) {
        // if this is a number
        if (CFDataGetTypeID() == CFGetTypeID(tCFTypeRef)) {
            // get its value
            long len = CFDataGetLength((CFDataRef)tCFTypeRef);
            if (*size >= len){
                CFRange range;
                range.location = 0;
                range.length = len;
                CFDataGetBytes((CFDataRef)tCFTypeRef, range, data);
                *size = len;
            }
        }
    }
    return result;
}

static Boolean IOHIDDevice_GetDataPtrProperty(IOHIDDeviceRef inDeviceRef, CFStringRef inKey, const unsigned char **data, long *size)
{
    Boolean result = FALSE;

    CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty(inDeviceRef, inKey);
    if (tCFTypeRef) {
        // if this is a number
        if (CFDataGetTypeID() == CFGetTypeID(tCFTypeRef)) {
            // get its value
            *data = CFDataGetBytePtr((CFDataRef)tCFTypeRef);
            *size = CFDataGetLength((CFDataRef)tCFTypeRef);
        }
    }
    return result;
}

static void attach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
    printf("attach callback\n");

    hid_t **hidp = (hid_t **)context;
    if (!hidp) return;

    // open device
    if (IOHIDDeviceOpen(dev, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
        return;

    hid_t *hid = (hid_t *)malloc(sizeof(hid_t));
    if (!hid) return;
    memset(hid, 0, sizeof(hid_t));

    // register device
    IOHIDDeviceScheduleWithRunLoop(dev, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceRegisterInputReportCallback(dev, hid->buffer, sizeof(hid->buffer), input_callback, hid);
    hid->ref = dev;
    hid->open = 1;

    *hidp = hid;

    count++;
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
    CFMutableDictionaryRef dict;
    CFNumberRef num;
    IOReturn ret;
    hid_t *hid;

    printf("rawhid_open\n");
    if (hid_init()) return NULL;

    if (vid > 0 || pid > 0 || usage_page > 0 || usage > 0) {
        // Tell the HID Manager what type of devices we want
        dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                         &kCFTypeDictionaryKeyCallBacks,
                                         &kCFTypeDictionaryValueCallBacks);
        if (!dict)
            return NULL;
        if (vid > 0) {
            num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vid);
            CFDictionarySetValue(dict, CFSTR(kIOHIDVendorIDKey), num);
            CFRelease(num);
        }
        if (pid > 0) {
            num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &pid);
            CFDictionarySetValue(dict, CFSTR(kIOHIDProductIDKey), num);
            CFRelease(num);
        }
        if (usage_page > 0) {
            num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
            CFDictionarySetValue(dict, CFSTR(kIOHIDPrimaryUsagePageKey), num);
            CFRelease(num);
        }
        if (usage > 0) {
            num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
            CFDictionarySetValue(dict, CFSTR(kIOHIDPrimaryUsageKey), num);
            CFRelease(num);
        }
        IOHIDManagerSetDeviceMatching(hid_manager, dict);
        CFRelease(dict);
    } else {
        IOHIDManagerSetDeviceMatching(hid_manager, NULL);
    }
    // set up a callbacks for device attach & detach
    IOHIDManagerScheduleWithRunLoop(hid_manager,
                                    CFRunLoopGetCurrent(),
                                    kCFRunLoopDefaultMode);
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, attach_callback, &hid);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, detach_callback, NULL);
    ret = IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone);
    if (ret != kIOReturnSuccess) {
        IOHIDManagerUnscheduleFromRunLoop(hid_manager,
                                          CFRunLoopGetCurrent(),
                                          kCFRunLoopDefaultMode);
        CFRelease(hid_manager);
        return NULL;
    }
    printf("run loop\n");
    // let it do the callback for all devices
    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) == kCFRunLoopRunHandledSource) ;

    return hid;
}

static void match_filter_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
    printf("attach callback\n");

    struct callback_context *ctx = (struct callback_context *)context;
    if (!ctx) return;

    // get vid/pid
    long result;
    IOHIDDevice_GetLongProperty(dev, CFSTR(kIOHIDVendorIDKey), &result);
    ctx->detail.vid = (unsigned short)(result & 0xFFFF);
    IOHIDDevice_GetLongProperty(dev, CFSTR(kIOHIDProductIDKey), &result);
    ctx->detail.pid = (unsigned short)(result & 0xFFFF);

    // callback with device info
    ctx->detail.step = RAWHID_STEP_DEV;
    if (!ctx->cb(ctx->user, &ctx->detail)) {
        return;
    }

    // open device
    if (IOHIDDeviceOpen(dev, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
        return;

    // get usage
    IOHIDDevice_GetLongProperty(dev, CFSTR(kIOHIDPrimaryUsagePageKey), &result);
    ctx->detail.usage_page = (unsigned short)(result & 0xFFFF);
    IOHIDDevice_GetLongProperty(dev, CFSTR(kIOHIDPrimaryUsageKey), &result);
    ctx->detail.usage = (unsigned short)(result & 0xFFFF);

    // get report descriptor
    const unsigned char *data;
    long len;
    IOHIDDevice_GetDataPtrProperty(dev, CFSTR(kIOHIDReportDescriptorKey), &data, &len);
    ctx->detail.report_desc = data;
    ctx->detail.rdesc_len = len;

    // callback with report info
    ctx->detail.step = RAWHID_STEP_REPORT;
    if (!ctx->cb(ctx->user, &ctx->detail)) {
        return;
    }

    hid_t *hid = (hid_t *)malloc(sizeof(hid_t));
    if (!hid) return;
    memset(hid, 0, sizeof(hid_t));

    // register device
    IOHIDDeviceScheduleWithRunLoop(dev, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceRegisterInputReportCallback(dev, hid->buffer, sizeof(hid->buffer), input_callback, hid);
    hid->ref = dev;
    hid->open = 1;

    // callback with open hid_t
    ctx->detail.step = RAWHID_STEP_OPEN;
    ctx->detail.hid = hid;
    if (!ctx->cb(ctx->user, &ctx->detail)) {
        rawhid_close(hid);
        return;
    }

    ctx->opencount++;
    count++;
}

int rawhid_openall_filter(rawhid_filter_cb cb, void *user)
{
    CFMutableDictionaryRef dict;
    CFNumberRef num;
    IOReturn ret;

    struct callback_context ctx;
    ctx.cb = cb;
    ctx.user = user;
    ctx.opencount = 0;
    memset(&ctx.detail, 0, sizeof(struct rawhid_detail));

    printf("rawhid_openll_filter\n");
    if (hid_init()) return 0;
    // match all
    IOHIDManagerSetDeviceMatching(hid_manager, NULL);

    IOHIDManagerScheduleWithRunLoop(hid_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    // set up a callbacks for device match and remove
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, match_filter_callback, &ctx);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, detach_callback, &ctx);

    ret = IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone);
    if (ret != kIOReturnSuccess) {
        IOHIDManagerUnscheduleFromRunLoop(hid_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(hid_manager);
        return 0;
    }

    printf("run loop\n");
    // let it do the callback for all devices
    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) == kCFRunLoopRunHandledSource) ;

    return ctx.opencount;
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
    hid->open = 0;
    free(hid);
}

static void hid_close(hid_t *hid)
{
    if (!hid || !hid->open || !hid->ref) return;
    IOHIDDeviceUnscheduleFromRunLoop(hid->ref, CFRunLoopGetCurrent( ), kCFRunLoopDefaultMode);
    IOHIDDeviceClose(hid->ref, kIOHIDOptionsTypeNone);
    hid->ref = NULL;
}
