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
parents=list(range(len(mesh.data.vertices)))
def component_root(index):
    while parents[index]!=index:
        parents[index]=parents[parents[index]];index=parents[index]
    return index
for polygon in mesh.data.polygons:
    for index in polygon.vertices:parents[component_root(index)]=component_root(polygon.vertices[0])
components={}
for vertex in mesh.data.vertices:components.setdefault(component_root(vertex.index),[]).append(vertex.index)
coat=set();bangs=set();coat_components=0
for ids in components.values():
    lo=[min(mesh.data.vertices[i].co[a] for i in ids) for a in range(3)]
    hi=[max(mesh.data.vertices[i].co[a] for i in ids) for a in range(3)]
    if lo[2]<.5 and hi[2]>1.65 and hi[0]-lo[0]>.55:
        coat.update(ids);coat_components+=1
    if lo[2]>1.8 and hi[2]>2.0 and lo[1]<-.05 and hi[1]<.06:
        bangs.update(ids)
if coat_components!=2 or len(bangs)<100:raise RuntimeError('Source garment/bang topology does not match the audited Wiz mesh')
definitions=[]
for xsign,side in [(1,'L'),(-1,'R')]:
    definitions.append(('Bang.'+side,'face',(xsign*.035,-.01,2.045),(xsign*.042,-.075,1.865)))
    fore=rig.data.bones['forearm.'+side]
    a=fore.head_local.copy();b=fore.tail_local.copy();offset=Vector((0,.075,0))
    definitions.append(('Sleeve.'+side,'forearm.'+side,a+offset,a.lerp(b,.65)+offset))
    definitions.append(('SleeveHem.'+side,'Sleeve.'+side,a.lerp(b,.65)+offset,b+offset))
for quadrant in range(8):
    angle=math.pi/8+quadrant*math.pi/4;axis=Vector((math.cos(angle),math.sin(angle),0))
    root=Vector((0,.045,1.10))+axis*.18;middle=Vector((0,.045,.72))+axis*.24;tip=Vector((0,.045,.30))+axis*.30
    name='Skirt.'+str(quadrant)
    definitions.append((name,'spine',root,middle));definitions.append((name+'.Hem',name,middle,tip))
bpy.context.view_layer.objects.active=rig;rig.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
for name,parent,head,tail in definitions:
    bone=rig.data.edit_bones.new(name);bone.parent=rig.data.edit_bones[parent];bone.head=head;bone.tail=tail
bpy.ops.object.mode_set(mode='OBJECT')
for name,_,_,_ in definitions:mesh.vertex_groups.new(name=name)
neighbors=[set() for _ in mesh.data.vertices]
for edge in mesh.data.edges:
    a,b=edge.vertices;neighbors[a].add(b);neighbors[b].add(a)
directions={i:Vector((mesh.data.vertices[i].co.x,mesh.data.vertices[i].co.y-.045)) for i in coat}
inside=[i for i,direction in directions.items() if direction.length<.14 and mesh.data.vertices[i].co.z<1.12]
for i,direction in directions.items():directions[i]=direction.normalized()
interior_ids={index:i for i,index in enumerate(inside)}
laplacian=np.zeros((len(inside),len(inside)));boundary=np.zeros((len(inside),2))
for index,row in interior_ids.items():
    for other in neighbors[index]:
        if other not in directions:continue
        weight=1/max(1e-8,(mesh.data.vertices[index].co-mesh.data.vertices[other].co).length_squared)
        laplacian[row,row]+=weight
        if other in interior_ids:laplacian[row,interior_ids[other]]-=weight
        else:boundary[row]+=np.array(directions[other])*weight
