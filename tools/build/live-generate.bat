where /q cmake
if ERRORLEVEL 1 (
  set "PATH=C:\Program Files\CMake\bin;%PATH%"
)
pushd %~dp0..\..
rmdir /Q /S build
mkdir build
cd build
conan install .. --build missing -s build_type=Debug
cmake ..
popd
