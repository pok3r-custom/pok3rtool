#include "proto_qmk.h"
#include "zlog.h"

#define UPDATE_PKT_LEN      64

#define VER_ADDR            0x2800
#define FW_ADDR             0x2c00

#define FLASH_LEN           0x20000
#define EEPROM_LEN          0x80000

#define REBOOT_SLEEP        5
#define ERASE_SLEEP         2

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
    if(!sendCmd(QMK_EEPROM, QMK_EEPROM_INFO))
        return false;

    // Get response
    ZBinary pkt(UPDATE_PKT_LEN);
    if(!dev->recv(pkt)){
        ELOG("recv error");
        return false;
    }
    DLOG("recv:");
    LOG(ZLog::RAW << pkt.dumpBytes(4, 8));

    /*
    ZBinary test;
    test.fill(0xaa, 56);
    writeEEPROM(0x1000, test);
    */

    eraseEEPROM(0x0000);

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

bool ProtoQMK::readEEPROM(zu32 addr, ZBinary &bin){
    DLOG("readEEPROM " << addr);
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    if(!sendCmd(QMK_EEPROM, QMK_EEPROM_READ, arg))
        return false;

    // Get response
    ZBinary pkt(UPDATE_PKT_LEN);
    if(!dev->recv(pkt)){
        ELOG("recv error");
        return false;
    }
    DLOG("recv:");
    DLOG(ZLog::RAW << pkt.dumpBytes(4, 8));

    bin.write(pkt);
    return true;
}

bool ProtoQMK::writeEEPROM(zu32 addr, ZBinary bin){
    DLOG("writeEEPROM " << addr);
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    arg.write(bin);
    if(!sendCmd(QMK_EEPROM, QMK_EEPROM_WRITE, arg))
        return false;

    // Get response
    ZBinary pkt(UPDATE_PKT_LEN);
    if(!dev->recv(pkt)){
        ELOG("recv error");
        return false;
    }
    DLOG("recv:");
    DLOG(ZLog::RAW << pkt.dumpBytes(4, 8));

    return true;
}

bool ProtoQMK::eraseEEPROM(zu32 addr){
    DLOG("eraseEEPROM " << addr);
    // Send command
    ZBinary arg;
    arg.writeleu32(addr);
    if(!sendCmd(QMK_EEPROM, QMK_EEPROM_ERASE, arg))
        return false;

    // Get response
    ZBinary pkt(UPDATE_PKT_LEN);
    if(!dev->recv(pkt)){
        ELOG("recv error");
        return false;
    }
    DLOG("recv:");
    DLOG(ZLog::RAW << pkt.dumpBytes(4, 8));

    return true;
}

// From http://mdfs.net/Info/Comp/Comms/CRC16.htm
// CRC-CCITT
#define poly 0x1021
zu16 crc16(unsigned char *addr, zu64 size) {
    zu32 crc = 0;
    for(zu64 i = 0; i < size; ++i){             /* Step through bytes in memory */
        crc ^= (zu16)(addr[i] << 8);            /* Fetch byte from memory, XOR into CRC top byte*/
        for(int j = 0; j < 8; j++){             /* Prepare to rotate 8 bits */
            crc = crc << 1;                     /* rotate */
            if(crc & 0x10000)                   /* bit 15 was set (now bit 16)... */
                crc = (crc ^ poly) & 0xFFFF;    /* XOR with XMODEM polynomic and ensure CRC remains 16-bit value */
        }
    }
    return (zu16)crc;
}

bool ProtoQMK::sendCmd(zu8 cmd, zu8 subcmd, ZBinary bin){
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

    packet.seek(2);
    packet.writeleu16(crc16(packet.raw(), UPDATE_PKT_LEN)); // CRC

    DLOG("send:");
    DLOG(ZLog::RAW << packet.dumpBytes(4, 8));

    // Send command (interrupt write)
    if(!dev->send(packet, (cmd == RESET_CMD ? true : false))){
        ELOG("send error");
        return false;
    }
    return true;
}