for index,value in zip(inside,np.linalg.solve(laplacian,boundary)):directions[index]=Vector(value)
clothing_vertices=0
for vertex in mesh.data.vertices:
    point=vertex.co;added={};fraction=0.;base=None
    arm_weight=sum(g.weight for g in vertex.groups if mesh.vertex_groups[g.group].name.startswith(('forearm.','hand.','upper_arm.')))
    if vertex.index in bangs:
        fraction=max(0.,min(1.,(2.045-point.z)/.115))
        added['Bang.L' if point.x>=0 else 'Bang.R']=1.
    elif vertex.index in coat and point.z<1.12 and arm_weight<.05:
        fraction=max(0.,min(1.,(1.12-point.z)/.22));fraction=fraction*fraction*(3-2*fraction)
        direction=directions[vertex.index];angle=math.atan2(direction.y,direction.x)
        phase=(angle-math.pi/8)/(math.pi/4);first=math.floor(phase);blend=phase-first
        sectors=[(1-blend,first%8),(blend,(first+1)%8)]
        hem=max(0.,min(1.,(.86-point.z)/.30))
        for weight,i in sectors:
            added['Skirt.'+str(i)]=weight*(1-hem);added['Skirt.'+str(i)+'.Hem']=weight*hem
        base={'spine':1.}
    elif vertex.index in coat and arm_weight>.05:
        side='L' if point.x>0 else 'R';fore=rig.data.bones['forearm.'+side];axis=fore.tail_local-fore.head_local
        along=(point-fore.head_local).dot(axis)/axis.length_squared
        fraction=max(0.,min(1.,(along-.05)/.5))*arm_weight;fraction=fraction*fraction*(3-2*fraction)
        hem=max(0.,min(1.,(along-.45)/.45))
        added={'Sleeve.'+side:1-hem,'SleeveHem.'+side:hem}
    if fraction<=0:continue
    if vertex.index in coat:clothing_vertices+=1
    original=[(g.group,g.weight) for g in vertex.groups]
    if base:
        for group,weight in original:mesh.vertex_groups[group].remove([vertex.index])
        original=[(mesh.vertex_groups[name].index,weight) for name,weight in base.items()]
    for group,weight in original:mesh.vertex_groups[group].add([vertex.index],weight*(1-fraction),'REPLACE')
    for name,weight in added.items():
        if weight*fraction>0:mesh.vertex_groups[name].add([vertex.index],weight*fraction,'REPLACE')
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
max_lost=0.;max_garment_lost=0.
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
                if any(names[j].startswith(('Skirt.','Sleeve.','SleeveHem.')) and w>0 for j,w in influence):
                    max_garment_lost=max(max_garment_lost,sum(w for _,w in influence[4:]))
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
    breathe=math.sin(t*2*math.pi/4.)
    wave=gate(t,3.1,5.85,.42)
    arm_wave=gate(t,3.45,5.5,.42)
    wave_fold=gate(t,2.98,5.95,.30)
    wave_travel=gate(t,3.12,3.95,.23)+gate(t,5.15,5.92,.23)
    anticipation=gate(t,7.4,7.94,.24)
    flight_start=7.86;flight_duration=.42;flight=t-flight_start;f=flight/flight_duration
    lift=.5*9.81*flight*(flight_duration-flight) if 0<=f<=1 else 0.
    landing=math.sin((t-8.28)/.38*math.pi)*math.exp(-(t-8.28)*5) if 8.28<t<8.66 else 0.
    jump_raise=math.sin(math.pi*f) if 0<=f<=1 else 0.
    hip=Vector((.025-.025*wave,-.008*wave,lift-.027-.055*anticipation-.043*landing))
    rig.pose.bones['spine'].location=rest['spine'].to_quaternion().inverted()@hip
    rotate('spine',(-.07*anticipation+.055*jump_raise+.04*landing,.035-.03*wave,-.03*wave))
    rotate('spine.001',(-.045*anticipation+.025*landing,-.024+.035*wave,-.035*wave))
    rotate('spine.002',(-.01*wave,-.012+.03*wave,-.035*wave))
    rotate('spine.003',(-.025*wave+.018*jump_raise,-.018+.025*wave,-.025*wave))
    rotate('spine.004',(-.015-.045*wave+.04*anticipation,-.028+.055*wave,.04*wave))
    for side,sign in [('L',1),('R',-1)]:
        rotate('shoulder.'+side,(-.015*wave if side=='L' else .012*wave,0,sign*.022*wave))
        amount=wave if side=='L' else 0.
        arm_amount=arm_wave if side=='L' else 0.
        wrist_phase=(t-3.65)*2*math.pi*1.65
        wrist=math.sin(wrist_phase+.12*math.sin(wrist_phase*.5))*gate(t,3.65,5.35,.27)
        rotate('upper_arm.'+side,(-.055+.045*amount+.11*anticipation-.12*jump_raise,sign*(.26-1.33*arm_amount-.17*jump_raise),sign*.025*amount))
        rotate('forearm.'+side,(-.075-.27*(side=='R')-.25*wave_travel*(side=='L')+.08*amount-.12*jump_raise,-sign*(.11+1.18*wave_fold*(side=='L')+.45*wave_travel*(side=='L')+.11*jump_raise),0))
        rotate('hand.'+side,(-.06+.07*amount,sign*.025,.29*wrist*(side=='L')+.018*landing))
        rotate('Eye.'+side,(0,0,0))
        target=bpy.data.objects['Ankle.'+side];target.location=Vector(target['base'])+Vector((0,0,lift))
        for digit in ['f_index.01.','f_middle.01.','f_ring.01.','f_pinky.01.']:
            curl={'f_index.01.':.045,'f_middle.01.':.085,'f_ring.01.':.12,'f_pinky.01.':.15}[digit]
            rotate(digit+side,(curl*(1-.8*amount),0,0))
    blink=0.
    for at in [1.55,4.05,4.34,7.25,10.6]:
        elapsed=t-at
        if 0<=elapsed<.17:blink=max(blink,math.sin(math.pi*elapsed/.17)**2)
    face_weights=(blink,.24+.43*wave+.5*jump_raise,.05*wave*(.5+.5*math.sin(t*2*math.pi*3.1)))
    bpy.context.view_layer.update()
    packed=[]
    for name in names:
        bone=rig.pose.bones[name];matrix=world@bone.matrix
        if bone.parent:matrix=(world@bone.parent.matrix).inverted()@matrix
        position,rotation,scale=matrix.decompose();rotation.normalize()
        packed.extend((*position,rotation.x,rotation.y,rotation.z,rotation.w,*scale))
    packed.extend((*face_weights,lift,float(wave>.2 or jump_raise>0)))
    frames.append(packed)
    for side in ['L','R']:
        expected=Vector(bpy.data.objects['Ankle.'+side]['base'])+Vector((0,0,lift))
        foot_drift=max(foot_drift,(world@rig.pose.bones['foot.'+side].head-expected).length)
