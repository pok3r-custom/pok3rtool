#include "updatepackage.h"
#include "proto_pok3r.h"
#include "proto_cykb.h"

#include "zfile.h"
#include "zhash.h"
#include "zmap.h"
#include "zlog.h"

typedef int (*decodeFunc)(ZFile *, ZBinary &);
int decode_maajonsn(ZFile *file, ZBinary &fw_out);
int decode_maav101(ZFile *file, ZBinary &fw_out);
int decode_maav102(ZFile *file, ZBinary &fw_out);
int decode_maav105(ZFile *file, ZBinary &fw_out);
int decode_kbp_v60(ZFile *file, ZBinary &fw_out);
int decode_kbp_v80(ZFile *file, ZBinary &fw_out);

enum PackType {
    PACKAGE_NONE = 0,
    MAAJONSN,   // .maajonsn
    MAAV101,    // .maaV101
    MAAV102,    // .maaV102
    MAAV105,    // .maaV105
    KBPV60,
    KBPV80,
};

const ZMap<zu64, PackType> packages = {
    // POK3R (141)
    { 0x62FCF913A689C9AE,   MAAJONSN }, // V1.1.3
    { 0xFE37430DB1FFCF5F,   MAAJONSN }, // V1.1.4
    { 0x8986F7893143E9F7,   MAAJONSN }, // V1.1.5
    { 0xA28E5EFB3F796181,   MAAJONSN }, // V1.1.6
    { 0xEA55CB190C35505F,   MAAJONSN }, // V1.1.7       pok3r/v117

    // POK3R RGB (167)
    { 0x882CB0E4ECE25454,   MAAV102 },  // V1.02.04
    { 0x6CFF0BB4F4086C2F,   MAAV102 },  // V1.03.00     pok3r_rgb/v130
    { 0xA6EE37F856CD24C1,   MAAV102 },  // V1.04.00     pok3r_rgb/v140

    // Vortex CORE (175)
    { 0x51BFA86A7FAF4EEA,   MAAV102 },  // V1.04.01
    { 0x0582733413943655,   MAAV102 },  // V1.04.03
    { 0x61F73244FA73079F,   MAAV102 },  // V1.04.05     core/v145
    { 0xAD80988AE986097B,   MAAV105 },  // contains two firmwares:
                                        // CORE by HWP (V1.04.05), the original firmware, programmable by keyboard controls
                                        // CORE by MPC (V1.04.06), the new firmware, programmable by web UI-generated keymaps (http://www.vortexgear.tw/mpc/index.html)

    // Vortex CORE RGB (product 293, pid 175?)
    { 0xA85878CBD05591A1,   MAAV102 },  // V1.04.06

    // Vortex RACE3 (New 75) (192)
    { 0xB542D0D86B9A85C3,   MAAV102 },  // V1.02.01
    { 0xFBF40BEE5D0A3C70,   MAAV102 },  // V1.02.04     race/v124
    { 0xAD8B210C77D9D90F,   MAAV102 },  // V1.02.05

    // Vortex Cypher (282)
    { 0xC259BB38A57783D,    MAAV102 },  // V1.03.06     cypher/v136

    // POK3R RGB V2 (207)
    { 0x8AA1AEA217DA685B,   MAAV102 },  // V1.00.05     pok3r_rgb2/v105

    // Vortex ViBE (216)
    { 0xCE7C8EAA3D28B10D,   MAAV102 },  // V1.01.03     vibe/v113

    // Vortex Tab 60 (304)
    { 0xF5ED2438D4445703,   MAAV102 },  // V1.01.13     tab60/v1113

    // Vortex Tab 75 (344)
    { 0x4399C7232F89BBDD,   MAAV105 },  // V1.00.04     tab75/v104

    // Vortex Tab 90 (346)
    { 0xBFCCB61A61996BB3,   MAAV105 },  // V1.00.04     tab90/v104

    // KBP V60 (112)
    { 0x6064D8C4EE74BE18,   KBPV60 },   // V1.0.7       kbpv60

    // KBP V80 (129)
    { 0xBCF4C9830D800D8C,   KBPV80 },   // V1.0.7       kbpv80

    // TEX Yoda II (163)
    { 0xF5A3714FA9A3CA40,   MAAV102 },  // V1.01.01

    // Mistel Barocco MD600 (143)
    { 0xFA5DF5F231700316,   MAAV102 },  // V1.04.08     md600/v148

    // Mistel Freeboard MD200 (200)
    // Vortex Tester (200)
    { 0x58B42FF4B1C57C09,   MAAV102 },  // V1.01.02     md200/v112

    // Cooler Master Masterkeys Pro L White
    { 0x38cc849b2e54b6df,   MAAV102 },  // V1.08.00     cmprolwhite/v180

    // Cooler Master Masterkeys Pro M White
    { 0x12fbf4668bdfe188,   MAAV102 },  // V1.06.00

    // Cooler Master Masterkeys Pro S RGB
    { 0x91d591ac1a77b2d,    MAAV101 },  // 1.2.1        cmprosrgb/v121
    { 0x836c83cc7d4e9f1,    MAAV101 },  // 1.2.2        cmprosrgb/v122

    // Cooler Master Masterkeys Pro M RGB
    { 0xfdf7ac5b93d67ead,   MAAV102 },  // V1.04.00
    { 0x2f69c079f9d53765,   MAAV102 },  // V1.04.01

    // Cooler Master Masterkeys Pro L RGB
    { 0x57ca9d8e07d0c95a,   MAAV101 },  // 1.2.1
    { 0x2adc9b96d5cf26c7,   MAAV101 },  // 1.2.2
};

