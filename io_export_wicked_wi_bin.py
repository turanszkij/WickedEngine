info = {
    "name": "Wicked Model Format",
    "author": "Turánszki János",
    "version": (0, 0, 1),
    "blender": (2, 7, 2),
    "location": "File > Export > Wicked (.wi)",
    "description": "Export Wicked Model Format (.wi)",
    "warning": "",
    "wiki_url": ""\
        "Scripts/Import-Export/Wicked_Exporter",
    "tracker_url": ""\
        "func=detail&aid=22795",
    "category": "Import-Export"}
    
EXPORTER_VERSION=1000
    
export_objects = True
export_armatures = True
export_actions = True
export_materials = True
export_lights = True
export_hitspheres = True
export_cameras = True
export_worldinfo = True

import os
import bpy
import struct
from mathutils import *
bpy.context.scene.objects.active=None
bpy.context.scene.frame_current=0
#bpy.context.scene.frame_current=bpy.context.scene.frame_start
for arm in bpy.data.armatures:
    arm.pose_position='REST'

class weightD:
    name=""
    value=0
    def __init__(self,nam,val):
        self.name=nam
        self.value=val
class vertexD:
    def __init__(self):
        self.pos=Vector()
        self.nor=Vector()
        self.wei=[]
        self.texX=0
        self.texY=0
        self.matI=0
    def write( self,file ):
        file.write(struct.pack('ffffffffii', self.pos.x, self.pos.y, self.pos.z, self.nor.x, self.nor.y, self.nor.z, self.texX, self.texY, self.matI, len(self.wei)))
        for w in self.wei:
            file.write(struct.pack('i'+str(len(w.name))+'s',len(w.name),bytes(w.name,'ascii')))
            file.write(struct.pack('f',w.value))
        

def WriteVertex( array,index ):
    vert = array[index]
    try:
        out.write("v " + str(vert.pos.x) + " " + str(vert.pos.y) + " " + str(vert.pos.z) + "\n")
        out.write("n " + str(vert.nor.x) + " " + str(vert.nor.y) + " " + str(vert.nor.z) + "\n")
        out.write("u " + str(vert.texX) + " " + str(vert.texY) + "\n")
        for weight in vert.wei:
            out.write("w " + weight.name + " " + str(weight.value) + "\n")
        if vert.matI>0: out.write("a " + str(vert.matI) + "\n")
    except: pass

#path=bpy.path.display_name(bpy.data.filepath)+".wi"
pathO=bpy.path.display_name(bpy.data.filepath)+".wio"
pathM=bpy.path.display_name(bpy.data.filepath)+".wim"
pathA=bpy.path.display_name(bpy.data.filepath)+".wia"
pathE=bpy.path.display_name(bpy.data.filepath)+".wierror"
pathAC=bpy.path.display_name(bpy.data.filepath)+".wiact"
pathL=bpy.path.display_name(bpy.data.filepath)+".wil"
pathH=bpy.path.display_name(bpy.data.filepath)+".wih"
pathW=bpy.path.display_name(bpy.data.filepath)+".wiw"
pathC=bpy.path.display_name(bpy.data.filepath)+".wic"
pathD=bpy.path.display_name(bpy.data.filepath)+".wid"

if export_objects:
    #out = open(path, "w")
    outO = open(pathO, "w")
outM = open(pathM, "w")
outA = open(pathA, "w")
outE = open(pathE, "w")
outAC = open(pathAC, "w")
outL = open(pathL, "w")
outH = open(pathH, "w")
outW = open(pathW, "w")
outC = open(pathC, "w")
outD = open(pathD, "w")

outC.write("CAMERALIST\n")

try:
    world=bpy.data.worlds[0]
    outW.write("h "+str(world.horizon_color.r)+" "+str(world.horizon_color.g)+" "+str(world.horizon_color.b)+"\n")
    outW.write("z "+str(world.zenith_color.r)+" "+str(world.zenith_color.g)+" "+str(world.zenith_color.b)+"\n")
    outW.write("a "+str(world.ambient_color.r)+" "+str(world.ambient_color.g)+" "+str(world.ambient_color.b)+"\n")
    if world.mist_settings.use_mist:
        outW.write("m "+str(world.mist_settings.start)+" "+str(world.mist_settings.depth)+" "+str(world.mist_settings.height)+"\n")
