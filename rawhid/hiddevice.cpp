#include "hiddevice.h"
#include "zlog.h"
#include "zmutex.h"
#include "zlock.h"

#include "hid.h"

#if LIBCHAOS_PLATFORM == _PLATFORM_WINDOWS
    #include <windows.h>
#elif LIBCHAOS_PLATFORM == _PLATFORM_LINUX
    #include <usb.h>
    #include <errno.h>
#endif

struct HIDDeviceData {
    hid_t *hid;
};

HIDDevice::HIDDevice(){
    device = new HIDDeviceData;
    device->hid = NULL;
}

HIDDevice::HIDDevice(void *hidt){
    device = new HIDDeviceData;
    device->hid = (hid_t *)hidt;
}

HIDDevice::~HIDDevice(){
    close();
    delete device;
}

bool HIDDevice::open(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage){
    device->hid = rawhid_open(vid, pid, usage_page, usage);
    return (device->hid != NULL);
}

void HIDDevice::close(){
    rawhid_close(device->hid);
    device->hid = NULL;
}

bool HIDDevice::isOpen() const {
    return !!(device->hid);
}

bool HIDDevice::send(const ZBinary &data, bool tolerate_dc){
    if(!isOpen())
        return false;
    int ret = rawhid_send(device->hid, data.raw(), data.size(), SEND_TIMEOUT);
    if(ret < 0){
#if LIBCHAOS_PLATFORM == _PLATFORM_WINDOWS
        zu32 err = ZError::getSystemErrorCode();
        if(tolerate_dc && (
            err == ERROR_GEN_FAILURE ||
            err == ERROR_DEVICE_NOT_CONNECTED
        )){
            // ignore some errors when devices may disconnect
            return true;
        }
        ELOG("hid send win32 error: " << err);
#elif LIBCHAOS_PLATFORM == _PLATFORM_LINUX
        if(tolerate_dc && (ret == -EPIPE || ret == -ENXIO)){
            // ignore some errors when devices may disconnect
            return true;
        }
        ELOG("hid send error: " << ret << ": " << usb_strerror());
#else
        ELOG("hid send error: " << ret);
#endif
        return false;
    }
    if((zu64)ret != data.size())
        return false;
    return true;
}

bool HIDDevice::recv(ZBinary &data){
    if(!isOpen())
        return false;
    if(data.size() == 0)
        return false;

    int ret;
    do {
        ret = rawhid_recv(device->hid, data.raw(), data.size(), RECV_TIMEOUT);
    } while(ret == 0);

    if(ret < 0){
#if LIBCHAOS_PLATFORM == _PLATFORM_LINUX
        ELOG("hid recv error: " << ret << ": " << usb_strerror());
#else
        ELOG("hid recv error: " << ret);
#endif
        return false;
    }
    data.resize((zu64)ret);
    return true;
}

ZArray<ZPointer<HIDDevice> > HIDDevice::openAll(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage){
    ZArray<ZPointer<HIDDevice>> devs;
#if LIBCHAOS_PLATFORM == _PLATFORM_MACOSX
    ELOG("HID openAll not supported on OSX yet");
#else
    hid_t *hid[128];
    int count = rawhid_openall(hid, 128, vid, pid, usage_page, usage);

    for(int i = 0; i < count; ++i){
        devs.push(new HIDDevice(hid[i]));
    }
#endif
    return devs;
}

int open_cb(void *user, const rawhid_detail *detail){
    auto *func = (std::function<bool(const rawhid_detail *)> *)user;
    return (int)(*func)(detail);
}

zu32 HIDDevice::openFilter(std::function<bool(const rawhid_detail *)> func){
    return rawhid_openall_filter(open_cb, (void *)&func);
}