const ZMap<PackType, decodeFunc> types = {
    { MAAJONSN, decode_maajonsn },
    { MAAV101,  decode_maav101 },
    { MAAV102,  decode_maav102 },
    { KBPV60,   decode_kbp_v60 },
    { KBPV80,   decode_kbp_v80 },
    { MAAV105,  decode_maav105 },
};

UpdatePackage::UpdatePackage(){

}

bool UpdatePackage::loadFromExe(ZPath exe, int index){
    LOG("Extract from " << exe);
    ZFile file;
    if(!file.open(exe, ZFile::READ)){
        ELOG("Failed to open file");
        return false;
    }

    zu64 exehash = ZFile::fileHash(exe);
    if(packages.contains(exehash)){
        int ret = types[packages[exehash]](&file, firmware);
        return !ret;
    } else {
        ELOG("Unknown updater executable: " << ZString::ItoS(exehash, 16));
        return false;
    }

    return true;
}

const ZBinary &UpdatePackage::getFirmware() const {
    return firmware;
}

/*  Decode the encryption scheme used by the updater program.
 *  Produced from IDA disassembly in sub_401000 of v117 updater.
 *  First, swap the 1st and 4th bytes, every 5 bytes
 *  Second, reverse each pair of bytes
 *  Third, shift the bits in each byte, sub 7 from MSBs
 */
void decode_package_data(ZBinary &bin){
    // Swap bytes 4 apart, skip 5
    for(zu64 i = 4; i < bin.size(); i+=5){
        zbyte a = bin[i-4];
        zbyte b = bin[i];
        bin[i-4] = b;
        bin[i] = a;
    }

    // Swap bytes in each set of two bytes
    for(zu64 i = 1; i < bin.size(); i+=2){
        zbyte d = bin[i-1];
        zbyte b = bin[i];
        bin[i-1] = b;
        bin[i] = d;
    }

    // y = ((x - 7) << 4) + (x >> 4)
    for(zu64 i = 0; i < bin.size(); ++i){
        bin[i] = (((bin[i] - 7) << 4) + (bin[i] >> 4)) & 0xFF;
    }
}

/*  Encrypt using the encryption scheme used by the updater program
 *   Reverse engineered from the above
 */
void encode_package_data(ZBinary &bin){
    // x = (y >> 4 + 7 & 0xF) | (x << 4)
    for(zu64 i = 0; i < bin.size(); ++i){
        bin[i] = ((((bin[i] >> 4) + 7) & 0xF) | (bin[i] << 4)) & 0xFF;
    }

    // Swap bytes in each set of two bytes
    for(zu64 i = 1; i < bin.size(); i+=2){
        zbyte d = bin[i-1];
        zbyte b = bin[i];
        bin[i-1] = b;
        bin[i] = d;
    }

    // Swap bytes 4 apart, skip 5
    for(zu64 i = 4; i < bin.size(); i+=5){
        zbyte a = bin[i-4];
        zbyte b = bin[i];
        bin[i-4] = b;
        bin[i] = a;
    }
}