except:
    outE.write("World Information cannot be read!\n")



outH.write("HITSPHERELIST\n")
outD.write("DECALS\n")
for e in bpy.data.objects:
    if e.type=='EMPTY':
        try: 
            decal = e.get('decal')
            if decal:
                outD.write("d " + e.name + " " + str(e.scale.x) + " " + str(e.scale.z) + " " + str(e.scale.y) + " " + str(e.location.x) + " " + str(e.location.z) + " " + str(e.location.y) + " " + str(e.rotation_quaternion.x) + " " + str(e.rotation_quaternion.z) + " " + str(e.rotation_quaternion.y) + " " + str((-1)*e.rotation_quaternion.w) + "\n")
                tex=e.get('texture')
                nor=e.get('normal')
                if tex:
                    outD.write("t " + tex + "\n")
                if nor:
                    outD.write("n " + nor + "\n")
            parented=False
            if e.parent or e.parent_bone :
                parented=True
            if parented:
                pmi=e.matrix_parent_inverse
                pti=pmi.translation
                pri=pmi.to_quaternion()
                psi=pmi.to_scale()
                outH.write("H " + e.name + " " + str(e.scale.x) + " " + str(e.location.x) + " " + str(e.location.z) + " " + str(e.location.y) + " " + e.parent.name+ " ")
                p=e.get("property")
                if p: outH.write(str(p)+"\n")
                else: outH.write("hit\n")
                if e.parent_bone:
                    outH.write("b " + e.parent_bone + "\n")
                outH.write("I " + str(pti.x) + " " + str(pti.z) + " " + str(pti.y) + " " + str(pri.x) + " " + str(pri.z) + " " + str(pri.y) + " " + str((-1)*pri.w))
                outH.write(" " + str(psi.x) + " " + str(psi.z) + " " + str(psi.y) + "\n")
        except: 
            pass

