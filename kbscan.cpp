#include "kbscan.h"
#include "proto_pok3r.h"
#include "proto_cykb.h"

#include <functional>

#include "zmap.h"
#include "zset.h"
#include "zlog.h"

#define INTERFACE_CLASS_HID     3
#define INTERFACE_SUBCLASS_NONE 0
#define INTERFACE_PROTOCOL_NONE 0

#define HOLTEK_VID              0x04d9
#define QMK_VID                 0xfeed

#define BOOT_PID                0x1000
#define QMK_PID                 0x8000

#define POK3R_PID               0x0141
#define POK3R_RGB_PID           0x0167
#define POK3R_RGB2_PID          0x0207
#define VORTEX_CORE_PID         0x0175
#define VORTEX_TESTER_PID       0x0200
#define VORTEX_RACE3_PID        0x0192
#define VORTEX_VIBE_PID         0x0216
#define KBP_V60_PID             0x0112
#define KBP_V80_PID             0x0129
#define TEX_YODA_II_PID         0x0163

#define CONSOLE_USAGE_PAGE      0xff31
#define CONSOLE_USAGE           0x0074

static const ZMap<DeviceType, DeviceInfo> known_devices = {
    { DEV_POK3R,            { "Vortex POK3R",       HOLTEK_VID, POK3R_PID,                  BOOT_PID | POK3R_PID,           PROTO_POK3R } },
    { DEV_POK3R_RGB,        { "Vortex POK3R RGB",   HOLTEK_VID, POK3R_RGB_PID,              BOOT_PID | POK3R_RGB_PID,       PROTO_CYKB } },
    { DEV_POK3R_RGB2,       { "Vortex POK3R RGB2",  HOLTEK_VID, POK3R_RGB2_PID,             BOOT_PID | POK3R_RGB2_PID,      PROTO_CYKB } },
    { DEV_VORTEX_CORE,      { "Vortex Core",        HOLTEK_VID, VORTEX_CORE_PID,            BOOT_PID | VORTEX_CORE_PID,     PROTO_CYKB } },
    { DEV_VORTEX_TESTER,    { "Vortex Tester",      HOLTEK_VID, VORTEX_TESTER_PID,          BOOT_PID | VORTEX_TESTER_PID,   PROTO_CYKB } },
    { DEV_VORTEX_RACE3,     { "Vortex Race 3",      HOLTEK_VID, VORTEX_RACE3_PID,           BOOT_PID | VORTEX_RACE3_PID,    PROTO_CYKB } },
    { DEV_VORTEX_VIBE,      { "Vortex ViBE",        HOLTEK_VID, VORTEX_VIBE_PID,            BOOT_PID | VORTEX_VIBE_PID,     PROTO_CYKB } },
    { DEV_KBP_V60,          { "KBP V60",            HOLTEK_VID, KBP_V60_PID,                BOOT_PID | KBP_V60_PID,         PROTO_POK3R } },
    { DEV_KBP_V80,          { "KBP V80",            HOLTEK_VID, KBP_V80_PID,                BOOT_PID | KBP_V80_PID,         PROTO_POK3R } },
//  { DEV_KBP_V100,         { "KBP V100",           HOLTEK_VID, KBP_V100_PID,               BOOT_PID | KBP_V100_PID,        PROTO_POK3R } },
    { DEV_TEX_YODA_II,      { "Tex Yoda II",        HOLTEK_VID, TEX_YODA_II_PID,            BOOT_PID | TEX_YODA_II_PID,     PROTO_CYKB } },

//  { DEV_QMK_POK3R,        { "POK3R [QMK]",        HOLTEK_VID, QMK_PID | POK3R_PID,        BOOT_PID | POK3R_PID,           PROTO_POK3R } },
//  { DEV_QMK_POK3R_RGB,    { "POK3R RGB [QMK]",    HOLTEK_VID, QMK_PID | POK3R_RGB_PID,    BOOT_PID | POK3R_RGB_PID,       PROTO_CYKB } },
//  { DEV_QMK_VORTEX_CORE,  { "Vortex Core [QMK]",  HOLTEK_VID, QMK_PID | VORTEX_CORE_PID,  BOOT_PID | VORTEX_CORE_PID,     PROTO_CYKB } },
};

static ZMap<zu32, DeviceType> known_ids;