ZBinary pkg_file_read(ZFile *file, zu64 pos, zu64 len){
    ZBinary data;
    if(file->seek(pos) != pos)
        throw ZException("File too short - seek");
    if(file->read(data, len) != len)
        throw ZException("File too short - read");
    return data;
}

/*  Decode the updater for the POK3R.
 */
int decode_maajonsn(ZFile *file, ZBinary &fw_out){
    zu64 exelen = file->fileSize();

    zu64 strings_len = 0x4B8;

    zu64 offset_company = 0x10;
    zu64 offset_product = offset_company + 0x208; // 0x218
    zu64 offset_version = 0x460;
    zu64 offset_sig = 0x4AE;

    zu64 strings_start = exelen - strings_len;

    // Read strings
    ZBinary strs = pkg_file_read(file, strings_start, strings_len);
    // Decrypt strings
    decode_package_data(strs);

    ZString company;
    ZString product;
    ZString version;

    // Company name
    company.parseUTF16((const zu16 *)(strs.raw() + offset_company), 0x200);
    // Product name
    product.parseUTF16((const zu16 *)(strs.raw() + offset_product), 0x200);
    // Version
    version = ZString(strs.raw() + offset_version, 12);

    LOG("Company:     " << company);
    LOG("Product:     " << product);
    LOG("Version:     " << version);

    LOG("Signature:   " << ZString(strs.raw() + offset_sig, strings_len - offset_sig));

//    LOG("String Dump:");
//    RLOG(strs.dumpBytes(4, 8));

    // Decode other encrypted sections

    zu64 total = strings_len;
    ZArray<zu64> sections;

    zu64 sec_len = ZBinary::decleu32(strs.raw() + 0x420); // Firmware length

    ZString layout;
    layout.parseUTF16((const zu16 *)(strs.raw() + 0x424), 0x20);
    LOG("Layout: " << layout);

    total += sec_len;
    sections.push(sec_len);

    zu64 sec_start = exelen - total;

    LOG("Offset: 0x" << ZString::ItoS(sec_start, 16));
    LOG("Length: 0x" << ZString::ItoS(sec_len, 16));

    // Read section
    ZBinary sec = pkg_file_read(file, sec_start, sec_len);
    // Decode section
    decode_package_data(sec);

    // Decrypt firmware
    ProtoPOK3R::decode_firmware(sec);

//    LOG("Section Dump:");
//    RLOG(sec.dumpBytes(4, 8, 0));

    // Write firmware
    fw_out = sec;

    return 0;

}

/*  Decode the updater for the CM MK Pro S/L RGB
 */
int decode_maav101(ZFile *file, ZBinary &fw_out){
    zu64 exelen = file->fileSize();

    zu64 strings_len = 0x4BC;

    zu64 offset_company = 0x10;
    zu64 offset_product = 0x218;
    zu64 offset_version = 0x461;
    zu64 offset_sig = 0x4AF;

    zu64 strings_start = exelen - strings_len;

    // Read strings
    ZBinary strs = pkg_file_read(file, strings_start, strings_len);
    // Decrypt strings
    decode_package_data(strs);

    ZString company;
    ZString product;
    ZString version;

    // Company name
    company.parseUTF16((const zu16 *)(strs.raw() + offset_company), 0x200);
    // Product name
    product.parseUTF16((const zu16 *)(strs.raw() + offset_product), 0x200);
    // Version
    version = ZString(strs.raw() + offset_version, 12);

    LOG("Company:     " << company);
    LOG("Product:     " << product);
    LOG("Version:     " << version);

    LOG("Signature:   " << ZString(strs.raw() + offset_sig, strings_len - offset_sig));

//    LOG("String Dump:");
//    RLOG(strs.dumpBytes(4, 8));

    // Decode other encrypted sections

    zu64 total = strings_len;
    ZArray<zu64> sections;

    zu64 sec_len = ZBinary::decleu32(strs.raw() + 0x420); // Firmware length

    ZString layout;
    layout.parseUTF16((const zu16 *)(strs.raw() + 0x424), 0x20);
    LOG("Layout: " << layout);

    total += sec_len;
    sections.push(sec_len);

    zu64 sec_start = exelen - total;

    LOG("Offset: 0x" << ZString::ItoS(sec_start, 16));
    LOG("Length: 0x" << ZString::ItoS(sec_len, 16));

    // Read section
    ZBinary sec = pkg_file_read(file, sec_start, sec_len);
    // Decode section
    decode_package_data(sec);

    // Decrypt firmware
    ProtoPOK3R::decode_firmware(sec);

//    LOG("Section Dump:");
//    RLOG(sec.dumpBytes(4, 8, 0));

    // Write firmware
    fw_out = sec;

    return 0;

}

