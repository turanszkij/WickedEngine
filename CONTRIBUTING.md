# Contributions are welcome!
Here are some guidelines on contributing. You can submit a pull request with your changes, briefly describe them and I will review as soon as I can!

## Some guidelines to what kind of code to write:
- use the default Visual Studio 2017 formatting
- all engine source and header files are in the project root directory
- all engine header files have .h extension, while compiled source files have .cpp extension
- all third party libraries are in their respective folders within the project directory
- generally, function names start with Uppercase, variable names with lowercase
- very short, inline function names can start with lowercase
- member and global variables usually use camelCase formatting: int myGlobalVariable;
- temporary variables on stack sometimes use lowercase formatting like this: int tmp_variable_on_stack;
- prefer having braces on their own line when writing loops, branches, functions or whatever
- unit of time should be seconds
- use std::string to store strings
- avoid new and malloc and allocating on general purpose heap wherever you can. Especially avoid it in frequently called code
- you can use auto keyword, but aim for as small scope as possible
- using auto keyword for iterators is encouraged
- aim to write const-correct code
- using references, and pointers is encouraged to indicate if a resource must be initialized or not
- you can write templated code, but aim for simplicity, and it must be commented thoroughly
- aim for minimal header inclusion to keep build times fast. Use forward declarations wherever you can
- use namespaces
- avoid "using namespace xyz" in header (.h) files
- prefer "using namespace xyz" in source (.cpp) files
- use assert()
- do not use c++ exceptions, instead prefer to return error codes or bool
- avoid using geometry shaders, if possible
- when writing shaders, use the resource declaration macros and define proper named bind-points at compile time. See examples in ShaderInterop.h, ShaderInterop_Renderer.h, etc...
- speaking of macros; use them, but use them wisely (use short and simple macros)

I am open to discussion about any of these
