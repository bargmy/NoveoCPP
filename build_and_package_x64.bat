@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem =========================
rem Config
rem =========================
set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

rem Default source dir is project root (your request).
set "SRC_DIR=%ROOT_DIR%"
set "BUILD_DIR=%ROOT_DIR%\build-x64"
set "DIST_DIR=%ROOT_DIR%\dist-x64"

set "MINGW_PREFIX=C:\msys64\mingw64"
set "QT5_DIR=%MINGW_PREFIX%\lib\cmake\Qt5"
set "WINDEPLOYQT=%MINGW_PREFIX%\bin\windeployqt.exe"
set "QT_PLUGINS_DIR=%MINGW_PREFIX%\share\qt5\plugins"

set "EXE_NAME=NoveoDesktop.exe"
set "BUILD_EXE=%BUILD_DIR%\%EXE_NAME%"
set "BUILD_EXE_RELEASE=%BUILD_DIR%\Release\%EXE_NAME%"

rem =========================
rem Sanity checks
rem =========================
rem If root has no CMakeLists, fallback to root\cpp automatically.
if not exist "%SRC_DIR%\CMakeLists.txt" (
  if exist "%ROOT_DIR%\cpp\CMakeLists.txt" (
    set "SRC_DIR=%ROOT_DIR%\cpp"
  )
)

if not exist "%SRC_DIR%\CMakeLists.txt" (
  echo [ERROR] CMakeLists.txt not found in "%SRC_DIR%"
  echo         Checked:
  echo         - "%ROOT_DIR%\CMakeLists.txt"
  echo         - "%ROOT_DIR%\cpp\CMakeLists.txt"
  exit /b 1
)

if not exist "%QT5_DIR%\Qt5Config.cmake" (
  echo [ERROR] Qt5Config.cmake not found:
  echo         "%QT5_DIR%\Qt5Config.cmake"
  echo         Check your Qt5 MinGW x64 install path.
  exit /b 1
)

if not exist "%WINDEPLOYQT%" (
  echo [ERROR] windeployqt not found:
  echo         "%WINDEPLOYQT%"
  echo         Install Qt5 tools package in MSYS2.
  exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cmake not found in PATH.
  exit /b 1
)

rem =========================
rem Configure + build
rem =========================
echo [0/4] Releasing old executable locks...
taskkill /f /im "%EXE_NAME%" >nul 2>nul
if exist "%BUILD_EXE%" (
  attrib -r "%BUILD_EXE%" >nul 2>nul
  del /f /q "%BUILD_EXE%" >nul 2>nul
)
if exist "%BUILD_EXE_RELEASE%" (
  attrib -r "%BUILD_EXE_RELEASE%" >nul 2>nul
  del /f /q "%BUILD_EXE_RELEASE%" >nul 2>nul
)

echo [1/4] Configuring...
echo       Source: "%SRC_DIR%"
cmake -G "MinGW Makefiles" ^
  -S "%SRC_DIR%" ^
  -B "%BUILD_DIR%" ^
  -DCMAKE_PREFIX_PATH="%MINGW_PREFIX%" ^
  -DQt5_DIR="%QT5_DIR%" ^
  -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  exit /b 1
)

echo [2/4] Building...
cmake --build "%BUILD_DIR%" -j
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

rem =========================
rem Prepare dist folder
rem =========================
echo [3/4] Preparing dist folder...
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"

if not exist "%BUILD_EXE%" (
  if exist "%BUILD_EXE_RELEASE%" (
    set "BUILD_EXE=%BUILD_EXE_RELEASE%"
  )
)

if not exist "%BUILD_EXE%" (
  echo [ERROR] Built EXE not found:
  echo         "%BUILD_EXE%"
  exit /b 1
)

copy /y "%BUILD_EXE%" "%DIST_DIR%\%EXE_NAME%" >nul

rem =========================
rem Deploy Qt + runtime DLLs
rem =========================
echo [4/4] Running windeployqt...
set "DEPLOY_WARN=0"
"%WINDEPLOYQT%" --release --force --no-angle --no-opengl-sw "%DIST_DIR%\%EXE_NAME%"
if errorlevel 1 (
  echo [WARN] windeployqt returned error. Continuing with manual plugin fallback...
  set "DEPLOY_WARN=1"
)

rem Ensure platform plugin exists (most common startup failure)
if not exist "%DIST_DIR%\platforms" mkdir "%DIST_DIR%\platforms"
if exist "%QT_PLUGINS_DIR%\platforms\qwindows.dll" (
  copy /y "%QT_PLUGINS_DIR%\platforms\qwindows.dll" "%DIST_DIR%\platforms\qwindows.dll" >nul
)

rem MinGW runtime DLLs fallback (usually needed)
for %%F in (libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll) do (
  if exist "%MINGW_PREFIX%\bin\%%F" copy /y "%MINGW_PREFIX%\bin\%%F" "%DIST_DIR%\%%F" >nul
)

if not exist "%DIST_DIR%\platforms\qwindows.dll" (
  echo [ERROR] qwindows.dll is missing. Qt platform plugin is not deployed.
  echo         Expected at: "%DIST_DIR%\platforms\qwindows.dll"
  exit /b 1
)

echo.
echo Done.
echo Output folder:
echo   "%DIST_DIR%"
if "%DEPLOY_WARN%"=="1" echo Note: windeployqt had warnings, but required platform plugin was copied.
echo.
echo Run this EXE:
echo   "%DIST_DIR%\%EXE_NAME%"
exit /b 0
