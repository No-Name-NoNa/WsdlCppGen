WsdlCppGen
==========

MSYS2 + Vulkan + ImGui + C++26.

This project generates C++ SOAP proxy files from the assignment WSDL shape.

Usage
-----

CLI:

```powershell
.\cmake-build-debug\WsdlCppGen.exe --wsdl samples\AddService.wsdl --out generated
```

No arguments starts the ImGui interface.

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
