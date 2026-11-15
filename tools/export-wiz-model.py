import bpy,sys,math,struct,zlib,json,tempfile,subprocess
from pathlib import Path
from mathutils import Vector,Quaternion
import numpy as np
args=sys.argv[sys.argv.index('--')+1:]
if len(args)!=2:
    raise SystemExit('blender -b -t 2 -P tools/export-wiz-model.py -- SOURCE_FOLDER OUTPUT_MODEL')
source=Path(args[0]);output=Path(args[1])
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(source/'Wiz.fbx'))
mesh=next(o for o in bpy.data.objects if o.type=='MESH')
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
scene=bpy.context.scene;scene.render.fps=24
names=[b.name for b in rig.data.bones];bone_ids={n:i for i,n in enumerate(names)}
rest={b.name:b.matrix_local.copy() for b in rig.data.bones}
world=rig.matrix_world.copy();world_inv=world.inverted()
mesh_world=mesh.matrix_world.copy();normal_world=mesh_world.to_3x3().inverted().transposed()
for p in rig.pose.bones:p.rotation_mode='QUATERNION'
for side in ['L','R']:
    foot=rig.pose.bones['foot.'+side]
    target=bpy.data.objects.new('Ankle.'+side,None);scene.collection.objects.link(target)
    target.location=world@foot.head;target.rotation_mode='QUATERNION';target.rotation_quaternion=(world@foot.matrix).to_quaternion()
    pole=bpy.data.objects.new('Knee.'+side,None);scene.collection.objects.link(pole)
    pole.location=(target.location.x,-.65,.6)
    ik=rig.pose.bones['shin.'+side].constraints.new('IK');ik.target=target;ik.pole_target=pole;ik.chain_count=2
    ik.use_stretch=False
    follow=foot.constraints.new('COPY_ROTATION');follow.target=target;follow.owner_space='WORLD';follow.target_space='WORLD'
    target['base']=target.location[:]
mesh.data.calc_loop_triangles()
shapes=mesh.data.shape_keys.key_blocks
vertices=[];indices=[];ranges=[];lookup={}
max_lost=0.
for material in range(len(mesh.data.materials)):
    start=len(indices)
    for triangle in mesh.data.loop_triangles:
        if triangle.material_index!=material:continue
        for li in triangle.loops:
            loop=mesh.data.loops[li];vid=loop.vertex_index
            uv=mesh.data.uv_layers.active.data[li].uv;normal=normal_world@mesh.data.corner_normals[li].vector
            key=(vid,round(uv.x,7),round(uv.y,7),*(round(v,5) for v in normal),material)
            if key not in lookup:
                pos=mesh_world@mesh.data.vertices[vid].co
                influence=sorted([(bone_ids[mesh.vertex_groups[g.group].name],g.weight) for g in mesh.data.vertices[vid].groups if mesh.vertex_groups[g.group].name in bone_ids],key=lambda x:-x[1])
                max_lost=max(max_lost,sum(w for _,w in influence[4:]))
                influence=influence[:4] or [(0,1.)];total=sum(w for _,w in influence)
                joints=[j for j,_ in influence]+[0]*(4-len(influence));weights=[int(round(w/total*65535)) for _,w in influence]+[0]*(4-len(influence));weights[0]+=65535-sum(weights)
                morph=[]
                for shape in ['Blink','Smile','Speak']:
                    delta=mesh_world.to_3x3()@(shapes[shape].data[vid].co-mesh.data.vertices[vid].co);morph.extend(delta)
                lookup[key]=len(vertices);vertices.append((*pos,*normal,uv.x,1.-uv.y,*joints,*weights,*morph))
            indices.append(lookup[key])
    ranges.append((start,len(indices)-start))
positions=np.array([v[:3] for v in vertices],dtype=np.float64)
joints=np.array([v[8:12] for v in vertices],dtype=np.int32)
weights=np.array([v[12:16] for v in vertices],dtype=np.float64)/65535.
morph=np.array([v[16:] for v in vertices],dtype=np.float64).reshape((-1,3,3))
def smooth(v):
    v=max(0.,min(1.,v));return v*v*(3.-2.*v)
def gate(t,a,b,edge=.4):return smooth((t-a)/edge)*(1.-smooth((t-(b-edge))/edge))
def rotate(name,angles):
    q=Quaternion((1.,0.,0.,0.))
    for axis,angle in zip([(1,0,0),(0,1,0),(0,0,1)],angles):q=Quaternion(axis,angle)@q
    basis=rest[name].to_quaternion();rig.pose.bones[name].rotation_quaternion=basis.inverted()@q@basis
