VULKAN_SDK = "/home/jeremy/VulkanSDK/1.2.135.0/x86_64"

CFILES = Glob("WickedEngine/*.c")
CPPFILES = Glob("WickedEngine/*.cpp")

env = Environment(CPPPATH = ["."])
env.Program(WickedEngine, CFILES, CPPFILES)
Repository("./include")
Repository(VULKAN_SDK + "/include")
