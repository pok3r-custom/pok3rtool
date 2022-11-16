#ifndef PROTO_HOLTEK_H
#define PROTO_HOLTEK_H

#include "kbproto.h"
#include "proto_qmk.h"
#include "rawhid/hiddevice.h"

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

class ProtoHoltek : public ProtoQMK {
public:
    enum pok3r_cmd {
        ERASE_CMD               = 0,    //! Erase
        ERASE_PAGE_SUBCMD       = 8,    //! Page erase
        ERASE_MASS_SUBCMD       = 10,   //! Mass erase

        FLASH_CMD               = 1,    //! Read/write flash
        FLASH_CHECK_SUBCMD      = 0,    //! Compare bytes in flash with sent bytes
        FLASH_WRITE_SUBCMD      = 1,    //! Write 52 bytes to flash
        FLASH_READ_SUBCMD       = 2,    //! Read 64 bytes from flash
        FLASH_BLANK_SUBCMD      = 3,    //! Blank check bytes in flash

        CRC_CMD                 = 2,    //! CRC part of flash

        INFO_CMD                = 3,    //! Info

        RESET_CMD               = 4,    //! Reset processor
        RESET_BOOT_SUBCMD       = 0,    //! Reset to opposite firmware (main -> builtin, builtin -> main)
        RESET_BUILTIN_SUBCMD    = 1,    //! Reset to builtin firmware

        DISCONNECT_CMD          = 5,    //! Only in builtin firmware, disconnect USB and force reset
    };

public:
    //! Construct unopened device.
    ProtoHoltek(zu16 vid, zu16 pid, zu16 boot_pid);
    //! Construct open device from open HIDDevice.
    ProtoHoltek(zu16 vid, zu16 pid, zu16 boot_pid, bool builtin, ZPointer<HIDDevice> dev);

    ProtoHoltek(const ProtoHoltek &) = delete;
    ~ProtoHoltek(){}

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

    bool eraseAndCheck();

    //! Read 64 bytes at \a addr.
    bool readFlash(zu32 addr, ZBinary &bin);
    //! Write 52 bytes at \a addr.
    bool writeFlash(zu32 addr, ZBinary bin);
    //! Check 52 bytes at \a addr.
    bool checkFlash(zu32 addr, ZBinary bin);
    //! Mass erase flash.
    bool massEraseFlash();
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
    //! Get number of successful command status entries
    zu32 getCmdStatus();

private:
    bool builtin;
    bool debug;
    bool nop;
    zu16 vid;
    zu16 pid;
    zu16 boot_pid;
};

#endif // PROTO_HOLTEK_H
