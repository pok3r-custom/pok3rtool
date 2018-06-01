#include "proto_qmk.h"
#include "keymap.h"
#include "keycodes.h"
#include "zlog.h"

#define UPDATE_PKT_LEN      64

#define QMKID_OFFSET        0x160
#define EEPROM_LEN          0x80000
#define KM_READ_LAYOUT      0x10000
#define KM_READ_LSTRS       0x20000

#define HEX(A) (ZString::ItoS((zu64)(A), 16))

bool ProtoQMK::isQMK() {
    DLOG("isQMK");
    if(isBuiltin())
        return false;

    ZBinary bin;
    if(!readFlash(baseFirmwareAddr() + QMKID_OFFSET, bin, true))
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
    if(!sendRecvCmdQmk(QMK_EEPROM, SUB_EE_INFO, data))
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
    for(zu32 addr = 0; addr < EEPROM_LEN; addr += 60){
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

    LOG("Keymap Dump: " << keymap->numKeys() << " keys, " << keymap->numLayers() << " layers");
    LOG("Current Layout: " << keymap->layoutName());

    keymap->printLayers();
    //keymap->printMatrix();

    return true;
}

ZBinary ProtoQMK::getMatrix(){
    DLOG("loadKeymap");
    // Send command
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_INFO, data))
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
        for(zu32 off = 0; off < kmsize; off += 60){
            if(!readKeymap((kmsize * l) + off, dump))
                break;
        }
        dump.resize(kmsize);
        matrices.write(dump);
    }
    cachedMatrix = matrices;

    return matrices;
}

ZPointer<Keymap> ProtoQMK::loadKeymap(){
    DLOG("loadKeymap");
    // Send command
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_INFO, data))
        return nullptr;

    const zu8 layers = data[0];
    const zu8 rows = data[1];
    const zu8 cols = data[2];
    const zu8 kcsize = data[3];
    const zu8 nlayout = data[4];
    zu8 clayout = data[5];

    const zu16 ksize = rows * cols;
    const zu16 kmsize = kcsize * rows * cols;

    // Read layout strs
    ZBinary lstr;
    for(zu32 off = 0; true; off += 60){
        ZBinary dump;
        if(!readKeymap(KM_READ_LSTRS + off, dump))
            return nullptr;
        lstr.write(dump);

        bool nullt = false;
        for(zu64 i = 0; i < dump.size(); ++i){
            if(dump[i] == 0){
                nullt = true;
                break;
            }
        }
        if(nullt)
            break;
    }
    lstr.nullTerm();
    ZArray<ZString> lstrs = ZString(lstr.asChar()).explode(',');
    zassert(lstrs.size() == nlayout, "Layout string count does not match num layouts");

    // Read layouts
    ZArray<ZBinary> layouts;
    for(int l = 0; l < nlayout; ++l){
        ZBinary dump;
        for(zu32 off = 0; off < ksize; off += 60){
            if(!readKeymap(KM_READ_LAYOUT + (ksize * l) + off, dump))
                return nullptr;
        }
        dump.resize(ksize);
        layouts.push(dump);
    }

    zassert(clayout < layouts.size(), "Invalid current layout");

    ZPointer<Keymap> keymap = new Keymap(rows, cols);
    keymap->loadLayout(lstrs[clayout], layouts[clayout]);

    // Read each layer into keymap
    ZBinary matrices;
    for(int l = 0; l < layers; ++l){
        ZBinary dump;
        for(zu32 off = 0; off < kmsize; off += 60){
            if(!readKeymap((kmsize * l) + off, dump))
                return nullptr;
        }
        dump.resize(kmsize);
        matrices.write(dump);
        keymap->loadLayerMap(dump);
    }
    cachedMatrix = matrices;

    return keymap;
}

