#include "proto_cmmk.h"
#include "proto_pok3r.h"
#include "zlog.h"

#define UPDATE_PKT_LEN      64

#define VER_ADDR_OFFSET     -0x400

#define FLASH_LEN           0x10000

#define WAIT_SLEEP          5

#define HEX(A) (ZString::ItoS((zu64)(A), 16))

ProtoCMMK::ProtoCMMK(zu16 vid_, zu16 pid_, zu16 boot_pid_) :
    ProtoCMMK(vid_, pid_, boot_pid_, false, new HIDDevice, 0)
{

}

ProtoCMMK::ProtoCMMK(zu16 vid_, zu16 pid_, zu16 boot_pid_, bool builtin_, ZPointer<HIDDevice> dev_, zu32 fw_addr_) :
    ProtoQMK(PROTO_CMMK, dev_),
    builtin(builtin_), debug(false), nop(false),
    vid(vid_), pid(pid_), boot_pid(boot_pid_),
    fw_addr(fw_addr_)
{
    //dev->setStream(true);
}

ProtoCMMK::~ProtoCMMK(){

}

bool ProtoCMMK::open(){
    // Try firmware vid and pid
    if(dev->open(vid, pid, UPDATE_USAGE_PAGE, UPDATE_USAGE)){
        builtin = false;
        return true;
    }
    // Try builtin vid and pid
    if(dev->open(vid, boot_pid, UPDATE_USAGE_PAGE, UPDATE_USAGE)){
        builtin = true;
        return true;
    }
    return false;
}

void ProtoCMMK::close(){
    dev->close();
}

bool ProtoCMMK::isOpen() const {
    return dev->isOpen();
}

bool ProtoCMMK::isBuiltin(){
    return builtin;
}

bool ProtoCMMK::rebootFirmware(bool reopen){
    if(!builtin){
        //LOG("In Firmware");
        return true;
    }

    LOG("Reset to Firmware");
    if(!sendCmd(RESET_CMD, RESET_BOOT_SUBCMD))
        return false;
    close();

    if(reopen){
        ZThread::sleep(WAIT_SLEEP);

        // does not work
        if(!open()){
            ELOG("open error");
            return false;
        }

        if(builtin)
            return false;
    }
    return true;
}

bool ProtoCMMK::rebootBootloader(bool reopen){
    if(builtin){
        //LOG("In Bootloader");
        return true;
    }

    LOG("Reset to Bootloader");
    if(!sendCmd(RESET_CMD, RESET_BUILTIN_SUBCMD))
        return false;
    close();

    if(reopen){
        ZThread::sleep(WAIT_SLEEP);

        // does not work
        if(!open()){
            ELOG("open error");
            return false;
        }

        if(!builtin)
            return false;
    }
    return true;
}

bool ProtoCMMK::getInfo(){
    ZBinary data;
    if(!sendRecvCmd(UPDATE_START_CMD, 0, data))
        return false;

    RLOG(data.dumpBytes(4, 8));

    zu32 a = data.readleu32();
    zu16 fw_addr_ = data.readleu16();
    zu16 page_size = data.readleu16();
    zu16 e = data.readleu16() + 10;
    zu16 f = data.readleu16() + 10;
    zu32 ver_addr = data.readleu32();

    LOG(ZString::ItoS((zu64)a, 16));
    LOG("firmware address: 0x" << HEX(fw_addr_));
    LOG("page size: 0x" << HEX(page_size));
    LOG(e);
    LOG(f);
    LOG("version address: 0x" << HEX(ver_addr));

    return true;
}

ZString ProtoCMMK::getVersion(){
    DLOG("getVersion");

    ZBinary data;
    if(!sendRecvCmd(FLASH_CMD, FLASH_READ_VER_SUBCMD, data))
        return "ERROR";

    ZBinary tst;
    tst.fill(0xFF, 64);
    if(data == tst)
        return "CLEARED";

    data.rewind();
    zu32 len = MIN(data.readleu32(), 64U);
    ZString ver = ZString(data.raw() + 4, len);
    DLOG("version: " << ver);

    return ver;
}

