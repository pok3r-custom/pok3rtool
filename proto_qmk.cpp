#include "proto_qmk.h"
#include "keymap.h"
#include "keycodes.h"
#include "zlog.h"

#define QMKID_OFFSET        0x160
#define EEPROM_LEN          0x80000
#define KM_READ_LAYOUT      0x10000
#define KM_READ_LSTRS       0x20000

bool ProtoQMK::isQMK() {
    DLOG("isQMK");
    if(isBuiltin())
        return false;

    ZBinary bin;
    if(!readFlash(baseFirmwareAddr() + QMKID_OFFSET, bin))
        return false;
    bin.resize(32);

    DLOG("qmk id:");
    DLOG(ZLog::RAW << bin.dumpBytes(4, 8));

    bin.rewind();
    if(ZString(bin.raw() + 2, 9) == "qmk_pok3r"){
        zu16 pid = bin.readleu16();
        DLOG("qmk pid: " << ZString::ItoS((zu64)pid, 16));
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
    auto keymap = loadKeymap();
    if(!keymap.get()){
        ELOG("Unable to load keymap");
        return false;
    }

    keymap->printLayers();
    //keymap->printMatrix();

    return true;
}

ZBinary ProtoQMK::getMatrix(){
    DLOG("loadKeymap");
    // Send command
    ZBinary data;
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_INFO, data))
        return nullptr;

    const zu8 layers = data[0];
    const zu8 rows = data[1];
    const zu8 cols = data[2];
    const zu8 kcsize = data[3];
    const zu8 nlayout = data[4];
    const zu8 clayout = data[5];

    const zu16 kmsize = kcsize * rows * cols;

    ZBinary matrices;
    for(int l = 0; l < layers; ++l){
        ZBinary dump;
        for(zu32 off = 0; off < kmsize; off += 64){
            if(!readKeymap((kmsize * l) + off, dump))
                break;
        }
        dump.resize(kmsize);
        matrices.write(dump);
    }

    return matrices;
}

ZPointer<Keymap> ProtoQMK::loadKeymap(){
    DLOG("loadKeymap");
    // Send command
    ZBinary data;
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_INFO, data))
        return nullptr;

    const zu8 layers = data[0];
    const zu8 rows = data[1];
    const zu8 cols = data[2];
    const zu8 kcsize = data[3];
    const zu8 nlayout = data[4];
    const zu8 clayout = data[5];

    const zu16 kmsize = kcsize * rows * cols;

    // Read layout str
//  ZBinary lstr;
//  for(zu32 off = 0; true; off += 64){
//      ZBinary dump;
//      if(!readKeymap(KM_READ_LSTRS + off, dump))
//          return nullptr;
//      RLOG(dump.dumpBytes(4, 8));
//      lstr.write(dump);
//      bool nullt = false;

//      for(zu64 i = 0; i < dump.size(); ++i){
//          if(dump[i] == 0){
//              nullt = true;
//              break;
//          }
//      }
//      if(nullt)
//          break;
//  }
//  lstr.nullTerm();
//  ZString str = lstr.asChar();
//  LOG(str);

    // Read layouts
    ZArray<ZBinary> layouts;
    for(int l = 0; l < nlayout; ++l){
        ZBinary dump;
        for(zu32 off = 0; off < kmsize; off += 64){
            if(!readKeymap(KM_READ_LAYOUT + (kmsize * l) + off, dump))
                return nullptr;
        }
        dump.resize(kmsize);
        layouts.push(dump);
    }

    ZPointer<Keymap> keymap = new Keymap(rows, cols);
    keymap->loadLayout(layouts[clayout]);

    // Read each layer into keymap
    for(int l = 0; l < layers; ++l){
        ZBinary dump;
        for(zu32 off = 0; off < kmsize; off += 64){
            if(!readKeymap((kmsize * l) + off, dump))
                return nullptr;
        }
        dump.resize(kmsize);
        keymap->loadLayerMap(dump);
    }

    return keymap;
}

bool ProtoQMK::uploadKeymap(ZPointer<Keymap> keymap){
    DLOG("uploadKeymap");

    ZBinary dump = getMatrix();
    //RLOG(dump.dumpBytes(2, 16));

    ZBinary map = keymap->toMatrix();
    //LOG("Writing " << map.size() << " matrix bytes");
    //RLOG(map.dumpBytes(2, 16));

    ZBinary diff;
    zu64 offset = dump.subDiff(map, diff);
    zassert(offset != ZU64_MAX, "invalid diff");
    LOG("Diff start " << ZString::ItoS(offset, 16));
    RLOG(diff.dumpBytes(2, 16));

    for(zu32 off = 0; off < diff.size(); off += 56){
        ZBinary bin;
        diff.read(bin, 56);
        RLOG(bin.dumpBytes(2, 16));
        if(!writeKeymap(offset + off, bin))
            return false;
    }
    return true;
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

bool ProtoQMK::readKeymap(zu32 offset, ZBinary &bin){
    DLOG("readKeymap " << offset);
    // Send command
    ZBinary data;
    data.writeleu32(offset);
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_READ, data))
        return false;
    bin.write(data);
    return true;
}

bool ProtoQMK::writeKeymap(zu16 offset, ZBinary bin){
    if(bin.size() > 56){
        ELOG("keymap write too large");
        return false;
    }
    DLOG("writeKeymap " << offset << " " << bin.size());
    // Send command
    ZBinary data;
    data.writeleu16(offset);
    data.writeleu16(bin.size());
    data.write(bin);
    if(!sendRecvCmd(CMD_KEYMAP, SUB_KM_WRITE, data))
        return false;
    return true;
}
