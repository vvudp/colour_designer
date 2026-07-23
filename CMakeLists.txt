cmake_minimum_required(VERSION 3.24)

project(MachineColorDesigner
    VERSION 1.3.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(MCD_STATIC_WINDOWS "Create a self-contained MinGW Windows executable" OFF)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
)

qt_add_executable(MachineColorDesigner
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/PreviewGLWidget.cpp
    src/PreviewGLWidget.h
    src/ColorPickerWidget.cpp
    src/ColorPickerWidget.h
    src/ControlPanel.cpp
    src/ControlPanel.h
    src/RecolorState.cpp
    src/RecolorState.h
    src/PaletteSerializer.cpp
    src/PaletteSerializer.h
    src/ImageRecolorer.cpp
    src/ImageRecolorer.h
    src/ExportComposer.cpp
    src/ExportComposer.h
    resources.qrc
)

target_link_libraries(MachineColorDesigner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

if(MSVC)
    target_compile_options(MachineColorDesigner PRIVATE /W4 /permissive-)
else()
    target_compile_options(MachineColorDesigner PRIVATE -Wall -Wextra -Wpedantic)
endif()

# The MSYS2 qt6-static package already provides static Qt libraries and static
# Qt plugins. These options also link the MinGW C/C++ runtime into the EXE so
# libgcc_s_seh-1.dll, libstdc++-6.dll and libwinpthread-1.dll are not needed.
if(WIN32 AND MINGW AND MCD_STATIC_WINDOWS)
    target_compile_options(MachineColorDesigner PRIVATE
        -ffunction-sections
        -fdata-sections
    )
    target_link_options(MachineColorDesigner PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        -Wl,--gc-sections
    )
endif()

set_target_properties(MachineColorDesigner PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS MachineColorDesigner
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

