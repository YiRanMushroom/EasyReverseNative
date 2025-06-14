cmake_minimum_required(VERSION 4.0)
project(vulkan_study)

set(PROJECT_NAME vulkan_study)

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

target_compile_definitions(${PROJECT_NAME} PRIVATE
        "VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1"
)