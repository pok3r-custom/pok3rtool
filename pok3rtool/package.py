
import io
import logging
from pathlib import Path

from .pok3r import POK3R_Device
from .cykb import CYKB_Device

log = logging.getLogger(__name__)


#  Decode the encryption scheme used by the updater program.
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


def extract_maajonsn(file: Path, output: Path | None):
    """
    Decode the updater for the POK3R.
    """
    with open(file, "rb") as f:
        f.seek(0, io.SEEK_END)
        exelen = f.tell()

        strings_len = 0x4B8
        f.seek(exelen - strings_len, io.SEEK_SET)
        strs = decode_package_data(f.read(strings_len))
        log.debug(strs)

        signature = strs[0x4AE:-1]
        assert signature == b".maajonsn"

        company = strs[0x10:][:0x200].decode("utf-16le").rstrip("\0")
        product = strs[0x218:][:0x200].decode("utf-16le").rstrip("\0")
        version = strs[0x460:][:12].decode("ascii").rstrip("\0")

        log.info(f"Company: {company}")
        log.info(f"Product: {product}")
        log.info(f"Version: {version}")

        sec_len = int.from_bytes(strs[0x420:][:4], "little")

        layout = strs[0x424:][:0x20].decode("utf-16le").rstrip("\0")
        log.info(f"Layout: {layout}")

        sec_start = exelen - strings_len - sec_len
        f.seek(sec_start, io.SEEK_SET)
        sec = decode_package_data(f.read(sec_len))

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


def extract_maav102(file: Path, output: Path | None):
    """
    Decode the updater for the POK3R RGB / Vortex Core.
    """
    with open(file, "rb") as f:
        f.seek(0, io.SEEK_END)
        exelen = f.tell()

        # from IDA disassembly in sub_403830 of v130 updater
        # same size in v104
        strings_len = 0xB24
        f.seek(exelen - strings_len, io.SEEK_SET)
        strs = decode_package_data(f.read(strings_len))
        log.debug(strs)

        signature = strs[0xb19:-3]
        assert signature == b".maaV102"

        desc = strs[0x26:][:0x200].decode("utf-16le").rstrip("\0")
        company = strs[0x22e:][:0x200].decode("utf-16le").rstrip("\0")
        product = strs[0x436:][:0x200].decode("utf-16le").rstrip("\0")
        version = strs[0x63e:][:0x200].decode("utf-16le").rstrip("\0")

        log.info(f"Description: {desc}")
        log.info(f"Company: {company}")
        log.info(f"Product: {product}")
        log.info(f"Version: {version}")

        total = strings_len
        sections = []

        start = 0xac8 - (0x50 * 8)
        for i in range(8):
            # firmware length
            fwl = int.from_bytes(strs[start:][:4], "little")
            # info length
            strl = int.from_bytes(strs[start+4:][:4], "little")

            if fwl:
                layout = strs[start + 8:][:0x20].decode("utf-16le").rstrip("\0")
                log.info(f"  Layout: {layout} ({fwl}+{strl} bytes)")
                total += fwl
                total += strl
                sections.append((layout, fwl, strl))

            start += 0x50

        sec_start = exelen - total
        f.seek(sec_start, io.SEEK_SET)

        for layout, fwl, strl in sections:
            fsec = decode_package_data(f.read(fwl))

            dec_sec = CYKB_Device.decode_firmware(fsec)

            # built-in test for the encoding function
            check = CYKB_Device.encode_firmware(dec_sec)
            assert check == fsec, "re-encode failed"

            isec = decode_package_data(f.read(strl))
            log.debug(isec)
            dump_info_section(isec)

            if output:
                output.mkdir(exist_ok=True)
                name = f"{product}-{layout}-{version}.bin"
                name = name.replace(" ", "_")
                with open(output / name, "wb") as fo:
                    log.info(f"Save {fo.name}")
                    fo.write(dec_sec)


def extract_maav105(file: Path, output: Path | None):
    with open(file, "rb") as f:
        f.seek(0, io.SEEK_END)
        exelen = f.tell()

        # from decompiled FUN_4049d0 of TAB_75_V100
        strings_len = 0x2b58
        f.seek(exelen - strings_len, io.SEEK_SET)
        strs = decode_package_data(f.read(strings_len))
        log.debug(strs)

        signature = strs[-13:-5]
        assert signature == b".maaV105"

        desc = strs[0x232a:][:0x200].decode("utf-16le").rstrip("\0")
        company = strs[0x2532:][:0x200].decode("utf-16le").rstrip("\0")
        product = strs[0x273a:][:0x200].decode("utf-16le").rstrip("\0")
        version = strs[0x2942:][:0x200].decode("utf-16le").rstrip("\0")

        log.info(f"Description: {desc}")
        log.info(f"Company: {company}")
        log.info(f"Product: {product}")
        log.info(f"Version: {version}")

        sections = []

        start = 0xc8
        for i in range(4):
            sdesc = strs[start:][:0x200].decode("utf-16le").rstrip("\0")
            sversion = strs[start+0x208:][:0x200].decode("utf-16le").rstrip("\0")
            # firmware length
            fwl = int.from_bytes(strs[start+0x208+0x208:][:4], "little")
            # info length
            strl = int.from_bytes(strs[start+0x208+0x208+4:][:4], "little")

            if fwl:
                log.info(f"  Description: {sdesc}")
                log.info(f"  Version: {sversion}")

                log.info(f"  {fwl}+{strl} bytes")

                layout_start = start + 0x208 + 0x208 + 8
                layouts = []
                while True:
                    layout = strs[layout_start:][:60].decode("utf-16le").rstrip("\0")
                    if not layout:
                        break
                    a = int.from_bytes(strs[layout_start+60:][:4], "little")
                    b = int.from_bytes(strs[layout_start+64:][:4], "little")
                    log.info(f"    Layout: {layout} {a:x} {b:x}")
                    layouts.append(layout)
                    layout_start += 80

                slayout = " ".join(layouts)
                sections.append((sdesc, sversion, slayout, fwl, strl))

            start += 0x208 + 0x208 + 8 + 0x2c8

        section_start = 0x1f1600
        f.seek(section_start, io.SEEK_SET)

        for sdesc, sversion, slayout, fwl, strl in sections:
            fsec = decode_package_data(f.read(fwl))

            dec_sec = CYKB_Device.decode_firmware(fsec)

            # built-in test for the encoding function
            check = CYKB_Device.encode_firmware(dec_sec)
            assert check == fsec, "re-encode failed"

            isec = decode_package_data(f.read(strl))
            log.debug(isec)
            dump_info_section(isec)

            if output:
                output.mkdir(exist_ok=True)
                name = f"{product}-{version}-{sdesc}-{slayout}-{sversion}.bin"
                name = name.replace(" ", "_")
                with open(output / name, "wb") as fo:
                    log.info(f"Save {fo.name}")
                    fo.write(dec_sec)


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
