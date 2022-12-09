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

#define BOOT_PID                0x1000

#define POK3R_PID               0x0141
#define POK3R_RGB_PID           0x0167
#define POK3R_RGB2_PID          0x0207
#define VORTEX_CORE_PID         0x0175
//#define VORTEX_TESTER_PID       0x0200
#define VORTEX_RACE3_PID        0x0192
#define VORTEX_VIBE_PID         0x0216
#define VORTEX_CYPHER_PID       0x0282
#define VORTEX_TAB60_PID        0x0304
#define VORTEX_TAB75_PID        0x0344
#define VORTEX_TAB90_PID        0x0346
#define KBP_V60_PID             0x0112
#define KBP_V80_PID             0x0129
//#define KBP_V100_PID            0x0
#define TEX_YODA_II_PID         0x0163
#define MISTEL_MD600_PID        0x0143
#define MISTEL_MD200_PID        0x0200

#define FW_ADDR_2C00            0x2c00
#define FW_ADDR_3200            0x3200
#define FW_ADDR_3400            0x3400

#define CONSOLE_USAGE_PAGE      0xff31
#define CONSOLE_USAGE           0x0074

static const ZMap<DeviceType, DeviceInfo> known_devices = {
    { DEV_POK3R,            { "vortex/pok3r",       "Vortex POK3R",             HOLTEK_VID, POK3R_PID,          BOOT_PID | POK3R_PID,           PROTO_POK3R,    FW_ADDR_2C00 } },
    { DEV_POK3R_RGB,        { "vortex/pok3r_rgb",   "Vortex POK3R RGB",         HOLTEK_VID, POK3R_RGB_PID,      BOOT_PID | POK3R_RGB_PID,       PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_POK3R_RGB2,       { "vortex/pok3r_rgb2",  "Vortex POK3R RGB2",        HOLTEK_VID, POK3R_RGB2_PID,     BOOT_PID | POK3R_RGB2_PID,      PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_VORTEX_CORE,      { "vortex/core",        "Vortex Core",              HOLTEK_VID, VORTEX_CORE_PID,    BOOT_PID | VORTEX_CORE_PID,     PROTO_CYKB,     FW_ADDR_3400 } },
//  { DEV_VORTEX_TESTER,    { "vortex/tester",      "Vortex Tester",            HOLTEK_VID, VORTEX_TESTER_PID,  BOOT_PID | VORTEX_TESTER_PID,   PROTO_CYKB,     FW_ADDR_3400 } }, // same as MD200
    { DEV_VORTEX_RACE3,     { "vortex/race3",       "Vortex Race 3",            HOLTEK_VID, VORTEX_RACE3_PID,   BOOT_PID | VORTEX_RACE3_PID,    PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_VORTEX_VIBE,      { "vortex/vibe",        "Vortex ViBE",              HOLTEK_VID, VORTEX_VIBE_PID,    BOOT_PID | VORTEX_VIBE_PID,     PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_VORTEX_CYPHER,    { "vortex/cypher",      "Vortex Cypher",            HOLTEK_VID, VORTEX_CYPHER_PID,  BOOT_PID | VORTEX_CYPHER_PID,   PROTO_CYKB,     FW_ADDR_3200 } },
    { DEV_VORTEX_TAB60,     { "vortex/tab60",       "Vortex Tab 60",            HOLTEK_VID, VORTEX_TAB60_PID,   BOOT_PID | VORTEX_TAB60_PID,    PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_VORTEX_TAB75,     { "vortex/tab75",       "Vortex Tab 75",            HOLTEK_VID, VORTEX_TAB75_PID,   BOOT_PID | VORTEX_TAB75_PID,    PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_VORTEX_TAB90,     { "vortex/tab90",       "Vortex Tab 90",            HOLTEK_VID, VORTEX_TAB90_PID,   BOOT_PID | VORTEX_TAB90_PID,    PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_KBP_V60,          { "kbp/v60",            "KBP V60",                  HOLTEK_VID, KBP_V60_PID,        BOOT_PID | KBP_V60_PID,         PROTO_POK3R,    FW_ADDR_2C00 } },
    { DEV_KBP_V80,          { "kbp/v80",            "KBP V80",                  HOLTEK_VID, KBP_V80_PID,        BOOT_PID | KBP_V80_PID,         PROTO_POK3R,    FW_ADDR_2C00 } },
//  { DEV_KBP_V100,         { "kbp/100",            "KBP V100",                 HOLTEK_VID, KBP_V100_PID,       BOOT_PID | KBP_V100_PID,        PROTO_POK3R,    FW_ADDR_2C00 } },
    { DEV_TEX_YODA_II,      { "tex/yoda",           "Tex Yoda II",              HOLTEK_VID, TEX_YODA_II_PID,    BOOT_PID | TEX_YODA_II_PID,     PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_MISTEL_MD600,     { "mistel/md600",       "Mistel Barocco MD600",     HOLTEK_VID, MISTEL_MD600_PID,   BOOT_PID | MISTEL_MD600_PID,    PROTO_CYKB,     FW_ADDR_3400 } },
    { DEV_MISTEL_MD200,     { "mistel/md200",       "Mistel Freeboard MD200",   HOLTEK_VID, MISTEL_MD200_PID,   BOOT_PID | MISTEL_MD200_PID,    PROTO_CYKB,     FW_ADDR_3400 } },
};

static ZMap<zu32, DeviceType> known_ids;

KBScan::KBScan(){
    if(!known_ids.size()){
        for(auto it = known_devices.begin(); it.more(); ++it){
            DeviceInfo info = known_devices[it.get()];
            zu32 id = zu32(info.pid | (info.vid << 16));
            zu32 bid = zu32(info.boot_pid | (info.vid << 16));
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
                iface = new ProtoCYKB(ldev.dev.vid, ldev.dev.pid, ldev.dev.boot_pid, ldev.boot, ldev.hid, ldev.dev.fw_addr);
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
