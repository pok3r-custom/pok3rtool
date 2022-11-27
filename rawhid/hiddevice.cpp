#include "hiddevice.h"
#include "zlog.h"
#include "zmutex.h"
#include "zlock.h"

#include "hid.h"

#include <libusb.h>

struct HIDDeviceData {
    hid_t *hid;
};

HIDDevice::HIDDevice(){
    hid = NULL;
}

HIDDevice::HIDDevice(hid_t *hidt){
    hid = hidt;
}

HIDDevice::~HIDDevice(){
    close();
}

bool HIDDevice::open(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage){
    hid = rawhid_open(vid, pid, usage_page, usage);
    return (hid != NULL);
}

void HIDDevice::close(){
    rawhid_close(hid);
    hid = NULL;
}

bool HIDDevice::isOpen() const {
    return !!(hid);
}

bool HIDDevice::send(const ZBinary &data, bool tolerate_dc){
    if(!isOpen())
        return false;
    int ret = rawhid_send(hid, data.raw(), data.size(), SEND_TIMEOUT);
    if(ret < 0){
        if(tolerate_dc && (ret == LIBUSB_ERROR_PIPE || ret == LIBUSB_ERROR_NO_DEVICE)){
            // ignore some errors when devices may disconnect
            return true;
        }
        ELOG("hid send error: " << ret << ": " << libusb_strerror(ret));
        return false;
    }
    if((zu64)ret < data.size())
        return false;
    return true;
}

bool HIDDevice::recv(ZBinary &data){
    if(!isOpen())
        return false;
    if(data.size() == 0)
        return false;

    ZClock clock;
    int ret;
    do {
        ret = rawhid_recv(hid, data.raw(), data.size(), RECV_TIMEOUT);
    } while(ret == 0 && !clock.passedMs(RECV_TIMEOUT_MAX));
//    } while(ret == 0);

    //if(ret == 0){
    //    ELOG("hid recv timeout");
    //    return false;
    //} else
    if(ret < 0){
        ELOG("hid recv error: " << ret << ": " << libusb_strerror(ret));
        return false;
    }
    data.resize((zu64)ret);
    return true;
}

ZArray<ZPointer<HIDDevice> > HIDDevice::openAll(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage){
    ZArray<ZPointer<HIDDevice>> devs;
    hid_t *hid[128];
    int count = rawhid_openall(hid, 128, vid, pid, usage_page, usage);

    for(int i = 0; i < count; ++i){
        devs.push(new HIDDevice(hid[i]));
    }
    return devs;
}

int open_cb(void *user, rawhid_detail *detail){
    auto *func = (std::function<bool(rawhid_detail *)> *)user;
    return (int)(*func)(detail);
}

zu32 HIDDevice::openFilter(std::function<bool(rawhid_detail *)> func){
    return rawhid_openall_filter(open_cb, (void *)&func);
}
