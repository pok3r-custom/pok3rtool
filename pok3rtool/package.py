
import io
import logging
from pathlib import Path
import zipfile

import rarfile
import pefile
from dissect import cstruct


from .pok3r import POK3R_Device
from .cykb import CYKB_Device

log = logging.getLogger(__name__)


# Decode the encryption scheme used by the updater program.
# Produced from IDA disassembly in sub_401000 of v117 updater.
# First, swap the 1st and 4th bytes, every 5 bytes
# Second, reverse each pair of bytes
# Third, shift the bits in each byte, sub 7 from MSBs
def decode_package_data(data: bytes):
    bin = bytearray(data)
    # Swap bytes 4 apart, skip 5
    for i in range(4, len(bin), 5):
        a = bin[i-4]
        b = bin[i]
        bin[i-4] = b
        bin[i] = a

    # Swap bytes in each set of two bytes
    for i in range(1, len(bin), 2):
        d = bin[i-1]
        b = bin[i]
        bin[i-1] = b
        bin[i] = d

    # y = ((x - 7) << 4) + (x >> 4)
    for i in range(len(bin)):
        bin[i] = (((bin[i] - 7) << 4) + (bin[i] >> 4)) & 0xFF

    return bytes(bin)


def dump_info_section(data: bytes):
    if data[:4] == b"\xff\xff\xff\xff":
        ver = "CLEARED"
    else:
        vlen = min(int.from_bytes(data[:4], "little"), 60)
        ver = data[4:vlen].decode("utf-16le").rstrip("\0")
    log.info(f"Version: {ver}")

    a = int.from_bytes(data[0x78:][:4], "little")
    b = int.from_bytes(data[0x7c:][:4], "little")
    c = int.from_bytes(data[0x80:][:4], "little")
    d = int.from_bytes(data[0x84:][:4], "little")
    e = int.from_bytes(data[0x88:][:4], "little")
    f = int.from_bytes(data[0x8c:][:4], "little")
    ivid = int.from_bytes(data[0x90:][:2], "little")
    ipid = int.from_bytes(data[0x92:][:2], "little")
    h = int.from_bytes(data[0xb0:][:4], "little")

    log.info(f"a: {a:#x}")
    log.info(f"b: {b:#x}")
    log.info(f"c: {c:#x}")
    log.info(f"d: {d:#x}")
    log.info(f"e: {e:#x}")
    log.info(f"f: {f:#x}")
    log.info(f"VID/PID: {ivid:#x}/{ipid:#x}")
    log.info(f"h: {h:#x}")


cparser = cstruct.cstruct("""
    struct maajonsn_info {
        uint32 app_vid;
        uint32 app_pid;
        uint32 boot_vid;
        uint32 boot_pid;
        
        wchar_t company[0x104];
        wchar_t product[0x104];
        
        uint32 firmware_size;
        
        wchar_t layout_name[30];
        char version[12];
        
        char unknown_46c[66];
        char sig[10];
    };

    struct maav101_layout {
        uint32 firmware_size;
        wchar_t name[30];
        char version[6];
        uint16 unknown_46;
    };

    struct maav101_info {
        uint32 app_vid;
        uint32 app_pid;
        uint32 boot_vid;
        uint32 boot_pid;
        
        wchar_t company[0x104];
        wchar_t product[0x104];
        
        struct maav101_layout layouts[2];
        
        char sig[12];
    };

    struct maav102_device {
        uint32 unknown_00;
        uint16 vid;
        uint16 pid;
        uint16 page_size;
        uint16 unknown_0a;
        uint32 unknown_0c;
        uint32 unknown_10;
        uint32 boot_vid;
        uint32 boot_pid;
        uint32 app_vid;
        uint32 app_pid;
        uint16 unknown_24;
    };

    struct maav102_layout {
        uint32 firmware_size;
        uint32 version_size;
        wchar_t name[30];
        uint32 unknown_44;
        uint32 unknown_48;
        uint32 unknown_4c;
    };

    struct maav102_info {
        struct maav102_device device;
        
        wchar_t desc[0x104];
        wchar_t company[0x104];
        wchar_t product[0x104];
        wchar_t version[0x104];
        
        uint16 unknown_846;
        
        struct maav102_layout layouts[9];
        
        uint16 sig1;
        char sig2[10];
    };

    struct maav105_device {
        uint32 unknown_00;
        uint16 vid;
        uint16 pid;
        uint16 page_size;
        uint16 unknown_0a;
        uint32 unknown_0c;
        uint32 unknown_10;
        uint32 boot_vid;
        uint32 boot_pid;
        uint32 app_vid;
        uint32 app_pid;
        uint32 unknown_24;
    };

    struct maav105_layout {
        uint32 firmware_size;
        uint32 version_size;
        wchar_t name[30];
        uint32 unknown_44;
        uint32 unknown_48;
        uint32 unknown_4c;
    };

    struct maav105_section {
        wchar_t desc[0x104];
        wchar_t version[0x104];

        struct maav105_layout layouts[9];
    };

    struct maav105_info {
        struct maav105_device devices[5];
        struct maav105_section sections[5];

        uint16 num;

        wchar_t desc[0x104];
        wchar_t company[0x104];
        wchar_t product[0x104];
        wchar_t version[0x104];

        uint16 sig1;
        char sig2[12];
    };
    
    struct maav106_device {
        uint32 unknown_00;
        uint16 vid;
        uint16 pid;
        uint32 unknown_08;
        uint32 unknown_0c;
        uint32 unknown_10;
        uint32 unknown_14;
        uint32 unknown_18;
        uint32 unknown_1c;
        uint32 unknown_20;
        uint32 unknown_24;
        uint32 unknown_28;
        uint32 unknown_2c;
        uint32 unknown_30;
        uint32 unknown_34;
        uint32 unknown_38;
        uint32 unknown_3c;
        uint32 unknown_40;
        uint32 unknown_44;
        uint32 unknown_48;
        uint32 unknown_4c;
        uint32 unknown_50;
        uint32 app_vid;
        uint32 app_pid;
        uint32 boot_vid;
        uint32 boot_pid;
        uint32 unknown_64;
    };

    struct maav106_layout {
        uint32 firmware_size;
        uint32 version_size;
        wchar_t name[30];
        uint32 unknown_44;
        uint32 unknown_48;
        uint32 unknown_4c;
    };

    struct maav106_section {
        wchar_t desc[0x104];
        wchar_t version[0x104];

        struct maav106_layout layouts[9];
    };

    struct maav106_info {
        struct maav106_device devices[5];
        struct maav106_section sections[5];

        uint16 num;

        wchar_t desc[0x104];
        wchar_t company[0x104];
        wchar_t product[0x104];
        wchar_t version[0x104];

        uint16 sig1;
        char sig2[12];
    };
""")