/*  Decode the updater for the POK3R RGB / Vortex Core.
 */
int decode_maav102(ZFile *file, ZBinary &fw_out){
    zu64 exelen = file->fileSize();

    zu64 strings_len = 0xB24;   // from IDA disassembly in sub_403830 of v130 updater
                                // same size in v104
    zu64 strings_start = exelen - strings_len;

    zu64 offset_desc = 0x26;
    zu64 offset_company = offset_desc + 0x208;      // 0x22e
    zu64 offset_product = offset_company + 0x208;   // 0x436
    zu64 offset_version = offset_product + 0x208;   // 0x63e
    zu64 offset_sig = 0xb19;

    // Read strings
    ZBinary strs = pkg_file_read(file, strings_start, strings_len);
    // Decrypt strings
    decode_package_data(strs);

    ZString desc;
    ZString company;
    ZString product;
    ZString version;

    // Description
    desc.parseUTF16((const zu16 *)(strs.raw() + offset_desc), 0x200);
    // Company name
    company.parseUTF16((const zu16 *)(strs.raw() + offset_company), 0x200);
    // Product name
    product.parseUTF16((const zu16 *)(strs.raw() + offset_product), 0x200);
    // Version
    version.parseUTF16((const zu16 *)(strs.raw() + offset_version), 0x200);

    LOG("==============================");

    LOG("Description: " << desc);
    LOG("Company:     " << company);
    LOG("Product:     " << product);
    LOG("Version:     " << version);

    LOG("Signature:   " << ZString(strs.raw() + offset_sig, strings_len - offset_sig));

//    LOG("String Dump:");
//    RLOG(strs.dumpBytes(4, 8));

    LOG("==============================");

    // Decode other encrypted sections

    zu64 total = strings_len;
    ZArray<zu64> sections;

    zu64 start = 0xAC8 - (0x50 * 8);
    for(zu8 i = 0; i < 8; ++i){
        zu32 fwl = ZBinary::decleu32(strs.raw() + start); // Firmware length
        zu32 strl = ZBinary::decleu32(strs.raw() + start + 4); // Info length

        if(fwl){
            ZString layout;
            layout.parseUTF16((const zu16 *)(strs.raw() + start + 8), 0x20);
            LOG("Layout " << i << ": " << layout);

            total += fwl;
            total += strl;
            sections.push(fwl);
            sections.push(strl);
        }
        start += 0x50;
    }

    LOG("==============================");

    zu64 sec_start = exelen - total;
    LOG("Section Count: " << sections.size());

    for(zu64 i = 0; i < sections.size(); ++i){
        zu64 sec_len = sections[i];
        if(sec_len == 0)
            continue;

        LOG("Section " << i << ":");
//        LOG("  Offset: 0x" << ZString::ItoS(sec_start, 16));
        LOG("  Length: " << sec_len);

        // Read section
        ZBinary sec = pkg_file_read(file, sec_start, sec_len);
        sec_start += sec_len;

        // Decode section
        decode_package_data(sec);

        // Decrypt RGB firmwares only
        if(sec.size() == 180){
            LOG("  Data");
            RLOG(sec.dumpBytes(4, 8, 0));
            ProtoCYKB::info_section(sec);
            continue;
        } else {
            LOG("  Firmware" << ZLog::NOLN);
            ProtoCYKB::decode_firmware(sec);
            if(i == 0){
                fw_out = sec;
                RLOG(" (output)");
            }
            RLOG(ZLog::NEWLN);
        }
    }

    return 0;
}