bool ProtoQMK::uploadKeymap(ZPointer<Keymap> keymap){
    DLOG("uploadKeymap");

    ZBinary dump = cachedMatrix;
    // use cached matrix if possible
    if(!dump.size())
        dump = getMatrix();
    //RLOG(dump.dumpBytes(2, 16));

    ZBinary map = keymap->toMatrix();
    //LOG("Writing " << map.size() << " matrix bytes");
    //RLOG(map.dumpBytes(2, 16));

    ZBinary diff;
    zu64 offset = dump.subDiff(map, diff);
    zassert(offset != ZU64_MAX, "invalid diff");
    LOG("Keymap Diff:");
    RLOG(diff.dumpBytes(2, 16, offset));

    for(zu32 off = 0; off < diff.size(); off += 56){
        ZBinary bin;
        diff.read(bin, 56);
        //RLOG(bin.dumpBytes(2, 16));
        if(!writeKeymap(offset + off, bin))
            return false;
    }

    // update cached matrix
    cachedMatrix = map;
    return true;
}

bool ProtoQMK::readEEPROM(zu32 addr, ZBinary &bin){
    DLOG("readEEPROM " << HEX(addr));
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmdQmk(QMK_EEPROM, SUB_EE_READ, data))
        return false;
    bin.write(data);
    return true;
}

bool ProtoQMK::writeEEPROM(zu32 addr, ZBinary bin){
    DLOG("writeEEPROM " << HEX(addr));
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    data.write(bin);
    if(!sendRecvCmdQmk(QMK_EEPROM, SUBB_EE_WRITE, data))
        return false;
    return true;
}

bool ProtoQMK::eraseEEPROM(zu32 addr){
    DLOG("eraseEEPROM " << HEX(addr));
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmdQmk(QMK_EEPROM, SUB_EE_ERASE, data))
        return false;
    return true;
}

bool ProtoQMK::readKeymap(zu32 offset, ZBinary &bin){
    DLOG("readKeymap " << HEX(offset));
    // Send command
    ZBinary data;
    data.writeleu32(offset);
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_READ, data))
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
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_WRITE, data))
        return false;
    return true;
}

bool ProtoQMK::commitKeymap(){
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_COMMIT, data))
        return false;
    return true;
}

bool ProtoQMK::reloadKeymap(){
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_RELOAD, data))
        return false;
    return true;
}

bool ProtoQMK::sendRecvCmdQmk(zu8 cmd, zu8 subcmd, ZBinary &data){
    if(data.size() > 60){
        ELOG("bad data size");
        return false;
    }

    ZBinary pkt_out(UPDATE_PKT_LEN);
    pkt_out.fill(0);
    pkt_out.writeu8(cmd);    // command
    pkt_out.writeu8(subcmd); // subcommand
    pkt_out.seek(4);
    pkt_out.write(data);      // data

    pkt_out.seek(2);
    zu16 crc = ZHash<ZBinary, ZHashBase::CRC16>(pkt_out).hash();
    pkt_out.writeleu16(crc); // CRC

    DLOG("send:");
    DLOG(ZLog::RAW << pkt_out.dumpBytes(4, 8));

    // Send command (interrupt write)
    if(!dev->send(pkt_out)){
        ELOG("send error");
        return false;
    }

    // Recv packet
    ZBinary pkt_in;
    pkt_in.resize(UPDATE_PKT_LEN);
    if(!dev->recvStream(pkt_in)){
        ELOG("recv error");
        return false;
    }

    DLOG("recv:");
    DLOG(ZLog::RAW << pkt_in.dumpBytes(4, 8));

    if(pkt_in.size() != UPDATE_PKT_LEN){
        DLOG("bad recv size");
        return false;
    }

    zu16 crc0 = pkt_in.readleu16();
    zu16 crc1 = pkt_in.readleu16();
    pkt_in.seek(2);
    pkt_in.writeleu16(0);
    zu16 crc2 = ZHash<ZBinary, ZHashBase::CRC16>(pkt_in).hash();

    if(crc1 != crc2){
        ELOG("corrupt response");
        return false;
    }

    if(crc != crc0){
        ELOG("packets out of sequence");
        return false;
    }

    pkt_in.seek(4);
    pkt_in.read(data, 60);
    data.rewind();
    return true;
}