def extract_maajonsn(file: Path, output: Path | None):
    """
    Decode the updater for the POK3R.
    """
    with open(file, "rb") as f:
        fdata = f.read()

    strings_len = len(cparser.maajonsn_info)
    strs = decode_package_data(fdata[-strings_len:])

    signature = strs[-10:-1]
    assert signature == b".maajonsn"

    # log.debug(strs)
    info = cparser.maajonsn_info(strs)
    log.debug(info)
    # cstruct.dumpstruct(info)

    company, _, _ = info.company.partition("\0")
    product, _, _ = info.product.partition("\0")
    layout, _, _ = info.layout_name.partition("\0")
    version, _, _ = info.version.decode().partition("\0")

    log.info(f"Company: {company}")
    log.info(f"Product: {product}")
    log.info(f"Layout: {layout}")
    log.info(f"Version: {version}")

    sec = decode_package_data(fdata[-strings_len-info.firmware_size:-strings_len])

    dec_sec = POK3R_Device.decode_firmware(sec)

    # built-in test for the encoding function
    check = POK3R_Device.encode_firmware(dec_sec)
    assert check == sec, "re-encode failed"

    if output:
        output.mkdir(exist_ok=True)
        name = f"{product}-{layout}-{version}.bin"
        name = name.replace(" ", "_")
        with open(output / name, "wb") as fo:
            log.info(f"Save {fo.name}")
            fo.write(dec_sec)


def extract_coolermater_installer(fdata: bytes):
    """
    Find EXEs in a self-extracting RAR installer.
    """
    pe = pefile.PE(data=fdata)

    for sec in pe.sections:
        if sec.Name.rstrip(b"\0").decode() == ".rsrc":
            rsrc_end = sec.PointerToRawData + sec.SizeOfRawData
            break
    else:
        assert False, "no rsrc section"

    log.debug(f"RAR addr: {rsrc_end:x}")
    rar_data = fdata[rsrc_end:]

    rfile = io.BytesIO(rar_data)
    with rarfile.RarFile(rfile) as rf:
        for info in rf.infolist():
            if info.is_dir():
                continue
            if info.filename.endswith(".exe"):
                log.debug(f"{info.filename}: {info.file_size} bytes")
                with rf.open(info) as f:
                    yield f.read()


