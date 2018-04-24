#ifndef KBPROTO_H
#define KBPROTO_H

#include "zstring.h"
#include "zbinary.h"
using namespace LibChaos;

#define UPDATE_USAGE_PAGE       0xff00
#define UPDATE_USAGE            0x01

enum KBType {
    PROTO_POK3R,    //!< Used in the POK3R and KBP keyboards.
    PROTO_CYKB,     //!< Used in Vortex keyboards marked with CYKB.
};

enum KBStatus {
    ERR_FALSE = false,
    ERR_TRUE = true,
    SUCCESS = ERR_TRUE,
    ERR_NOT_IMPLEMENTED,    //! Function not implemented.
    ERR_USAGE,              //! Incorrect function usage.
    ERR_IO,                 //! I/O error.
    ERR_FLASH,              //! Incorrect flash value.
    ERR_CRC,                //! Incorrect CRC value.
    ERR_FAIL = ERR_FALSE,   //! General failure (TODO where used).
};

class KBProto {
protected:
    KBProto(KBType type);
public:
    virtual ~KBProto(){}

    //! Open device.
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual bool isBuiltin(){ return false; }
    virtual bool isQMK(){ return false; }

    //! Reset to firmware and re-open device if needed.
    virtual bool rebootFirmware(bool reopen = true) = 0;
    //! Reset to bootloader and re-open device if needed.
    virtual bool rebootBootloader(bool reopen = true) = 0;

    //! Read the firmware version from the device.
    virtual ZString getVersion() = 0;

    virtual KBStatus clearVersion(){ return ERR_NOT_IMPLEMENTED; }
    virtual KBStatus setVersion(ZString version, zu8 opt_byte = 0){ return ERR_NOT_IMPLEMENTED; }

    virtual bool getInfo(){ return false; }
    virtual bool eepromTest(){ return false; }

    //! Dump the contents of flash.
    virtual ZBinary dumpFlash(){ return ZBinary(); }
    //! Dump the contents of external flash / eeprom.
    virtual ZBinary dumpEEPROM(){ return ZBinary(); }
    //! Dump the keymap.
    virtual bool keymapDump(){ return false; }

    //! Write the firmware.
    virtual bool writeFirmware(const ZBinary &fwbin) = 0;

    //! Complete update wrapper.
    virtual bool update(ZString version, const ZBinary &fwbin);

    virtual bool eraseAndCheck(){ return false; }

    KBType type() { return _type; }

private:
    KBType _type;
};

#endif // KBPROTO_H
