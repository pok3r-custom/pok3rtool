#include "proto_holtek.h"
#include "zlog.h"

#define UPDATE_PKT_LEN          64

#define FW_ADDR                 0x0000

#define HT32F52352_FLASH_LEN    0x20000
#define HT32F1654_FLASH_LEN     0x10000

#define REBOOT_SLEEP            3
#define ERASE_SLEEP             5

#define HEX(A) (ZString::ItoS((zu64)(A), 16))

ProtoHoltek::ProtoHoltek(zu16 vid_, zu16 pid_, zu16 boot_pid_) :
    ProtoQMK(PROTO_HOLTEK, new HIDDevice),
    builtin(false), debug(false), nop(false),
    vid(vid_), pid(pid_), boot_pid(boot_pid_)
{

}

ProtoHoltek::ProtoHoltek(zu16 vid_, zu16 pid_, zu16 boot_pid_, bool builtin_, ZPointer<HIDDevice> dev_) :
    ProtoQMK(PROTO_HOLTEK, dev_),
    builtin(builtin_), debug(false), nop(false),
    vid(vid_), pid(pid_), boot_pid(boot_pid_)
{
}

bool ProtoHoltek::open(){
    if(dev->open(vid, boot_pid, UPDATE_USAGE_PAGE, UPDATE_USAGE)){
        builtin = true;
        return true;
    }
    return false;
}

void ProtoHoltek::close(){
    dev->close();
}

bool ProtoHoltek::isOpen() const {
    return dev->isOpen();
}

bool ProtoHoltek::isBuiltin() {
    return builtin;
}

bool ProtoHoltek::rebootFirmware(bool reopen){
    if(!sendCmd(RESET_CMD, RESET_BOOT_SUBCMD))
        return false;
    close();

    if(reopen){
        ZThread::sleep(REBOOT_SLEEP);
        if(!open()){
            ELOG("open error");
            return false;
        }
    }
    return true;
}

bool ProtoHoltek::rebootBootloader(bool reopen){
    if(!sendCmd(RESET_CMD, RESET_BUILTIN_SUBCMD))
        return false;
    close();

    if(reopen){
        ZThread::sleep(REBOOT_SLEEP);
        if(!open()){
            ELOG("open error");
            return false;
        }

        if(!builtin)
            return false;
    }
    return true;
}

bool ProtoHoltek::getInfo(){
    ZBinary data;
    if(!sendRecvCmd(INFO_CMD, 0, data))
        return false;

    RLOG(data.dumpBytes(4, 8));

    data.seek(2);
    zu16 ver = data.readleu16();
    LOG("ISP version: v" << HEX(ver));

    zu32 model;
    switch(ver){
        case 257: // v101
            data.seek(16);
            model = data.readleu32();
            break;
        case 256: // v100
            data.rewind();
            model = (zu32)data.readleu16();
            break;
        default: // unknown format
            model = 0;
    }

    if(model != 0){
        LOG("model: HT32F" << HEX(model));
    } else {
        LOG("model: unknown");
    }

    data.seek(6);
    zu16 page_size = data.readleu16();
    LOG("page size in bytes: " << page_size);

    data.seek(8);
    zu32 flash_size = page_size * data.readleu16();
    LOG("flash size in bytes: " << flash_size);

    // TODO Read Flash Security and Protection

    return true;
}

ZString ProtoHoltek::getVersion(){
    DLOG("getVersion");

    ZBinary data;
    if(!sendRecvCmd(INFO_CMD, 0, data))
        return false;

    data.seek(2);
    ZString ver = HEX(data.readleu16());
    DLOG("version: " << ver);

    return ver;
}

KBStatus ProtoHoltek::clearVersion(){
    // Cannot write to bootrom
    // but report as cleared anyway
    return SUCCESS;
}

KBStatus ProtoHoltek::setVersion(ZString version){
    // Cannot write to bootrom
    // but report as set anyway
    return SUCCESS;
}

ZBinary ProtoHoltek::dumpFlash(){
    ZBinary dump;
    zu32 cp = HT32F1654_FLASH_LEN / 10;
    int perc = 0;
    RLOG(perc << "%...");
    for(zu32 addr = 0; addr < HT32F1654_FLASH_LEN; addr += 64){
        if(!readFlash(addr, dump))
            break;

        if(addr >= cp){
            perc += 10;
            RLOG(perc << "%...");
            cp += HT32F1654_FLASH_LEN / 10;
        }
    }
    RLOG("100%" << ZLog::NEWLN);

    return dump;
}