KBStatus ProtoCMMK::clearVersion(){
    DLOG("clearVersion");
    if(!rebootBootloader())
        return ERR_IO;

    LOG("Clear Version");
    if(!eraseFlash(fw_addr + VER_ADDR_OFFSET, fw_addr + VER_ADDR_OFFSET + 8))
        return ERR_IO;

    ZBinary bin;
    if(!sendRecvCmd(FLASH_CMD, FLASH_READ_VER_SUBCMD, bin))
        return ERR_IO;

    ZBinary tst;
    tst.fill(0xFF, 64);
    if(bin != tst)
        return ERR_IO;

    return SUCCESS;
}

KBStatus ProtoCMMK::setVersion(ZString version){
    DLOG("setVersion " << version);
    auto status = clearVersion();
    if(status != SUCCESS)
        return status;

    LOG("Writing Version: " << version);

    ZBinary vdata;
    zu64 vlen = version.size() + 4;
    vdata.fill(0, vlen + (4 - (vlen % 4)));
    vdata.writeleu32(version.size());
    vdata.write(version.bytes(), version.size());

    // write version
    if(!writeFlash(fw_addr + VER_ADDR_OFFSET, vdata)){
        LOG("write error");
        return ERR_FAIL;
    }

    // check version
    ZString nver = getVersion();

    if(nver != version){
        ELOG("failed to set version");
        return ERR_FLASH;
    }

    return SUCCESS;
}

ZBinary ProtoCMMK::dump(zu32 address, zu32 len){
    ZBinary dump;

    if (len < 0x40) {
        len = 0x40;
    }
    zu32 cp = len / 10;
    int percent = 0;
    RLOG(percent << "%...");
    zu32 addr_offset = 0;
    for(; addr_offset < len - 60; addr_offset += 60){
        if(!readFlash(address + addr_offset, dump))
            break;

        if(addr_offset >= cp){
            percent += 10;
            RLOG(percent << "%...");
            cp += len / 10;
        }
    }
    zu32 overshoot = addr_offset + 60 - len;
    if (addr_offset + 60 > len){
        ZBinary tmp;
        if(!readFlash(address + len - 60, tmp))
            return dump;
        dump.write(tmp.raw() + overshoot, 60 - overshoot);
    }

    RLOG("100%" << ZLog::NEWLN);

    return dump;
}

ZBinary ProtoCMMK::dumpFlash(){
    DLOG("dumpFlash");
    return dump(0, FLASH_LEN);
}

bool ProtoCMMK::writeFirmware(const ZBinary &fwbinin){
    DLOG("writeFirmware");

    ZBinary fwbin = fwbinin;
    encode_firmware(fwbin);

    // update reset
    ZBinary tmp;
    if(!sendRecvCmd(UPDATE_START_CMD, 0, tmp))
        return false;

    LOG("Erase...");
    if(!eraseFlash(fw_addr, fw_addr + fwbin.size()))
        return false;

    ZThread::sleep(WAIT_SLEEP);

    LOG("Write...");
    if(!writeFlash(fw_addr, fwbin))
        return false;

    LOG("Check...");
    if(!checkFlash(fw_addr, fwbin))
        return false;

    LOG("Flashed succesfully");

    return true;
}

bool ProtoCMMK::eraseAndCheck(){
    // not implemented
    return false;
}

void ProtoCMMK::test(){
    // not implemented
    return;
}

bool ProtoCMMK::eraseFlash(zu32 start, zu32 end){
    DLOG("eraseFlash " << HEX(start) << " " << HEX(end));
    // send command
    ZBinary arg;
    arg.writeleu32(start);
    arg.writeleu32(end);
    if(!sendCmd(ERASE_CMD, 8, arg))
        return false;
    return true;
}

