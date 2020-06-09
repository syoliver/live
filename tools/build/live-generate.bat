where /q cmake
if ERRORLEVEL 1 (
  set "PATH=C:\Program Files\CMake\bin;%PATH%"
)
pushd %~dp0..\..
set root=%CD%
set "profile=%root%/tools/conan/profiles/msvc"
pushd third-parties
conan create rocksdb rocksdb/6.8.1@ --build missing --build rocksdb --test-folder=None -s build_type=Debug --profile=%profile%
popd

rmdir /Q /S build
mkdir build
cd build
conan install .. --build missing -s build_type=Debug --profile=%profile%
cmake ..
popd
