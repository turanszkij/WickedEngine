#!/usr/bin/env python3


import bpy
import os
import ctypes
from mathutils import Vector, Quaternion, Matrix

import os
import sys
importpath=os.path.dirname(os.path.realpath(__file__))
sys.path.append(importpath)
import pywickedengine as wi


bl_info = {
    "name": "Wicked wiscene Extension",
    "category": "Generic",
    "version": (1, 0, 0),
    "blender": (4, 0, 0),
    'location': 'File > Export > wiscene',
    'description': 'Addon to add wiscene export to file functionality.',
    'tracker_url': "https://github.com/turanszkij/WickedEngine/issues/",
    'isDraft': True,
    'developer': "Matteo De Carlo", # Replace this
    'url': 'https://github.com/turanszkij/WickedEngine/',
}


def WIVector3(blvector3: Vector) -> wi.XMFLOAT3:
    return wi.XMFLOAT3(blvector3[0], blvector3[2], blvector3[1])

def WIQuaternion(q: Quaternion) -> wi.XMFLOAT4:
    # [W X Y Z] -> [X Y Z W] -> [X Z Y -W]
    # q2=[q1.x,q1.z,q1.y,−q1.w]
    return wi.XMFLOAT4(q[1], q[3], q[2], -q[0])

def wicked_add_mesh(obj, scene, root):
    print(f"Adding {obj.name} mesh ...")
    mesh = obj.data
    entity = scene.Entity_CreateObject(obj.name)
    print(f'{obj.name}={entity}')
    object = scene.objects().GetComponent(entity)
    scene.meshes().Create(entity)
    #entity = scene.Entity_CreateMesh(obj.name)
    #scene.Component_Attach(entity, root) #TODO this creates a self referencing entity, getting the loading screen stuck in an infintie while loop
    wimesh = scene.meshes().GetComponent(entity)
    transform = scene.transforms().GetComponent(entity)
    transform.translation_local = WIVector3(obj.location)
    transform.scale_local = WIVector3(obj.scale)
    if obj.rotation_mode == 'QUATERNION':
        transform.rotation_local = WIQuaternion(obj.rotation_quaternion)
    else:
        transform.rotation_local = WIQuaternion(obj.rotation_euler.to_matrix().to_quaternion())
    transform.UpdateTransform()
    object.meshID = entity

    if scene.materials().GetCount() == 0:
        scene.materials().Create(wi.CreateEntity())

    #vertices[i].co # position data
    #vertices[i].undeformed_co # position data - no deforming modifiers applied

    for mat in mesh.materials:
        pass
        #TODO create a subset per material
    # create only one submesh for now
    subset = wi.MeshComponent_MeshSubset()
    material = scene.materials().GetComponent(subset.materialID)
    vertex_offset = len(wimesh.vertex_positions)

    print("Generating vertex data")
    mesh.calc_loop_triangles()
    #After using mesh.calc_normals_split() you access the normals with loop.normal
    mesh.calc_normals_split()

    vertices_map = {}
    vertices_array = []

    #index_remap = [0,1,-1]
    index_remap = [0,0,0]

    for triangle in mesh.loop_triangles:
        face_index = mesh.loop_triangle_polygons[triangle.index].value
        face = mesh.polygons[face_index]
        #print(f'triangle[{triangle.index}]{triangle}={face_index} SmoothGroup={smooth_groups[face_index]}')
        offset=0
        for i in triangle.loops:
            i += index_remap[offset]
            ix = mesh.loops[i].vertex_index
            normal = mesh.loops[i].normal
            uv = mesh.uv_layers.active.data[i].uv
            assert len(uv) == 2
            v = (ix, WIVector3(normal), wi.XMFLOAT2(uv[0], uv[1]))
            if v in vertices_map:
                newix = vertices_map[v]
            else:
                newix = len(vertices_array)
                vertices_array.append(v)
                vertices_map[v] = newix
            wimesh.indices.append(newix)
            offset +=1

    print("Adding vertex data")
    for i, normal, uv in vertices_array:
        vertex = mesh.vertices[i]
        wimesh.vertex_positions.append(WIVector3(vertex.co))
        wimesh.vertex_normals.append(normal)
        wimesh.vertex_uvset_0.append(uv)

    if len(wimesh.vertex_normals) == 0 and len(wimesh.vertex_positions) > 0:
        print("Generating missing normal data")
        for _ in range(len(wimesh.vertex_positions)):
            wimesh.vertex_normals.append(wi.XMFLOAT3(0,0,0))
        wimesh.ComputeNormals(wi.MeshComponentComputeNormals.SMOOTH_FAST);

    subset.materialID = scene.materials().GetEntity(0)#max(0, face.material))
    subset.indexOffset = 0
    subset.indexCount = len(wimesh.indices)
    wimesh.subsets.append(subset)

    print(f"Finished adding {obj.name} mesh")