int decode_maav105(ZFile *file, ZBinary &fw_out){
    zu64 exelen = file->fileSize();
    zu64 strings_len = 0x2b58;   // from decompiled FUN_4049d0 of TAB_75_V100
    zu64 strings_start = exelen - strings_len;

//    zu64 offset_desc = 0x23de - 180;
    zu64 offset_desc = 0x232a;
    zu64 offset_company = offset_desc + 0x208;
    zu64 offset_product = offset_company + 0x208;
    zu64 offset_version = offset_product + 0x208;
    zu64 offset_sig = exelen - strings_start - 13;

    // Read strings
    ZBinary strs = pkg_file_read(file, strings_start, exelen - strings_start);
    // Decrypt strings
    decode_package_data(strs);

//    zu64 pos = 0x0;
//    for(int i = 0; i < 20; ++i){
//        zu32 d = ZBinary::decleu32(strs.raw() + pos); // Firmware length
//        LOG(ZString::ItoS((zu64)d, 16));
//        pos += 4;
//    }

    RLOG(strs.dumpBytes(4, 8));
    ZFile::writeBinary("manifest.bin", strs);

    ZString pkgdesc;
    ZString company;
    ZString product;
    ZString pkgver;

    // Description
    pkgdesc.parseUTF16((const zu16 *)(strs.raw() + offset_desc), 0x200);
    // Company name
    company.parseUTF16((const zu16 *)(strs.raw() + offset_company), 0x200);
    // Product name
    product.parseUTF16((const zu16 *)(strs.raw() + offset_product), 0x200);
    // Version
    pkgver.parseUTF16((const zu16 *)(strs.raw() + offset_version), 0x200);

    LOG("==============================");
    LOG("Description: " << pkgdesc);
    LOG("Company:     " << company);
    LOG("Product:     " << product);
    LOG("Version:     " << pkgver);
    LOG("Signature:   " << ZString(strs.raw() + offset_sig, 13));

    zu64 section_start = 0x1F1600;

    zu64 list_pos = 0xc8;
    for(int i = 0; i < 4; ++i){
        zu64 desc_start = list_pos;
        zu64 version_start = desc_start + 0x208;
        zu64 addr_pos = version_start + 0x208;
        zu64 layout_start = addr_pos + 8;

        LOG("==============================");

        // Product name
        ZString desc;
        desc.parseUTF16((const zu16 *)(strs.raw() + desc_start), 0x200);
        // Version
        ZString version;
        version.parseUTF16((const zu16 *)(strs.raw() + version_start), 0x200);

        LOG("Description: " << desc);
        LOG("Version:     " << version);

        list_pos = layout_start + 0x2c8;

        while(strs[layout_start]){
            // Layout
            ZString layout;
            layout.parseUTF16((const zu16 *)(strs.raw() + layout_start), 0x200);
            zu16 a1 = ZBinary::decleu16(strs.raw() + layout_start + 60);
            zu16 a2 = ZBinary::decleu16(strs.raw() + layout_start + 62);
            zu16 a3 = ZBinary::decleu16(strs.raw() + layout_start + 64);
            LOG("Layout:      " << layout << " " << a1 << " " << a2 << " " << a3);
            layout_start += 80;
        }

        zu32 fwl = ZBinary::decleu32(strs.raw() + addr_pos); // Firmware length
        zu32 strl = ZBinary::decleu32(strs.raw() + addr_pos + 4); // Info length

        // Read firmware
        ZBinary fw = pkg_file_read(file, section_start, fwl);
        section_start += fwl;
        // Decrypt firmware
        decode_package_data(fw);
        ProtoCYKB::decode_firmware(fw);

        // Read info
        ZBinary info = pkg_file_read(file, section_start, strl);
        section_start += strl;
        // Decrypt info
        decode_package_data(info);

        if(fw.size())
            fw_out = fw;

        LOG("Firmware:    " << fwl);
//        RLOG(fw.dumpBytes(4, 8));
        LOG("Info:        " << strl);
        if(info.size())
            RLOG(info.dumpBytes(4, 8));

    }

    return 0;
}

