@echo off
setlocal
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake was not found. Install Visual Studio 2022 with Desktop development with C++ and CMake tools.
  exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --target Live32Channel_VST3
if errorlevel 1 exit /b 1

echo.
echo Build complete.
echo Look in: build\Live32Channel_artefacts\Release\VST3\
endlocal
