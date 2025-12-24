
import io
import logging
from pathlib import Path

from . import pok3r, cykb

log = logging.getLogger(__name__)

#  Decode the encryption scheme used by the updater program.
# Produced from IDA disassembly in sub_401000 of v117 updater.
# First, swap the 1st and 4th bytes, every 5 bytes
# Second, reverse each pair of bytes
# Third, shift the bits in each byte, sub 7 from MSBs
#
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

        signature = strs[0x4AE:]
        assert signature == b".maajonsn\0"

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

        dec_sec = pok3r.decode_firmware(sec)

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

        signature = strs[0xb19:]
        assert signature == b".maaV102\0\0\0"

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
            dec_sec = cykb.decode_firmware(fsec)

            isec = decode_package_data(f.read(strl))
            log.debug(isec)
            cykb.dump_info_section(isec)

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

        signature = strs[-13:]
        assert signature == b".maaV105\0\0\0\0\0"

        with open(output / "manifest.bin", "wb") as fo:
            fo.write(strs)

        desc = strs[0x232a:][:0x200].decode("utf-16le").rstrip("\0")
        company = strs[0x2532:][:0x200].decode("utf-16le").rstrip("\0")
        product = strs[0x273a:][:0x200].decode("utf-16le").rstrip("\0")
        version = strs[0x2942:][:0x200].decode("utf-16le").rstrip("\0")

        log.info(f"Description: {desc}")
        log.info(f"Company: {company}")
        log.info(f"Product: {product}")
        log.info(f"Version: {version}")

        total = strings_len
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
            dec_sec = cykb.decode_firmware(fsec)

            isec = decode_package_data(f.read(strl))
            log.debug(isec)
            cykb.dump_info_section(isec)

            if output:
                output.mkdir(exist_ok=True)
                name = f"{product}-{version}-{sdesc}-{slayout}-{sversion}.bin"
                name = name.replace(" ", "_")
                with open(output / name, "wb") as fo:
                    log.info(f"Save {fo.name}")
                    fo.write(dec_sec)