void kbp_decrypt(zbyte *data, zu64 size, zu32 key){
    zbyte xor_key[4];
    ZBinary::encbeu32(xor_key, key);
    for(zu64 i = 0; i < size; ++i){
        data[i] = data[i] ^ xor_key[i % 4] ^ (i & 0xFF);
    }
}

/*  Decode the updater for the KBP V60 / V80.
 */
int decode_kbp_cykb(ZFile *file, ZBinary &fw_out, zu32 key){
    zu64 exelen = file->fileSize();
    zu64 strings_len = 588;
    zu64 strings_start = exelen - strings_len;

    // Read strings
    ZBinary strs;
    if(file->seek(strings_start) != strings_start){
        LOG("File too short - seek");
        return -4;
    }
    if(file->read(strs, strings_len) != strings_len){
        LOG("File too short - read");
        return -5;
    }

    // Decrypt strings
    kbp_decrypt(strs.raw(), strs.size(), key);

    LOG("String Dump:");
    RLOG(strs.dumpBytes(4, 8));

    zu64 fw_start = 0x54000;
    zu64 fw_len = ZBinary::decleu32(strs.raw() + 4);

    LOG("Firmware Size 0x" << ZString::ItoS(fw_len, 16));

    // Read firmware
    ZBinary fw;
    if(file->seek(fw_start) != fw_start){
        LOG("File too short - seek");
        return -2;
    }
    if(file->read(fw, fw_len) != fw_len){
        LOG("File too short - read");
        return -3;
    }

    // Decrypt firmware
    kbp_decrypt(fw.raw(), fw.size(), key);
    ProtoPOK3R::decode_firmware(fw);
    fw_out = fw;

    //RLOG(fw_out.dumpBytes(4, 8));

    return 0;
}

int decode_kbp_v60(ZFile *file, ZBinary &fw_out){
    zu32 key = 0xDA6282CD;  // v60
    return decode_kbp_cykb(file, fw_out, key);
}

int decode_kbp_v80(ZFile *file, ZBinary &fw_out){
    zu32 key = 0xF6F3111F;  // v80
    return decode_kbp_cykb(file, fw_out, key);
}

int encode_image(ZPath fwin, ZPath fwout){
    LOG("Input: " << fwin);

    // Read firmware
    ZBinary fwbin;
    if(!ZFile::readBinary(fwin, fwbin)){
        LOG("Failed to read file");
        return -1;
    }

//    Pok3r::encode_firmware(fwbin);
    ProtoCYKB::encode_firmware(fwbin);
    encode_package_data(fwbin);

    LOG("Output: " << fwout);

    // Write encoded image
    if(!ZFile::writeBinary(fwout, fwbin)){
        ELOG("Failed to write file");
        return -2;
    }

    return 0;
}

int encode_patch_updater(ZPath exein, ZPath fwin, ZPath exeout){
    LOG("In Updater: " << exein);
    LOG("In Firmware: " << fwin);

    // Read updater
    ZBinary exebin;
    if(!ZFile::readBinary(exein, exebin)){
        ELOG("Failed to read file");
        return -1;
    }

    // Read firmware
    ZBinary fwbin;
    if(!ZFile::readBinary(fwin, fwbin)){
        LOG("Failed to read file");
        return -2;
    }

    // Encode firmware
//    Pok3r::encode_firmware(fwbin);
    ProtoCYKB::encode_firmware(fwbin);
    encode_package_data(fwbin);

    // Write encoded firmware onto exe
//    exebin.seek(0x1A3800);
    exebin.seek(0x2BE000);
    exebin.write(fwbin);

    ZFile exefile;
    if(!exefile.open(exeout, ZFile::WRITE)){
        ELOG("Failed to open file");
        return -3;
    }
    // Write updated exe
    if(!exefile.write(exebin)){
        LOG("Write error");
        return -4;
    }

    LOG("Out Updater: " << exeout);

    return 0;
}
