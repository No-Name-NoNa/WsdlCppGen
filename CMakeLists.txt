cmake_minimum_required(VERSION 3.25)
project(WsdlCppGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

## CMAKE_PREFIX_PATH=D:\msys2\ucrt64

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui")

set(IMGUI_SRCS
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/font"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(WsdlCppGen
        src/main.cpp
        src/wsdl_codegen.cpp
        src/gui_app.cpp
)

target_sources(WsdlCppGen PRIVATE ${IMGUI_SRCS})

target_include_directories(WsdlCppGen PRIVATE
        src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
)


target_link_libraries(WsdlCppGen PRIVATE
        glfw
        Vulkan::Vulkan
)