def create_wiscene(scene_name):
    ### Load Scene
    scene = wi.Scene()
    rootEntity = wi.CreateEntity()
    print(f'rootEntity({scene_name})={rootEntity}')
    scene.transforms().Create(rootEntity)
    scene.names().Create(rootEntity)
    scene.names().GetComponent(rootEntity).name = scene_name

    #TODO materials
    for mat in bpy.data.materials:
        pass

    # Meshes
    print("Exporting meshes")
    for obj in bpy.data.objects:
        print(f"Element in scene {obj.name}")
        match obj.type:
            case 'MESH':
                wicked_add_mesh(obj, scene, rootEntity)
    print("Finished exporting meshes")

    #TODO armatures

    #TODO transform hierarchy

    #TODO armature-bone mappings

    #TODO animations

    #TODO lights

    #TODO cameras

    ## Correct orientation after importing
    #scene.Update(0)
    #TODO FlipZAxis
    ## Update hte scene to have up to date values immediately after loading:
    ## e.g. snap to camera functionality relies on this
    #scene.Update(0)

    print("FINISH")
    return scene


def write_wiscene(context, filepath, dump_to_header: bool):
    print("running write_wiscene...")

    filename = os.path.splitext(filepath)[0]
    scene_name = os.path.basename(filename)

    ar = wi.Archive() if dump_to_header else wi.Archive(filepath, False)
    if not ar.IsOpen():
        print("ERROR, ARCHIVE DID NOT OPEN")
        return {'ERROR'}

    scene = create_wiscene(scene_name)

    ### Serialize Scene
    scene.Serialize(ar)

    if dump_to_header:
        ar.SaveHeaderFile(filename+".h", scene_name)

    ar.Close()
    print("finished write_wiscene")
    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportWiScene(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_wicked.wiscene"  # important since its how bpy.ops.export_wicked.wiscene is constructed
    bl_label = "Export Wicked Scene"

    # ExportHelper mix-in class uses this.
    filename_ext = ".wiscene"

    filter_glob: StringProperty(
        default="*.wiscene",
        options={'HIDDEN'},
        maxlen=1023,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    dump_to_header: BoolProperty(
        name="Dump to Header",
        description="Creates a C/C++ header with a byte array containing the serialized scene",
        default=False,
    )

    type: EnumProperty(
        name="Example Enum",
        description="Choose between two items",
        items=(
            ('OPT_A', "First Option", "Description one"),
            ('OPT_B', "Second Option", "Description two"),
        ),
        default='OPT_A',
    )

    def execute(self, context):
        return write_wiscene(context, self.filepath, self.dump_to_header)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportWiScene.bl_idname, text="Wicked Scene (.wiscene)")


# Register and add to the "file selector" menu (required to use F3 search "Text Export Operator" for quick access).
def register():
    wi.init()
    bpy.utils.register_class(ExportWiScene)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportWiScene)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    wi.deinit()


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_wicked.wiscene('INVOKE_DEFAULT')

