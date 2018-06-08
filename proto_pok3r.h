#ifndef PROTO_POK3R_H
#define PROTO_POK3R_H

#include "kbproto.h"
#include "proto_qmk.h"
#include "rawhid/hiddevice.h"

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

class ProtoPOK3R : public ProtoQMK {
public:
    enum pok3r_cmd {
        ERASE_CMD               = 0,    //! Erase pages of flash
        ERASE_NOP               = 10,   //! Cancel erase

        FLASH_CMD               = 1,    //! Read/write flash
        FLASH_CHECK_SUBCMD      = 0,    //! Compare bytes in flash with sent bytes
        FLASH_WRITE_SUBCMD      = 1,    //! Write 52 bytes to flash
        FLASH_READ_SUBCMD       = 2,    //! Read 64 bytes from flash
        FLASH_3_SUBCMD          = 3,    // ?

        CRC_CMD                 = 2,    //! CRC part of flash

        UPDATE_START_CMD        = 3,    //! Start update

        RESET_CMD               = 4,    //! Reset processor
        RESET_BOOT_SUBCMD       = 0,    //! Reset to opposite firmware (main -> builtin, builtin -> main)
        RESET_BUILTIN_SUBCMD    = 1,    //! Reset to builtin firmware

        DISCONNECT_CMD          = 5,    //! Only in builtin firmware, disconnect USB and force reset

        DEBUG_CMD               = 6,    //! Collection of debug commands
        DEBUG_0_SUBCMD          = 0,    // ?
        DEBUG_1_SUBCMD          = 1,    // ?
        DEBUG_2_SUBCMD          = 2,    // ?
        DEBUG_3_SUBCMD          = 3,    // ?
        DEBUG_4_SUBCMD          = 4,    // ?
        DEBUG_5_SUBCMD          = 5,    // ?
    };

public:
    //! Construct unopened device.
    ProtoPOK3R(zu16 vid, zu16 pid, zu16 boot_pid);
    //! Construct open device from open HIDDevice.
    ProtoPOK3R(zu16 vid, zu16 pid, zu16 boot_pid, bool builtin, ZPointer<HIDDevice> dev);

    ProtoPOK3R(const ProtoPOK3R &) = delete;
    ~ProtoPOK3R(){}

    //! Find and open POK3R device.
    bool open();
    void close();
    bool isOpen() const;

    bool isBuiltin();

    //! Reset and re-open device.
    bool rebootFirmware(bool reopen = true);
    //! Reset to loader and re-open device.
    bool rebootBootloader(bool reopen = true);

    bool getInfo();

    //! Read the firmware version from the keyboard.
    ZString getVersion();

    KBStatus clearVersion();
    KBStatus setVersion(ZString version);

    //! Dump the contents of flash.
    ZBinary dumpFlash();

    //! Update the firmware.
    bool writeFirmware(const ZBinary &fwbin);

    //! Read 64 bytes at \a addr.
    bool readFlash(zu32 addr, ZBinary &bin);
    //! Write 52 bytes at \a addr.
    bool writeFlash(zu32 addr, ZBinary bin);
    //! Check 52 bytes at \a addr.
    bool checkFlash(zu32 addr, ZBinary bin);
    //! Erase flash pages starting at \a start, ending on the page of \a end.
    bool eraseFlash(zu32 start, zu32 end);

    //! Send CRC command.
    zu16 crcFlash(zu32 addr, zu32 len);

private:
    zu32 baseFirmwareAddr() const;
    //! Send command
    bool sendCmd(zu8 cmd, zu8 subcmd, ZBinary bin = ZBinary());
    //! Send command and recv response.
    bool sendRecvCmd(zu8 cmd, zu8 subcmd, ZBinary &data);

public:
    static void decode_firmware(ZBinary &bin);
    static void encode_firmware(ZBinary &bin);

private:
    bool builtin;
    bool debug;
    bool nop;
    zu16 vid;
    zu16 pid;
    zu16 boot_pid;
};

#endif // PROTO_POK3R_H
