#!/usr/bin/env python3
"""Convert Generals / Zero Hour .w3d models to .glb (opens in Blender and Unreal).

Usage:
  w3d2gltf.py model.w3d [-o out.glb] [--textures DIR] [--skeleton-dir DIR] [--no-flip-v] [--scale S]

What it does
  * Reads mesh geometry, normals, UVs and texture names.
  * If the file has an HLOD (multi-part vehicles/buildings), it places each mesh at its bone's
    rest position using the skeleton (HIERARCHY), which may live in a separate *_skl.w3d file
    (searched in --skeleton-dir, default: the model's folder).
  * Highest level of detail only; hidden and collision meshes are skipped.
  * Embeds textures (.tga/.dds -> PNG) if --textures is given and Pillow is installed.
What it does not do (yet): animations, skin weights (skinned meshes come out in bind pose),
team-colour masks, multi-pass shaders. Layout follows GeneralsMD/.../WW3D2/w3d_file.h.
"""
import argparse, base64, glob, io, json, math, os, struct, sys

CONTAINER = 0x80000000
MESH, VERTICES, NORMALS, MESH_HEADER3, TRIANGLES = 0x00, 0x02, 0x03, 0x1F, 0x20
TEXTURES, TEXTURE, TEXTURE_NAME = 0x30, 0x31, 0x32
MATERIAL_PASS, TEXTURE_STAGE, TEXTURE_IDS, STAGE_TEXCOORDS = 0x38, 0x48, 0x49, 0x4A
VERT_INFLUENCES = 0x0E
HIERARCHY, HIER_HEADER, PIVOTS = 0x100, 0x101, 0x102
HLOD, HLOD_HEADER, HLOD_LOD_ARRAY, HLOD_ARRAY_HEADER, HLOD_SUB_OBJECT = 0x700, 0x701, 0x702, 0x703, 0x704

FLAG_COLLISION_MASK, FLAG_HIDDEN, FLAG_GEOMETRY_MASK = 0x0FF0, 0x1000, 0x00FF0000
GEOM_AABOX, GEOM_OBBOX = 0x00040000, 0x00050000


def cstr(b):
    return b.split(b"\0", 1)[0].decode("latin-1")


class Chunk:
    __slots__ = ("type", "data", "children")

    def __init__(self, type_, data, children):
        self.type, self.data, self.children = type_, data, children

    def find(self, t):
        return [c for c in self.children if c.type == t]

    def first(self, t):
        for c in self.children:
            if c.type == t:
                return c
        return None


def parse_chunks(buf, start=0, end=None):
    end = len(buf) if end is None else end
    out, pos = [], start
    while pos + 8 <= end:
        t, size = struct.unpack_from("<II", buf, pos)
        has_kids = bool(size & CONTAINER)
        size &= 0x7FFFFFFF
        body = pos + 8
        if body + size > end:
            raise ValueError(f"chunk 0x{t:X} at {pos} overruns file")
        if has_kids:
            out.append(Chunk(t, b"", parse_chunks(buf, body, body + size)))
        else:
            out.append(Chunk(t, buf[body:body + size], []))
        pos = body + size
    return out


# ------------------------------------------------------------------ matrices (row-major 4x4, column vectors)
def ident():
    return [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]


def mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def pivot_matrix(t, q):
    x, y, z, w = q  # same expansion as Build_Matrix3D in WWMath/quat.h
    r = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (z * x + y * w)],
         [2 * (x * y + z * w), 1 - 2 * (z * z + x * x), 2 * (y * z - x * w)],
         [2 * (z * x - y * w), 2 * (y * z + x * w), 1 - 2 * (y * y + x * x)]]
    return [r[0] + [t[0]], r[1] + [t[1]], r[2] + [t[2]], [0, 0, 0, 1]]


def apply(m, p):
    return tuple(m[i][0] * p[0] + m[i][1] * p[1] + m[i][2] * p[2] + m[i][3] for i in range(3))


def apply_dir(m, n):
    return tuple(m[i][0] * n[0] + m[i][1] * n[1] + m[i][2] * n[2] for i in range(3))


