#include "kbproto.h"

#include "zlog.h"

KBProto::KBProto(KBType type) : _type(type){

}

bool KBProto::update(ZString version, const ZBinary &fwbin){
    // Reset to bootloader
    if(!rebootBootloader())
        return false;

    LOG("Current Version: " << getVersion());

    auto status = clearVersion();
    if(status != SUCCESS)
        return status;

    if(!writeFirmware(fwbin))
        return false;

    status = setVersion(version);
    if(status != SUCCESS)
        return status;

    // Reset to firmware
    if(!rebootFirmware(false))
        return false;
    return true;
}