frames=[];foot_drift=0.
for frame in range(289):
    t=frame/24.;scene.frame_set(frame)
    for p in rig.pose.bones:p.location=(0,0,0);p.rotation_quaternion=(1,0,0,0);p.scale=(1,1,1)
    breathe=math.sin(t*2*math.pi/3.)
    wave=gate(t,3.1,5.85,.42)
    arm_wave=gate(t,3.45,5.5,.42)
    anticipation=gate(t,7.38,7.82,.18)
    f=(t-7.8)/.68;lift=.16*4*f*(1-f) if 0<=f<=1 else 0.
    landing=math.sin((t-8.48)/.30*math.pi)*math.exp(-(t-8.48)*7) if 8.48<t<8.78 else 0.
    jump_raise=math.sin(math.pi*f) if 0<=f<=1 else 0.
    hip=Vector((.014*math.sin(t*2*math.pi/6.),0,lift-.014-.035*anticipation-.018*landing+.0015*breathe))
    rig.pose.bones['spine'].location=rest['spine'].to_quaternion().inverted()@hip
    rotate('spine',(0,.015*math.sin(t*2*math.pi/6.),.07*math.sin(t*2*math.pi/12.)))
    rotate('spine.001',(.006*breathe,.012*math.sin(t*2*math.pi/6.+.5),-.022*math.sin(t*2*math.pi/6.)))
    rotate('spine.002',(.007*breathe,0,0))
    rotate('spine.003',(.006*breathe,.014*math.sin(t*2*math.pi/6.+.8),.015*math.sin(t*2*math.pi/12.)))
    rotate('spine.004',(.025*math.sin(t*2*math.pi/6.+.5),.025*math.sin(t*2*math.pi/6.),.13*math.sin(t*2*math.pi/12.+.5)))
    for side,sign in [('L',1),('R',-1)]:
        rotate('shoulder.'+side,(.014*breathe,0,sign*.01*breathe))
        amount=wave if side=='L' else 0.
        arm_amount=arm_wave if side=='L' else 0.
        rotate('upper_arm.'+side,(.05*amount,sign*(.22-1.37*arm_amount-.20*jump_raise),sign*.018*math.sin(t*2*math.pi/6.+1.)))
        rotate('forearm.'+side,(.08*amount,-sign*(.14+1.21*amount+.15*jump_raise),0))
        rotate('hand.'+side,(.08*amount,0,.32*math.sin((t-3.1)*2*math.pi*2.5)*amount))
        rotate('Eye.'+side,(.035*math.sin(t*2*math.pi/6.),0,.055*math.sin(t*2*math.pi/12.+.8)))
        rotate('breast.'+side,(.008*math.sin(t*2*math.pi/3.-.4)+.022*landing,0,0))
        target=bpy.data.objects['Ankle.'+side];target.location=Vector(target['base'])+Vector((0,0,lift))
        for digit in ['f_index.01.','f_middle.01.','f_ring.01.','f_pinky.01.']:
            rotate(digit+side,(.05+.04*math.sin(t*2*math.pi/6.+sign),0,0))
    for index,name in enumerate(names):
        if name.startswith('Hair') and not name.endswith('_end'):
            delay=index*.12
            rotate(name,(.025*math.sin(t*2*math.pi/3.-delay)+.055*landing,.018*math.sin(t*2*math.pi/6.-delay),.022*math.sin(t*2*math.pi/6.-delay)))
    blink=0.
    for at in [1.55,4.05,4.34,7.25,10.6]:
        elapsed=t-at
        if 0<=elapsed<.17:blink=max(blink,math.sin(math.pi*elapsed/.17)**2)
    face_weights=(blink,.18+.45*wave+.5*jump_raise,.065*wave*(.5+.5*math.sin(t*2*math.pi*4.)))
    bpy.context.view_layer.update()
    palette=np.array([np.array(world@rig.pose.bones[n].matrix@rest[n].inverted()@world_inv) for n in names])
    modified=positions+np.einsum('nsc,s->nc',morph,np.array(face_weights))
    homogeneous=np.concatenate([modified,np.ones((len(modified),1))],axis=1)
    skinned=np.einsum('nkij,nj,nk->ni',palette[joints],homogeneous,weights)[:,:3]
    low=skinned.min(axis=0);high=skinned.max(axis=0)
    def point(name,tail=False):
        bone=rig.pose.bones[name];return world@(bone.tail if tail else bone.head)
    landmarks=[(point('Eye.L')+point('Eye.R'))*.5,point('spine.004'),(point('spine.003')+point('spine.004'))*.5,point('spine')]
    for part in ['upper_arm','forearm','hand','shin','foot']:
        landmarks.extend([point(part+'.L',part=='hand'),point(part+'.R',part=='hand')])
    landmarks.append((point('toe.L',True)+point('toe.R',True))*.5)
    packed=palette.transpose((0,2,1)).reshape(-1).tolist()+list(face_weights)+[v for p in landmarks for v in p]+low.tolist()+high.tolist()+[lift,float(wave>.2 or jump_raise>0)]
    frames.append(packed)
    for side in ['L','R']:
        expected=Vector(bpy.data.objects['Ankle.'+side]['base'])+Vector((0,0,lift))
        foot_drift=max(foot_drift,(point('foot.'+side)-expected).length)
raw=bytearray(struct.pack('<7If',1,len(vertices),len(indices),len(names),len(frames),24,len(ranges),12.))
for vertex in vertices:raw.extend(struct.pack('<8f4B4H9f',*vertex))
raw.extend(np.array(indices,dtype='<u4').tobytes())
with tempfile.TemporaryDirectory() as directory:
    script='from PIL import Image;import sys;im=Image.open(sys.argv[1]).convert("RGBA");im.thumbnail((int(sys.argv[3]),int(sys.argv[3])),Image.Resampling.LANCZOS);im.save(sys.argv[2])'
    for i,(first,count) in enumerate(ranges):
        stem=mesh.data.materials[i].name[:4];png=Path(directory)/(stem+'.png')
        subprocess.run(['/usr/bin/python3','-c',script,str(source/(stem+'_Base_color.png')),str(png),'512' if stem=='WizF' else '1024'],check=True)
        data=png.read_bytes();raw.extend(struct.pack('<3I',first,count,len(data)));raw.extend(data)
raw.extend(np.array(frames,dtype='<f4').tobytes())
output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(b'WIZSKN01'+struct.pack('<I',len(raw))+zlib.compress(raw,9))
report={'vertices':len(vertices),'triangles':len(indices)//3,'bones':len(names),'frames':len(frames),'fps':24,'duration':12,'frameFloats':len(frames[0]),'landmarks':len(landmarks),'maximumDiscardedWeight':max_lost,'maximumFootDrift':foot_drift,'packedBytes':output.stat().st_size,'rawBytes':len(raw)}
output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print('WIZ_EXPORT',json.dumps(report))