bool ProtoCMMK::readFlash(zu32 addr, ZBinary &bin){
    DLOG("readFlash " << HEX(addr));
    // send command
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmd(READ_CMD, READ_ADDR_SUBCMD, data))
        return false;
    bin.write(data.raw() + 4, 60);
    return true;
}

bool ProtoCMMK::writeFlash(zu32 addr, ZBinary bin){
    DLOG("writeFlash " << HEX(addr) << " " << bin.size());
    if(!bin.size())
        return false;
    // send command
    zu32 offset = 0;
    while(!bin.atEnd()){
        ZBinary chunk;
        ZBinary arg;
        bin.read(chunk, 52);
        zu32 start = addr + offset;
        zu32 end = start + chunk.size() - 1;
        arg.writeleu32(start);
        arg.writeleu32(end);
        arg.write(chunk);
        DLOG(__func__ << ": 0x" << HEX(start) << " 0x" << HEX(end));
        if(!sendCmd(FLASH_CMD, FLASH_WRITE_SUBCMD, arg))
            return false;
        offset += chunk.size();
    }
    return true;
}

bool ProtoCMMK::checkFlash(zu32 addr, ZBinary bin){
    DLOG("checkFlash " << HEX(addr) << " " << bin.size());
    if(!bin.size())
        return false;
    // send command
    zu32 offset = 0;
    while(!bin.atEnd()){
        ZBinary chunk;
        ZBinary arg;
        bin.read(chunk, 52);
        zu32 start = addr + offset;
        zu32 end = start + chunk.size() - 1;
        arg.writeleu32(start);
        arg.writeleu32(end);
        arg.write(chunk);
        DLOG(__func__ << ": 0x" << HEX(start) << " 0x" << HEX(end));
        if(!sendCmd(FLASH_CMD, FLASH_CHECK_SUBCMD, arg))
            return false;
        offset += chunk.size();
    }
    return true;
}

zu32 ProtoCMMK::baseFirmwareAddr() const {
    return fw_addr;
}

bool ProtoCMMK::sendCmd(zu8 cmd, zu8 a1, ZBinary data){
    if(data.size() > 60){
        ELOG("bad data size");
        return false;
    }

    ZBinary packet(UPDATE_PKT_LEN);
    packet.fill(0);
    packet.writeu8(cmd);    // command
    packet.writeu8(a1);     // argument
    packet.seek(4);
    packet.write(data);     // data

    packet.seek(2);
    zu16 crc = ZHash<ZBinary, ZHashBase::CRC16>(packet).hash();
    packet.writeleu16(crc); // CRC

    DLOG("send: CRC: 0x" << HEX(crc));

    DLOG("send:");
    DLOG(ZLog::RAW << packet.dumpBytes(4, 8));

    // Send packet
    if(!dev->send(packet, (cmd == RESET_CMD ? true : false))){
        ELOG("send error");
        return false;
    }
    return true;
}

bool ProtoCMMK::recvCmd(ZBinary &data){
    // Recv packet
    data.resize(UPDATE_PKT_LEN);
    if(!dev->recv(data)){
        ELOG("recv error");
        return false;
    }

    if(data.size() != UPDATE_PKT_LEN){
        DLOG("bad recv size");
        return false;
    }

    DLOG("recv:");
    DLOG(ZLog::RAW << data.dumpBytes(4, 8));

    data.rewind();
    return true;
}

bool ProtoCMMK::sendRecvCmd(zu8 cmd, zu8 a1, ZBinary &data){
    if(!sendCmd(cmd, a1, data))
        return false;
    return recvCmd(data);
}

void ProtoCMMK::decode_firmware(ZBinary &bin){
    ProtoPOK3R::decode_firmware(bin);
}

void ProtoCMMK::encode_firmware(ZBinary &bin){
    ProtoPOK3R::encode_firmware(bin);
}

void ProtoCMMK::info_section(ZBinary data){
    // not implemented
    return;
}
