#ifndef PROTO_QMK_H
#define PROTO_QMK_H

#include "kbproto.h"
#include "keymap.h"
#include "rawhid/hiddevice.h"

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

class ProtoQMK : public KBProto {
public:
    enum qmk_cmd {
        QMK_INFO                = 0x81,
        QMK_EEPROM              = 0x82,
        QMK_EEPROM_INFO         = 0,
        QMK_EEPROM_READ         = 1,
        QMK_EEPROM_WRITE        = 2,
        QMK_EEPROM_ERASE        = 3,

        CMD_KEYMAP              = 0x83, //!< Keymap commands.
        SUB_KM_INFO             = 0,    //!< Keymap info (layers, rows, cols, type size).
        SUB_KM_READ             = 1,    //!< Read keymap.
        SUB_KM_WRITE            = 2,    //!< Write to keymap.
        SUB_KM_COMMIT           = 3,    //!< Commit keymap to EEPROM.

        CMD_BACKLIGHT           = 0x84, //!< Backlight commands.
        SUB_BL_INFO             = 0,    //!< Backlight map info (layers, rows, cols, type size).
        SUB_BL_READ             = 1,    //!< Read backlight map.
        SUB_BL_WRITE            = 2,    //!< Write to backlight map.
        SUB_BL_COMMIT           = 3,    //!< Commit backlight map to EEPROM.
    };

protected:
    ProtoQMK(KBType type) : KBProto(type){}
public:
    virtual ~ProtoQMK(){}

    virtual bool isBuiltin() = 0;
    bool isQMK();

    bool eepromTest();

    //! Dump the contents of external flash / eeprom.
    ZBinary dumpEEPROM();
    //! Dump the keymap.
    bool keymapDump();

    ZBinary getMatrix();
    ZPointer<Keymap> loadKeymap();
    bool uploadKeymap(ZPointer<Keymap> keymap);

    bool readEEPROM(zu32 addr, ZBinary &bin);
    bool writeEEPROM(zu32 addr, ZBinary bin);
    bool eraseEEPROM(zu32 addr);

    bool readKeymap(zu32 offset, ZBinary &bin);
    bool writeKeymap(zu16 offset, ZBinary bin);

    virtual bool readFlash(zu32 addr, ZBinary &bin) = 0;

protected:
    virtual zu32 baseFirmwareAddr() const = 0;
    virtual bool sendCmd(zu8 cmd, zu8 subcmd, ZBinary bin = ZBinary()) = 0;
    virtual bool sendRecvCmd(zu8 cmd, zu8 subcmd, ZBinary &data) = 0;

};

#endif // PROTO_QMK_H
