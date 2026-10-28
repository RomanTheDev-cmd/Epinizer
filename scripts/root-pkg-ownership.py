#!/usr/bin/env python3
"""Set root:wheel in an expanded, unsigned pkg without chowning source files."""
import gzip
from pathlib import Path
import subprocess
import sys


def root_payload(payload):
    data = bytearray(payload)
    offset = 0
    while True:
        header = data[offset:offset + 76]
        if len(header) != 76 or header[:6] != b"070707":
            raise ValueError("Expected pkgbuild's odc cpio payload")
        name_size = int(header[59:65], 8)
        file_size = int(header[65:76], 8)
        end_name = offset + 76 + name_size
        end_file = end_name + file_size
        if name_size < 1 or end_file > len(data) or data[end_name - 1] != 0:
            raise ValueError("Truncated cpio entry")
        name = data[offset + 76:end_name - 1]
        # Only uid/gid change; paths, modes, links and file bytes stay intact.
        data[offset + 24:offset + 36] = b"000000000000"
        offset = end_file
        if name == b"TRAILER!!!":
            return bytes(data)


def normalize(folder):
    payload_path = folder / "Payload"
    with gzip.open(payload_path, "rb") as stream:
        original = stream.read()
    normalized = root_payload(original)
    assert root_payload(normalized) == normalized
    with gzip.open(payload_path, "wb") as stream:
        stream.write(normalized)
    listing = subprocess.check_output(["lsbom", str(folder / "Bom")], text=True)
    rows = []
    expected_rows = []
    for line in listing.splitlines():
        fields = line.split("\t")
        if len(fields) < 3:
            raise ValueError("Unexpected BOM row")
        if int(fields[1], 8) & 0o170000 == 0o040000:
            fields = fields[:3]  # pkgbuild can append unused zero fields to directory records.
        fields[2] = "0/0"
        expected_rows.append("\t".join(fields))
        fields[1] = "0" + fields[1]  # mkbom requires an explicit octal prefix.
        rows.append("\t".join(fields))
    filelist = folder / "ownership.txt"
    filelist.write_text("\n".join(rows) + "\n")
    subprocess.run(["mkbom", "-i", str(filelist), str(folder / "Bom")], check=True)
    filelist.unlink()
    actual = subprocess.check_output(["lsbom", str(folder / "Bom")], text=True)
    expected = "\n".join(expected_rows) + "\n"
    if actual != expected:
        raise ValueError("BOM content verification failed")


if __name__ == "__main__":
    normalize(Path(sys.argv[1]))
