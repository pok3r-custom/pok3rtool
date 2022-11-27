#ifndef HIDDEVICE_H
#define HIDDEVICE_H

#include "hid.h"
#include <functional>

#include "zbinary.h"
#include "zpointer.h"
using namespace LibChaos;

#define SEND_TIMEOUT        200
#define RECV_TIMEOUT        200
#define RECV_TIMEOUT_MAX    1000

struct rawhid_detail;

class HIDDevice {
public:
    typedef bool (*filter_func_type)(zu16 vid, zu16 pid, zu16 upage, zu16 usage);
public:
    HIDDevice();
    HIDDevice(hid_t *hidt);

    HIDDevice(const HIDDevice &other) = delete;
    ~HIDDevice();

    bool open(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage);
    void close();
    bool isOpen() const;

    bool send(const ZBinary &data, bool tolerate_dc = false);
    bool recv(ZBinary &data);

    static ZArray<ZPointer<HIDDevice>> openAll(zu16 vid, zu16 pid, zu16 usage_page, zu16 usage);

    static zu32 openFilter(std::function<bool(rawhid_detail *)> func);

private:
    hid_t *hid;
};

#endif // HIDDEVICE_H
