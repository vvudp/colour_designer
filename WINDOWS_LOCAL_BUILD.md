# Windows 本地编译与便携文件夹部署

## 推荐工具链

使用 Qt Online Installer 安装：

- Qt 6.9 或更高版本的 `MinGW 64-bit` 套件
- 对应版本的 MinGW 64-bit 工具链
- CMake
- Ninja
- Qt Creator（可选）

项目使用了 `QImage::flipped()`，因此不建议使用 Qt 6.8 或更早版本。若必须使用 Qt 6.8，需要把 `src/PreviewGLWidget.cpp` 中的 `flipped(Qt::Vertical)` 改回 `mirrored(false, true)`。

## 一键构建

在项目根目录打开 PowerShell：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\build-windows-mingw.ps1 -Clean
```

脚本会自动：

1. 查找 `C:\Qt` 中最新的 Qt MinGW 套件；
2. 生成 Release 构建；
3. 调用 `windeployqt` 收集 Qt DLL、Windows 平台插件和 MinGW 运行库；
4. 输出可直接复制到其他电脑的文件夹；
5. 生成 ZIP 压缩包。

输出位置：

```text
dist\MachineColorDesigner-Windows-x64\
dist\MachineColorDesigner-Windows-x64.zip
```

## 手动指定 Qt 路径

```powershell
.\scripts\build-windows-mingw.ps1 `
  -QtRoot "C:\Qt\6.11.0\mingw_64" `
  -MingwRoot "C:\Qt\Tools\mingw1310_64" `
  -Clean
```

具体版本目录以本机 `C:\Qt` 下实际安装内容为准。

## 分发规则

不要只发送 `MachineColorDesigner.exe`。必须发送整个：

```text
MachineColorDesigner-Windows-x64
```

文件夹，其中包括 Qt DLL、MinGW 运行库以及：

```text
platforms\qwindows.dll
```

图片、蒙版、着色器、QSS 和默认配色已经通过 Qt Resource System 编入 EXE，不需要单独复制 `assets`、`shaders` 或 `presets` 目录。

## 目标电脑

目标电脑无需安装 Qt、CMake、MinGW 或 Visual Studio。当前程序使用 OpenGL 3.3 着色器，目标电脑需有可用的 OpenGL 3.3 显卡驱动。
