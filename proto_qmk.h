#ifndef PROTO_QMK_H
#define PROTO_QMK_H

#include "kbproto.h"
#include "keymap.h"
#include "rawhid/hiddevice.h"

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

#define QMK_EE_PAGE_SIZE 0x1000
#define QMK_EE_CONF_PAGE 0x0
#define QMK_EE_KEYM_PAGE 0x1000

class ProtoQMK : public KBProto {
public:
    enum qmk_cmd {
        CMD_CTRL        = 0x81, //!< Control command.
        SUB_CT_INFO     = 0,    //!< Firmware info.
        SUB_CT_LAYOUT   = 1,    //!< Set layout.

        CMD_EEPROM      = 0x82, //!< EEPROM commands.
        SUB_EE_INFO     = 0,    //!< EEPROM info (RDID SPI command).
        SUB_EE_READ     = 1,    //!< Read EEPROM data.
        SUB_EE_WRITE    = 2,    //!< Write EEPROM data.
        SUB_EE_ERASE    = 3,    //!< Erase EEPROM page.

        CMD_KEYMAP      = 0x83, //!< Keymap commands.
        SUB_KM_INFO     = 0,    //!< Keymap info (layers, rows, cols, type size).
        SUB_KM_READ     = 1,    //!< Read keymap.
        KM_PAGE_MATRIX  = 0,    //!< Matrix page.
        KM_PAGE_LAYOUT  = 1,    //!< Layout page.
        KM_PAGE_STRS    = 2,    //!< Layout string page.
        SUB_KM_WRITE    = 2,    //!< Write to keymap.
        SUB_KM_COMMIT   = 3,    //!< Commit keymap to EEPROM.
        SUB_KM_RELOAD   = 4,    //!< Load keymap from EEPROM.
        SUB_KM_RESET    = 5,    //!< Load default keymap.

        CMD_BACKLIGHT   = 0x84, //!< Backlight commands.
        SUB_BL_INFO     = 0,    //!< Backlight map info (layers, rows, cols, type size).
        SUB_BL_READ     = 1,    //!< Read backlight map.
        SUB_BL_WRITE    = 2,    //!< Write to backlight map.
        SUB_BL_COMMIT   = 3,    //!< Commit backlight map to EEPROM.

        CMD_FLASH_QMK   = 0x85, //!< Flash commands.
        SUB_FL_READ     = 0,    //!< Read flash data.
    };

protected:
    ProtoQMK(KBType type, ZPointer<HIDDevice> dev) : KBProto(type), dev(dev){}
public:
    virtual ~ProtoQMK(){}

    virtual bool isBuiltin() = 0;
    bool isQMK();

    bool qmkInfo();
    ZString qmkVersion();

    bool eepromTest();

    //! Dump the contents of external flash / eeprom.
    ZBinary dumpEEPROM();
    //! Dump the keymap.
    bool keymapDump();

    ZBinary getMatrix();
    ZPointer<Keymap> loadKeymap();
    bool uploadKeymap(ZPointer<Keymap> keymap);

    bool getKeymapInfo(ZBinary &info);

    bool getLayouts(ZArray<ZString> &layouts);
    bool setLayout(zu8 layout);

    bool readEEPROM(zu32 addr, ZBinary &bin);
    bool writeEEPROM(zu32 addr, ZBinary bin);
    bool eraseEEPROM(zu32 addr);

    bool readKeymap(zu32 offset, ZBinary &bin);
    bool writeKeymap(zu16 offset, ZBinary bin);
    bool commitKeymap();
    bool reloadKeymap();
    bool resetKeymap();

protected:
    virtual zu32 baseFirmwareAddr() const = 0;

private:
    bool sendRecvCmdQmk(zu8 cmd, zu8 subcmd, ZBinary &data, bool quiet = false);

protected:
    ZPointer<HIDDevice> dev;
    ZBinary cachedMatrix;
};

#endif // PROTO_QMK_H