exportedmeshes=[]
for object in bpy.data.objects:
    object.rotation_mode='QUATERNION'
    S=object.scale
    R=object.rotation_quaternion
    T=object.location
    if object.type=='MESH' and export_objects:
        m=object.data
        outO.write("//OBJECT-"+str(object.name)+"\n")
        bpy.context.scene.objects.active=object #SET ACTIVE OBJECT
        parented=False
        if object.parent : 
            outO.write("p " + object.parent.name + "\n")
            parented=True
        if object.parent_bone: 
            outO.write("b " + object.parent_bone + "\n")
            parented=True
        if parented:
            pmi=object.matrix_parent_inverse
            pti=pmi.translation
            pri=pmi.to_quaternion()
            psi=pmi.to_scale()
            outO.write("I " + str(pti.x) + " " + str(pti.z) + " " + str(pti.y) + " " + str(pri.x) + " " + str(pri.z) + " " + str(pri.y) + " " + str((-1)*pri.w))
            outO.write(" " + str(psi.x) + " " + str(psi.z) + " " + str(psi.y) + "\n")
        outO.write("r " + str(R.x) + " " + str(R.z) + " " + str(R.y) + " " + str((-1)*R.w) + "\n")
        outO.write("s " + str(S.x) + " " + str(S.z) + " " + str(S.y) + "\n")
        outO.write("t " + str(T.x) + " " + str(T.z) + " " + str(T.y) + "\n")
        outO.write("mb " + m.name + "\n") #binary
        if len(object.particle_systems)>0:
            for pa in object.particle_systems:
                sett=pa.settings
                pamat=object.material_slots[sett.material-1].name
                if sett.type=='EMITTER':
                    visible=sett.use_render_emitter
                    visibleStr="0"
                    if visible==True: visibleStr="1"
                    try:
                        outO.write("E " + pa.name + " " + visibleStr + " " + pamat + " " + str(sett.particle_size) + " " + str(sett.factor_random) + " " + str(sett.normal_factor) + " ")
                        outO.write(str(sett.count) + " " + str(sett.lifetime) + " " + str(sett.lifetime_random))
                        outO.write(" " + str(sett.billboard_size[0]) + " " + str(sett.billboard_size[1]) + " " + str(sett.tangent_phase) + "\n")
                    except:
                        pass
                elif sett.type=='HAIR':
                    try:
                        outO.write("H " + pa.name + " " + pamat + " " + str(sett.hair_length) + " " + str(sett.count) + " ")
                        if pa.vertex_group_density=="":
                            outO.write("void ")
                        else:
                            outO.write(pa.vertex_group_density+" ")
                        if pa.vertex_group_length=="":
                            outO.write("void\n")
                        else:
                            outO.write(pa.vertex_group_length+"\n")
                    except:
                        outE.write(object.name+" hair emitter failed to export")
                        pass
        if object.rigid_body:
            rb=object.rigid_body
            kinem=0
            if rb.kinematic: kinem=1
            if rb.enabled: outO.write("P " + rb.collision_shape + " " + str(rb.mass) + " " + str(rb.friction) + " " + str(rb.restitution) + " " + str(rb.linear_damping) + " " + str(rb.type) + " " + str(kinem) + "\n")
            else: outO.write("P " + rb.collision_shape + " 0 " + str(rb.friction) + " " + str(rb.restitution) + " " + str(rb.linear_damping) + " " + str(rb.type) + " " + str(kinem) + "\n")
        
        
        
        if not (m.name in exportedmeshes):
            outB=open(m.name+".wimesh",'wb')
            outB.write(struct.pack('i',EXPORTER_VERSION))
            exportedmeshes.append(m.name)
            #out.write("//MESH-"+str(m.name)+"\n")
            m.calc_tessface()
            m.calc_normals()
            #try: out.write("b " + m.get("billboard") + "\n")
            #except: pass
            if m.get("billboard"):
                outB.write(struct.pack('i1s',1,bytes(m.get("billboard"),'ascii')))
            else:
                outB.write(struct.pack('i',0))
            #if object.parent : out.write("p " + object.parent.name + "\n")
            if object.parent:
                outB.write(struct.pack('i'+str(len(object.parent.name))+'s',len(object.parent.name),bytes(object.parent.name,'ascii')))
            else:
                outB.write(struct.pack('i',0))
            VERTICES=[]
            INDICES=[]
            SOFTIND=[]
            SOFTVERT=[]
            uvtex=m.tessface_uv_textures.active
            tessf=m.tessfaces
            
            renderable=True
            outB.write(struct.pack('i',len(m.materials)))
            for material in m.materials:
                try:
                    matNameLen = len(material.name)
                    outB.write(struct.pack('i',matNameLen))
                    outB.write(struct.pack(str(matNameLen)+'s',bytes(material.name,'ascii')))
                    #out.write("m " + material.name + "\n")
                except:pass
            if len(m.materials)==0: 
                renderable=False
                #outE.write(m.name + " material not present! (Is it a nonrender mesh?)\n")
            
            vi=0 #FOR LOOPING THROUGHT VERTICES ARRAY
            for v in m.vertices:
                vert=vertexD()
                vert.pos=v.co.xzy
                if object.soft_body:
                    SOFTVERT.append(vert.pos.x)
                    SOFTVERT.append(vert.pos.y)
                    SOFTVERT.append(vert.pos.z)
                    #out.write("V "+str(vert.pos.x)+" "+str(vert.pos.y)+" "+str(vert.pos.z)+"\n")
                vert.nor=v.normal.xzy
                for vg in object.vertex_groups:
                    try: vert.wei.append(weightD(vg.name,vg.weight(v.index)))
                    except: pass
                VERTICES.append(vert)
                vi+=1
            if renderable:
                outB.write(struct.pack('i',1)) #rendermesh
                for f in range(len(m.tessfaces)):
                    fMatI=m.tessfaces[f].material_index
                    indexLen = len(m.tessfaces[f].vertices)
                    for i in range(indexLen):
                        ind = m.tessfaces[f].vertices[i]
                        gotnewVertex=False
                        if fMatI > 0:
                            gotnewVertex=True
                        smoothing=False
                        if not m.tessfaces[f].use_smooth:
                            gotnewVertex=True
                            smoothing=True
                        TEXX=0.0
                        TEXY=0.0
                        try: tex=uvtex.data[f]
                        except: pass
                        else:
                            TEXX=tex.uv_raw[i*2]
                            TEXY=1-tex.uv_raw[i*2+1]
                            if not gotnewVertex:
                                if ( VERTICES[ind].texX or VERTICES[ind].texY ) and ( TEXX != VERTICES[ind].texX or TEXY != VERTICES[ind].texY ) :
                                    gotnewVertex=True
                                    
                        if gotnewVertex:
                            vert=vertexD()
                            vert.pos=m.vertices[ind].co.xzy
                            if smoothing:
                                vert.nor=m.tessfaces[f].normal.xzy
                            else:
                                vert.nor=m.vertices[ind].normal.xzy
                            for vg in object.vertex_groups:
                                try: vert.wei.append(weightD(vg.name,vg.weight(ind)))
                                except: pass
                            vert.texX=TEXX
                            vert.texY=TEXY
                            vert.matI=fMatI
                            VERTICES.append(vert)
                            INDICES.append(vi)
                            vi+=1
                        else:
                            VERTICES[ind].texX=TEXX
                            VERTICES[ind].texY=TEXY
                            INDICES.append(ind)
                        if object.soft_body:
                            SOFTIND.append(ind)
                    if indexLen==4:
                        INDICES.append(INDICES[-4])
                        INDICES.append(INDICES[-3])
                        if object.soft_body:
                            SOFTIND.append(SOFTIND[-4])
                            SOFTIND.append(SOFTIND[-3])
                          
