#ifndef HIDDEVICE_H
#define HIDDEVICE_H

#include <functional>

#include "zbinary.h"
#include "zpointer.h"
using namespace LibChaos;

#define SEND_TIMEOUT    1000
#define RECV_TIMEOUT    1000

struct rawhid_detail;
struct HIDDeviceData;

class HIDDevice {
public:
    typedef bool (*filter_func_type)(zu16 vid, zu16 pid, zu16 upage, zu16 usage);
public:
    HIDDevice();
    HIDDevice(void *hidt, zu16 vid, zu16 pid, zu16 usage_page, zu16 usage);

    HIDDevice(const HIDDevice &other) = delete;
    ~HIDDevice();

    bool open(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage);
    void close();
    bool isOpen() const;

    bool send(const ZBinary &data, bool tolerate_dc = false);
    bool recv(ZBinary &data);

    static ZArray<ZPointer<HIDDevice>> openAll(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage);

    static zu32 openFilter(std::function<bool(const rawhid_detail *)> func);

private:
    zu16 vid;
    zu16 pid;
    zu16 usage_page;
    zu16 usage;

private:
    HIDDeviceData *device;
};

#endif // HIDDEVICE_H
