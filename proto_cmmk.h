#ifndef PROTO_CMMK_H
#define PROTO_CMMK_H

#include "kbproto.h"
#include "proto_qmk.h"
#include "rawhid/hiddevice.h"

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

class ProtoCMMK : public ProtoQMK {
public:
    enum cm_cmd {
        ERASE_CMD               = 0,    //!< Erase pages of flash
        ERASE_NOP               = 10,   //!< Cancel erase (untested)

        FLASH_CMD               = 1,    //!< Flash command
        FLASH_CHECK_SUBCMD      = 0,    //!< Compare bytes in flash with sent bytes
        FLASH_WRITE_SUBCMD      = 1,    //!< Write 52 bytes
        FLASH_READ_VER_SUBCMD   = 2,    //!< Read version from flash

        READ_CMD                = 0x12, //!< Read command
        READ_UNK1_SUBCMD        = 0x0,
        READ_UNK2_SUBCMD        = 0x1,
        READ_VER1_SUBCMD        = 0x20, //!< Read version string
        READ_VER2_SUBCMD        = 0x22, //!< Read version data
        READ_ADDR_SUBCMD        = 0xff, //!< Patched command, read arbitrary address

        UPDATE_START_CMD        = 3,    //!< Start update, get info (?)

        RESET_CMD               = 4,    //!< Reset command
        RESET_BOOT_SUBCMD       = 0,    //!< Reset to opposite firmware (main -> builtin, builtin -> main)
        RESET_BUILTIN_SUBCMD    = 1,    //!< Reset to builtin firmware

        RESET_ALT_CMD           = 0x11, //!< Alternative reset to builtin command
        RESET_BOOT_ALT_CMD      = 0x1,  //!< Reset to firmware
        RESET_BUILTIN_ALT_CMD   = 0x0,  //!< Reset to builtin firmware
    };

public:
    //! Construct unopened device.
    ProtoCMMK(zu16 vid, zu16 pid, zu16 boot_pid);
    //! Construct open device from open HIDDevice.
    ProtoCMMK(zu16 vid, zu16 pid, zu16 boot_pid, bool builtin, ZPointer<HIDDevice> dev, zu32 fw_addr);

    ProtoCMMK(const ProtoCMMK &) = delete;
    ~ProtoCMMK();

    //! Find and open keyboard device.
    bool open();
    void close();
    bool isOpen() const;

    bool isBuiltin();

    //! Reset and re-open device.
    bool rebootFirmware(bool reopen = true);
    //! Reset to bootloader and re-open device.
    bool rebootBootloader(bool reopen = true);

    bool getInfo();

    //! Read the firmware version from the keyboard.
    ZString getVersion();

    KBStatus clearVersion();
    KBStatus setVersion(ZString version);

    //! Dump the contents of flash.
    ZBinary dump(zu32 address, zu32 len);
    ZBinary dumpFlash();
    //! Update the firmware.
    bool writeFirmware(const ZBinary &fwbin);

    bool eraseAndCheck();

    void test();

    //! Erase flash pages starting at \a start, ending on the page of \a end.
    bool eraseFlash(zu32 start, zu32 end);
    //! Read 64 bytes at \a addr.
    bool readFlash(zu32 addr, ZBinary &bin);
    //! Write 52 bytes at \a addr.
    bool writeFlash(zu32 addr, ZBinary bin);
    //! Check 52 bytes at \a addr.
    bool checkFlash(zu32 addr, ZBinary bin);

private:
    zu32 baseFirmwareAddr() const;
    //! Send command
    bool sendCmd(zu8 cmd, zu8 a1, ZBinary data = ZBinary());
    //! Recv command.
    bool recvCmd(ZBinary &data);
    //! Send command and recv response.
    bool sendRecvCmd(zu8 cmd, zu8 a1, ZBinary &data);

public:
    static void decode_firmware(ZBinary &bin);
    static void encode_firmware(ZBinary &bin);

    static void info_section(ZBinary data);

private:
    bool builtin;
    bool debug;
    bool nop;
    zu16 vid;
    zu16 pid;
    zu16 boot_pid;
    zu32 fw_addr;
};

#endif // PROTO_CMMK_H
