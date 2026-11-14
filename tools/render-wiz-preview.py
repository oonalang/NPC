import bpy,sys
from pathlib import Path
args=sys.argv[sys.argv.index("--")+1:]
if len(args)!=2:
    raise SystemExit("blender -b -t 2 -P tools/render-wiz-preview.py -- SOURCE_FOLDER OUTPUT_PNG")
source=Path(args[0]);output=Path(args[1])
from mathutils import Vector
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(source/'Wiz.fbx'))
for mat in bpy.data.materials:
 mat.use_nodes=True
 nodes=mat.node_tree.nodes;nodes.clear()
 out=nodes.new('ShaderNodeOutputMaterial');em=nodes.new('ShaderNodeEmission');tex=nodes.new('ShaderNodeTexImage')
 stem=mat.name[:4]
 tex.image=bpy.data.images.load(str(source/(stem+'_Base_color.png')),check_existing=True)
 mat.node_tree.links.new(tex.outputs['Color'],em.inputs['Color']);mat.node_tree.links.new(em.outputs[0],out.inputs['Surface'])
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=8;scene.cycles.use_denoising=False
scene.render.resolution_x=1024;scene.render.resolution_y=1536;scene.render.resolution_percentage=100
scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA'
scene.view_settings.view_transform='Standard';scene.view_settings.look='None';scene.view_settings.exposure=0;scene.view_settings.gamma=1
cam=bpy.data.cameras.new('Camera');obj=bpy.data.objects.new('Camera',cam);scene.collection.objects.link(obj)
obj.location=(0,-7,1.09);target=Vector((0,.05,1.09));obj.rotation_euler=(target-obj.location).to_track_quat('-Z','Y').to_euler()
cam.type='ORTHO';cam.ortho_scale=2.38;scene.camera=obj
scene.render.filepath=str(output)
bpy.ops.render.render(write_still=True)