# ------------------------------------------------------------------ W3D content
def read_hierarchy(chunk):
    hdr = chunk.first(HIER_HEADER)
    name = cstr(hdr.data[4:20])
    world, pivots = [], []
    for i in range(len(chunk.first(PIVOTS).data) // 60):
        rec = chunk.first(PIVOTS).data[i * 60:(i + 1) * 60]
        pname = cstr(rec[:16])
        parent = struct.unpack_from("<I", rec, 16)[0]
        trans = struct.unpack_from("<3f", rec, 20)
        quat = struct.unpack_from("<4f", rec, 44)
        local = pivot_matrix(trans, quat)
        m = local if parent == 0xFFFFFFFF or parent >= len(world) else mul(world[parent], local)
        world.append(m)
        pivots.append(pname)
    return name, pivots, world


def read_mesh(chunk):
    h = chunk.first(MESH_HEADER3).data
    attrs = struct.unpack_from("<I", h, 4)[0]
    name, container = cstr(h[8:24]), cstr(h[24:40])
    ntris, nverts = struct.unpack_from("<II", h, 40)
    v = chunk.first(VERTICES).data
    verts = [struct.unpack_from("<3f", v, i * 12) for i in range(nverts)]
    n = chunk.first(NORMALS)
    norms = [struct.unpack_from("<3f", n.data, i * 12) for i in range(nverts)] if n else None
    t = chunk.first(TRIANGLES).data
    tris = [struct.unpack_from("<3I", t, i * 32) for i in range(ntris)]  # W3dTriStruct is 32 bytes

    tex_names = []
    tex_root = chunk.first(TEXTURES)
    if tex_root:
        for tc in tex_root.find(TEXTURE):
            nm = tc.first(TEXTURE_NAME)
            tex_names.append(cstr(nm.data) if nm else "")

    uvs, tex_ids = None, None
    for mp in chunk.find(MATERIAL_PASS):
        for st in mp.find(TEXTURE_STAGE):
            uv = st.first(STAGE_TEXCOORDS)
            ids = st.first(TEXTURE_IDS)
            if uv and uvs is None:
                uvs = [struct.unpack_from("<2f", uv.data, i * 8) for i in range(len(uv.data) // 8)]
            if ids and tex_ids is None:
                k = len(ids.data) // 4
                tex_ids = list(struct.unpack_from(f"<{k}I", ids.data))
        if uvs is not None:
            break
    influences = chunk.first(VERT_INFLUENCES) is not None
    return dict(name=name, container=container, attrs=attrs, verts=verts, norms=norms, tris=tris,
                uvs=uvs, tex_names=tex_names, tex_ids=tex_ids, skinned=influences)


def read_hlod(chunk):
    hdr = chunk.first(HLOD_HEADER).data
    name, hier = cstr(hdr[8:24]), cstr(hdr[24:40])
    arrays = chunk.find(HLOD_LOD_ARRAY)
    subs = []
    if arrays:  # first array = highest detail (see W3dHLodArrayHeaderStruct comment)
        for so in arrays[0].find(HLOD_SUB_OBJECT):
            bone = struct.unpack_from("<I", so.data, 0)[0]
            subs.append((bone, cstr(so.data[4:36])))
    return name, hier, subs


def find_hierarchy(name, search_dir, cache):
    if not name:
        return None
    for path in glob.glob(os.path.join(search_dir, "**", "*.w3d"), recursive=True):
        if path not in cache:
            try:
                with open(path, "rb") as f:
                    cache[path] = [c for c in parse_chunks(f.read()) if c.type == HIERARCHY]
            except (ValueError, struct.error, OSError):
                cache[path] = []
        for hc in cache[path]:
            h = read_hierarchy(hc)
            if h[0].lower() == name.lower():
                return h
    return None


# ------------------------------------------------------------------ glTF writing
def to_gltf_space(p, scale):  # W3D is Z-up; glTF is Y-up (proper rotation, keeps winding)
    return (p[0] * scale, p[2] * scale, -p[1] * scale)


class Glb:
    def __init__(self):
        self.bin = bytearray()
        self.views, self.accessors, self.meshes, self.nodes = [], [], [], []
        self.materials, self.textures, self.images, self.mat_index = [], [], [], {}

    def _view(self, data, target=None):
        while len(self.bin) % 4:
            self.bin.append(0)
        v = {"buffer": 0, "byteOffset": len(self.bin), "byteLength": len(data)}
        if target:
            v["target"] = target
        self.bin += data
        self.views.append(v)
        return len(self.views) - 1

    def accessor(self, data, comp, ctype, count, target=None, minmax=None):
        a = {"bufferView": self._view(data, target), "componentType": comp, "count": count, "type": ctype}
        if minmax:
            a["min"], a["max"] = minmax
        self.accessors.append(a)
        return len(self.accessors) - 1

    def material(self, tex_name, png):
        key = tex_name.lower()
        if key in self.mat_index:
            return self.mat_index[key]
        m = {"name": tex_name or "default", "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 0.9}}
        if png:
            self.images.append({"bufferView": self._view(png), "mimeType": "image/png", "name": tex_name})
            self.textures.append({"source": len(self.images) - 1})
            m["pbrMetallicRoughness"]["baseColorTexture"] = {"index": len(self.textures) - 1}
        self.materials.append(m)
        self.mat_index[key] = len(self.materials) - 1
        return self.mat_index[key]

    def write(self, path, name):
        doc = {"asset": {"version": "2.0", "generator": "w3d2gltf"},
               "scene": 0, "scenes": [{"name": name, "nodes": list(range(len(self.nodes)))}],
               "nodes": self.nodes, "meshes": self.meshes, "accessors": self.accessors,
               "bufferViews": self.views, "buffers": [{"byteLength": len(self.bin)}]}
        for k, v in (("materials", self.materials), ("textures", self.textures), ("images", self.images)):
            if v:
                doc[k] = v
        js = json.dumps(doc, separators=(",", ":")).encode()
        js += b" " * (-len(js) % 4)
        binb = bytes(self.bin) + b"\0" * (-len(self.bin) % 4)
        total = 12 + 8 + len(js) + 8 + len(binb)
        with open(path, "wb") as f:
            f.write(struct.pack("<4sII", b"glTF", 2, total))
            f.write(struct.pack("<I4s", len(js), b"JSON") + js)
            f.write(struct.pack("<I4s", len(binb), b"BIN\0") + binb)


def load_texture_png(name, tex_dir, cache):
    if not name or not tex_dir:
        return None
    key = name.lower()
    if key in cache:
        return cache[key]
    png = None
    stem = os.path.splitext(name)[0]
    for ext in (".tga", ".dds", ".png"):
        for cand in glob.glob(os.path.join(tex_dir, "**", stem + ext), recursive=True) + \
                glob.glob(os.path.join(tex_dir, "**", stem.upper() + ext.upper()), recursive=True):
            try:
                from PIL import Image
                buf = io.BytesIO()
                Image.open(cand).convert("RGBA").save(buf, "PNG")
                png = buf.getvalue()
            except Exception as e:  # missing Pillow or unsupported DDS variant
                print(f"  texture {name}: {e}", file=sys.stderr)
            break
        if png:
            break
    cache[key] = png
    return png


def add_mesh(glb, mesh, xform, args, tex_cache):
    scale = args.scale
    pos = [to_gltf_space(apply(xform, p), scale) for p in mesh["verts"]]
    nrm = None
    if mesh["norms"]:
        nrm = []
        for n in mesh["norms"]:
            d = apply_dir(xform, n)
            L = math.sqrt(sum(c * c for c in d)) or 1.0
            nrm.append(to_gltf_space(tuple(c / L for c in d), 1.0))
    uvs = mesh["uvs"]
    groups = {}  # texture id -> triangle list
    for i, tri in enumerate(mesh["tris"]):
        ids = mesh["tex_ids"]
        tid = 0 if not ids else (ids[0] if len(ids) == 1 else ids[i] if i < len(ids) else 0)
        groups.setdefault(tid, []).append(tri)

    pos_b = b"".join(struct.pack("<3f", *p) for p in pos)
    lo = [min(p[i] for p in pos) for i in range(3)]
    hi = [max(p[i] for p in pos) for i in range(3)]
    attrs = {"POSITION": glb.accessor(pos_b, 5126, "VEC3", len(pos), 34962, ([*lo], [*hi]))}
    if nrm:
        attrs["NORMAL"] = glb.accessor(b"".join(struct.pack("<3f", *n) for n in nrm), 5126, "VEC3", len(nrm), 34962)
    if uvs and len(uvs) == len(pos):
        vfix = (lambda v: v) if args.no_flip_v else (lambda v: 1.0 - v)
        attrs["TEXCOORD_0"] = glb.accessor(b"".join(struct.pack("<2f", u, vfix(v)) for u, v in uvs),
                                           5126, "VEC2", len(uvs), 34962)
    prims = []
    for tid, tris in sorted(groups.items()):
        idx = [i for t in tris for i in t]
        tname = mesh["tex_names"][tid] if tid < len(mesh["tex_names"]) else ""
        png = load_texture_png(tname, args.textures, tex_cache)
        prims.append({"attributes": attrs, "mode": 4,
                      "indices": glb.accessor(struct.pack(f"<{len(idx)}I", *idx), 5125, "SCALAR", len(idx), 34963),
                      "material": glb.material(tname, png)})
    glb.meshes.append({"name": f'{mesh["container"]}.{mesh["name"]}', "primitives": prims})
    glb.nodes.append({"name": mesh["name"], "mesh": len(glb.meshes) - 1})


def convert(path, out, args):
    with open(path, "rb") as f:
        top = parse_chunks(f.read())
    meshes = [read_mesh(c) for c in top if c.type == MESH and c.first(MESH_HEADER3)]
    hlods = [read_hlod(c) for c in top if c.type == HLOD]
    hier = next((read_hierarchy(c) for c in top if c.type == HIERARCHY), None)

    def visible(m):
        a = m["attrs"]
        return not (a & FLAG_HIDDEN or a & FLAG_COLLISION_MASK or (a & FLAG_GEOMETRY_MASK) in (GEOM_AABOX, GEOM_OBBOX))

    by_full = {f'{m["container"]}.{m["name"]}'.lower(): m for m in meshes}
    by_name = {m["name"].lower(): m for m in meshes}
    glb, tex_cache, placements = Glb(), {}, []

    if hlods:
        _, hname, subs = hlods[0]
        if hname and (not hier or hier[0].lower() != hname.lower()):
            sdir = args.skeleton_dir or os.path.dirname(os.path.abspath(path))
            hier = find_hierarchy(hname, sdir, {}) or hier
            if hier is None:
                print(f"warning: skeleton '{hname}' not found; parts will be stacked at the origin "
                      f"(use --skeleton-dir)", file=sys.stderr)
        for bone, sub in subs:
            m = by_full.get(sub.lower()) or by_name.get(sub.split(".")[-1].lower())
            if m and visible(m):
                placements.append((m, hier[2][bone] if hier and bone < len(hier[2]) else ident()))
    else:
        placements = [(m, ident()) for m in meshes if visible(m)]

    for m, xf in placements:
        # skinned vertices are already stored in model space; rigid parts are in bone space
        add_mesh(glb, m, ident() if m["skinned"] else xf, args, tex_cache)
    if not glb.nodes:
        raise SystemExit("no visible meshes found")
    glb.write(out, os.path.splitext(os.path.basename(path))[0])
    tris = sum(len(m["tris"]) for m, _ in placements)
    print(f"{out}: {len(placements)} meshes, {tris} triangles", file=sys.stderr)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input")
    ap.add_argument("-o", "--output")
    ap.add_argument("--textures", help="folder of extracted textures (.tga/.dds)")
    ap.add_argument("--skeleton-dir", help="folder containing *_skl.w3d files")
    ap.add_argument("--no-flip-v", action="store_true", help="do not flip the V texture coordinate")
    ap.add_argument("--scale", type=float, default=1.0)
    a = ap.parse_args(argv)
    convert(a.input, a.output or os.path.splitext(a.input)[0] + ".glb", a)


if __name__ == "__main__":
    main()