bool ProtoHoltek::writeFirmware(const ZBinary &fwbinin){
    ZBinary fwbin = fwbinin;

    // TODO check page and flash size with info command

    LOG("Mass Erase...");
    if(!massEraseFlash()){
        ELOG("mass erase error");
        return false;
    }

    ZThread::sleep(ERASE_SLEEP);

    // reset
    LOG("Mass erase reset");
    if(!rebootBootloader(true))
        return false;

    // clear option bytes
    if(!eraseFlash(0x1ff00000, 0x1ff00400)){
        return false;
    }
    ZBinary ob(0x400);
    ob.fill(0xff);
    for(zu64 o = 0; o < 0x400; o += 52){
        ZBinary packet;
        ob.read(packet, 52);
        if(!writeFlash(0x1ff00000 + o, packet)){
            return false;
        }
    }

    // erase pages
    if(!eraseFlash(0x0, 0xfbff)){ // HT32F1654
        ELOG("erase error");
        return false;
    }

    ZThread::sleep(ERASE_SLEEP);

    // Write firmware
    LOG("Write...");
    for(zu64 o = 0; o < fwbin.size(); o += 52){
        ZBinary packet;
        fwbin.read(packet, 52);
        if(!writeFlash(FW_ADDR + o, packet)){
            LOG("error writing: 0x" << ZString::ItoS(FW_ADDR + o, 16));
            return false;
        }
    }

    fwbin.rewind();

    LOG("Check...");
    for(zu64 o = 0; o < fwbin.size(); o += 52){
        ZBinary packet;
        fwbin.read(packet, 52);
        if(!checkFlash(FW_ADDR + o, packet)){
            LOG("error checking: 0x" << ZString::ItoS(FW_ADDR + o, 16));
            return false;
        }
    }
    return true;
}

bool ProtoHoltek::eraseAndCheck(){
    LOG("Mass Erase...");
    if(!massEraseFlash()){
        ELOG("mass erase error");
        return false;
    }

    ZThread::sleep(ERASE_SLEEP);

    // reset
    LOG("Mass erase reset");
    if(!rebootBootloader(true))
        return false;
    return true;
}

bool ProtoHoltek::readFlash(zu32 addr, ZBinary &bin){
    DLOG("readFlash " << HEX(addr));
    // Send command
    ZBinary data;
    data.writeleu32(addr);
    data.writeleu32(addr + 64);
    if(!sendRecvCmd(FLASH_CMD, FLASH_READ_SUBCMD, data))
        return false;
    bin.write(data);
    return true;
}

bool ProtoHoltek::writeFlash(zu32 addr, ZBinary bin){
    DLOG("writeFlash " << HEX(addr) << " " << bin.size());
    if(!bin.size())
        return false;
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    arg.writeleu32(addr + bin.size() - 1);
    arg.write(bin);
    if(!sendCmd(FLASH_CMD, FLASH_WRITE_SUBCMD, arg))
        return false;
    return true;
}

bool ProtoHoltek::checkFlash(zu32 addr, ZBinary bin){
    DLOG("checkFlash " << HEX(addr) << " " << bin.size());
    if(!bin.size())
        return false;
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    arg.writeleu32(addr + bin.size() - 1);
    arg.write(bin);
    if(!sendCmd(FLASH_CMD, FLASH_CHECK_SUBCMD, arg))
        return false;
    return true;
}

bool ProtoHoltek::massEraseFlash(){
    DLOG("massEraseFlash");
    // Send command
    ZBinary arg;
    arg.writeleu32(0);
    //arg.writeleu32(0xfbff);
    arg.writeleu32(0);
    //if(!sendCmd(ERASE_CMD, ERASE_PAGE_SUBCMD, arg))
    if(!sendCmd(ERASE_CMD, ERASE_MASS_SUBCMD, arg))
        return false;
    return true;
}

bool ProtoHoltek::eraseFlash(zu32 start, zu32 end){
    DLOG("eraseFlash " << HEX(start) << " " << end);
    // Send command
    ZBinary arg;
    arg.writeleu32(start);
    arg.writeleu32(end);
    if(!sendCmd(ERASE_CMD, ERASE_PAGE_SUBCMD, arg))
        return false;
    return true;
}

zu16 ProtoHoltek::crcFlash(zu32 addr, zu32 len){
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    arg.writeleu32(len);
    sendCmd(CRC_CMD, 0, arg);
    return 0;
}

zu32 ProtoHoltek::baseFirmwareAddr() const {
    return FW_ADDR;
}

bool ProtoHoltek::sendCmd(zu8 cmd, zu8 subcmd, ZBinary bin){
    if(bin.size() > 60){
        ELOG("bad data size");
        return false;
    }

    ZBinary packet(UPDATE_PKT_LEN);
    packet.fill(0);
    packet.writeu8(cmd);    // command
    packet.writeu8(subcmd); // subcommand
    packet.seek(4);
    packet.write(bin);      // data

    zu64 se = packet.seek(2);
    zu16 crc = ZHash<ZBinary, ZHashBase::CRC16>(packet).hash();
    packet.writeleu16(crc); // CRC

    DLOG("send:");
    DLOG(ZLog::RAW << packet.dumpBytes(4, 8));

    // Send command (interrupt write)
    if(!dev->send(packet, (cmd == RESET_CMD ? true : false))){
        ELOG("send error");
        return false;
    }
    return true;
}

bool ProtoHoltek::sendRecvCmd(zu8 cmd, zu8 subcmd, ZBinary &data){
    if(!sendCmd(cmd, subcmd, data))
        return false;

    // Recv packet
    data.resize(UPDATE_PKT_LEN);
    if(!dev->recv(data)){
        ELOG("recv error");
        return false;
    }

    DLOG("recv:");
    DLOG(ZLog::RAW << data.dumpBytes(4, 8));

    if(data.size() != UPDATE_PKT_LEN){
        DLOG("bad recv size");
        return false;
    }

    data.rewind();
    return true;
}
