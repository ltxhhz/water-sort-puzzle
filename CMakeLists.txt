# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CMakeProject1")

#set(CMAKE_TOOLCHAIN_FILE "D:\\programming\\cpp\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake")
#set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/share/opencv4")
# add_definitions(-D_ATL_CSTRING_EXPLICIT_CONSTRUCTORS)
find_package(OpenCV REQUIRED)
# 将源代码添加到此项目的可执行文件。
add_executable (CMakeProject1 WIN32 "CMakeProject1.cpp" "CMakeProject1.h" "DFS.cpp" "DFS.h" "resource.h" )
add_compile_definitions(_UNICODE UNICODE)

# 设置资源文件
if(WIN32)
    enable_language(RC)
    target_sources(CMakeProject1 PRIVATE "resource.rc")
endif()

# 设置 C++ 标准
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeProject1 PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
set_target_properties(CMakeProject1 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
target_link_libraries(CMakeProject1 ${OpenCV_LIBS})
target_link_libraries(CMakeProject1 gdiplus Msimg32 atls)

# 添加 include 路径
# include_directories(${CMAKE_SOURCE_DIR})