#                ni=0
#                waste=0
#                wasteAr=[]
#                VERTICES_OPT=[]
#                for i in range(len(VERTICES)):
#                    if i in INDICES:
#                        VERTICES_OPT.append(VERTICES[i])
#                        ni+=1
#                    else:
#                        waste+=1
#                    wasteAr.append(waste)
#                    
#                
#                vertexCount=len(VERTICES_OPT)
#                outB.write(struct.pack('i',vertexCount))
#                for i in range(vertexCount):
#                    VERTICES_OPT[i].write(outB)
#                
#                outB.write(struct.pack('i',len(INDICES)))
#                for i in INDICES:
#                    outB.write(struct.pack('I',i-wasteAr[i]))
                
                outB.write(struct.pack('i',len(VERTICES)))
                for v in VERTICES:
                    v.write(outB)
                    
                outB.write(struct.pack('i',len(INDICES)))
                for i in INDICES:
                    outB.write(struct.pack('I',i))
                
                
                if object.soft_body:
                    outB.write(struct.pack('i',1)) #softbody
                    outB.write(struct.pack('ii',len(SOFTIND),len(SOFTVERT)))
                    #out.write("I " + str(len(SOFTIND)))
                    for i in SOFTIND:
                        outB.write(struct.pack('I',i))
                        #out.write(" "+str(i))
                    #out.write("\n")
                    for v in SOFTVERT:
                        outB.write(struct.pack('f',v))
                else:
                    outB.write(struct.pack('i',0)) #nosoftbody
                
                #out.write("EFFECTIVENESS:"+str(ni)+"/"+str(len(INDICES))+"\n")
            else:
                vertexCount=len(VERTICES)
                outB.write(struct.pack('ii',0,vertexCount)) #nonrendermesh,vertexCount
                for i in range(vertexCount):
                    #WriteVertex(VERTICES,i)
                    VERTICES[i].write(outB);
            
            aabb=object.bound_box
            #out.write("B ")
            for c in aabb:
                outB.write(struct.pack('fff',c[0],c[2],c[1]))
                #out.write(str(c[0]) + " " + str(c[2]) + " " + str(c[1]) + " ")
            #out.write("\n")
            
            
            if object.soft_body:
                sb=object.soft_body
                vgm=sb.vertex_group_mass
                if vgm=='': vgm='*'
                vgg=sb.vertex_group_goal
                if vgg=='': vgg='*'
                vgs=sb.vertex_group_spring
                if vgs=='': vgs='*'
                #out.write("S " + str(sb.mass) + " " + str(sb.friction) + " " + vgg + " " + vgm + " " + vgs + "\n") 
                outB.write(struct.pack('i',1))
                outB.write(struct.pack('ff',sb.mass,sb.friction))
                
                outB.write(struct.pack('iii',len(vgg),len(vgm),len(vgs)))
                outB.write(struct.pack(str(len(vgg))+'s',bytes(vgg,'ascii')))
                outB.write(struct.pack(str(len(vgm))+'s',bytes(vgm,'ascii')))
                outB.write(struct.pack(str(len(vgs))+'s',bytes(vgs,'ascii')))
            else:
                outB.write(struct.pack('i',0))
            
            outB.close()
          
    elif object.type=='ARMATURE':
        arm=object.data
        arm.pose_position='POSE'
        outA.write("//ARMATURE-" + arm.name + "\n")
        outA.write("r " + str(R.x) + " " + str(R.z) + " " + str(R.y) + " " + str((-1)*R.w) + "\n")
        outA.write("s " + str(S.x) + " " + str(S.z) + " " + str(S.y) + "\n")
        outA.write("t " + str(T.x) + " " + str(T.z) + " " + str(T.y) + "\n")
        for bone in arm.bones:
            #LOCAL (REST) MATRIX (From object space to rest space)
            outA.write("b " + bone.name + "\n")
            try: outA.write("p " + bone.parent.name + "\n")
            except: pass
            if bone.get('physicsbone'): outA.write("P\n")
            if bone.get('cloth'): outA.write("C\n")
            if bone.get('ragdoll'): outA.write("g\n")
            if bone.use_connect: outA.write("c\n")
            springs = bone.get('spring')
            if springs: 
                for spring in springs:
                    outA.write("i " + spring + "\n")
            outA.write("l ")
            matrixLocal = bone.matrix_local
            if bone.parent:
                matrixLocal = bone.parent.matrix_local.inverted() * matrixLocal
            outA.write(str(matrixLocal.to_quaternion().x) + " " + str(matrixLocal.to_quaternion().z) + " " + str(matrixLocal.to_quaternion().y) + " " + str((-1)*matrixLocal.to_quaternion().w) + " ")
            outA.write(str(matrixLocal.translation.x) + " " + str(matrixLocal.translation.z) + " " + str(matrixLocal.translation.y) + "\n")
            outA.write("h " + str((bone.tail-bone.head).length) + "\n")
        
       
        bpy.context.scene.objects.active=object #SET ACTIVE OBJECT
        succeeded=False
        while not succeeded:
            try:
                outAC.write("//ARMATURE-" + arm.name + "\n")
                actions=bpy.data.actions
                for action in actions:
                    object.animation_data.action=None #UNLINK ACTION
                    bpy.ops.pose.user_transforms_clear(only_selected=False) #THEN RESET TO REST POSE
                    object.animation_data.action=action #THEN LINK ACTION AGAIN
                    if action.name.find(arm.name,0,len(action.name))>-1 and len(action.fcurves):
                        animationquery=action.groups
                        outAC.write("C " + action.name + "\n")
                        frame_start=bpy.context.scene.frame_start
                        frame_end=0
                        for f in action.fcurves:
                            for k in f.keyframe_points:
                                if k.co.x<frame_start: frame_start=k.co.x
                                if k.co.x>frame_end: frame_end=k.co.x
                        outAC.write("A " + str(int(frame_end-frame_start)) + "\n")
                        for bone in arm.bones:
                            outAC.write("b " + bone.name + "\n")
                            #LOCAL MATRIX MULTIPLIED BY POSE MATRIX AND MULTIPLIED BY ITS WEIGHT ON THE VERTEX GIVES THE FINAL VERTEX POSITION
                            setframesPos=[]
                            setframesRot=[]
                            setframesSca=[]
                            try: groupquery=animationquery[bone.name]
                            except: outE.write(action.name + " [bone " + bone.name + "] has not got any fcurves!\n")
                            else:
                                for fcurve in groupquery.channels:
                                    d=fcurve.data_path
                                    if d.find("rotation",0,len(d))>-1:
                                        for kFrame in fcurve.keyframe_points:
                                            frame = int(kFrame.co.x)
                                            contains=0
                                            for iterator in setframesRot:
                                                if iterator == frame:
                                                    contains = 1
                                                    break
                                            if not contains: setframesRot.append(frame)
                                    else:
                                        if d.find("location",0,len(d))>-1:
                                            for kFrame in fcurve.keyframe_points:
                                                frame = int(kFrame.co.x)
                                                contains=0
                                                for iterator in setframesPos:
                                                    if iterator == frame:
                                                        contains = 1
                                                        break
                                                if not contains: setframesPos.append(frame)
                                        else:
                                            if d.find("scale",0,len(d))>-1:
                                                for kFrame in fcurve.keyframe_points:
                                                    frame = int(kFrame.co.x)
                                                    contains=0
                                                    for iterator in setframesSca:
                                                        if iterator == frame:
                                                            contains = 1
                                                            break
                                                    if not contains: setframesSca.append(frame)
                            setframesRot.sort()
                            setframesPos.sort()
                            setframesSca.sort()
                            for keyframe in setframesRot:
                                if keyframe>=frame_start and keyframe<=frame_end:
                                    try: bpy.context.scene.frame_set(keyframe)
                                    except: outE.write(bone.name + " setting scene frame failed!\n")
                                    else:
                                        outAC.write("r " + str(keyframe) + " ")
                                        try: posequery = object.pose.bones[bone.name]
                                        except: outE.write(bone.name + " query for corresponding bone failed! Check the outliner!\n")
                                        else:
                                            #matrixPose=posequery.matrix
                                            #quat = matrixPose.to_quaternion()
                                            quat = posequery.rotation_quaternion
                                            outAC.write(str(quat.x) + " " + str(quat.z) + " " + str(quat.y) + " " + str((-1)*quat.w) + "\n")
                            for keyframe in setframesPos:
                                if keyframe>=frame_start and keyframe<=frame_end:
                                    try: bpy.context.scene.frame_set(keyframe)
                                    except: outE.write(bone.name + " setting scene frame failed!\n")
                                    else:
                                        outAC.write("t " + str(keyframe) + " ")
                                        try: posequery = object.pose.bones[bone.name]
                                        except: outE.write(bone.name + " query for corresponding bone failed! Check the outliner!\n")
                                        else:
                                            #matrixPose=posequery.matrix
                                            #loc = matrixPose.translation
                                            loc = posequery.location
                                            outAC.write(str(loc.x) + " " + str(loc.z) + " " + str(loc.y) + "\n")
                            for keyframe in setframesSca:
                                if keyframe>=frame_start and keyframe<=frame_end:
                                    try: bpy.context.scene.frame_set(keyframe)
                                    except: outE.write(bone.name + " setting scene frame failed!\n")
                                    else:
                                        outAC.write("s " + str(keyframe) + " ")
                                        try: posequery = object.pose.bones[bone.name]
                                        except: outE.write(bone.name + " query for corresponding bone failed! Check the outliner!\n")
                                        else:
                                            #matrixPose=posequery.matrix
                                            #sca = matrixPose.to_scale()
                                            sca = posequery.scale
                                            outAC.write(str(sca.x) + " " + str(sca.z) + " " + str(sca.y) + "\n")
                succeeded=True
            except: bpy.ops.object.posemode_toggle() #RESOLVING PROBLEMS IF ARMATURE IN OBJECT MODE

    elif object.type=='CAMERA':
        if object.parent and object.parent_bone:
            outC.write('c ' + object.name + ' ' + object.parent.name + ' ' + object.parent_bone + ' ' + str(T.x) + ' ' + str(T.z) + ' ' + str(T.y) + ' ' + str(R.x) + ' ' + str(R.z) + ' ' + str(R.y) + ' ' + str((-1)*R.w) + "\n")
            pmi=object.matrix_parent_inverse
            pti=pmi.translation
            pri=pmi.to_quaternion()
            psi=pmi.to_scale()
            outC.write("I " + str(pti.x) + " " + str(pti.z) + " " + str(pti.y) + " " + str(pri.x) + " " + str(pri.z) + " " + str(pri.y) + " " + str((-1)*pri.w))
            outC.write(" " + str(psi.x) + " " + str(psi.z) + " " + str(psi.y) + "\n")
            
    elif object.type=='LAMP':
        lamp=object.data
        shadow=0
        if lamp.shadow_method!='NOSHADOW':
            shadow=1
        got=False
        if lamp.type=='SUN':
            outL.write("D " + object.name + "\n")
            got=True
        if lamp.type=='POINT':
            outL.write("P " + object.name + " " + str(shadow) + "\n")
            got=True
        if lamp.type=='SPOT':
            outL.write("S " + object.name + " " + str(shadow) + " " + str(lamp.spot_size) + "\n")
            got=True
        if got:
            parented=False
            if object.parent : 
                outL.write("p " + object.parent.name + "\n")
                parented=True
            if object.parent_bone: 
                outL.write("b " + object.parent_bone + "\n")
                parented=True
            if parented:
                pmi=object.matrix_parent_inverse
                pti=pmi.translation
                pri=pmi.to_quaternion()
                psi=pmi.to_scale()
                outL.write("I " + str(pti.x) + " " + str(pti.z) + " " + str(pti.y) + " " + str(pri.x) + " " + str(pri.z) + " " + str(pri.y) + " " + str((-1)*pri.w))
                outL.write(" " + str(psi.x) + " " + str(psi.z) + " " + str(psi.y) + "\n")
            outL.write("t " + str(T.x) + " " + str(T.z) + " " + str(T.y) + "\n")
            outL.write("r " + str(R.x) + " " + str(R.z) + " " + str(R.y) + " " + str((-1)*R.w) + "\n")
            outL.write("c " + str(lamp.color.r) + " " + str(lamp.color.g) + " " + str(lamp.color.b) + "\n")
            outL.write("e " + str(lamp.energy) + "\n")
            outL.write("d " + str(lamp.distance) + "\n")
            for texslot in lamp.texture_slots:
                if texslot:
                    outL.write("l "+texslot.texture.image.name+"\n")
                
    elif object.type=='EMPTY':
        if object.field:
            if object.field.type=='WIND':
                wind = object.field
                outW.write("W " + str(R.x) + " " + str(R.z) + " " + str(R.y) + " " + str((-1)*R.w) + " " + str(wind.strength) + "\n")


