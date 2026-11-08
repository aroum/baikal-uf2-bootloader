#!/usr/bin/env python3
"""Convert binary or HEX files to UF2 format for Baikal BE-U1000 EVU-LI-2.1."""
import argparse
import os
import re
import struct
import sys

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END    = 0x0AB16F30

DEFAULT_BASE_ADDR = 0xA0004000
DEFAULT_FAMILY_ID = 0x6ecb67a8

BUILTIN_FAMILIES = {
    "BAIKAL_EVU_LI_2_1": 0x6ecb67a8,
    "RP2040": 0xe48bff56,
}

appstartaddr = DEFAULT_BASE_ADDR
familyid = DEFAULT_FAMILY_ID


def is_uf2(buf):
    if len(buf) < 512:
        return False
    w = struct.unpack("<II", buf[0:8])
    return w[0] == UF2_MAGIC_START0 and w[1] == UF2_MAGIC_START1


def is_hex(buf):
    try:
        w = buf[0:30].decode("utf-8")
    except UnicodeDecodeError:
        return False
    return w.startswith(":") and bool(re.match(rb"^[:0-9a-fA-F\r\n]+$", buf))


def convert_from_hex_to_bin(buf):
    upper = 0
    outp = bytearray()
    curr_addr = 0
    for line in buf.decode("utf-8").splitlines():
        if not line.startswith(":"):
            continue
        line = line[1:].strip()
        if not line:
            continue
        byte_cnt = int(line[0:2], 16)
        addr = int(line[2:6], 16)
        rec_type = int(line[6:8], 16)
        data = bytes.fromhex(line[8:8 + 2 * byte_cnt])
        if rec_type == 0:
            full_addr = upper + addr
            if curr_addr == 0 and not outp:
                curr_addr = full_addr
            if full_addr > curr_addr:
                outp.extend(b"\x00" * (full_addr - curr_addr))
                curr_addr = full_addr
            outp.extend(data)
            curr_addr += len(data)
        elif rec_type == 1:
            break
        elif rec_type == 2:
            upper = int(line[8:12], 16) << 4
        elif rec_type == 4:
            upper = int(line[8:12], 16) << 16
    return bytes(outp)


def convert_to_uf2(file_content):
    global familyid, appstartaddr
    datapadding = b"\x00" * (512 - 256 - 32 - 4)
    numblocks = (len(file_content) + 255) // 256
    outp = []
    for blockno in range(numblocks):
        ptr = 256 * blockno
        chunk = file_content[ptr:ptr + 256]
        flags = 0x2000 if familyid else 0x0
        hd = struct.pack(
            b"<IIIIIIII",
            UF2_MAGIC_START0,
            UF2_MAGIC_START1,
            flags,
            ptr + appstartaddr,
            256,
            blockno,
            numblocks,
            familyid,
        )
        if len(chunk) < 256:
            chunk = chunk + b"\x00" * (256 - len(chunk))
        block = hd + chunk + datapadding + struct.pack(b"<I", UF2_MAGIC_END)
        assert len(block) == 512
        outp.append(block)
    return b"".join(outp)


def convert_from_uf2(buf):
    global appstartaddr, familyid
    numblocks = len(buf) // 512
    curraddr = None
    outp = bytearray()
    for blockno in range(numblocks):
        ptr = blockno * 512
        block = buf[ptr:ptr + 512]
        hd = struct.unpack(b"<IIIIIIII", block[0:32])
        if hd[0] != UF2_MAGIC_START0 or hd[1] != UF2_MAGIC_START1:
            continue
        if hd[2] & 1:
            continue
        datalen = hd[4]
        if datalen > 476:
            continue
        newaddr = hd[3]
        if curraddr is None:
            curraddr = newaddr
            appstartaddr = newaddr
        if newaddr > curraddr:
            outp.extend(b"\x00" * (newaddr - curraddr))
            curraddr = newaddr
        outp.extend(block[32:32 + datalen])
        curraddr += datalen
    return bytes(outp)


def main():
    global appstartaddr, familyid
    parser = argparse.ArgumentParser(description="Convert to UF2 or unpack UF2.")
    parser.add_argument("input", nargs="?", help="Input file (BIN, HEX, or UF2)")
    parser.add_argument(
        "-b", "--base", default=hex(DEFAULT_BASE_ADDR),
        help=f"Target base address (default: 0x{DEFAULT_BASE_ADDR:08X})"
    )
    parser.add_argument(
        "-f", "--family", default=hex(DEFAULT_FAMILY_ID),
        help=f"Microcontroller Family ID (default: 0x{DEFAULT_FAMILY_ID:08X})"
    )
    parser.add_argument("-o", "--output", help="Output file")
    parser.add_argument("-c", "--convert", action="store_true", help="Convert to UF2")
    parser.add_argument("-i", "--info", action="store_true", help="Display UF2 info")

    args = parser.parse_args()
    if not args.input:
        parser.print_help()
        sys.exit(1)

    appstartaddr = int(args.base, 0)
    fam_upper = args.family.upper()
    if fam_upper in BUILTIN_FAMILIES:
        familyid = BUILTIN_FAMILIES[fam_upper]
    else:
        familyid = int(args.family, 0)

    with open(args.input, "rb") as f:
        content = f.read()

    if is_uf2(content):
        if args.info:
            numblocks = len(content) // 512
            print(f"UF2 file: {numblocks} blocks ({numblocks * 512} bytes)")
            hd = struct.unpack(b"<IIIIIIII", content[0:32])
            print(f"First block target address: 0x{hd[3]:08X}")
            print(f"Family ID: 0x{hd[7]:08X}")
            return
        bin_content = convert_from_uf2(content)
        out_name = args.output or "out.bin"
        with open(out_name, "wb") as f:
            f.write(bin_content)
        print(f"Wrote {len(bin_content)} bytes to {out_name}")
        return

    if is_hex(content):
        content = convert_from_hex_to_bin(content)

    uf2_content = convert_to_uf2(content)
    out_name = args.output or os.path.splitext(args.input)[0] + ".uf2"
    with open(out_name, "wb") as f:
        f.write(uf2_content)
    print(f"Wrote {len(uf2_content)} bytes to {out_name} (base=0x{appstartaddr:08X}, family=0x{familyid:08X})")


if __name__ == "__main__":
    main()
