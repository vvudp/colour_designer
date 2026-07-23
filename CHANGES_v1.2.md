# v1.2.0：带参数 PNG 导出

## 新增

- 导出图片右侧自动生成配色记录面板。
- 记录上部机壳和下部机壳的 HEX、RGB、HSV、HSL。
- 记录哑光程度、高光保留、阴影深度和整体明度。
- 记录本地导出时间。
- 默认文件名包含上下部 HEX 色值。
- PNG 内嵌 `MachineColorDesigner.State` JSON 元数据。

## 文件变更

- 新增 `src/ExportComposer.cpp`。
- 新增 `src/ExportComposer.h`。
- 更新 `src/MainWindow.cpp` 的导出流程。
- 更新 `src/ControlPanel.cpp` 的导出按钮文字。
- 项目版本升级为 1.2.0。
