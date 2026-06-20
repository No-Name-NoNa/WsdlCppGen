cmake_minimum_required(VERSION 3.25)
project(WsdlCppGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(WSDLCPPGEN_BUILD_CLIENT_DEMO "Build the generated AddService C++ client demo" OFF)

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
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/img"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

add_library(WsdlCppGenCore STATIC
        src/wsdl_codegen.cpp
)

target_include_directories(WsdlCppGenCore PUBLIC
        src
)

add_executable(WsdlCppGen
        WIN32
        src/gui_main.cpp
        src/gui_app.cpp
        src/native_dialogs.cpp
)

target_sources(WsdlCppGen PRIVATE ${IMGUI_SRCS})

target_include_directories(WsdlCppGen PRIVATE
        src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(WsdlCppGen PRIVATE
        WsdlCppGenCore
        glfw
        Vulkan::Vulkan
)

if(WIN32)
        target_link_libraries(WsdlCppGen PRIVATE ole32 uuid shell32)
endif()

add_executable(WsdlCppGenCli
        src/cli_main.cpp
)

target_link_libraries(WsdlCppGenCli PRIVATE
        WsdlCppGenCore
)

if(WSDLCPPGEN_BUILD_CLIENT_DEMO)
        add_executable(AddServiceClientDemo
                samples/cpp_client_demo.cpp
                generated/AddServiceClient.cpp
        )
        target_include_directories(AddServiceClientDemo PRIVATE generated)
        if(WIN32)
                target_link_libraries(AddServiceClientDemo PRIVATE winhttp)
        else()
                find_package(CURL REQUIRED)
                target_compile_definitions(AddServiceClientDemo PRIVATE WSDLCPPGEN_USE_LIBCURL)
                target_link_libraries(AddServiceClientDemo PRIVATE CURL::libcurl)
        endif()
endif()
