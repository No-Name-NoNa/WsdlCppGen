WsdlCppGen
==========

MSYS2 + Vulkan + ImGui + C++26.

This project generates C++ SOAP proxy files from the assignment WSDL shape.

Usage
-----

GUI:

```powershell
.\cmake-build-debug\WsdlCppGen.exe
```

The GUI executable is intended for double-click usage and opens without a
console window on Windows.

CLI:

```powershell
.\cmake-build-debug\WsdlCppGenCli.exe --wsdl samples\AddService.wsdl --out generated
```

The GUI uses native system dialogs for the WSDL file and output directory.
Paths may still be pasted manually.

Generated output
----------------

The project itself uses `.hpp` + `.cpp` for internal source files.

Generated proxy files intentionally use Visual Studio-friendly `.h` + `.cpp`,
for example:

- `generated/AddServiceClient.h`
- `generated/AddServiceClient.cpp`

The generated client uses conditional compilation:

- `_WIN32` without `WSDLCPPGEN_USE_LIBCURL`: WinHTTP, no third-party dependency in Visual Studio.
- `WSDLCPPGEN_USE_LIBCURL` or non-Windows: libcurl, caller must provide headers and link curl.

AddService experiment
---------------------

Start the Go SOAP service:

```powershell
cd service\addservice-go
go run .
```

The service listens at:

- `http://localhost:8080/services/AddService`
- `http://localhost:8080/services/AddService?wsdl`

Generate the proxy:

```powershell
.\cmake-build-debug\WsdlCppGenCli.exe --wsdl samples\AddService.wsdl --out generated
```

Build the generated C++ demo client with CMake:

```powershell
cmake -S . -B cmake-build-debug -DWSDLCPPGEN_BUILD_CLIENT_DEMO=ON
cmake --build cmake-build-debug
.\cmake-build-debug\AddServiceClientDemo.exe
```

Expected output:

```text
3 + 5 = 8
```

Project assets
--------------

- `font/JetBrainsMono-Regular.ttf`: GUI font.
- `img/wsdlcppgen-brand.png`: generated brand image for reports/screenshots.
- `img/img.png`: existing GUI screenshot.
