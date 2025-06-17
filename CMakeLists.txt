cmake_minimum_required(VERSION 4.0)
project(EasyReverseNative)

set(PROJECT_NAME EasyReverseNative)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "src/*.h" "src/*.hpp")
file(GLOB_RECURSE MODULE_FILES "src/*.cppm" "src/*.ixx")

add_executable(
        ${PROJECT_NAME}
        ${SOURCE_FILES}
        ${HEADER_FILES}
        ${MODULE_FILES})

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

add_subdirectory(vendor/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

add_subdirectory(vendor/glm)
target_link_libraries(${PROJECT_NAME} glm)

set(IMGUI_BACKEND_RENDERER "vulkan" CACHE STRING "ImGui backend renderer")
set(IMGUI_BACKEND_PLATFORM "glfw" CACHE STRING "ImGui backend platform")
add_subdirectory(vendor/imgui)
target_link_libraries(${PROJECT_NAME} imgui)

# MSVC UTF8
target_compile_options(${PROJECT_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        "VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1"
        "GLFW_EXPOSE_NATIVE_WIN32=1"
        "VK_USE_PLATFORM_WIN32_KHR=1"
        "GLFW_INCLUDE_VULKAN=1"
        "VULKAN_HPP_NO_EXCEPTIONS=1"
        "VULKAN_HPP_RAII_NO_EXCEPTIONS=1"
        "VULKAN_HPP_CPP_VERSION=23"
)

# Copy all files with .spv in shaders to the build directory
file(GLOB_RECURSE SHADER_FILES "shaders/*.spv")
foreach (SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHADER_FILE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/${SHADER_NAME}"
    )
endforeach ()