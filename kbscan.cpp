#include "kbscan.h"
#include "proto_pok3r.h"
#include "proto_cykb.h"
#include "rawhid/hid.h"

#include <functional>

#include "zmap.h"
#include "zset.h"
#include "zlog.h"

#define INTERFACE_CLASS_HID     3
#define INTERFACE_SUBCLASS_NONE 0
#define INTERFACE_PROTOCOL_NONE 0

#define VENDOR_USAGE_PAGE   0xff00
#define VENDOR_USAGE        0x01

static const ZMap<Device, DeviceInfo> known_devices = {
    { DEV_POK3R,            { "POK3R",          HOLTEK_VID, POK3R_PID,          POK3R_BOOT_PID,         PROTO_POK3R } },
    { DEV_POK3R_RGB,        { "POK3R RGB",      HOLTEK_VID, POK3R_RGB_PID,      POK3R_RGB_BOOT_PID,     PROTO_CYKB } },
    { DEV_VORTEX_CORE,      { "Vortex Core",    HOLTEK_VID, VORTEX_CORE_PID,    VORTEX_CORE_BOOT_PID,   PROTO_CYKB } },
    { DEV_VORTEX_TESTER,    { "Vortex Tester",  HOLTEK_VID, VORTEX_TESTER_PID,  VORTEX_TESTER_BOOT_PID, PROTO_CYKB } },
    { DEV_VORTEX_RACE3,     { "Vortex Race 3",  HOLTEK_VID, VORTEX_RACE3_PID,   VORTEX_RACE3_BOOT_PID,  PROTO_CYKB } },
    { DEV_VORTEX_VIBE,      { "Vortex ViBE",    HOLTEK_VID, VORTEX_VIBE_PID,    VORTEX_VIBE_BOOT_PID,   PROTO_CYKB } },
    { DEV_KBP_V60,          { "KBP V60",        HOLTEK_VID, KBP_V60_PID,        KBP_V60_BOOT_PID,       PROTO_POK3R } },
    { DEV_KBP_V80,          { "KBP V80",        HOLTEK_VID, KBP_V80_PID,        KBP_V80_BOOT_PID,       PROTO_POK3R } },
//    { DEV_KBP_V100,         { "KBP V100",       HOLTEK_VID, KBP_V100_PID,       KBP_V100_BOOT_PID,      PROTO_POK3R } },
    { DEV_TEX_YODA_II,      { "Tex Yoda II",    HOLTEK_VID, TEX_YODA_II_PID,    TEX_YODA_II_BOOT_PID,   PROTO_CYKB } },
};

static ZMap<zu16, Device> known_pids;

KBScan::KBScan(){
    if(!known_pids.size()){
        for(auto it = known_devices.begin(); it.more(); ++it){
            DeviceInfo info = known_devices[it.get()];
            known_pids.add(info.pid, it.get());
            known_pids.add(info.boot_pid, it.get());
        }
    }
}

zu32 KBScan::find(Device devtype){
    if(!known_devices.contains(devtype)){
        ELOG("Unknown device!");
        return 0;
    }
    DeviceInfo dev = known_devices[devtype];

    auto filter_func = [this,dev](const rawhid_detail *detail){
        switch(detail->step){
        case RAWHID_STEP_DEV:
            return (detail->vid == dev.vid && detail->pid == dev.pid);
        case RAWHID_STEP_IFACE:
            return (detail->ifclass == INTERFACE_CLASS_HID &&
                    detail->subclass == INTERFACE_SUBCLASS_NONE &&
                    detail->protocol == INTERFACE_PROTOCOL_NONE);
        case RAWHID_STEP_REPORT:
            return (detail->usage_page == VENDOR_USAGE_PAGE && detail->usage == VENDOR_USAGE);
        case RAWHID_STEP_OPEN:
            DLOG("OPEN " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
            ZPointer<HIDDevice> ptr = new HIDDevice(detail->hid);
            devices.push({ dev, ptr, (detail->pid & 0x1000) });
            // if hid pointer is taken, MUST return true here or it WILL double-free
            return true;
        }
        return false;
    };

    zu32 devs = HIDDevice::openFilter(filter_func);
    DLOG("Found " << devs << " devices");
    return devs;
}

zu32 KBScan::scan(){

    auto filter_func = [this](const rawhid_detail *detail){
        switch(detail->step){
        case RAWHID_STEP_DEV:
//            LOG("DEV " << detail->bus << " " << detail->device << " " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
            return (detail->vid == HOLTEK_VID && known_pids.contains(detail->pid));

        case RAWHID_STEP_IFACE:
//            LOG("IFACE " << detail->interface << " " << detail->ifclass << " " << detail->subclass << " " << detail->protocol);
            return (detail->ifclass == INTERFACE_CLASS_HID &&
                    detail->subclass == INTERFACE_SUBCLASS_NONE &&
                    detail->protocol == INTERFACE_PROTOCOL_NONE);

        case RAWHID_STEP_REPORT:
//            LOG("USAGE " << ZString::ItoS((zu64)detail->usage_page, 16, 4) << " " << ZString::ItoS((zu64)detail->usage, 16, 2));
            if(detail->usage_page == VENDOR_USAGE_PAGE && detail->usage == VENDOR_USAGE){
                ZBinary rdesc(detail->report_desc, detail->rdesc_len);
//                LOG("REPORT " << rdesc.strBytes(1));
                return true;
            }
            return false;

        case RAWHID_STEP_OPEN:
            DLOG("OPEN " <<
                ZString::ItoS((zu64)detail->vid, 16, 4) << " " <<
                ZString::ItoS((zu64)detail->pid, 16, 4) << " " <<
                detail->interface
            );
            ZPointer<HIDDevice> ptr = new HIDDevice(detail->hid);
            devices.push({
                known_devices[known_pids[detail->pid]],
                ptr,
                (detail->pid & 0x1000)
            });
            // if hid pointer is taken, MUST return true here or it WILL double-free
            return true;
        }
        return false;
    };

    zu32 devs = HIDDevice::openFilter(filter_func);
    DLOG("Found " << devs << " devices");
    return devs;
}

ZList<KBDevice> KBScan::open(){
    ZList<KBDevice> devs;

    // Read version from each device
    for(auto it = devices.begin(); it.more(); ++it){
        ListDevice ldev = it.get();
        // Check device
        if(ldev.hid.get() && ldev.hid->isOpen()){
            ZPointer<UpdateInterface> iface;
            // Select protocol
            if(ldev.dev.type == PROTO_POK3R){
                iface = new ProtoPOK3R(ldev.dev.vid, ldev.dev.pid, ldev.dev.boot_pid, ldev.boot, ldev.hid.get());
            } else if(ldev.dev.type == PROTO_CYKB){
                iface = new ProtoCYKB(ldev.dev.vid, ldev.dev.pid, ldev.dev.boot_pid, ldev.boot, ldev.hid.get());
            } else {
                ELOG("Unknown protocol");
                continue;
            }

            ldev.hid.divorce();
//            iface->getVersion();
            KBDevice kdev;
            kdev.info = ldev.dev;
            kdev.iface = iface;
            devs.push(kdev);

        } else {
            LOG(ldev.dev.name << " not open");
        }
    }

    return devs;
}
