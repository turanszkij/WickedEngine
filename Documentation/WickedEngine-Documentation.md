# WickedEngine Documentation (work in progress)

## Classes
The most important classes are related as described in the following diagram: 
![ClassDiagram](classdiagram.png)
<i>(Diagram generated with nomnoml.com)</i>

### Components
The "engine components" which provide the high level runtime flow control logic. The application usually overrides these to specify desired specialized logic and rendering.

#### MainComponent
This class represents the engine entry point and the highest level of runtime flow control. At any time it holds a RenderableComponent of a certain type while continuously calling its Update and Render functions.
This class is completely overridable to the user's desired control flow logic.

#### RenderableComponent
This is an abstract class which is void of any logic, but its overriders can implement several methods of it like Load, Update, Render, etc. 
Any class implementing this interface can be "activated" on the MainComponent and that will just run it.
The available functions for overriding are the following:

	- void Initialize() : Initialize common state
	- void Load() : Use it for resource loading
	- void Unload() : Destroy resources inside of this
	- void Start() : This function will be called when this component is activated by the MainComponent
	- void Stop() : When activating a RenderableComponent in the MainComponent, that component will Start(), but the previously activated component will call Stop()
	- void FixedUpdate() : Implement you fixed timestep logic in this function
	- void Update(float dt) : Used to implements variable timestep logic. dt paraeter is elapsed time in seconds
	- void Render() : Implement rendering to an off screen buffer
	- void Compose() : Implement rendering to the final buffer which will be presented to the screen

#### Renderable2DComponent
It can manage the rendering of a 2D overlay. It supports layers for sprites and font rendering. The whole class is overridable.

#### Renderable3DComponent
It manages the rendering of a 3D scene with a 2D overlay because it inherits everything from Renderable2DComponent too. This is an interface because multiple rendering methods are provided for performance intesive 3D rendering.
It is important to note that Renderable3DComponents only specify rendering flow and render buffers (render targets). The management of the scene graphs is not their responsibility. That is the wiRenderer static class.
The application can use any of the deriving renderers like ForwardRenderableComponent,  DeferredRenderableComponent or TiledForwardRenderableComponent for example, or even create a custom renderable component (advanced).

##### ForwardRenderableComponent
The simplest kind of 3D renderable component. It supports only directional light for the time being. It is also the least flexible and scalable one.

##### TiledForwardRenderableComponent
This implements the Forward+ rendering pipeline which is a forward rendering using per-screenspace-tile light list while rendering objects. It has a Z-buffer creation prepass to eficiently cull lights per tile and reduce overdraw.

#### DeferredRenderableComponent
This implements a Deferred rendering pipeline. Several screen space geometry buffers are created while rendering the objects, then the lights are rendered in screen space while looking up the geometry information.
It has a lower entry barrier performance wise than Tiled Forward rendering but has some disadvantages. It scales worse for increasing number of lights, has lower precision output and transparent objects are rendered using the 
Forward rendering pipeline.

#### LoadingScreenComponent
A helper engine component to load resources or an entire component. It can be activated by the MainComponent as a regular component but has some additional features which are not available in any other.

	- void addLoadingFunction(function<void()> loadingFunction) : Assign a function to do on a separate loading thread. Multiple functions can be assigned
	- void addLoadingComponent(RenderableComponent* component, MainComponent* main) : A helper for loading a whole component. When it's finished, the MainComponent which is provided in the arguments, will activate that component
	- void onFinshed(function<void()> finishFunction) : Tell the component what to do when loading is completely finished
	- int getPercentageComplete() : Retrieve a number between 0 and 100 indicating the finished amount of work the component has done
	- bool isActive() : See if the component is currently working on the loading or not


# TODO
