#!/usr/bin/env python3
"""List / extract files from Generals & Zero Hour .big archives.

Format (from Win32BIGFileSystem.cpp): "BIGF", u32 archive size (LE), u32 file count (BE),
u32 header size (BE), then per file: u32 offset (BE), u32 size (BE), NUL-terminated path.

Usage:
  bigtool.py list    W3D.big [--filter tank]
  bigtool.py extract W3D.big OUTDIR [--filter .w3d]
"""
import argparse, os, struct, sys


def read_big(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"BIGF":
        raise ValueError(f"{path}: not a BIGF archive")
    count = struct.unpack(">I", data[8:12])[0]
    pos, entries = 16, []
    for _ in range(count):
        off, size = struct.unpack(">II", data[pos:pos + 8])
        end = data.index(b"\0", pos + 8)
        name = data[pos + 8:end].decode("latin-1").replace("\\", "/")
        entries.append((name, off, size))
        pos = end + 1
    return data, entries


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["list", "extract"])
    ap.add_argument("archive")
    ap.add_argument("outdir", nargs="?")
    ap.add_argument("--filter", default="", help="case-insensitive substring of the path")
    a = ap.parse_args(argv)

    data, entries = read_big(a.archive)
    sel = [e for e in entries if a.filter.lower() in e[0].lower()]
    if a.cmd == "list":
        for name, _, size in sel:
            print(f"{size:>10}  {name}")
        print(f"{len(sel)} of {len(entries)} files", file=sys.stderr)
        return 0
    if not a.outdir:
        ap.error("extract needs OUTDIR")
    root = os.path.realpath(a.outdir)
    for name, off, size in sel:
        dest = os.path.realpath(os.path.join(root, name))
        if not dest.startswith(root + os.sep):  # refuse paths that escape OUTDIR
            print(f"skipping unsafe path {name}", file=sys.stderr)
            continue
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as out:
            out.write(data[off:off + size])
    print(f"extracted {len(sel)} files to {a.outdir}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