for mat in bpy.data.materials:
    outM.write("//MATERIAL-" + mat.name + "\n")
    if mat.use_cast_buffer_shadows==False:
        outM.write("X\n")
    outM.write("f " + str(mat.physics.friction) + "\n")
    outM.write("d " + str(mat.diffuse_color.r) + " " + str(mat.diffuse_color.g) + " " + str(mat.diffuse_color.b) + "\n") #DIFFUSE COLOR
    if mat.use_shadeless: 
        outM.write("h\n")
    #TEXTURES
    for texslot in mat.texture_slots:
        if texslot:
            #imgname=os.path.basename(texslot.texture.image.filepath)
            imgname=texslot.texture.image.name
            if len(imgname):
                if texslot.use_map_displacement:
                    outM.write("D " + imgname + "\n") #DISPLACEMENTMAP
                if texslot.use_map_mirror:
                    outM.write("r " + imgname + "\n") #REFLECTIONMAP
                if texslot.use_map_normal: 
                    outM.write("n " + imgname + "\n") #NORMALMAP
                if texslot.use_map_specular:
                    outM.write("S " + imgname + "\n") #SPECULARMAP
                if texslot.use_map_color_diffuse:
                    outM.write("t " + imgname) #TEXTURE
                    if texslot.texture.image.alpha_mode=='PREMUL':
                        outM.write(" 1\n")
                    else: 
                        outM.write(" 0\n")
                    outM.write("b "+texslot.blend_type+"\n")
    #TRANSPARENCY
    if mat.use_transparency :
        outM.write("a " + str(mat.alpha) + "\n")
        refindex=mat.raytrace_transparency.fresnel
        if refindex!='None': outM.write("R " + str(refindex) + "\n")
    #SUBSURFACE SCATTERING
    if mat.subsurface_scattering.use:
        outM.write("u\n")
    #SPECULAR
    outM.write("s " + str(mat.specular_color.r) + " " + str(mat.specular_color.g) + " " + str(mat.specular_color.b) + " " + str(mat.specular_intensity) + "\np " + str(mat.specular_hardness) + "\n") #AMOUNT (INTENSITY), POWER
    #ENVIRO REFLECTIONS
    outM.write("e " + str(mat.raytrace_mirror.reflect_factor) + "\n")
    #SKY
    if mat.use_sky : outM.write("k\n")
    #MOVINGTEX
    try:
        move=mat.get("movingTex")
        try: 
            if len(move)==2: outM.write("m " + str(move[0]) + " " + str(move[1]) + " 0\n")
            else:
                if len(move)==3: outM.write("m " + str(move[0]) + " " + str(move[1]) + " " + str(move[2]) + "\n")
        except: pass
    except: pass
    try:
        water=mat.get("water")
        if water: outM.write("w\n")
    except: pass

if export_objects:
    #out.close()
    outO.close()
outM.close()
outA.close()
outE.close()
outAC.close()
outL.close()
outH.close()
outW.close()
outC.close()
outD.close()
