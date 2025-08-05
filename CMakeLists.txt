cmake_minimum_required(VERSION 3.16)


set(CMAKE_GENERATOR "MinGW Makefiles" CACHE INTERNAL "" FORCE)
set(CMAKE_C_COMPILER "C:/msys64/mingw64/bin/gcc.exe" CACHE PATH "" FORCE)
set(CMAKE_CXX_COMPILER "C:/msys64/mingw64/bin/g++.exe" CACHE PATH "" FORCE)

project(VulkanViewer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.321.1")
set(GLFW_PATH "C:/msys64/mingw64")


set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include")
set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")
find_package(Vulkan REQUIRED)


set(glfw3_DIR "${GLFW_PATH}/lib/cmake/glfw3")
find_package(glfw3 REQUIRED)


set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)

set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)
file(GLOB IMGUI_SOURCES 
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)


set(GLM_DIR ${EXTERNAL_DIR}/glm)


set(TINYGLTF_DIR ${EXTERNAL_DIR}/tinygltf)


set(ASSIMP_ROOT_DIR "C:/vcpkg/installed/x64-mingw-static")
set(ASSIMP_INCLUDE_DIRS "${ASSIMP_ROOT_DIR}/include")
set(ASSIMP_LIBRARIES "${ASSIMP_ROOT_DIR}/lib/libassimp.a")


set(STB_DIR ${EXTERNAL_DIR}/stb)


set(VMA_DIR ${EXTERNAL_DIR}/VulkanMemoryAllocator)


file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.hpp"
    "src/*.h"
)


set(INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLM_DIR}
    ${TINYGLTF_DIR}
    ${STB_DIR}
    ${VMA_DIR}/include
    C:/vcpkg/installed/x64-mingw-static/include
)


add_executable(${PROJECT_NAME} 
    ${SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${INCLUDE_DIRS}
    ${GLFW_PATH}/include
    ${GLFW_PATH}/include/glm
)


target_link_libraries(${PROJECT_NAME} 
    Vulkan::Vulkan
    glfw
    ${ASSIMP_LIBRARIES}
    C:/vcpkg/installed/x64-mingw-static/lib/libpugixml.a
    C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a
    C:/vcpkg/installed/x64-mingw-static/lib/libminizip.a
    C:/vcpkg/installed/x64-mingw-static/lib/libpolyclipping.a
    C:/vcpkg/installed/x64-mingw-static/lib/libpoly2tri.a
)


if(MINGW)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        VK_USE_PLATFORM_WIN32_KHR
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
endif()


file(GLOB SHADER_FILES "shaders/*")
file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/shaders)


file(GLOB_RECURSE ASSET_FILES "assets/*")
if(ASSET_FILES)
    file(COPY ${ASSET_FILES} DESTINATION ${CMAKE_BINARY_DIR}/assets)
endif()


find_program(GLSL_VALIDATOR glslc HINTS ${VULKAN_SDK_PATH}/Bin)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)

add_custom_command(
    OUTPUT ${SHADER_DIR}/basic_vert.spv
    COMMAND ${GLSL_VALIDATOR} ${SHADER_DIR}/basic.vert -o ${SHADER_DIR}/basic_vert.spv
    DEPENDS ${SHADER_DIR}/basic.vert
    COMMENT "Compiling basic vertex shader"
)

add_custom_command(
    OUTPUT ${SHADER_DIR}/basic_frag.spv
    COMMAND ${GLSL_VALIDATOR} ${SHADER_DIR}/basic.frag -o ${SHADER_DIR}/basic_frag.spv
    DEPENDS ${SHADER_DIR}/basic.frag
    COMMENT "Compiling basic fragment shader"
)

add_custom_command(
    OUTPUT ${SHADER_DIR}/grid_vert.spv
    COMMAND ${GLSL_VALIDATOR} ${SHADER_DIR}/grid.vert -o ${SHADER_DIR}/grid_vert.spv
    DEPENDS ${SHADER_DIR}/grid.vert
    COMMENT "Compiling grid vertex shader"
)

add_custom_command(
    OUTPUT ${SHADER_DIR}/grid_frag.spv
    COMMAND ${GLSL_VALIDATOR} ${SHADER_DIR}/grid.frag -o ${SHADER_DIR}/grid_frag.spv
    DEPENDS ${SHADER_DIR}/grid.frag
    COMMENT "Compiling grid fragment shader"
)

add_custom_command(
    OUTPUT ${SHADER_DIR}/model_vert.spv
    COMMAND ${GLSL_VALIDATOR} ${SHADER_DIR}/model.vert -o ${SHADER_DIR}/model_vert.spv
    DEPENDS ${SHADER_DIR}/model.vert
    COMMENT "Compiling model vertex shader"
)

add_custom_command(
    OUTPUT ${SHADER_DIR}/model_frag.spv
    COMMAND ${GLSL_VALIDATOR} ${SHADER_DIR}/model.frag -o ${SHADER_DIR}/model_frag.spv
    DEPENDS ${SHADER_DIR}/model.frag
    COMMENT "Compiling model fragment shader"
)

add_custom_target(shaders DEPENDS 
    ${SHADER_DIR}/basic_vert.spv 
    ${SHADER_DIR}/basic_frag.spv
    ${SHADER_DIR}/grid_vert.spv
    ${SHADER_DIR}/grid_frag.spv
    ${SHADER_DIR}/model_vert.spv
    ${SHADER_DIR}/model_frag.spv
)

add_dependencies(${PROJECT_NAME} shaders)