def extract_maav101(file: Path, output: Path | None):
    """
    Decode the updater for the CM MK Pro S/L RGB.
    """
    if file.suffix == ".zip":
        with zipfile.ZipFile(file) as zf:
            for info in zf.infolist():
                if info.filename.endswith(".exe"):
                    log.debug(f"{info.filename}: {info.file_size} bytes")
                    with zf.open(info) as f:
                        fdata = f.read()
                        break
    else:
        with open(file, "rb") as f:
            fdata = f.read()

    strings_len = len(cparser.maav101_info)
    strs = decode_package_data(fdata[-strings_len:])

    signature = strs[-13:-5]
    if signature != b".maaV101":
        for fdata in extract_coolermater_installer(fdata):
            strs = decode_package_data(fdata[-strings_len:])
            signature = strs[-13:-5]
            if signature == b".maaV101":
                break
        else:
            assert False, "did not find updater exe"

    # log.debug(strs)
    info = cparser.maav101_info(strs)
    log.debug(info)
    # cstruct.dumpstruct(info)

    company, _, _ = info.company.partition("\0")
    product, _, _ = info.product.partition("\0")

    log.info(f"Company: {company}")
    log.info(f"Product: {product}")

    total = strings_len
    sections = []

    for i, layout in enumerate(info.layouts):
        if layout.firmware_size:
            slayout, _, _ = layout.name.partition("\0")
            sversion, _, _ = layout.version.decode().partition("\0")

            log.info(f"  Layout {i}: firmware {layout.firmware_size} bytes")
            log.info(f"    \"{slayout}\" \"{sversion}\"")

            total += layout.firmware_size
            sections.append((slayout, sversion, layout.firmware_size))

    pos = -total
    for layout, version, fwl in sections:
        fsec = decode_package_data(fdata[pos:pos+fwl])
        pos += fwl

        dec_sec = POK3R_Device.decode_firmware(fsec)

        # built-in test for the encoding function
        check = POK3R_Device.encode_firmware(dec_sec)
        assert check == fsec, "re-encode failed"

        if output:
            output.mkdir(exist_ok=True)
            name = f"{product}-{layout}-{version}.bin"
            name = name.replace(" ", "_")
            with open(output / name, "wb") as fo:
                log.info(f"Save {fo.name}")
                fo.write(dec_sec)


def extract_maav102(file: Path, output: Path | None):
    """
    Decode the updater for the POK3R RGB / Vortex Core.
    """
    if file.suffix == ".zip":
        with zipfile.ZipFile(file) as zf:
            for info in zf.infolist():
                if info.filename.endswith(".exe"):
                    log.debug(f"{info.filename}: {info.file_size} bytes")
                    with zf.open(info) as f:
                        fdata = f.read()
                        break
    else:
        with open(file, "rb") as f:
            fdata = f.read()

    strings_len = len(cparser.maav102_info)
    strs = decode_package_data(fdata[-strings_len:])

    signature = strs[-11:-3]
    if signature != b".maaV102":
        for fdata in extract_coolermater_installer(fdata):
            strs = decode_package_data(fdata[-strings_len:])
            signature = strs[-11:-3]
            if signature == b".maaV102":
                break
        else:
            assert False, "did not find updater exe"

    # log.debug(strs)
    info = cparser.maav102_info(strs)
    log.debug(info)
    # cstruct.dumpstruct(info)

    desc, _, _ = info.desc.partition("\0")
    company, _, _ = info.company.partition("\0")
    product, _, _ = info.product.partition("\0")
    version, _, _ = info.version.partition("\0")

    log.info(f"Description: {desc}")
    log.info(f"Company: {company}")
    log.info(f"Product: {product}")
    log.info(f"Version: {version}")

    total = strings_len
    sections = []

    for i, layout in enumerate(info.layouts):
        if layout.firmware_size:
            slayout, _, _ = layout.name.partition("\0")

            log.info(f"  Layout {i}: firmware {layout.firmware_size} bytes, version {layout.version_size} bytes")
            log.info(f"    \"{slayout}\"")

            total += layout.firmware_size
            total += layout.version_size
            sections.append((slayout, layout.firmware_size, layout.version_size))

    pos = -total
    for layout, fwl, strl in sections:
        fsec = decode_package_data(fdata[pos:pos+fwl])
        pos += fwl

        dec_sec = CYKB_Device.decode_firmware(fsec)

        # built-in test for the encoding function
        check = CYKB_Device.encode_firmware(dec_sec)
        assert check == fsec, "re-encode failed"

        isec = decode_package_data(fdata[pos:pos+strl])
        pos += strl

        log.debug(isec)
        dump_info_section(isec)

        if output:
            output.mkdir(exist_ok=True)
            name = f"{product}-{layout}-{version}.bin"
            name = name.replace(" ", "_")
            with open(output / name, "wb") as fo:
                log.info(f"Save {fo.name}")
                fo.write(dec_sec)


