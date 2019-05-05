cmake_minimum_required(VERSION 3.0)

include(cmake/extern.cmake)

register_extern_include()
register_extern_lib(x64_v141/Release/cdi.lib)
register_extern_runtime(x64_v141/Release/cdi.dll)