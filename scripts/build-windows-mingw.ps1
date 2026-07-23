[CmdletBinding()]
param(
    [string]$QtRoot = "",
    [string]$MingwRoot = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDir = Join-Path $ProjectRoot "build-windows-release"
$DistRoot = Join-Path $ProjectRoot "dist"
$DistDir = Join-Path $DistRoot "MachineColorDesigner-Windows-x64"
$ZipPath = Join-Path $DistRoot "MachineColorDesigner-Windows-x64.zip"

function Resolve-LatestQtMinGW {
    $candidates = Get-ChildItem -Path "C:\Qt\6.*\mingw_64" -Directory -ErrorAction SilentlyContinue
    if (-not $candidates) {
        return $null
    }

    return ($candidates |
        Sort-Object { [version]$_.Parent.Name } -Descending |
        Select-Object -First 1).FullName
}

function Resolve-LatestMinGWToolchain {
    $toolsDir = "C:\Qt\Tools"
    if (-not (Test-Path $toolsDir)) {
        return $null
    }

    $candidates = Get-ChildItem -Path $toolsDir -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^mingw[0-9]+_64$' }

    if (-not $candidates) {
        return $null
    }

    return ($candidates |
        Sort-Object Name -Descending |
        Select-Object -First 1).FullName
}

if ([string]::IsNullOrWhiteSpace($QtRoot)) {
    $QtRoot = Resolve-LatestQtMinGW
}
if ([string]::IsNullOrWhiteSpace($MingwRoot)) {
    $MingwRoot = Resolve-LatestMinGWToolchain
}

if ([string]::IsNullOrWhiteSpace($QtRoot) -or -not (Test-Path $QtRoot)) {
    throw "未找到 Qt MinGW。请安装 Qt 6.9 或更高版本的 mingw_64 组件，或通过 -QtRoot 指定路径。"
}
if ([string]::IsNullOrWhiteSpace($MingwRoot) -or -not (Test-Path $MingwRoot)) {
    throw "未找到 Qt 自带的 MinGW 工具链。请在 Qt Maintenance Tool 中安装对应的 MinGW 64-bit 组件，或通过 -MingwRoot 指定路径。"
}

$QtCMake = Join-Path $QtRoot "bin\qt-cmake.bat"
$WinDeployQt = Join-Path $QtRoot "bin\windeployqt.exe"
$CMakeExe = "C:\Qt\Tools\CMake_64\bin\cmake.exe"
$NinjaDir = "C:\Qt\Tools\Ninja"
$NinjaExe = Join-Path $NinjaDir "ninja.exe"

foreach ($requiredFile in @($QtCMake, $WinDeployQt, $CMakeExe, $NinjaExe)) {
    if (-not (Test-Path $requiredFile)) {
        throw "缺少构建工具：$requiredFile。请在 Qt Maintenance Tool 中安装 CMake、Ninja 和 MinGW。"
    }
}

$env:Path = @(
    (Join-Path $QtRoot "bin")
    (Join-Path $MingwRoot "bin")
    (Split-Path $CMakeExe)
    $NinjaDir
    $env:Path
) -join ";"

Write-Host "项目目录 : $ProjectRoot"
Write-Host "Qt        : $QtRoot"
Write-Host "MinGW     : $MingwRoot"
Write-Host "构建目录 : $BuildDir"
Write-Host "发布目录 : $DistDir"

if ($Clean) {
    Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item $DistRoot -Recurse -Force -ErrorAction SilentlyContinue
}

New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

Write-Host "`n[1/5] 配置 Release 构建..."
& $QtCMake `
    -S $ProjectRoot `
    -B $BuildDir `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DMCD_STATIC_WINDOWS=OFF
if ($LASTEXITCODE -ne 0) {
    throw "CMake 配置失败，退出码：$LASTEXITCODE"
}

Write-Host "`n[2/5] 编译..."
& $CMakeExe --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) {
    throw "编译失败，退出码：$LASTEXITCODE"
}

$BuiltExe = Join-Path $BuildDir "MachineColorDesigner.exe"
if (-not (Test-Path $BuiltExe)) {
    throw "未找到编译结果：$BuiltExe"
}

Write-Host "`n[3/5] 创建可分发文件夹..."
Remove-Item $DistDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
$DeployedExe = Join-Path $DistDir "MachineColorDesigner.exe"
Copy-Item $BuiltExe $DeployedExe -Force

# 不要添加 --no-opengl-sw：如当前 Qt 套件提供软件 OpenGL，
# windeployqt 可将相关运行库一起复制，能提高部分 Windows 电脑的兼容性。
& $WinDeployQt `
    --release `
    --compiler-runtime `
    --no-translations `
    --force `
    --verbose 1 `
    $DeployedExe
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt 部署失败，退出码：$LASTEXITCODE"
}

$RunGuide = @"
Machine Color Designer - Windows x64

运行方法：
1. 保持本文件夹内的所有 DLL 和子目录不变。
2. 双击 MachineColorDesigner.exe。
3. 不需要在目标电脑安装 Qt 或 MinGW。

系统建议：
- 64 位 Windows 10（1809 或更高）/ Windows 11
- 显卡驱动支持 OpenGL 3.3

请不要只复制 EXE；platforms、styles 等目录和 DLL 都属于程序运行环境。
"@
Set-Content -Path (Join-Path $DistDir "README_运行说明.txt") -Value $RunGuide -Encoding UTF8

$RequiredDeploymentFiles = @(
    $DeployedExe,
    (Join-Path $DistDir "Qt6Core.dll"),
    (Join-Path $DistDir "Qt6Gui.dll"),
    (Join-Path $DistDir "Qt6Widgets.dll"),
    (Join-Path $DistDir "platforms\qwindows.dll")
)
foreach ($requiredFile in $RequiredDeploymentFiles) {
    if (-not (Test-Path $requiredFile)) {
        throw "部署结果不完整，缺少：$requiredFile"
    }
}

Write-Host "`n[4/5] 生成 ZIP..."
New-Item -ItemType Directory -Path $DistRoot -Force | Out-Null
Remove-Item $ZipPath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path $DistDir -DestinationPath $ZipPath -CompressionLevel Optimal

Write-Host "`n[5/5] 完成。"
$ExeSize = [math]::Round((Get-Item $DeployedExe).Length / 1MB, 2)
$FolderSize = [math]::Round(((Get-ChildItem $DistDir -Recurse -File | Measure-Object Length -Sum).Sum) / 1MB, 2)
$ZipSize = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)

Write-Host "EXE 大小    : $ExeSize MB"
Write-Host "文件夹大小 : $FolderSize MB"
Write-Host "ZIP 大小    : $ZipSize MB"
Write-Host "发布文件夹 : $DistDir"
Write-Host "压缩包     : $ZipPath"
Write-Host "`n请将整个发布文件夹或 ZIP 发送给其他 Windows 用户。"
