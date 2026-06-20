WsdlCppGen
==========

WsdlCppGen 是一个基于 MSYS2、Vulkan、ImGui 和 C++26 的 WSDL 到 C++ SOAP
客户端代理代码生成工具。

项目可以从示例 WSDL 生成 C++ 代理文件，并提供图形界面和命令行两种使用方式。

功能概览
--------

- 图形界面选择 WSDL 文件和输出目录。
- 命令行批量生成 C++ SOAP 客户端代理代码。
- 生成 Visual Studio 友好的 `.h` + `.cpp` 文件。
- Windows 默认使用 WinHTTP，无需额外第三方运行库。
- 非 Windows 或定义 `WSDLCPPGEN_USE_LIBCURL` 时使用 libcurl。
- GitHub Actions 会在 Windows、Ubuntu、macOS 上构建并上传产物。

使用方式
--------

图形界面：

```powershell
.\cmake-build-debug\WsdlCppGen.exe
```

Windows 下 GUI 程序适合双击运行，不会打开额外控制台窗口。界面会调用系统原生
文件选择对话框选择 WSDL 文件和输出目录，也可以手动粘贴路径。

命令行：

```powershell
.\cmake-build-debug\WsdlCppGenCli.exe --wsdl samples\AddService.wsdl --out generated
```

生成结果
--------

项目自身源码使用 `.hpp` + `.cpp`。

生成的代理代码有意使用 Visual Studio 更常见的 `.h` + `.cpp`，例如：

- `generated/AddServiceClient.h`
- `generated/AddServiceClient.cpp`

生成的客户端会根据编译条件选择 HTTP 后端：

- `_WIN32` 且未定义 `WSDLCPPGEN_USE_LIBCURL`：使用 WinHTTP，Visual Studio 下不需要额外第三方依赖。
- 定义 `WSDLCPPGEN_USE_LIBCURL` 或非 Windows 平台：使用 libcurl，需要调用方提供头文件并链接 curl。

本地构建
--------

配置并构建：

```powershell
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
```

如需同时构建示例 C++ 客户端：

```powershell
cmake -S . -B cmake-build-debug -DWSDLCPPGEN_BUILD_CLIENT_DEMO=ON
cmake --build cmake-build-debug
```

AddService 示例
---------------

启动 Go SOAP 服务：

```powershell
cd service\addservice-go
go run .
```

服务地址：

- `http://localhost:8080/services/AddService`
- `http://localhost:8080/services/AddService?wsdl`

生成代理代码：

```powershell
.\cmake-build-debug\WsdlCppGenCli.exe --wsdl samples\AddService.wsdl --out generated
```

构建并运行生成的 C++ 示例客户端：

```powershell
cmake -S . -B cmake-build-debug -DWSDLCPPGEN_BUILD_CLIENT_DEMO=ON
cmake --build cmake-build-debug
.\cmake-build-debug\AddServiceClientDemo.exe
```

预期输出：

```text
3 + 5 = 8
```

GitHub Actions 产物
------------------

仓库的 GitHub Actions 会在每次 push 到 `main` / `master` 或创建 Pull Request 时运行构建。

构建成功后，可以在对应 workflow run 的 Artifacts 区域下载：

- `WsdlCppGen-windows`
- `WsdlCppGen-ubuntu`
- `WsdlCppGen-macos`

每个平台产物包含：

- GUI 可执行文件：`WsdlCppGen` / `WsdlCppGen.exe`
- CLI 可执行文件：`WsdlCppGenCli` / `WsdlCppGenCli.exe`
- GUI 运行资源：`font/`、`img/`

项目资源
--------

- `font/JetBrainsMono-Regular.ttf`：GUI 字体。
- `img/wsdlcppgen-brand.png`：用于报告或截图的品牌图。
- `img/img.png`：现有 GUI 截图。
