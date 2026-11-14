"""Import ambientCG CC0 textures and build world-aligned tiling master M_HallTiled + MI_Hall_Wall/Floor/Ceiling.
Run: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this> (editor closed or PIE not running)."""
import os, unreal
PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(PROJECT, "SourceArt", "AmbientCG")
DEST = "/Game/MissionCrew/Construction/Env"
TEXDIR = DEST + "/Textures"
mel = unreal.MaterialEditingLibrary
tasks = []
for f in sorted(os.listdir(SRC)):
    if f.endswith(".jpg"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, f).replace("\\", "/")
        t.destination_path = TEXDIR
        t.automated = True; t.replace_existing = True; t.save = False
        tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for f in os.listdir(SRC):
    if f.endswith(".jpg"):
        name = f[:-4]
        tex = unreal.load_asset(TEXDIR + "/" + name)
        if "_Rough" in name:
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
        unreal.log("MC_TEX: " + name)

MP = DEST + "/M_HallTiled"
if unreal.EditorAssetLibrary.does_asset_exist(MP):
    unreal.EditorAssetLibrary.delete_asset(MP)
m = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_HallTiled", DEST, unreal.Material, unreal.MaterialFactoryNew())
def node(cls, x, y):
    return mel.create_material_expression(m, cls, x, y)
tc = node(unreal.MaterialExpressionTextureObjectParameter, -900, -200); tc.set_editor_property("parameter_name", "TexColor")
tr = node(unreal.MaterialExpressionTextureObjectParameter, -900, 100); tr.set_editor_property("parameter_name", "TexRough")
wp = node(unreal.MaterialExpressionWorldPosition, -900, 300)
nm = node(unreal.MaterialExpressionVertexNormalWS, -900, 400)
sz = node(unreal.MaterialExpressionScalarParameter, -900, 500); sz.set_editor_property("parameter_name", "TileSizeCm"); sz.set_editor_property("default_value", 400.0)
def custom(code, y):
    c = node(unreal.MaterialExpressionCustom, -500, y)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    ins = []
    for n in ("Tex", "P", "N", "S"):
        i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
    c.set_editor_property("inputs", ins)
    return c
body = ("float3 w=pow(abs(N),8.0); w=w/(w.x+w.y+w.z); "
        "float4 a=Texture2DSample(Tex,TexSampler,P.yz/S); float4 b=Texture2DSample(Tex,TexSampler,P.xz/S); float4 c=Texture2DSample(Tex,TexSampler,P.xy/S); "
        "return a*w.x+b*w.y+c*w.z;")
cc = custom(body, -100); cr = custom(body, 200)
for c, t in ((cc, tc), (cr, tr)):
    mel.connect_material_expressions(t, "", c, "Tex")
    mel.connect_material_expressions(wp, "", c, "P")
    mel.connect_material_expressions(nm, "", c, "N")
    mel.connect_material_expressions(sz, "", c, "S")
tint = node(unreal.MaterialExpressionVectorParameter, -300, -300); tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(0.2, 0.25, 0.35, 1))
mul = node(unreal.MaterialExpressionMultiply, -100, -150)
mel.connect_material_expressions(cc, "", mul, "A"); mel.connect_material_expressions(tint, "", mul, "B")
mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
rs = node(unreal.MaterialExpressionScalarParameter, -300, 300); rs.set_editor_property("parameter_name", "RoughScale"); rs.set_editor_property("default_value", 1.0)
rm = node(unreal.MaterialExpressionMultiply, -100, 250)
mel.connect_material_expressions(cr, "", rm, "A"); mel.connect_material_expressions(rs, "", rm, "B")
mel.connect_material_property(rm, "", unreal.MaterialProperty.MP_ROUGHNESS)
mt = node(unreal.MaterialExpressionScalarParameter, -300, 400); mt.set_editor_property("parameter_name", "Metal"); mt.set_editor_property("default_value", 0.2)
mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)
mel.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)

specs = {"MI_Hall_Wall": ("T_Hall_Wall", 400.0, (0.16, 0.22, 0.34), 0.2, 1.0),
         "MI_Hall_Floor": ("T_Hall_Floor", 300.0, (0.5, 0.6, 0.8), 0.7, 1.0),
         "MI_Hall_Ceiling": ("T_Hall_Ceiling", 500.0, (0.18, 0.22, 0.3), 0.5, 1.0)}
for name, (tx, size, col, metal, rough) in specs.items():
    p = DEST + "/" + name
    mi = unreal.load_asset(p)
    mi.set_editor_property("parent", m)
    mel.set_material_instance_texture_parameter_value(mi, "TexColor", unreal.load_asset(TEXDIR + "/" + tx + "_Color"))
    mel.set_material_instance_texture_parameter_value(mi, "TexRough", unreal.load_asset(TEXDIR + "/" + tx + "_Rough"))
    mel.set_material_instance_scalar_parameter_value(mi, "TileSizeCm", size)
    mel.set_material_instance_scalar_parameter_value(mi, "Metal", metal)
    mel.set_material_instance_scalar_parameter_value(mi, "RoughScale", rough)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(col[0], col[1], col[2], 1))
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    unreal.log("MC_MAT: " + name)
unreal.log("MC_MAT: DONE")
