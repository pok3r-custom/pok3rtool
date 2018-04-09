#ifndef KBSCAN_H
#define KBSCAN_H

#include "rawhid/hiddevice.h"
#include "updateinterface.h"

#include "zstring.h"
#include "zpointer.h"
using namespace LibChaos;

enum Device {
    DEV_NONE = 0,
    DEV_POK3R,          //!< Vortex POK3R
    DEV_POK3R_RGB,      //!< Vortex POK3R RGB
    DEV_VORTEX_CORE,    //!< Vortex Core
    DEV_VORTEX_RACE3,   //!< Vortex Race 3
    DEV_VORTEX_TESTER,  //!< Vortex 22-Key Switch Tester
    DEV_VORTEX_VIBE,    //!< Vortex ViBE
    DEV_KBP_V60,        //!< KBParadise v60 Mini
    DEV_KBP_V80,        //!< KBParadise v80
    DEV_TEX_YODA_II,    //!< Tex Yoda II

    DEV_POK3R_QMK,
};

enum DevType {
    PROTO_POK3R,    //!< Used in the POK3R and KBP keyboards.
    PROTO_CYKB,     //!< Used in Vortex keyboards marked with CYKB.
};

struct DeviceInfo {
    ZString name;
    zu16 vid;
    zu16 pid;
    zu16 boot_pid;
    DevType type;
};

struct ListDevice {
    DeviceInfo dev;
    ZPointer<HIDDevice> hid;
    bool boot;
};

struct KBDevice {
    DevType type;
    ZString name;
    zu16 vid;
    zu16 pid;
    DeviceInfo info;
    ZPointer<UpdateInterface> iface;
};

class KBScan {
public:
    KBScan();

    zu32 find(Device dev);
    zu32 scan();

    ZList<KBDevice> open();

private:
    ZList<ListDevice> devices;
};

#endif // KBSCAN_H
