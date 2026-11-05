#!/usr/bin/env python3
"""Linux equivalent of stamp_app_descriptor.ps1, plus final image verification."""
import hashlib
from pathlib import Path
import re
import struct
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def stamp(path, version, name):
    data = bytearray(path.read_bytes())

    def u16(offset):
        require(0 <= offset and offset + 2 <= len(data), 'Truncated ELF.')
        return struct.unpack_from('<H', data, offset)[0]

    def u32(offset):
        require(0 <= offset and offset + 4 <= len(data), 'Truncated ELF.')
        return struct.unpack_from('<I', data, offset)[0]

    require(len(data) >= 52 and data[:4] == b'\x7fELF'
            and data[4:6] == b'\x01\x01' and u16(16) == 2 and u16(18) == 94,
            'Expected an ELF32 little-endian Xtensa executable.')
    table, size, count, name_index = u32(32), u16(46), u16(48), u16(50)
    require(size == 40 and count > 0 and name_index < count
            and table + size * count <= len(data),
            'Unsupported or truncated ELF section table.')
    header = table + size * name_index
    names_offset, names_size = u32(header + 16), u32(header + 20)
    require(names_offset + names_size <= len(data), 'Truncated ELF section names.')
    matches = []
    for i in range(count):
        header = table + size * i
        start = u32(header)
        require(start < names_size, 'Invalid ELF section name.')
        start += names_offset
        end = data.find(b'\0', start, names_offset + names_size)
        require(end != -1, 'Unterminated ELF section name.')
        if data[start:end] == b'.flash.appdesc':
            matches.append(header)
    require(len(matches) == 1, 'Expected exactly one .flash.appdesc section.')
    header = matches[0]
    offset, length = u32(header + 16), u32(header + 20)
    require(u32(header + 4) == 1 and length >= 256
            and offset + length <= len(data) and u32(offset) == 0xABCD5432,
            'Expected an ESP-IDF app descriptor at the start of .flash.appdesc; refusing to stamp.')
    for relative, value in ((16, version), (48, name)):
        data[offset + relative:offset + relative + 32] = value.encode('ascii').ljust(32, b'\0')
    path.write_bytes(data)
    print(f'ESP-IDF app descriptor: {name} {version}')


def verify(path, version, name):
    data = path.read_bytes()
    require(len(data) >= 288 and data[0] == 0xE9
            and struct.unpack_from('<I', data, 32)[0] == 0xABCD5432,
            'Generated binary does not contain the expected ESP-IDF app descriptor.')
    actual_version = data[48:80].rstrip(b'\0')
    actual_name = data[80:112].rstrip(b'\0')
    require(actual_version == version.encode('ascii') and actual_name == name.encode('ascii'),
            f'Descriptor mismatch: expected {name!r} {version!r}, found {actual_name!r} {actual_version!r}.')
    require(data[23] == 1, 'Expected an appended SHA-256 digest in the application image.')
    require(hashlib.sha256(data[:-32]).digest() == data[-32:],
            'Application image SHA-256 verification failed.')
    print(f'Verified descriptor: {name} {version}')


def main():
    require(len(sys.argv) == 5 and sys.argv[1] in ('stamp', 'verify'),
            'Usage: stamp_app_descriptor.py {stamp|verify} FILE VERSION APPLICATION_NAME')
    mode, filename, version, name = sys.argv[1:]
    require(re.fullmatch(r'[0-9A-Za-z][0-9A-Za-z.+_-]{0,30}', version) is not None,
            'Version must be 1-31 allowed ASCII release-identifier characters.')
    require(re.fullmatch(r'[0-9A-Za-z][0-9A-Za-z .+_-]{0,30}', name) is not None,
            'Application name must be 1-31 allowed ASCII characters.')
    (stamp if mode == 'stamp' else verify)(Path(filename), version, name)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, struct.error) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
