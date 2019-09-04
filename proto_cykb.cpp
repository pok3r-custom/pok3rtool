#include "proto_cykb.h"
#include "zlog.h"

#define UPDATE_PKT_LEN      64

#define UPDATE_ERROR        0xaaff

#define VER_ADDR            0x3000

#define FLASH_LEN           0x10000

#define WAIT_SLEEP          5
#define ERASE_SLEEP         2

#define HEX(A) (ZString::ItoS((zu64)(A), 16))

ProtoCYKB::ProtoCYKB(zu16 vid_, zu16 pid_, zu16 boot_pid_) :
    ProtoCYKB(vid_, pid_, boot_pid_, false, new HIDDevice, 0)
{

}

ProtoCYKB::ProtoCYKB(zu16 vid_, zu16 pid_, zu16 boot_pid_, bool builtin_, ZPointer<HIDDevice> dev_, zu32 fw_addr_) :
    ProtoQMK(PROTO_CYKB, dev_),
    builtin(builtin_), debug(false), nop(false),
    vid(vid_), pid(pid_), boot_pid(boot_pid_),
    fw_addr(fw_addr_)
{
    //dev->setStream(true);
}

ProtoCYKB::~ProtoCYKB(){

}

bool ProtoCYKB::open(){
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

void ProtoCYKB::close(){
    dev->close();
}

bool ProtoCYKB::isOpen() const {
    return dev->isOpen();
}

bool ProtoCYKB::isBuiltin(){
    return builtin;
}

bool ProtoCYKB::rebootFirmware(bool reopen){
    if(!builtin){
//        LOG("In Firmware");
        return true;
    }

    LOG("Reset to Firmware");
    if(!sendCmd(RESET, RESET_FW))
        return false;
    close();

    if(reopen){
        ZThread::sleep(WAIT_SLEEP);

        if(!open()){
            ELOG("open error");
            return false;
        }

        if(builtin)
            return false;
    }
    return true;
}

bool ProtoCYKB::rebootBootloader(bool reopen){
    if(builtin){
//        LOG("In Bootloader");
        return true;
    }

    LOG("Reset to Bootloader");
    if(!sendCmd(RESET, RESET_BL))
        return false;
    close();

    if(reopen){
        ZThread::sleep(WAIT_SLEEP);

        if(!open()){
            ELOG("open error");
            return false;
        }

        if(!builtin)
            return false;
    }
    return true;
}

bool ProtoCYKB::getInfo(){
    ZBinary bin;
    ZBinary data;

    for(zu8 i = 0x20; i < 0x23; ++i){
        bin.clear();
        if(!sendRecvCmd(READ, i, bin))
            return false;
        data.write(bin.getSub(4, 60));
    }
    RLOG(data.dumpBytes(4, 8, VER_ADDR));

    info_section(data);

    LOG("READ_400");
    bin.clear();
    if(!sendRecvCmd(READ, READ_400, bin))
        return false;
    RLOG(bin.getSub(4, 52).dumpBytes(4, 8));

    LOG("READ_3c00");
    bin.clear();
    if(!sendRecvCmd(READ, READ_3C00, bin))
        return false;
    RLOG(bin.getSub(4, 4).dumpBytes(4, 8));

    return true;
}

ZString ProtoCYKB::getVersion(){
    DLOG("getVersion");

    // version 1
    ZBinary data;
    if(!sendRecvCmd(READ, READ_VER1, data))
        return "ERROR";
//    RLOG(data.dumpBytes(4, 8));

    ZBinary tst;
    tst.fill(0xFF, 60);

    ZString ver;
    if(data.getSub(4) == tst){
        ver = "CLEARED";
    } else {
        data.seek(4);
        zu32 len = MIN(data.readleu32(), 60U);
        ver.parseUTF16((zu16 *)(data.raw() + 8), len);
    }
    DLOG("version: " << ver);

    // version 2
//    ZBinary data2;
//    if(!sendRecvCmd(READ, READ_VER2, data2))
//        return "ERROR";
//    DLOG("ver2:");
//    DLOG(ZLog::RAW << data2.getSub(4).dumpBytes(4, 8));

    return ver;
}

KBStatus ProtoCYKB::clearVersion(){
    DLOG("clearVersion");
    if(!rebootBootloader())
        return ERR_FAIL;

    if(!eraseFlash(VER_ADDR, 0xB4))
        return ERR_FAIL;

    ZBinary data;
    if(!sendRecvCmd(READ, READ_VER2, data))
        return ERR_FAIL;

    ZBinary tst;
    tst.fill(0xFF, 60);
    if(data.getSub(4) != tst){
        ELOG("version not cleared");
        ELOG(ZLog::RAW << data.dumpBytes(4, 8));
        return ERR_FLASH;
    }

    return SUCCESS;
}

const zu32 ver2[15] = {
    0x00800004, 0x00010300, 0x00000041, 0xefffffff,
    0x00000001, 0x00000000, 0x016704d9, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0x001c5aa5,
};

KBStatus ProtoCYKB::setVersion(ZString version){
    DLOG("setVersion " << version);
    auto status = clearVersion();
    if(status != SUCCESS)
        return status;

    LOG("Writing Version: " << version);

    // UTF-16 encoded version string
    zu16 str[256];
    zu64 len = version.readUTF16(str, 255);
    str[len++] = 0;

    // Write version string
    ZBinary sdata;
    sdata.writeleu32(len * 2);
    for(zu64 i = 0; i < len; ++i)
        sdata.writeleu16(str[i]);

    ZBinary vdata;
    vdata.fill(0xFF, 0x78);
    vdata.write(sdata);
    vdata.seek(0x78);
    vdata.write((const zbyte *)ver2, sizeof(ver2));
//    RLOG(vdata.dumpBytes(4, 8));

    // write version
    if(!writeFlash(VER_ADDR, vdata)){
        ELOG("write error");
        return ERR_FAIL;
    }

    // check version
    ZBinary data;
    if(!sendRecvCmd(READ, READ_VER2, data))
        return ERR_FAIL;
    ZBinary cdata(ver2, sizeof(ver2));
    if(data.getSub(4) != cdata){
        ELOG("failed to set version");
        return ERR_FLASH;
    }

    ZString nver = getVersion();
//    LOG("New Version: " << nver);
    if(nver != version){
        ELOG("failed to set version string");
        return ERR_FLASH;
    }

    return SUCCESS;
}

ZBinary ProtoCYKB::dumpFlash(){
    ZBinary dump;
    /*
    for(zu16 i = 0; i < FLASH_LEN - 60; i += 60){
        if(!readFlash(i, dump))
            return dump;
    }
    */

    // readable flash is not a multiple of 60,
    // so read the last little bit for a full dump
    /*
    ZBinary tmp;
    if(!readFlash(FLASH_LEN - 60, tmp))
        return dump;
    dump.write(tmp.raw() + 44, 16);
    */

    for(zu16 i = 0; i < FLASH_LEN - 60; i += 60){
        if(!readFlash(i, dump))
            return dump;
    }

    return dump;
}

bool ProtoCYKB::writeFirmware(const ZBinary &fwbinin){
    ZBinary fwbin = fwbinin;
    // Encode the firmware for the POK3R RGB
    encode_firmware(fwbin);
    zu32 crc0 = ZHash<ZBinary, ZHashBase::CRC32>(fwbinin).hash();
    LOG("Firmware CRC D: " << ZString::ItoS((zu64)crc0, 16, 8));
    zu32 crc1 = ZHash<ZBinary, ZHashBase::CRC32>(fwbin).hash();
    LOG("Firmware CRC E: " << ZString::ItoS((zu64)crc1, 16, 8));

//    zu32 ccrc = crcFlash(fw_addr, 0xc000);
    zu32 ccrc = crcFlash(fw_addr, fwbin.size());
    LOG("Current CRC: " << ZString::ItoS((zu64)ccrc, 16, 8));

    LOG("Erase...");
    if(!eraseFlash(fw_addr, fwbin.size()))
//    if(!eraseFlash(fw_addr, FLASH_LEN - fw_addr))
        return false;

    ZThread::sleep(WAIT_SLEEP);

    LOG("Write...");
    if(!writeFlash(fw_addr, fwbin))
        return false;

    zu32 crc2 = crcFlash(fw_addr, fwbin.size());
    LOG("New CRC: " << ZString::ItoS((zu64)crc2, 16, 8));

    if(crc2 != crc1){
        ELOG("CRCs do not match, firmware write failed");
        return false;
    }

    return true;
}

bool ProtoCYKB::eraseAndCheck(){
    // Reset to bootloader
    if(!rebootBootloader())
        return false;

    zu32 crc_before = crcFlash(VER_ADDR, FLASH_LEN - VER_ADDR);
    LOG("Current CRC: " << ZString::ItoS((zu64)crc_before, 16, 8));

    zu32 addr = VER_ADDR;
    for(zu32 i = 0; i < 16-3; ++i){
        LOG("Erase 0x" << HEX(addr));
        if(!eraseFlash(addr, 0x1000)){
            ELOG("erase failed");
            return false;
        }
        addr += 0x1000;
    }

    zu32 crc_after = crcFlash(VER_ADDR, FLASH_LEN - VER_ADDR);
    LOG("New CRC: " << ZString::ItoS((zu64)crc_after, 16, 8));

    return true;
}

void ProtoCYKB::test(){
    ZBinary bin;

    LOG("READ_400");
    bin.clear();
    if(!sendRecvCmd(READ, READ_400, bin))
        return;
    RLOG(bin.getSub(4, 52).dumpBytes(4, 8));

    LOG("READ_3c00");
    bin.clear();
    if(!sendRecvCmd(READ, READ_3C00, bin))
        return;
    RLOG(bin.getSub(4, 4).dumpBytes(4, 8));

    LOG("READ_MODE");
    bin.clear();
    if(!sendRecvCmd(READ, READ_MODE, bin))
        return;
    RLOG(bin.getSub(4, 1).dumpBytes(4, 8));

    if(!rebootBootloader())
        return;

    LOG("READ_MODE");
    bin.clear();
    if(!sendRecvCmd(READ, READ_MODE, bin))
        return;
    RLOG(bin.getSub(4, 1).dumpBytes(4, 8));

    ZBinary data;
    data.writeleu32(0x400);
    data.writeleu32(0x4da4);

    LOG("SUM");
    if(!sendCmd(FW, FW_SUM, data))
        return;
    if(!dev->recv(bin))
        return;
    bin.rewind();
    bin.seek(4);
    zu32 sum = bin.readleu32();
    LOG("SUM " << ZString::ItoS((zu64)sum, 16));

    LOG("CRC");
    if(!sendCmd(FW, FW_CRC, data))
        return;
    if(!dev->recv(bin))
        return;
    bin.rewind();
    bin.seek(4);
    LOG("CRC " << ZString::ItoS((zu64)bin.readleu32(), 16));
}

bool ProtoCYKB::eraseFlash(zu32 start, zu32 length){
    DLOG("eraseFlash 0x" << HEX(start) << " " << length);
    if(start < VER_ADDR){
        ELOG("bad address");
        return false;
    }

    ZBinary data;
    data.writeleu32(start - VER_ADDR);
    data.writeleu32(length);

    if(!sendCmd(FW, FW_ERASE, data))
        return false;

    // give time for erase
    ZThread::sleep(ERASE_SLEEP);

    return recvCmd(data);
}

bool ProtoCYKB::readFlash(zu32 addr, ZBinary &bin){
    DLOG("readFlash 0x" << HEX(addr));
    ZBinary data;
    data.writeleu32(addr);
    if(!sendRecvCmd(READ, READ_ADDR, data))
        return false;
    bin.write(data.raw() + 4, 60);
    return true;
}

bool ProtoCYKB::writeFlash(zu32 addr, ZBinary bin){
    DLOG("writeFlash 0x" << HEX(addr) << " " << bin.size());
    if(addr < VER_ADDR){
        ELOG("bad address");
        return false;
    }

    // Set address
    ZBinary adata;
    adata.writeleu32(addr - VER_ADDR);
    if(!sendRecvCmd(ADDR, ADDR_SET, adata))
        return false;

    // Get address
    adata.clear();
    if(!sendRecvCmd(ADDR, ADDR_GET, adata))
        return false;
    adata.seek(4);
    zu32 saddr = adata.readleu32();

    if(saddr != addr - VER_ADDR){
        ELOG("failed to set write address");
        return false;
    }

    // Write
    zu16 pos = addr - VER_ADDR;
    bin.rewind();
    while(!bin.atEnd()){
        ZBinary data;
        bin.read(data, 52);
        //bin.read(data, 60);
        zu8 sz = data.size();
        DLOG("write " << HEX(VER_ADDR + pos) << ", " << sz << " bytes");
        if(!sendRecvCmd(WRITE, data.size(), data))
            return false;
        data.seek(4);
        zu16 next = data.readleu16();
        pos += sz;
        if(next != pos){
            ELOG("write sequence error " << HEX(next) << " " << HEX(pos));
        }
    }

    return true;
}

zu32 ProtoCYKB::crcFlash(zu32 addr, zu32 len){
    if(addr < VER_ADDR){
        ELOG("bad address");
        return 0;
    }

    DLOG("crcFlash 0x" << HEX(addr) << " 0x" << HEX(len));

    // CRC command
    ZBinary data1;
    data1.writeleu32(addr - VER_ADDR);
    //data1.writeleu32(0);
    data1.writeleu32(len);
    if(!sendRecvCmd(FW, FW_CRC, data1))
        return 0;
    data1.seek(4);
    zu32 crc = data1.readleu32();
    LOG("crc " << HEX(crc));

    // SUM command
    ZBinary data2;
    data2.writeleu32(addr - VER_ADDR);
    //data2.writeleu32(0);
    data2.writeleu32(len);
    if(!sendRecvCmd(FW, FW_SUM, data2))
        return 0;
    data2.seek(4);
    zu32 sum = data2.readleu32();
    LOG("sum " << HEX(sum));

    return crc;
}

zu32 ProtoCYKB::baseFirmwareAddr() const {
    return fw_addr;
}

bool ProtoCYKB::sendCmd(zu8 cmd, zu8 a1, ZBinary data){
    if(data.size() > 52){
        ELOG("bad data size");
        return false;
    }

    ZBinary packet(UPDATE_PKT_LEN);
    packet.fill(0);
    packet.writeu8(cmd);    // command
    packet.writeu8(a1);     // argument
    packet.seek(4);
    packet.write(data);     // data

//    packet.seek(2);
//    zu16 crc = ZHash<ZBinary, ZHashBase::CRC16>(packet).hash();
//    packet.writeleu16(crc); // CRC

    DLOG("send:");
    DLOG(ZLog::RAW << packet.dumpBytes(4, 8));

    // Send packet
    if(!dev->send(packet, (cmd == RESET ? true : false))){
        ELOG("send error");
        return false;
    }
    return true;
}

bool ProtoCYKB::recvCmd(ZBinary &data){
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

//    data.seek(2);
//    zu16 crc0 = data.readleu16();
//    data.seek(2);
//    data.writeleu16(0);
//    data.rewind();
//    zu16 crc1 = ZHash<ZBinary, ZHashBase::CRC16>(data).hash();
//    DLOG("crc: " << HEX(crc0) << " " << HEX(crc1));

    // Check error
    data.rewind();
    if(data.readleu16() == UPDATE_ERROR){
        DLOG("error response: " << HEX(data[4]) << " " << HEX(data[5]));
        DLOG(ZLog::RAW << data.dumpBytes(4, 8));
        return false;
    }
    data.rewind();

    return true;
}

bool ProtoCYKB::sendRecvCmd(zu8 cmd, zu8 a1, ZBinary &data){
    if(!sendCmd(cmd, a1, data))
        return false;
    return recvCmd(data);
}

// POK3R RGB XOR encryption/decryption key
// Somone somewhere thought a random XOR key was any better than the one they
// used in the POK3R firmware. Yeah, good one.
// See fw_xor_decode.c for the hilarious way this key was obtained.
static const zu32 xor_key[] = {
    0xe7c29474,
    0x79084b10,
    0x53d54b0d,
    0xfc1e8f32,
    0x48e81a9b,
    0x773c808e,
    0xb7483552,
    0xd9cb8c76,
    0x2a8c8bc6,
    0x0967ada8,
    0xd4520f5c,
    0xd0c3279d,
    0xeac091c5,
};

// Decode the encryption scheme used by the POK3R RGB firmware
// Just XOR encryption with 52-byte key seen above.
void xor_decode_encode(ZBinary &bin){
    // XOR decryption
    zu32 *words = (zu32 *)bin.raw();
    for(zu64 i = 0; i < bin.size() / 4; ++i){
        words[i] = words[i] ^ xor_key[i % 13];
    }
}

void ProtoCYKB::decode_firmware(ZBinary &bin){
    xor_decode_encode(bin);
}

void ProtoCYKB::encode_firmware(ZBinary &bin){
    xor_decode_encode(bin);
}

void ProtoCYKB::info_section(ZBinary data){
    ZString ver;
    if(data.readleu32() == 0xFFFFFFFF){
        ver = "CLEARED";
    } else {
        data.rewind();
        zu32 len = MIN(data.readleu32(), 60U);
        ver.parseUTF16((zu16 *)(data.raw() + 4), len);
    }
    LOG("Version String: " << ver);
    
    data.seek(120);
    zu32 a = data.readleu32();
    zu32 b = data.readleu32();
    zu32 c = data.readleu32();
    zu32 d = data.readleu32();
    zu32 e = data.readleu32();
    zu32 f = data.readleu32();
    zu32 ivid = data.readleu16();
    zu32 ipid = data.readleu16();
    data.seek(176);
    zu32 h = data.readleu32();

    LOG("a: " << ZString::ItoS((zu64)a, 16, 8));
    LOG("Version: " << ZString::ItoS((zu64)b, 16, 8));
    LOG("c: " << ZString::ItoS((zu64)c, 16, 8));
    LOG("d: " << ZString::ItoS((zu64)d, 16, 8));
    LOG("e: " << ZString::ItoS((zu64)e, 16, 8));
    LOG("f: " << ZString::ItoS((zu64)f, 16, 8));
    LOG("VID/PID: " << ZString::ItoS((zu64)ivid, 16, 4) << " " << ZString::ItoS((zu64)ipid, 16, 4));
    LOG("h: " << ZString::ItoS((zu64)h, 16, 8));
}