raw=bytearray(struct.pack('<7If',3,len(vertices),len(indices),len(names),len(frames),24,len(ranges),12.))
for vertex in vertices:raw.extend(struct.pack('<8f4B4H9f',*vertex))
raw.extend(np.array(indices,dtype='<u4').tobytes())
with tempfile.TemporaryDirectory() as directory:
    script='from PIL import Image;import sys;im=Image.open(sys.argv[1]).convert("RGBA");im.thumbnail((int(sys.argv[3]),int(sys.argv[3])),Image.Resampling.LANCZOS);im.save(sys.argv[2])'
    for i,(first,count) in enumerate(ranges):
        stem=mesh.data.materials[i].name[:4];png=Path(directory)/(stem+'.png')
        subprocess.run(['/usr/bin/python3','-c',script,str(source/(stem+'_Base_color.png')),str(png),'512' if stem=='WizF' else '1024'],check=True)
        data=png.read_bytes();raw.extend(struct.pack('<3I',first,count,len(data)));raw.extend(data)
springs=[]
for name in names:
    group=None
    if name.startswith('Bang.'):group=0
    elif name.startswith('HairF.') and not name.endswith('_end'):group=1
    elif name.startswith(('Hairb.','HairM')) and not name.endswith('_end'):group=2
    elif name.startswith('HairA') and not name.endswith('_end'):group=3
    elif name.startswith('Skirt.'):group=4
    elif name.startswith(('Sleeve.','SleeveHem.')):group=5
    elif name in ['breast.L','breast.R']:group=6
    if group is not None:
        stiffness,damping,gravity,radius,angle=[(150,12,.10,.007,.12),(85,7,.18,.009,.28),(52,5,.28,.012,.34),(110,4.5,.08,.006,.46),(95,8,.24,.012,.28),(105,8,.18,.009,.24),(170,9,.12,.025,.18)][group]
        springs.append((bone_ids[name],group,1 if group<4 else 2 if group==4 else 4 if group==5 else 8,stiffness,damping,gravity,radius,angle))
spring_ids={names[s[0]]:i for i,s in enumerate(springs)}
links=[]
for sector in range(8):
    next_sector=(sector+1)%8
    for suffix in ['', '.Hem']:
        a='Skirt.'+str(sector)+suffix;b='Skirt.'+str(next_sector)+suffix
        length=(rig.data.bones[a].tail_local-rig.data.bones[b].tail_local).length
        links.append((spring_ids[a],spring_ids[b],length,2e-6))
    for a,b in [('Skirt.'+str(sector),'Skirt.'+str(next_sector)+'.Hem'),('Skirt.'+str(sector)+'.Hem','Skirt.'+str(next_sector))]:
        links.append((spring_ids[a],spring_ids[b],(rig.data.bones[a].tail_local-rig.data.bones[b].tail_local).length,8e-6))
    for suffix in ['', '.Hem']:
        a='Skirt.'+str(sector)+suffix;b='Skirt.'+str((sector+2)%8)+suffix
        links.append((spring_ids[a],spring_ids[b],(rig.data.bones[a].tail_local-rig.data.bones[b].tail_local).length,2e-5))
colliders=[]
def collider(name,a,b,radius,mask):
    inverse=(world@rest[name]).inverted();colliders.append((bone_ids[name],mask,*(inverse@Vector(a)),*(inverse@Vector(b)),radius))
