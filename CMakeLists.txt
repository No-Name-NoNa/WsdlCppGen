cmake_minimum_required(VERSION 3.25)
project(WsdlCppGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(WSDLCPPGEN_BUILD_CLIENT_DEMO "Build the generated AddService C++ client demo" OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

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

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/font"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/img"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

add_library(WsdlCppGenCore STATIC
        src/core/wsdl_codegen.cpp
)

target_include_directories(WsdlCppGenCore PUBLIC
        src/core
)

add_executable(WsdlCppGen
        WIN32
        apps/gui/main.cpp
        src/gui/gui_app.cpp
        src/gui/ui_sprites.cpp
        src/platform/native_dialogs.cpp
)

target_sources(WsdlCppGen PRIVATE ${IMGUI_SRCS})

target_include_directories(WsdlCppGen PRIVATE
        src/gui
        src/platform
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(WsdlCppGen PRIVATE
        WsdlCppGenCore
        glfw
        Vulkan::Vulkan
        Threads::Threads
)

if (WIN32)
    target_link_libraries(WsdlCppGen PRIVATE ole32 uuid shell32)
endif ()

add_executable(WsdlCppGenCli
        apps/cli/main.cpp
)

target_link_libraries(WsdlCppGenCli PRIVATE
        WsdlCppGenCore
)

if (WSDLCPPGEN_BUILD_CLIENT_DEMO)
    set(ADD_SERVICE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/addservice")
    set(ADD_SERVICE_GENERATED_HEADER "${ADD_SERVICE_GENERATED_DIR}/AddServiceClient.h")
    set(ADD_SERVICE_GENERATED_SOURCE "${ADD_SERVICE_GENERATED_DIR}/AddServiceClient.cpp")

    add_custom_command(
            OUTPUT
                "${ADD_SERVICE_GENERATED_HEADER}"
                "${ADD_SERVICE_GENERATED_SOURCE}"
            COMMAND
                "$<TARGET_FILE:WsdlCppGenCli>"
                --wsdl "${CMAKE_CURRENT_SOURCE_DIR}/examples/addservice/wsdl/AddService.wsdl"
                --out "${ADD_SERVICE_GENERATED_DIR}"
            DEPENDS
                WsdlCppGenCli
                "${CMAKE_CURRENT_SOURCE_DIR}/examples/addservice/wsdl/AddService.wsdl"
            VERBATIM
    )

    add_executable(AddServiceClientDemo
            examples/addservice/client-cpp/main.cpp
            "${ADD_SERVICE_GENERATED_SOURCE}"
    )
    target_include_directories(AddServiceClientDemo PRIVATE "${ADD_SERVICE_GENERATED_DIR}")
    if (WIN32)
        target_link_libraries(AddServiceClientDemo PRIVATE winhttp)
    else ()
        find_package(CURL REQUIRED)
        target_compile_definitions(AddServiceClientDemo PRIVATE WSDLCPPGEN_USE_LIBCURL)
        target_link_libraries(AddServiceClientDemo PRIVATE CURL::libcurl)
    endif ()
endif ()
