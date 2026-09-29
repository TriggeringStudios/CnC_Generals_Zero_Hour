#!/usr/bin/env python3
"""Self-test using synthetic files laid out per w3d_file.h (no game data needed)."""
import json, os, struct, sys, tempfile
import bigtool, w3d2gltf as w


def chunk(t, payload=b"", kids=()):
    if kids:
        payload = b"".join(kids)
        return struct.pack("<II", t, len(payload) | 0x80000000) + payload
    return struct.pack("<II", t, len(payload)) + payload


def name(s, n=16):
    return s.encode().ljust(n, b"\0")


def mesh(container, mname, verts, tris, attrs=0, tex="hull.tga"):
    hdr = struct.pack("<II", 0x40000, attrs) + name(mname) + name(container) + struct.pack("<II", len(tris), len(verts)) + b"\0" * 60
    v = b"".join(struct.pack("<3f", *p) for p in verts)
    n = b"".join(struct.pack("<3f", 0, 0, 1) for _ in verts)
    t = b"".join(struct.pack("<3I", *tr) + struct.pack("<I3ff", 0, 0, 0, 1, 0) for tr in tris)
    uv = b"".join(struct.pack("<2f", p[0], p[1]) for p in verts)
    tex_chunk = chunk(0x30, kids=[chunk(0x31, kids=[chunk(0x32, tex.encode() + b"\0")])])
    pas = chunk(0x38, kids=[chunk(0x48, kids=[chunk(0x49, struct.pack("<I", 0)), chunk(0x4A, uv)])])
    return chunk(0, kids=[chunk(0x1F, hdr), chunk(2, v), chunk(3, n), chunk(0x20, t), tex_chunk, pas])


def pivot(nm, parent, tr, q=(0, 0, 0, 1)):
    return name(nm) + struct.pack("<I", parent) + struct.pack("<3f", *tr) + struct.pack("<3f", 0, 0, 0) + struct.pack("<4f", *q)


def skeleton():
    hdr = struct.pack("<I", 0x40000) + name("TANK_SKL") + struct.pack("<I3f", 2, 0, 0, 0)
    piv = pivot("ROOT", 0xFFFFFFFF, (0, 0, 0)) + pivot("TURRET", 0, (0, 0, 10))
    return chunk(0x100, kids=[chunk(0x101, hdr), chunk(0x102, piv)])


def hlod():
    hdr = struct.pack("<II", 0x10000, 1) + name("TANK") + name("TANK_SKL")
    arr_hdr = struct.pack("<If", 2, 0.0)
    so = lambda bone, nm: chunk(0x704, struct.pack("<I", bone) + name(nm, 32))
    return chunk(0x700, kids=[chunk(0x701, hdr), chunk(0x702, kids=[chunk(0x703, arr_hdr), so(0, "TANK.HULL"), so(1, "TANK.TURRET"), so(0, "TANK.COLBOX")])])


def main():
    tri = [(0, 1, 2)]
    quad = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    with tempfile.TemporaryDirectory() as d:
        model = mesh("TANK", "HULL", quad, tri) + mesh("TANK", "TURRET", quad, tri) + \
            mesh("TANK", "COLBOX", quad, tri, attrs=0x1000 | 0x10) + hlod()
        open(os.path.join(d, "tank.w3d"), "wb").write(model)
        open(os.path.join(d, "tank_skl.w3d"), "wb").write(skeleton())

        out = os.path.join(d, "tank.glb")
        w.main([os.path.join(d, "tank.w3d"), "-o", out])
        raw = open(out, "rb").read()
        magic, ver, total = struct.unpack_from("<4sII", raw)
        assert magic == b"glTF" and ver == 2 and total == len(raw), "bad GLB header"
        jlen = struct.unpack_from("<I", raw, 12)[0]
        doc = json.loads(raw[20:20 + jlen])
        assert len(doc["meshes"]) == 2, f"collision/hidden mesh must be skipped: {len(doc['meshes'])}"
        blen_off = 20 + jlen
        blen = struct.unpack_from("<I", raw, blen_off)[0]
        assert doc["buffers"][0]["byteLength"] <= blen
        for a in doc["accessors"]:  # every accessor must fit inside its buffer view
            v = doc["bufferViews"][a["bufferView"]]
            need = a["count"] * {"VEC3": 12, "VEC2": 8, "SCALAR": 4}[a["type"]]
            assert need <= v["byteLength"], "accessor overruns view"
        # turret sits 10 units up W3D-Z, which is +Y in glTF
        turret_pos = doc["accessors"][[m for m in doc["meshes"] if m["name"].endswith("TURRET")][0]["primitives"][0]["attributes"]["POSITION"]]
        assert abs(turret_pos["min"][1] - 10.0) < 1e-4, turret_pos
        print("w3d2gltf ok:", [m["name"] for m in doc["meshes"]])

        # BIG round trip
        files = {"Art/W3D/tank.w3d": model, "Art/Textures/hull.tga": b"abc"}
        head, body, off = b"", b"", 0
        table = b"".join(struct.pack(">II", 0, len(v)) + k.encode() + b"\0" for k, v in files.items())
        base = 16 + len(table)
        pos = base
        entries = b""
        for k, v in files.items():
            entries += struct.pack(">II", pos, len(v)) + k.replace("/", "\\").encode() + b"\0"
            body += v
            pos += len(v)
        big = b"BIGF" + struct.pack("<I", base + len(body)) + struct.pack(">II", len(files), base) + entries + body
        bp = os.path.join(d, "t.big")
        open(bp, "wb").write(big)
        bigtool.main(["extract", bp, os.path.join(d, "x")])
        assert open(os.path.join(d, "x", "Art", "W3D", "tank.w3d"), "rb").read() == model
        assert open(os.path.join(d, "x", "Art", "Textures", "hull.tga"), "rb").read() == b"abc"
        print("bigtool ok")


if __name__ == "__main__":
    main()