def extract_maav105_maav106(file: Path, output: Path | None, sig: bytes, struct_type: type[cstruct.Structure]):
    if file.suffix == ".zip":
        with zipfile.ZipFile(file) as zf:
            for info in zf.infolist():
                if info.filename.endswith(".exe"):
                    log.debug(f"{info.filename}: {info.file_size} bytes")
                    with zf.open(info) as f:
                        fdata = f.read()
                        break
    else:
        with open(file, "rb") as f:
            fdata = f.read()

    strings_len = len(struct_type)
    strs = decode_package_data(fdata[-strings_len:])

    signature = strs[-len(sig):]
    if signature != sig:
        for fdata in extract_coolermater_installer(fdata):
            strs = decode_package_data(fdata[-strings_len:])
            signature = strs[-len(sig):]
            if signature == sig:
                break
        else:
            assert False, "did not find updater exe"

    # log.debug(strs)
    info = struct_type(strs)
    log.debug(info)
    # cstruct.dumpstruct(info)

    desc, _, _ = info.desc.partition("\0")
    company, _, _ = info.company.partition("\0")
    product, _, _ = info.product.partition("\0")
    version, _, _ = info.version.partition("\0")

    log.info(f"Description: {desc}")
    log.info(f"Company: {company}")
    log.info(f"Product: {product}")
    log.info(f"Version: {version}")

    total = strings_len
    sections = []

    for i, section in enumerate(info.sections):
        sdesc, _, _ = section.desc.partition("\0")
        sversion, _, _ = section.version.partition("\0")

        log.info(f"Section {i}:")
        log.info(f"  \"{sdesc}\" \"{sversion}\"")

        for j, layout in enumerate(section.layouts):
            if layout.firmware_size:
                slayout, _, _ = layout.name.partition("\0")

                log.info(f"  Layout {j}: firmware {layout.firmware_size} bytes, version {layout.version_size} bytes")
                log.info(f"    \"{slayout}\"")

                total += layout.firmware_size
                total += layout.version_size
                sections.append((sdesc, sversion, slayout, layout.firmware_size, layout.version_size))

    pos = -total
    for sdesc, sversion, slayout, fwl, strl in sections:
        fsec = decode_package_data(fdata[pos:pos+fwl])
        pos += fwl

        dec_sec = CYKB_Device.decode_firmware(fsec)

        # built-in test for the encoding function
        check = CYKB_Device.encode_firmware(dec_sec)
        assert check == fsec, "re-encode failed"

        isec = decode_package_data(fdata[pos:pos+strl])
        pos += strl
        log.debug(isec)
        dump_info_section(isec)

        if output:
            output.mkdir(exist_ok=True)
            name = f"{product}-{version}-{sdesc}-{slayout}-{sversion}.bin"
            name = name.replace(" ", "_")
            with open(output / name, "wb") as fo:
                log.info(f"Save {fo.name}")
                fo.write(dec_sec)


def extract_maav105(file: Path, output: Path | None):
    extract_maav105_maav106(file, output, b".maaV105\0\0\0\0\0", cparser.maav105_info)


def extract_maav106(file: Path, output: Path | None):
    extract_maav105_maav106(file, output, b".maaV106\0\0\0\0\0", cparser.maav106_info)


def kbp_decrypt(enc: bytes, key: int, strs: bool):
    xor_key = key.to_bytes(4, "big")

    data = bytearray(enc)
    for i in range(len(data)):
        data[i] = data[i] ^ xor_key[i % 4]
        if strs:
            # strings are encrypted with a different schedule
            data[i] = data[i] ^ (i & 0x0F)
            if i >= 0x10:
                data[i] = data[i] ^ ((i - 0x10) & 0xF0)
        else:
            data[i] = data[i] ^ (i & 0xFF)
    return bytes(data)


def extract_kbp_cykb(file: Path, output: Path | None):
    with open(file, "rb") as f:
        f.seek(0, io.SEEK_END)
        exelen = f.tell()

        strings_len = 588
        f.seek(exelen - strings_len, io.SEEK_SET)
        enc_strs = f.read(strings_len)

        key = int.from_bytes(enc_strs[:4], "big") ^ 0x10203
        log.info(f"Key: {key:08X}")

        strs = kbp_decrypt(enc_strs, key, True)
        log.debug(strs)

        signature = strs[-4:]
        assert signature == b"lins"

        name = strs[0xb8:][:32].decode("ascii").rstrip("\0")

        log.info(f"Name: {name}")

        fw_len = int.from_bytes(strs[4:4+4], "little")

        f.seek(0x54000, io.SEEK_SET)
        fw = kbp_decrypt(f.read(fw_len), key, False)

        dec_fw = POK3R_Device.decode_firmware(fw)

        # built-in test for the encoding function
        check = POK3R_Device.encode_firmware(dec_fw)
        assert check == fw, "re-encode failed"

        if output:
            output.mkdir(exist_ok=True)
            name = f"{name}.bin"
            name = name.replace(" ", "_")
            with open(output / name, "wb") as fo:
                log.info(f"Save {fo.name}")
                fo.write(dec_fw)
