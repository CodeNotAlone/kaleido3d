set(NGFX_HEADER ${Kaleido3D_SOURCE_DIR}/Source/NGFX/src/ngfx.h)
set(NGFX_YML ${Kaleido3D_SOURCE_DIR}/Tools/RHIGenerator/ngfx.yml)

set_source_files_properties(${NGFX_HEADER}
	PROPERTIES GENERATED TRUE)

if (WIN32 OR ANDROID OR LINUX)
    generate_win_res(ngfx_vulkan VER "1.0.0.1" DESC "Next Generation Graphics (Vulkan) Module")
    generate_win_res(ngfxu VER "1.0.0.1" DESC "Next Generation Graphics Utility Module")
    generate_win_res(ngfx_vulkan_glslang VER "1.0.0.1" DESC "Next Generation Graphics Shader Compiler")

	add_library(ngfx_vulkan_glslang SHARED vulkan_glslang.h vulkan_glslang.cpp ${ngfx_vulkan_glslang_RC})
	target_compile_definitions(ngfx_vulkan_glslang PRIVATE BUILD_VULKAN_GLSLANG=1)
	target_link_libraries(ngfx_vulkan_glslang ${GLSLANG_LIBRARIES} Kaleido3D.Core ${SPIRVTOOLS_LIBRARIES})
	set_target_properties(ngfx_vulkan_glslang PROPERTIES FOLDER "Runtime/NGFX")

	add_library(ngfx_vulkan SHARED 
        ${NGFX_HEADER} ${NGFX_YML} 
        ${ngfx_vulkan_RC} vulkan.cpp 
        ${Kaleido3D_SOURCE_DIR}/Tools/RHIGenerator/generator/gen.py
    )
	target_compile_definitions(ngfx_vulkan PRIVATE BUILD_SHARED_LIB=1)

	add_library(ngfxu SHARED ngfx.h ngfxu.h ngfxu.cpp ${ngfxu_RC})
	set_target_properties(ngfxu PROPERTIES FOLDER "Runtime/NGFX")
	target_link_libraries(ngfxu Kaleido3D.Core)

	add_custom_command(TARGET ngfx_vulkan PRE_BUILD COMMAND
		${PYTHON_EXECUTABLE} ${Kaleido3D_SOURCE_DIR}/Tools/RHIGenerator/run.py -o 
		${NGFX_HEADER} ${NGFX_YML})

	target_link_libraries(ngfx_vulkan Kaleido3D.Core ${SPIRVCROSS_LIBRARIES} ngfx_vulkan_glslang ${VULKAN_LIBRARIES})
	set_target_properties(ngfx_vulkan PROPERTIES FOLDER "Runtime/NGFX")

    if(WIN32)
        generate_win_res(ngfx_d3d12 VER "1.0.0.1" DESC "Next Generation Graphics (Direct3D 12) Module")
	    add_library(ngfx_d3d12 SHARED 
            ${NGFX_HEADER} ${NGFX_YML} 
            ${ngfx_d3d12_RC} d3d12.cpp 
            ${Kaleido3D_SOURCE_DIR}/Tools/RHIGenerator/generator/gen.py
        )
        target_link_libraries(ngfx_d3d12 Kaleido3D.Core)
	    target_compile_definitions(ngfx_d3d12 PRIVATE BUILD_SHARED_LIB=1)
        set_target_properties(ngfx_d3d12 PROPERTIES FOLDER "Runtime/NGFX")
    endif()

elseif(MACOS)
	add_library(ngfx_metal SHARED metal.cpp)
	add_library(ngfxu SHARED ngfx.h ngfxu.h ngfxu.cpp)

	add_custom_command(TARGET ngfx_metal PRE_BUILD COMMAND
		${PYTHON_EXECUTABLE} ${Kaleido3D_SOURCE_DIR}/Tools/RHIGenerator/run.py -o 
		${NGFX_HEADER} ${NGFX_YML})

	set_target_properties(ngfx_metal PROPERTIES FOLDER "Runtime/NGFX")
	set_target_properties(ngfxu PROPERTIES FOLDER "Runtime/NGFX")
	target_link_libraries(ngfx_metal Kaleido3D.Core "-framework Metal")
  
elseif(ANDROID)
	add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
endif()
