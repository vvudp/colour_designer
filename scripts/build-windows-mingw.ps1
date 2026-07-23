$ErrorActionPreference = "Stop"
cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/mingw_64"
cmake --build build-windows --parallel
Write-Host "Built executable: build-windows/MachineColorDesigner.exe"
Write-Host "Run windeployqt on the executable before distributing it."
