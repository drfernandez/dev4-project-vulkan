cmake_minimum_required(VERSION 3.10)

project(Project_Vulkan)

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

if (WIN32)
	# shaderc_combined.lib in Vulkan requires this for debug & release (runtime shader compiling)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")
	# by default CMake selects "ALL_BUILD" as the startup project 
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
		PROPERTY VS_STARTUP_PROJECT Project_Vulkan
	)
	# set the source code
	set(SOURCE
		source/gateware.h
		source/VkHelper.h
		source/main.cpp
		source/renderer.h
		source/h2bParser.h
		source/structures.h
		source/chronotimer.h
		source/leveldata.h
		source/leveldata.cpp
		source/texturemanager.h
		source/texturemanager.cpp
	)
	# add vertex shader (.hlsl) files here
	set(VERTEX_SHADERS
		Shaders/VertexShader.hlsl
		Shaders/VertexShaderSkybox.hlsl
	)	
	# add pixel shader (.hlsl) files here
	set(PIXEL_SHADERS
		Shaders/PixelShader.hlsl
		Shaders/PixelShaderSkybox.hlsl
	)
	# add any new c/c++ source code here
	add_executable (Project_Vulkan 
		${SOURCE}
		${VERTEX_SHADERS}
		${PIXEL_SHADERS}
	)
	# link the vulkan sdk include and lib files
	target_include_directories(Project_Vulkan PUBLIC $ENV{VULKAN_SDK}/Include/)
	target_link_directories(Project_Vulkan PUBLIC $ENV{VULKAN_SDK}/Lib/)
	# add support for ktx texture loading
	include_directories(${CMAKE_SOURCE_DIR}/ktx/include)
	# Find the libraries
	find_library(KTX_LIB_D NAMES ktx PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Debug)
	find_library(KTX_LIB_R NAMES ktx PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Release)
	# link the ktx sdk include and lib files
	target_link_libraries(Project_Vulkan debug ${KTX_LIB_D} optimized ${KTX_LIB_R})
	# set the vertex shader properties
	set_source_files_properties(${VERTEX_SHADERS} PROPERTIES
		VS_SHADER_TYPE Vertex 
		VS_SHADER_MODEL 5.0
		VS_SHADER_ENTRYPOINT main
		VS_TOOL_OVERRIDE "None"
		# VS_TOOL_OVERRIDE "FxCompile" # This sets it to default "HLSL Compiler"
	)
	#set the pixel shader properties
	set_source_files_properties(${PIXEL_SHADERS} PROPERTIES
		VS_SHADER_TYPE Pixel
		VS_SHADER_MODEL 5.0
		VS_SHADER_ENTRYPOINT main
		VS_TOOL_OVERRIDE "None"
	)
	#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VERTEX_SHADERS})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PIXEL_SHADERS})
endif(WIN32)

if(UNIX AND NOT APPLE)
	# libshaderc_combined.a is required for runtime shader compiling
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lX11 -lshaderc_combined")
    find_package(X11)
	find_package(Vulkan REQUIRED)
    link_libraries(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    include_directories(${Vulkan_INCLUDE_DIR}) 
	#link_directories(${Vulkan_LIBRARY}) this is currently not working
	link_libraries(${Vulkan_LIBRARIES})
	# the path is (properly)hardcoded because "${Vulkan_LIBRARY}" currently does not 
	# return a proper path on MacOS (it has the .dynlib appended)
    link_libraries(/usr/lib/x86_64-linux-gnu/libshaderc_combined.a)
    add_executable (Project_Vulkan main.cpp renderer.h)
endif(UNIX AND NOT APPLE)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fmodules -fcxx-modules")
	set(Architecture ${CMAKE_OSX_ARCHITECTURES})
	find_package(Vulkan REQUIRED)
	include_directories(${Vulkan_INCLUDE_DIR}) 
	#link_directories(${Vulkan_LIBRARY}) this is currently not working
	link_libraries(${Vulkan_LIBRARIES})
	# libshaderc_combined.a is required for runtime shader compiling
	# the path is (properly)hardcoded because "${Vulkan_LIBRARY}" currently does not 
	# return a proper path on MacOS (it has the .dynlib appended)
	link_libraries(/usr/local/lib/libshaderc_combined.a)
	add_executable (Project_Vulkan main.mm)
endif(APPLE)
