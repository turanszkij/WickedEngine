#!/usr/bin/env python3

import bpy
import os
import ctypes
from mathutils import Vector, Quaternion, Matrix


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


def WIVector3(blvector3: Vector):
    import pywickedengine as wi
    return wi.XMFLOAT3(blvector3[0], blvector3[2], blvector3[1])

def WIQuaternion(q: Quaternion):
    import pywickedengine as wi
    # [W X Y Z] -> [X Y Z W] -> [X Z Y -W]
    # q2=[q1.x,q1.z,q1.y,−q1.w]
    return wi.XMFLOAT4(q[1], q[3], q[2], -q[0])

def wicked_add_mesh(item, scene, root):
    print(f"Adding {item.name} mesh ...")
    import pywickedengine as wi
    entity = scene.Entity_CreateObject(item.name)
    object = scene.objects().GetComponent(entity)
    scene.meshes().Create(entity)
    #entity = scene.Entity_CreateMesh(item.name)
    #scene.Component_Attach(entity, root) #TODO this creates a self referencing entity, getting the loading screen stuck in an infintie while loop
    mesh = scene.meshes().GetComponent(entity)
    transform = scene.transforms().GetComponent(entity)
    transform.translation_local = WIVector3(item.location)
    transform.scale_local = WIVector3(item.scale)
    if item.rotation_mode == 'QUATERNION':
        transform.rotation_local = WIQuaternion(item.rotation_quaternion)
    else:
        transform.rotation_local = WIQuaternion(item.rotation_euler.to_matrix().to_quaternion())
    transform.UpdateTransform()
    object.meshID = entity

    if scene.materials().GetCount() == 0:
        scene.materials().Create(wi.CreateEntity())

    #vertices[i].co # position data
    #vertices[i].undeformed_co # position data - no deforming modifiers applied

    # create only one submesh for now
    #TODO later create a submesh for each material (or similar)
    subset = wi.MeshComponent_MeshSubset()
    material = scene.materials().GetComponent(subset.materialID)
    vertex_offset = len(mesh.vertex_positions)

    #index_remap = [0,1,-1]
    index_remap = [0,0,0]

    # append vertices
    print(f"Adding vertex data (len={len(item.data.vertices)})")
    for vertex in item.data.vertices:
        mesh.vertex_positions.append(WIVector3(vertex.co))
        mesh.vertex_normals.append(WIVector3(vertex.normal))
        mesh.vertex_uvset_0.append(wi.XMFLOAT2(0,0))

    print("Adding index data")
    for face in item.data.loop_triangles:
        #print("Polygon index: %d, length: %d" % (face.index, 3))
        offset=0
        for i in face.loops:
            ix = item.data.loops[i + index_remap[offset]].vertex_index
            mesh.indices.append(ix)
            #print("   Vertex: %d %r" % (item.data.loops[i].vertex_index, item.data.vertices[ix].co))
            #print("   UV: %r"     % item.data.uv_layers.active.data[i].uv)
            offset +=1
        #verts_in_face = face.vertices[:]
        #print("face index ", face.index)
        #print("normal ", face.normal)
        #for vert in verts_in_face:
        #    print("vertex coords ", item.data.vertices[vert].co)

    if len(mesh.vertex_normals) == 0 and len(mesh.vertex_positions) > 0:
        for _ in range(len(mesh.vertex_positions)):
            mesh.vertex_normals.append(wi.XMFLOAT3(0,0,0))
        mesh.ComputeNormals(wi.MeshComponentComputeNormals.SMOOTH_FAST);

    subset.materialID = scene.materials().GetEntity(0)#max(0, face.material))
    subset.indexOffset = 0
    subset.indexCount = len(mesh.indices)
    mesh.subsets.append(subset)

    print(f"Finished adding {item.name} mesh")


def create_wiscene(scene_name):
    import pywickedengine as wi
    ### Load Scene
    scene = wi.Scene()
    rootEntity = wi.CreateEntity()
    scene.transforms().Create(rootEntity)
    scene.names().Create(rootEntity)
    scene.names().GetComponent(rootEntity).name = scene_name

    #TODO materials

    # Meshes
    print("Exporting meshes")
    for item in bpy.data.objects:
        print(f"Element in scene {item.name}")
        match item.type:
            case 'MESH':
                wicked_add_mesh(item, scene, rootEntity)
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
    import pywickedengine as wi

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
    bpy.utils.register_class(ExportWiScene)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportWiScene)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_wicked.wiscene('INVOKE_DEFAULT')

