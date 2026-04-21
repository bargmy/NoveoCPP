cmake -G "MinGW Makefiles" -S . -B build-x64 ^
  -DCMAKE_PREFIX_PATH=C:/msys64/mingw64 ^
  -DQt5_DIR=C:/msys64/mingw64/lib/cmake/Qt5
cmake --build build-x64 -j
