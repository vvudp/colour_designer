# Machine Color Designer

这是一个使用 **C++20 + Qt 6 Widgets + QOpenGLWidget** 编写的固定视角机器配色工具。左侧显示设备实时效果，右侧提供上/下壳体选择、HSV 调色盘、HEX/RGB/HSV 数值输入和哑光光影参数。

## 已实现功能

- 上部机壳和下部机壳独立着色。
- 下部主体、底座和后部浅色罩壳已经合并为一个完整的 `lower_mask.png`。
- 脚轮、显示屏、急停按钮、旋钮、接口和文字保持原始颜色。
- GPU GLSL 实时重新着色。
- 在线性 RGB 空间中保留原始曲面明暗、阴影和柔和高光。
- 哑光程度、高光保留、阴影深度和整体明度调节。
- 滚轮缩放、鼠标拖动、双击恢复适配。
- 按住按钮查看原始图片。
- 撤销、重做、保存/载入 JSON 配色方案。
- 导出带参数记录的 PNG：左侧保留原始 717 × 2048 渲染结果，右侧自动附加颜色与材质信息面板。
- PNG 内部同时写入完整 JSON 元数据，便于归档、核对和后续程序自动读取。
- 默认文件名自动包含上下部 HEX 色值，例如 `machine_U-D0D4D7_L-68A6C4.png`。

## 带参数 PNG 导出

点击右侧的“导出带参数 PNG”后，程序会生成一张组合图片：

- 左侧：完整机器配色效果，仍以原始 717 × 2048 分辨率渲染。
- 右侧：上部和下部颜色块，以及 HEX、RGB、HSV、HSL 数值。
- 右侧：哑光程度、高光保留、阴影深度、整体明度。
- 底部：导出时间。

除了肉眼可见的信息面板，PNG 的文本块中还会保存键名：

```text
MachineColorDesigner.State
```

该字段的内容是紧凑 JSON，包括颜色、材质参数、应用版本、导出时间和原始渲染尺寸。Linux 下可以使用 `exiftool` 检查：

```bash
exiftool -MachineColorDesigner.State machine_U-D0D4D7_L-68A6C4.png
```

或者查看 PNG 的全部文本信息：

```bash
identify -verbose machine_U-D0D4D7_L-68A6C4.png
```

信息面板不会覆盖机器图像，而是扩展画布宽度，因此机器造型、比例和细节不会被压缩。

## Fedora 编译

```bash
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel mesa-libGL-devel

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/MachineColorDesigner
```

也可以运行：

```bash
./scripts/build-fedora.sh
./build/MachineColorDesigner
```

## Windows 编译

建议安装 Qt 6 的 MinGW 64-bit 套件、CMake 和 Ninja。修改 `scripts/build-windows-mingw.ps1` 中的 Qt 路径后执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-windows-mingw.ps1
C:\Qt\6.8.0\mingw_64\bin\windeployqt.exe build-windows\MachineColorDesigner.exe
```

## 蒙版资源

当前版本只使用两个颜色蒙版：

- `assets/upper_mask.png`：上部壳体。
- `assets/lower_mask.png`：下部主体、底座和后部罩壳的合并区域。

`lower_body_mask.png` 与 `lower_base_mask.png` 已被移除。上、下蒙版不是分别独立模糊，而是通过联合羽化算法生成，使共享边界满足：

```text
upper_alpha + lower_alpha = housing_alpha
```

因此上、下部分的结构交界处不会再出现漏色灰边或双重染色。

调试图 `assets/mask_debug.png` 中：

- 蓝色：上部机壳。
- 橙色：完整的下部区域。

重新生成蒙版：

```bash
python3 -m pip install pillow opencv-python numpy
python3 tools/generate_masks.py \
  --input assets/machine_original.png \
  --output assets
```

生成器中的轮廓坐标针对当前 717 × 2048 效果图。如果替换设备图片，需要重新描绘轮廓。

## 着色算法

预览和导出使用相同的核心逻辑：

1. 将原图和目标颜色从 sRGB 转为线性 RGB。
2. 分别计算上部和下部区域的参考中位亮度。
3. 用原图亮度生成曲面光照系数。
4. 将目标颜色与光照系数相乘。
5. 对高光峰值进行哑光压缩，同时保留柔和中性高光。
6. 根据抗锯齿蒙版与原图混合。
7. 转回 sRGB 显示或导出。

## 主要文件

```text
src/PreviewGLWidget.*   OpenGL 预览、缩放和拖动
src/ColorPickerWidget.* 自定义 HSV 调色盘
src/ControlPanel.*      区域、材质、历史和操作面板
src/ImageRecolorer.*    原始分辨率 CPU 渲染
src/ExportComposer.*    导出信息面板、文件名和 PNG JSON 元数据
shaders/recolor.frag    GPU 实时着色算法
assets/upper_mask.png   上部蒙版
assets/lower_mask.png   合并后的下部蒙版
tools/generate_masks.py 蒙版生成与联合羽化
```

## Windows 本地构建与便携部署

参见 [WINDOWS_LOCAL_BUILD.md](WINDOWS_LOCAL_BUILD.md)。可直接运行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\build-windows-mingw.ps1 -Clean
```

输出为 `dist\MachineColorDesigner-Windows-x64` 文件夹和同名 ZIP。
