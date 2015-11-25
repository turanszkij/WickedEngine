# Wicked Engine Scripting API Documentation
This is a collection and explanation of scripting features in Wicked Engine.

## Contents
1. Introduction and usage
2. Engine manipulation
	1. BackLog (Console)
	2. Renderer
3. Utility Tools
	1. Font
	2. Sprite
		1. ImageEffects
		2. SpriteAnim
	3. Texture
	4. Sound
		1. SoundEffect
		2. Music
	5. Vector
	6. Matrix
	7. Scene
		1. Node
		2. Transform
		3. Cullable
		4. Object
		5. Armature
		6. Ray
		7. AABB
	8. Renderable Components
		1. Renderable2DComponent
		2. ForwardRenderableComponent
		3. DeferredRenderableComponent
		4. LoadingScreenComponent
		
## Introduction and usage
Scripting in Wicked Engine is powered by Lua, meaning that the user can make use of the 
syntax and features of the widely used Lua language, accompanied by its fast interpreter.
Apart from the common features, certain engine features are also available to use.
You can load lua script files and execute them, or the engine scripting console (also known as the BackLog)
can also be used to execute single line scripts.
Upon startup, the engine will try to load a startup script file named startup.lua in the root directory of 
the application. If not found, an error message will be thrown follwed by the normal execution by the program.
In the startup file, you can specify any startup logic, for example loading content or anything.

The setting up and usage of the BackLog is the responsibility of the target application, but the recommended way to set it up
is included in all of the demo projects. It is mapped to te HOME button by default.

## Engine manipulation
The scripting API provides functions for the developer to manipulate engine behaviour or query it for information.

### BackLog

### Renderer

## Utility Tools
The scripting API provides certain engine features to be used in development. 
Tip: You can inspect any object's functionality by calling 
getprops(YourObject) on them (where YourObject is the object which is to be inspected). The result will be displayed on the BackLog.

### Font
Gives you the ability to render text with a custom font.
### Sprite
Render images on the screen.
#### ImageEffects
Specify Sprite properties, like position, size, etc.
#### SpriteAnim
Animate Sprites easily with this helper.
### Texture
Just holds texture information in VRAM.
### Sound
Load a Sound file, either sound effect or music.
#### SoundEffect
Sound Effects are for playing a sound file once. It comes with its own configurable settings.
#### Music
Music is for playing sound files in the background, along with sound effects. It comes with its own configurable settings.
### Vector
A four component floating point vector. Provides efficient calculations with SIMD support.
### Matrix
A four by four matrix, efficient calculations with SIMD support.
### Scene
Manipulate the 3D scene with these objects. 
#### Node
The basic entity in the scene. Everything is a node. It has a name.
#### Transform
Everything in the scene is a transform. It defines a point in the space by location, size, and rotation.
It provides several key features of parenting. It is a Node.
#### Cullable
Can be tested againt the view frustum, AABBs, rays, space partitioning trees. It is a Transform.
#### Object
It is a renderable entity (optionally), which contains a Mesh and is Cullable.
#### Armature
It animates meshes. It is a Transform.
#### Ray
Can intersect with AABBs, Cullables.
#### AABB
Axis Aligned Bounding Box. Can be intersected with any shape, or ray.
### Renderable Components
A RenderableComponent describes a scene wich can render itself.
#### Renderable2DComponent
It can hold Sprites and Fonts and can sort them by layers, update and render them.
#### ForwardRenderableComponent
It renders the scene contained by the Renderer in a forward render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.
#### DeferredRenderableComponent
It renders the scene contained by the Renderer in a deferred render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.
#### LoadingScreenComponent
It is a Renderable2DComponent but one that internally manages resource loading and can display information about the process.
