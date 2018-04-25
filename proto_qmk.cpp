#include "proto_qmk.h"
#include "keymap.h"
#include "zlog.h"

#define VER_ADDR            0x2800
#define FW_ADDR             0x2c00

#define EEPROM_LEN          0x80000

bool ProtoQMK::isQMK() {
    if(isBuiltin())
        return false;

    ZBinary bin;
    if(!readFlash(FW_ADDR + 0x160, bin))
        return false;

    //LOG(ZLog::RAW << bin.dumpBytes(4, 8));
    if(!bin.raw()){
        ELOG("isQMK error");
        return false;
    }

    bin.rewind();
    if(ZString(bin.raw() + 2, 9) == "qmk_pok3r"){
        //zu16 pid = bin.readleu16();
        return true;
    }
    return false;
}

bool ProtoQMK::eepromTest(){
    // Send command
    ZBinary data;
    if(!sendRecvCmd(QMK_EEPROM, QMK_EEPROM_INFO, data))
        return false;

    LOG(ZLog::RAW << data.dumpBytes(4, 8));

    /*
    ZBinary test;
    test.fill(0xaa, 56);
    writeEEPROM(0x1000, test);
    */

    //eraseEEPROM(0x0000);

    return true;
}

ZBinary ProtoQMK::dumpEEPROM(){
    ZBinary dump;
    zu32 cp = EEPROM_LEN / 10;
    int perc = 0;
    RLOG(perc << "%...");
    for(zu32 addr = 0; addr < EEPROM_LEN; addr += 64){
        if(!readEEPROM(addr, dump))
            break;

        if(addr >= cp){
            perc += 10;
            RLOG(perc << "%...");
            cp += EEPROM_LEN / 10;
        }
    }
    RLOG("100%" << ZLog::NEWLN);

    return dump;
}

bool ProtoQMK::keymapDump(){
    // Send command
    ZBinary data;
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_INFO, data))
        return false;

    const int layers = data[0];
    const int rows = data[1];
    const int cols = data[2];
    const int kcsize = data[3];
    const int kmsize = kcsize * rows * cols;

    LOG("Keymap Size: " << kmsize << ", " << layers << " layers");

    for(int l = 0; l < layers; ++l){
        ZBinary dump;
        for(zu32 off = 0; off < kmsize; off += 64){
            if(!readKeymap((kmsize * l) + off, dump))
                break;
        }
        dump.rewind();
        dump.resize(kmsize);

        LOG("Layer " << l << ":");
        LOG(ZLog::RAW << dump.dumpBytes(2, 16));
        for(int i = 0; i < rows; ++i){
            for(int j = 0; j < cols; ++j){
                zu16 kc = dump.readleu16();
                ZString str = Keymap::keycodeAbbrev(kc);
                //LOG(i << "," << j << ": " << str);
                RLOG(ZString(str).pad(' ', 8));
            }
            RLOG(ZLog::NEWLN);
        }
    }
}

bool ProtoQMK::readEEPROM(zu32 addr, ZBinary &bin){
    DLOG("readEEPROM " << addr);
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmd(QMK_EEPROM, QMK_EEPROM_READ, data))
        return false;
    bin.write(data);
    return true;
}

bool ProtoQMK::writeEEPROM(zu32 addr, ZBinary bin){
    DLOG("writeEEPROM " << addr);
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    data.write(bin);
    if(!sendRecvCmd(QMK_EEPROM, QMK_EEPROM_WRITE, data))
        return false;
    return true;
}

bool ProtoQMK::eraseEEPROM(zu32 addr){
    DLOG("eraseEEPROM " << addr);
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmd(QMK_EEPROM, QMK_EEPROM_ERASE, data))
        return false;
    return true;
}

bool ProtoQMK::readKeymap(zu32 addr, ZBinary &bin){
    DLOG("readKeymap " << addr);
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_READ, data))
        return false;
    bin.write(data);
    return true;
}

bool ProtoQMK::writeKeymap(zu32 addr, ZBinary bin){
    DLOG("writeKeymap " << addr);
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    data.write(bin);
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_WRITE, data))
        return false;
    return true;
}
