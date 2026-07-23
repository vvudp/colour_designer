# GitHub Actions: Windows 单文件 EXE

本项目使用 GitHub Actions 的 Windows runner、MSYS2 UCRT64 和 `qt6-static` 生成单一的 `MachineColorDesigner.exe`。

## 自动构建

将项目推送到 `main` 分支后，工作流会自动运行。也可以在 GitHub 的 Actions 页面手动运行 `Build Windows single EXE`。

构建产物位于工作流运行详情底部的 Artifacts：

- `MachineColorDesigner-Windows-x64-single-exe`

GitHub 会把 Artifact 包装成 ZIP；解压后只有一个 `MachineColorDesigner.exe`。

## 发布版本

创建并推送 `v` 开头的标签后，例如：

```bash
git tag v1.1.0
git push origin v1.1.0
```

工作流会把 EXE 同时上传到对应的 GitHub Release。

## 单文件边界

EXE 内已经包含 Qt、MinGW C/C++ 运行库、图片、蒙版、着色器、QSS 和默认配色 JSON。它仍会导入 Windows 自带的系统 DLL，例如 `KERNEL32.dll`、`USER32.dll` 和 `OPENGL32.dll`；这些属于操作系统组件，不需要随程序分发。
