#include "proto_holtek.h"
#include "zlog.h"

#define UPDATE_PKT_LEN          64

#define FW_ADDR                 0x0000
#define OB_ADDR                 0x1ff00000

#define HT32F52352_FLASH_LEN    0x20000
#define HT32F1654_FLASH_LEN     0x10000

#define REBOOT_SLEEP            5
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
        case 0x101: // v101
            data.seek(16);
            model = data.readleu32();
            break;
        case 0x100: // v100
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

    // Read Flash Security and Protection
    ZBinary ob;
    if(!readFlash(OB_ADDR, ob)) {
        ELOG("Cannot read Option Bytes");
    } else {
        ob.seek(16);
        zu8 ob_cp = ob.readu8();
        LOG("flash security: " << ((ob_cp & 1) == 0));
        LOG("option byte protection: " << ((ob_cp & 2) == 0));
        RLOG("flash page protection:");
        for(int i = 0; i < 4; i++){
            ob.seek(i*4);
            zu32 ob_pp = ob.readleu32();
            RLOG(" " << HEX(ob_pp));
        }
        RLOG(ZLog::NEWLN);
    }

    // check status
    ZBinary status(64);
    LOG("status: " << getCmdStatus(status));

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
    zu32 flash_size = HT32F1654_FLASH_LEN;

    // get flash size from info
    ZBinary info;
    sendRecvCmd(INFO_CMD, 0, info);
    info.seek(6);
    zu16 page_size = info.readleu16();
    info.seek(8);
    flash_size = page_size * info.readleu16();
    LOG("dumpFlash: " << flash_size);

    ZBinary dump;
    zu32 cp = flash_size / 10;
    int perc = 0;
    RLOG(perc << "%...");
    for(zu32 addr = 0; addr < flash_size; addr += 64){
        if(!readFlash(addr, dump))
            break;

        if(addr >= cp){
            perc += 10;
            RLOG(perc << "%...");
            cp += flash_size / 10;
        }
    }
    RLOG("100%" << ZLog::NEWLN);

    return dump;
}

bool ProtoHoltek::writeFirmware(const ZBinary &fwbinin){
    ZBinary fwbin = fwbinin;

    // clear status
    ZBinary status(64);
    getCmdStatus(status);

    zu16 crc = crcFlash(FW_ADDR, fwbin.size());
    LOG("Current CRC: " << HEX(crc));
    zu16 fw_crc = ZHash<ZBinary, ZHashBase::CRC16>(fwbin).hash();
    LOG("Firmware CRC: " << HEX(fw_crc));

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

    // clear status
    getCmdStatus(status);

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

    crc = crcFlash(FW_ADDR, fwbin.size());
    LOG("Final CRC: " << HEX(crc));

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
    data.writeleu32(addr + UPDATE_PKT_LEN - 1);
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

    zu32 count;
    ZBinary status(64);
    do {
        count = getCmdStatus(status);
    }while(count < 1);

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

    ZThread::usleep(500);

    ZBinary status(64);
    zu32 count = getCmdStatus(status);
    return count > 0;
}

bool ProtoHoltek::massEraseFlash(){
    DLOG("massEraseFlash");
    // Send command
    ZBinary arg;
    arg.writeleu32(0);
    arg.writeleu32(0);
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
    DLOG("crcFlash " << HEX(addr) << " " << len);
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    arg.writeleu32(len);
    sendCmd(CRC_CMD, 0, arg);

    ZThread::usleep(5000);

    // get crc from status buffer
    ZBinary status(64);
    getCmdStatus(status);
    status.seek(0);
    zu16 crc = status.readleu16();
    DLOG("CRC: " << HEX(crc));
    return crc;
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

    if(data.size() < UPDATE_PKT_LEN){
        DLOG("bad recv size");
        return false;
    }

    data.rewind();
    return true;
}

zu32 ProtoHoltek::getCmdStatus(ZBinary &data){
    //ZBinary data(64);
    data.fill(0);
    int rc = dev->xferControl(0xa1, 0x01, 0x100, 0, data);
    if(rc < 0){
        DLOG("xferControl error");
        return 0;
    }

    DLOG("status:");
    DLOG(ZLog::RAW << data.dumpBytes(4, 8));

    // count number of successful entries in status buffer
    zu32 count = 0;
    for (int i = 0; i < 64; i++) {
        data.seek(i);
        zu8 val = data.readu8();
        if(val == 0x4f) count++;
    }
    return count;
}
