#include "proto_qmk.h"
#include "keymap.h"
#include "keycodes.h"
#include "zlog.h"

#define UPDATE_PKT_LEN      64
#define UPDATE_ERROR        0xaaff

#define QMKID_OFFSET        0x160
#define EEPROM_LEN          0x80000
#define KM_READ_LAYOUT      0x10000
#define KM_READ_LSTRS       0x20000

#define HEX(A) (ZString::ItoS((zu64)(A), 16))

bool ProtoQMK::isQMK() {
    DLOG("isQMK");
    if(isBuiltin())
        return false;

    //dev->setStream(true);
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_CTRL, SUB_CT_INFO, data, true)){
        //dev->setStream(false);
        return false;
    }

    ArZ info = ZString(data.raw() + 4, 56).explode(';');
    if(info.size() < 1 || info[0] != "qmk_pok3r"){
        //dev->setStream(false);
        return false;
    }

    zu16 pid = data.readleu16();
    zu16 ver = data.readleu16();
    DLOG("qmk info " << HEX(pid) << " " << ver);
    return true;
}

bool ProtoQMK::qmkInfo(){
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_CTRL, SUB_CT_INFO, data, true)){
        return false;
    }
    //LOG(ZLog::RAW << data.dumpBytes(4, 8));

    zu16 pid = data.readleu16();
    zu16 ver = data.readleu16();
    LOG("pid: " << HEX(pid) << ", ver: " << ver);

    ArZ info = ZString(data.raw() + 4, 56).explode(';');
    for(zsize i = 0; i < info.size(); ++i){
        LOG(i << ": " << info[i]);
    }
    return true;
}

ZString ProtoQMK::qmkVersion(){
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_CTRL, SUB_CT_INFO, data, true)){
        return false;
    }

    ArZ info = ZString(data.raw() + 4, 56).explode(';');
    if(info.size() < 2){
        return "ERROR";
    } else {
        return info[1];
    }
}

bool ProtoQMK::eepromTest(){
    // Send command
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_EEPROM, SUB_EE_INFO, data))
        return false;

    LOG(ZLog::RAW << data.dumpBytes(4, 8));

    ZBinary test;
    test.fill(0xaa, 56);
    writeEEPROM(0x1000, test);

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
    ZBinary data;
    getKeymapInfo(data);

    const zu8 layers = data[0];
    const zu8 rows = data[1];
    const zu8 cols = data[2];
    const zu8 kcsize = data[3];
    const zu8 nlayout = data[4];
    zu8 clayout = data[5];

    const zu16 ksize = rows * cols;
    const zu16 kmsize = kcsize * rows * cols;

    ZArray<ZString> lstrs;
    getLayouts(lstrs);
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

bool ProtoQMK::getKeymapInfo(ZBinary &info){
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_INFO, info))
        return false;
    return true;
}

bool ProtoQMK::getLayouts(ZArray<ZString> &layouts){
    // Read layout strs
    ZBinary lstr;
    for(zu32 off = 0; true; off += 60){
        ZBinary dump;
        if(!readKeymap(KM_READ_LSTRS + off, dump))
            return false;
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
    layouts = ZString(lstr.asChar()).explode(',');
    return true;
}

bool ProtoQMK::setLayout(zu8 layout){
    ZBinary data;
    data.writeu8(layout);
    if(!sendRecvCmdQmk(CMD_CTRL, SUB_CT_LAYOUT, data))
        return false;
    return true;
}

bool ProtoQMK::readEEPROM(zu32 addr, ZBinary &bin){
    DLOG("readEEPROM " << HEX(addr));
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmdQmk(CMD_EEPROM, SUB_EE_READ, data))
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
    if(!sendRecvCmdQmk(CMD_EEPROM, SUB_EE_WRITE, data))
        return false;
    return true;
}

bool ProtoQMK::eraseEEPROM(zu32 addr){
    DLOG("eraseEEPROM " << HEX(addr));
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmdQmk(CMD_EEPROM, SUB_EE_ERASE, data))
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

bool ProtoQMK::resetKeymap(){
    ZBinary data;
    if(!sendRecvCmdQmk(CMD_KEYMAP, SUB_KM_RESET, data))
        return false;
    return true;
}

bool ProtoQMK::sendRecvCmdQmk(zu8 cmd, zu8 subcmd, ZBinary &data, bool quiet){
    if(data.size() > 60){
        ELOG("bad data size");
        return false;
    }

    // discard any unread data
    ZBinary tmp_buff;
    while(dev->recv(tmp_buff)){
        DLOG("discard recv");
        DLOG(ZLog::RAW << tmp_buff.dumpBytes(4, 8));
    }

    ZBinary pkt_out(UPDATE_PKT_LEN);
    pkt_out.fill(0);
    pkt_out.writeu8(cmd);    // command
    pkt_out.writeu8(subcmd); // subcommand
    pkt_out.seek(4);
    pkt_out.write(data);      // data

    pkt_out.seek(2);
    zu16 crc_out = ZHash<ZBinary, ZHashBase::CRC16>(pkt_out).hash();
    pkt_out.writeleu16(crc_out); // CRC

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
    if(!dev->recv(pkt_in)){
        ELOG("recv error");
        return false;
    }

    DLOG("recv:");
    DLOG(ZLog::RAW << pkt_in.dumpBytes(4, 8));

    if(pkt_in.size() != UPDATE_PKT_LEN){
        DLOG("bad recv size");
        return false;
    }

    pkt_in.seek(4);
    pkt_in.read(data, 60); // read data
    data.rewind();
    pkt_in.rewind();
    zu16 crc0 = pkt_in.readleu16(); // crc for request
    zu16 crc1 = pkt_in.readleu16(); // crc for response
    pkt_in.seek(2);
    pkt_in.writeleu16(0);
    zu16 crc_in = ZHash<ZBinary, ZHashBase::CRC16>(pkt_in).hash();

    // check for error
    if(crc0 == UPDATE_ERROR && crc1 == 0){
        if(quiet)
            return true;
        ELOG("command error");
        return false;
    }

    // compare response crc
    if(crc1 != crc_in){
        ELOG("response invalid crc " << HEX(crc1) << " expected " << HEX(crc_in));
        return false;
    }

    // compare request crc
    if(crc_out != crc0){
        ELOG("response out of sequence" << HEX(crc0) << " expected " << HEX(crc_out));
        return false;
    }

    return true;
}
