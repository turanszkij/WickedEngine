# Contributions are welcome!
You should be familiar with the following few guidelines before you contribute any code. These are not set in stone, if there is a good argument, I am very much open to change. You can submit a pull request with your changes, briefly describe them and I will review as soon as I can!

## First, let's get the boring code formatting guidelines out of the way:
- use the default Visual Studio 2017 formatting
- generally, function names start with Uppercase, variable names with lowercase
- Very short, inline function names can start with lowercase
- member and global variables usually use camelCase formatting: int myGlobalVariable;
- temporary variables on stack sometimes use lowercase formatting like this: int tmp_variable_on_stack;
- prefer having braces on their own line when writing loops, branches, functions or whatever

## Now, on to some more interesting stuff:
- avoid using c++ stl containers if possible (std::vector is fine) (todo: there are still some offenders remaining in the code)
- avoid new and malloc and generally allocating on heap wherever you can. Especially avoid it in frequently called code! Note that by default, c++ std:: lib will allocate on heap.
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
- avoid using geometry shaders if possible
- when writing shaders, use the resource declaration macros and define proper named bind-points at compile time. See examples in ShaderInterop.h, ShaderInterop_Renderer.h, etc...
- speaking of macros, I don't have a problem if you use them wisely (use short and simple macros)
