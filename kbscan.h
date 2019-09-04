#ifndef KBSCAN_H
#define KBSCAN_H

#include "kbproto.h"
#include "rawhid/hiddevice.h"

#include "zstring.h"
#include "zpointer.h"
using namespace LibChaos;

enum DeviceType {
    DEV_NONE = 0,
    DEV_POK3R,          //!< Vortex POK3R
    DEV_POK3R_RGB,      //!< Vortex POK3R RGB
    DEV_POK3R_RGB2,     //!< Vortex POK3R RGB (PCB v2)
    DEV_VORTEX_CORE,    //!< Vortex Core
    DEV_VORTEX_RACE3,   //!< Vortex Race 3
//  DEV_VORTEX_TESTER,  //!< Vortex 22-Key Switch Tester (same as MD200)
    DEV_VORTEX_VIBE,    //!< Vortex ViBE
    DEV_VORTEX_CYPHER,  //!< Vortex Cypher
    DEV_VORTEX_TAB60,   //!< Vortex Tab 60
    DEV_VORTEX_TAB75,   //!< Vortex Tab 75
    DEV_VORTEX_TAB90,   //!< Vortex Tab 90
    DEV_KBP_V60,        //!< KBParadise v60 Mini
    DEV_KBP_V80,        //!< KBParadise v80
    DEV_TEX_YODA_II,    //!< Tex Yoda II
    DEV_MISTEL_MD600,   //!< Mistel Barocco MD600
    DEV_MISTEL_MD200,   //!< Mistel Freeboard MD200

    DEV_QMK_POK3R,
    DEV_QMK_POK3R_RGB,
    DEV_QMK_VORTEX_CORE,
};

struct DeviceInfo {
    ZString slug;
    ZString name;
    zu16 vid;
    zu16 pid;
    zu16 boot_pid;
    KBType type;
    zu32 fw_addr;
};

struct ListDevice {
    DeviceType devtype;
    DeviceInfo dev;
    ZPointer<HIDDevice> hid;
    bool boot;
};

struct KBDevice {
    KBType type;
    DeviceType devtype;
    DeviceInfo info;
    ZPointer<KBProto> iface;
};

class KBScan {
public:
    KBScan();

    zu32 find(DeviceType devtype);
    zu32 scan();

    void dbgScan();

    ZList<KBDevice> open();

    static ZPointer<HIDDevice> openConsole(DeviceType devtype);

private:
    ZList<ListDevice> devices;
};

#endif // KBSCAN_H