collider('face',(0,.06,1.86),(0,.06,1.97),.07,1)
collider('spine.004',(0,.06,1.69),(0,.06,1.78),.042,1)
collider('spine.003',(0,.07,1.20),(0,.075,1.52),.105,15)
collider('spine',(0,.045,1.02),(0,.045,1.14),.12,3)
for side in ['L','R']:
    shoulder=rig.data.bones['shoulder.'+side];collider(shoulder.name,world@shoulder.head_local,world@shoulder.tail_local,.045,5)
    thigh=rig.data.bones['thigh.'+side];collider(thigh.name,world@thigh.head_local,world@thigh.tail_local,.055,2)
    shin=rig.data.bones['shin.'+side];collider(shin.name,world@shin.head_local,world@shin.tail_local,.042,2)
    upper=rig.data.bones['upper_arm.'+side];collider(upper.name,world@upper.head_local,world@upper.tail_local,.04,5)
    fore=rig.data.bones['forearm.'+side];collider(fore.name,world@fore.head_local,world@fore.tail_local,.032,4)
    breast=rig.data.bones['breast.'+side];collider('Boobbase',world@breast.head_local.lerp(breast.tail_local,.65),world@breast.head_local.lerp(breast.tail_local,.8),.045,1)
anchors=[]
def anchor(name,tail=False,other=None,other_tail=False):
    def local(n,end):return (world@rest[n]).inverted()@(world@(rig.data.bones[n].tail_local if end else rig.data.bones[n].head_local))
    other=other or name;anchors.append((bone_ids[name],bone_ids[other],*local(name,tail),*local(other,other_tail),.5 if other!=name else 0.))
anchor('Eye.L',other='Eye.R');anchor('spine.004');anchor('spine.003',other='spine.004');anchor('spine')
for part in ['upper_arm','forearm','hand','shin','foot']:
    for side in ['L','R']:anchor(part+'.'+side,part=='hand')
anchor('toe.L',True,'toe.R',True)
roles=[bone_ids[n] for n in ['spine.004','face','Eye.L','Eye.R','spine.003','spine.002','f_index.01.L','f_index.01.R']]
raw.extend(struct.pack('<5I',len(names),len(springs),len(colliders),len(anchors),len(links)))
weight_counts={}
for index,name in enumerate(names):
    bind=world@rest[name];inverse=bind.inverted();parent=rig.data.bones[name].parent
    local=(world@rest[parent.name]).inverted()@bind if parent else bind
    position,rotation,scale=local.decompose();rotation.normalize()
    tail=inverse@(world@rig.data.bones[name].tail_local)
    active=np.any((joints==index)&(weights>0),axis=1);weight_counts[name]=int(active.sum())
    if active.any():
        inv=np.array(inverse);points=np.concatenate([positions[active],np.ones((active.sum(),1))],axis=1)@inv.T
        deltas=np.einsum('ij,nsj->nsi',inv[:3,:3],morph[active]);lo=(points[:,:3]+np.minimum(deltas,0).sum(axis=1)).min(axis=0);hi=(points[:,:3]+np.maximum(deltas,0).sum(axis=1)).max(axis=0)
    else:lo=hi=np.zeros(3)
    encoded=name.encode();raw.extend(struct.pack('<iI10f16f3f6fI',bone_ids[parent.name] if parent else -1,len(encoded),*position,rotation.x,rotation.y,rotation.z,rotation.w,*scale,*np.array(inverse).T.reshape(-1),*tail,*lo,*hi,int(active.any())));raw.extend(encoded)
for spring in springs:
    if not weight_counts[names[spring[0]]]:raise RuntimeError('Unweighted physics bone '+names[spring[0]])
    raw.extend(struct.pack('<3I5f',*spring))
for c in colliders:raw.extend(struct.pack('<2I7f',*c))
for a in anchors:raw.extend(struct.pack('<2I7f',*a))
raw.extend(struct.pack('<8I',*roles))
for link in links:raw.extend(struct.pack('<2I2f',*link))
raw.extend(np.array(frames,dtype='<f4').tobytes())
output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(b'WIZSKN03'+struct.pack('<I',len(raw))+zlib.compress(raw,9))
report={'format':'WIZSKN03','vertices':len(vertices),'triangles':len(indices)//3,'bones':len(names),'frames':len(frames),'fps':24,'duration':12,'frameFloats':len(frames[0]),'landmarks':len(anchors),'springBones':len(springs),'colliders':len(colliders),'clothLinks':len(links),'addedBones':[n for n,_,_,_ in definitions],'physicsGroups':{str(g):[names[s[0]] for s in springs if s[1]==g] for g in range(7)},'reweightedGarmentVertices':clothing_vertices,'bangVertices':len(bangs),'maximumDiscardedWeight':max_lost,'maximumNewGarmentDiscardedWeight':max_garment_lost,'maximumFootDrift':foot_drift,'packedBytes':output.stat().st_size,'rawBytes':len(raw)}
output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print('WIZ_EXPORT',json.dumps(report))