KBScan::KBScan(){
    if(!known_ids.size()){
        for(auto it = known_devices.begin(); it.more(); ++it){
            DeviceInfo info = known_devices[it.get()];
            zu32 id = info.pid | (info.vid << 16);
            zu32 bid = info.boot_pid | (info.vid << 16);
            if(known_ids.contains(id))
                LOG("Duplicate known id");
            if(known_ids.contains(bid))
                LOG("Duplicate known boot id");
            known_ids.add(id, it.get());
            known_ids.add(bid, it.get());
        }
    }
}

zu32 KBScan::find(DeviceType devtype){
    if(!known_devices.contains(devtype)){
        ELOG("Unknown device!");
        return 0;
    }
    DeviceInfo dev = known_devices[devtype];

    auto filter_func = [this,devtype,dev](rawhid_detail *detail){
        switch(detail->step){
            case RAWHID_STEP_DEV:
                return (detail->vid == dev.vid && (detail->pid == dev.pid || detail->pid == dev.boot_pid));
            case RAWHID_STEP_IFACE:
                return (detail->ifclass == INTERFACE_CLASS_HID &&
                        detail->subclass == INTERFACE_SUBCLASS_NONE &&
                        detail->protocol == INTERFACE_PROTOCOL_NONE &&
                        (detail->vid == QMK_VID ? detail->ifnum == 1 : 1) &&
                        (detail->epin_size == 0 || detail->epin_size == 64) &&
                        (detail->epout_size == 0 || detail->epout_size == 64));
            case RAWHID_STEP_REPORT:
                return (detail->usage_page == UPDATE_USAGE_PAGE && detail->usage == UPDATE_USAGE);
            case RAWHID_STEP_OPEN:
                DLOG("OPEN " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
                ZPointer<HIDDevice> ptr = new HIDDevice(detail->hid);
                devices.push({ devtype, dev, ptr, !!(detail->pid & 0x1000) });
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

    auto filter_func = [this](rawhid_detail *detail){
        zu32 id = detail->pid | (detail->vid << 16);
        switch(detail->step){
            case RAWHID_STEP_DEV:
//                DLOG("DEV " << detail->bus << " " << detail->device << " " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
                DLOG("DEV " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
//                return true;
                return known_ids.contains(id);

            case RAWHID_STEP_IFACE:
                DLOG("IFACE " << detail->ifnum << ": " <<
                     detail->ifclass << " " <<
                     detail->subclass << " " <<
                     detail->protocol << " " <<
                     detail->epin_size << " " <<
                     detail->epout_size);
                return (detail->ifclass == INTERFACE_CLASS_HID &&
                        detail->subclass == INTERFACE_SUBCLASS_NONE &&
                        detail->protocol == INTERFACE_PROTOCOL_NONE &&
                        (detail->epin_size == 0 || detail->epin_size == 64) &&
                        (detail->epout_size == 0 || detail->epout_size == 64));

            case RAWHID_STEP_REPORT: {
                DLOG("USAGE " << ZString::ItoS((zu64)detail->usage_page, 16, 4) << " " << ZString::ItoS((zu64)detail->usage, 16, 2));
                if(detail->usage_page == UPDATE_USAGE_PAGE && detail->usage == UPDATE_USAGE){
                    ZBinary rdesc(detail->report_desc, detail->rdesc_len);
                    DLOG("REPORT " << detail->rdesc_len << ": " << rdesc.strBytes(1));
                    return true;
                }
                return false;
            }

            case RAWHID_STEP_OPEN: {
                DLOG("OPEN " <<
                    ZString::ItoS((zu64)detail->vid, 16, 4) << " " <<
                    ZString::ItoS((zu64)detail->pid, 16, 4) << " " <<
                    detail->ifnum
                );
                if(!known_ids.contains(id))
                    return false;
                // make high-level wrapper object
                ZPointer<HIDDevice> ptr = new HIDDevice(detail->hid);
                devices.push({
                    known_ids[id],
                    known_devices[known_ids[id]],
                    ptr,
                    !!(detail->pid & 0x1000)
                });
                // if hid pointer is taken, MUST return true here or it WILL double-free
                return true;
            }
        }
        return false;
    };

    zu32 devs = HIDDevice::openFilter(filter_func);
    DLOG("Found " << devs << " devices");
    return devs;
}

void KBScan::dbgScan(){

    auto filter_func = [this](rawhid_detail *detail){
        zu32 id = detail->pid | (detail->vid << 16);
        switch(detail->step){
            case RAWHID_STEP_DEV:
                if(known_ids.contains(id)){
                    LOG("DEV " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
                    return true;
                }
                return false;

            case RAWHID_STEP_IFACE:
                return (detail->ifclass == INTERFACE_CLASS_HID &&
                        detail->subclass == INTERFACE_SUBCLASS_NONE &&
                        detail->protocol == INTERFACE_PROTOCOL_NONE);

            case RAWHID_STEP_REPORT: {
                if(detail->usage_page == UPDATE_USAGE_PAGE && detail->usage == UPDATE_USAGE){
                    LOG("  update");
                } else if(detail->usage_page == CONSOLE_USAGE_PAGE && detail->usage == CONSOLE_USAGE){
                    LOG("  console");
                }
                return false;
            }

            case RAWHID_STEP_OPEN: {
                DLOG("OPEN " <<
                    ZString::ItoS((zu64)detail->vid, 16, 4) << " " <<
                    ZString::ItoS((zu64)detail->pid, 16, 4) << " " <<
                    detail->ifnum
                );
                if(!known_ids.contains(id))
                    return false;
                return false;
            }
        }
        return false;
    };

    zu32 devs = HIDDevice::openFilter(filter_func);
    DLOG("Found " << devs << " devices");
}

ZList<KBDevice> KBScan::open(){
    ZList<KBDevice> devs;

    // Make protocol interface object for each device
    for(auto it = devices.begin(); it.more(); ++it){
        ListDevice ldev = it.get();
        // Check device
        if(ldev.hid.get() && ldev.hid->isOpen()){
            ZPointer<KBProto> iface;
            // Select protocol
            if(ldev.dev.type == PROTO_POK3R){
                iface = new ProtoPOK3R(ldev.dev.vid, ldev.dev.pid, ldev.dev.boot_pid, ldev.boot, ldev.hid);
            } else if(ldev.dev.type == PROTO_CYKB){
                iface = new ProtoCYKB(ldev.dev.vid, ldev.dev.pid, ldev.dev.boot_pid, ldev.boot, ldev.hid);
            } else {
                ELOG("Unknown protocol");
                continue;
            }

            KBDevice kdev;
            kdev.devtype = ldev.devtype;
            kdev.info = ldev.dev;
            kdev.iface = iface;
            devs.push(kdev);

        } else {
            LOG(ldev.dev.name << " not open");
        }
    }

    return devs;
}

ZPointer<HIDDevice> KBScan::openConsole(DeviceType devtype){
    if(!known_devices.contains(devtype)){
        ELOG("Unknown device!");
        return 0;
    }
    DeviceInfo dev = known_devices[devtype];
    ZPointer<HIDDevice> ptr;

    auto filter_func = [devtype,dev,&ptr](rawhid_detail *detail){
        switch(detail->step){
            case RAWHID_STEP_DEV:
                return (!ptr.get() && (detail->vid == dev.vid && (detail->pid == dev.pid || detail->pid == dev.boot_pid)));
            case RAWHID_STEP_IFACE:
                return (detail->ifclass == INTERFACE_CLASS_HID &&
                        detail->subclass == INTERFACE_SUBCLASS_NONE &&
                        detail->protocol == INTERFACE_PROTOCOL_NONE &&
                        (detail->epin_size == 0 || detail->epin_size == 32) &&
                        (detail->epout_size == 0 || detail->epout_size == 32));
            case RAWHID_STEP_REPORT:
                return (detail->usage_page == CONSOLE_USAGE_PAGE && detail->usage == CONSOLE_USAGE);
            case RAWHID_STEP_OPEN:
                DLOG("OPEN " << ZString::ItoS((zu64)detail->vid, 16, 4) << " " << ZString::ItoS((zu64)detail->pid, 16, 4));
                ptr = new HIDDevice(detail->hid);
                // if hid pointer is taken, MUST return true here or it WILL double-free
                return true;
        }
        return false;
    };

    zu32 devs = HIDDevice::openFilter(filter_func);
    DLOG("Found " << devs << " devices");
    return ptr;
}
