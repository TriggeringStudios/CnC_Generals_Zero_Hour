# Getting Generals models into Blender / Unreal

Two small Python scripts. They read **your own copy of the game** and write normal `.glb` files
that Blender (File → Import → glTF 2.0) and Unreal 5 (drag into the Content Browser) both open.
Keep the extracted files on your machine; they are EA's art and are not part of this repository.

## One-time setup
1. Install Python 3 from python.org (tick "Add Python to PATH"). 
2. For textures only: open a terminal and run `pip install pillow`.

## Steps (from a terminal, in this `tools/w3d` folder)
1. Find your game folder (the one containing files ending in `.big`).
2. See what is inside: `python bigtool.py list "C:\path\to\W3D.big" --filter tank`
3. Unpack the models and textures. Do the same for each archive:
   ```
   python bigtool.py extract "C:\path\to\W3D.big"      extracted
   python bigtool.py extract "C:\path\to\Textures.big" extracted
   ```
   (Archive names differ between the original game and Zero Hour; use `list` to look around.
   Zero Hour's `...ZH.big` files hold the expansion units.)
4. Convert a model:
   ```
   python w3d2gltf.py extracted/art/w3d/avcrusader.w3d -o crusader.glb --textures extracted/art/textures
   ```
   Multi-part vehicles use a separate skeleton file (`*_skl.w3d`). The script looks for it next to the
   model; if it says it can't find one, add `--skeleton-dir extracted/art/w3d`.
5. Open `crusader.glb` in Blender and use it as a reference for your own model.

## Check the install works
`python test_tools.py` runs a self-test on made-up files (no game needed).

## Limits (honest list)
- Written from the format definitions in `w3d_file.h`, and tested only on synthetic files: no real game
  files were available when this was written. Expect to fix small things on first contact with real data.
- Not yet converted: animations, skin weights (skinned meshes come out in their bind pose), team-colour
  masks, multi-pass shaders. Only the highest level of detail is exported.
- If textures look upside-down, re-run with `--no-flip-v`.
- Some `.dds` texture variants may not load in Pillow; the model still converts, just untextured.
