@echo off
rem Build, run and clean the seam carving project on Windows.
rem
rem Double-click (or run with no arguments) for a menu, or pass a command:
rem   run.bat demo [image]                  interactive demo (seam_demo)
rem   run.bat compare [image width height]  all algorithms compared (seam_carving)
rem   run.bat dp [image width height]       DP only + seam visualisation (dp_seam)
rem   run.bat mincut                        max-flow / min-cut test (mincut_demo)
rem   run.bat build                         build everything
rem   run.bat clean                         delete compiled files, keep downloaded OpenCV
rem   run.bat clean all                     delete the whole build folder
rem Missing image/size arguments are asked for. Results go to output\.
rem The first build downloads and compiles OpenCV, which takes several minutes.

setlocal EnableExtensions
cd /d "%~dp0"

set "BUILD_DIR=build"
set "CONFIG=Release"
set "BIN=%BUILD_DIR%\%CONFIG%"

if "%~1"=="" goto menu

call :find_cmake || exit /b 1
if /i "%~1"=="demo"    call :cmd_demo "%~2" & goto done
if /i "%~1"=="compare" call :cmd_compare "%~2" "%~3" "%~4" & goto done
if /i "%~1"=="dp"      call :cmd_dp "%~2" "%~3" "%~4" & goto done
if /i "%~1"=="mincut"  call :cmd_mincut & goto done
if /i "%~1"=="build"   call :cmd_build & goto done
if /i "%~1"=="clean"   call :cmd_clean "%~2" & goto done
if /i "%~1"=="help"    call :usage & goto done
echo Unknown command "%~1".
echo.
call :usage
exit /b 1
:done
rem Explicit code: a bare "exit /b" does not reliably pass errors to the caller.
exit /b %errorlevel%

rem ===========================================================================
rem Menu
rem ===========================================================================
:menu
call :find_cmake || (pause & exit /b 1)
:menu_loop
cls
echo ==================================================
echo   Seam Carving - Assignment 2
echo ==================================================
echo   1  Interactive demo              (seam_demo)
echo   2  Compare all algorithms        (seam_carving)
echo   3  DP only, seams visualised     (dp_seam)
echo   4  Max-flow / min-cut test       (mincut_demo)
echo.
echo   5  Build
echo   6  Clean   - keeps the downloaded OpenCV
echo   7  Clean everything - OpenCV is rebuilt next time
echo.
echo   0  Exit
echo.
set "CHOICE=none"
set /p "CHOICE=Choose an option: "
set "CHOICE=%CHOICE:"=%"
echo.
if "%CHOICE%"=="0" exit /b 0
if "%CHOICE%"=="1" call :cmd_demo ""
if "%CHOICE%"=="2" call :cmd_compare "" "" ""
if "%CHOICE%"=="3" call :cmd_dp "" "" ""
if "%CHOICE%"=="4" call :cmd_mincut
if "%CHOICE%"=="5" call :cmd_build
if "%CHOICE%"=="6" call :cmd_clean ""
if "%CHOICE%"=="7" call :confirm_clean_all
echo.
pause
goto menu_loop

:confirm_clean_all
set "ANSWER=n"
set /p "ANSWER=Delete build\ including OpenCV? Rebuilding takes several minutes. [y/N]: "
set "ANSWER=%ANSWER:"=%"
if /i "%ANSWER%"=="y" call :cmd_clean all
exit /b 0

rem ===========================================================================
rem Commands
rem ===========================================================================
:cmd_demo
call :do_build || exit /b 1
echo === Starting the interactive demo ===
if "%~1"=="" (
  start "" "%BIN%\seam_demo.exe"
) else (
  start "" "%BIN%\seam_demo.exe" "%~1"
)
exit /b 0

:cmd_compare
rem Runs greedy, greedy (all starts), DP and graph cut on the same image.
set "IMG=%~1"
set "W=%~2"
set "H=%~3"
if "%IMG%"=="" (
  echo Graph cut is slow: about 15 s per seam on a 651x717 image.
  echo Keep the number of removed seams small.
  call :ask_image_and_size "images\tower.png" 646 717
)
call :check_image || exit /b 1
call :do_build || exit /b 1
call :output_stem
echo === seam_carving "%IMG%" output\%STEM% %W% %H% ===
"%BIN%\seam_carving.exe" "%IMG%" "output\%STEM%" %W% %H%
if errorlevel 1 exit /b 1
echo.
echo Results: output\%STEM%_^<algorithm^>.png and output\%STEM%_^<algorithm^>_seams.png
exit /b 0

:cmd_dp
set "IMG=%~1"
set "W=%~2"
set "H=%~3"
if "%IMG%"=="" call :ask_image_and_size "images\tower.jpg" 1200 800
call :check_image || exit /b 1
call :do_build || exit /b 1
call :output_stem
echo === dp_seam "%IMG%" output\%STEM%_dp.png %W% %H% ===
"%BIN%\dp_seam.exe" "%IMG%" "output\%STEM%_dp.png" %W% %H%
exit /b %errorlevel%

:cmd_mincut
call :do_build || exit /b 1
echo === mincut_demo ===
"%BIN%\mincut_demo.exe"
exit /b %errorlevel%

:cmd_build
call :do_build || exit /b 1
echo.
echo Done. Programs are in %BIN%\
exit /b 0

:cmd_clean
if /i "%~1"=="all" goto clean_all
if not exist "%BUILD_DIR%\CMakeCache.txt" (
  echo Nothing to clean.
  exit /b 0
)
echo === Removing compiled files - the downloaded OpenCV is kept ===
"%CMAKE%" --build "%BUILD_DIR%" --config %CONFIG% --target clean
exit /b %errorlevel%
:clean_all
if not exist "%BUILD_DIR%" (
  echo Nothing to clean.
  exit /b 0
)
echo === Deleting %BUILD_DIR%\ - OpenCV is downloaded and rebuilt on the next build ===
rmdir /s /q "%BUILD_DIR%"
exit /b 0

rem ===========================================================================
rem Helpers
rem ===========================================================================
:do_build
if not exist "%BUILD_DIR%\CMakeCache.txt" (
  echo === Configuring - the first time this downloads and builds OpenCV, which takes a few minutes ===
  "%CMAKE%" -S . -B "%BUILD_DIR%"
  if errorlevel 1 goto build_failed
)
echo === Building %CONFIG% ===
"%CMAKE%" --build "%BUILD_DIR%" --config %CONFIG% --parallel
if errorlevel 1 goto build_failed
exit /b 0
:build_failed
echo.
echo Build failed - see the messages above.
exit /b 1

rem Asks for image, width and height; Enter keeps the default shown.
:ask_image_and_size
set "IMG=%~1"
set "W=%~2"
set "H=%~3"
set /p "IMG=Image [%IMG%]: "
set "IMG=%IMG:"=%"
set /p "W=Target width [%W%]: "
set /p "H=Target height [%H%]: "
exit /b 0

:check_image
if exist "%IMG%" exit /b 0
echo Image not found: %IMG%
exit /b 1

rem output\ is where results go; the programs do not create it themselves.
:output_stem
if not exist output mkdir output
for %%F in ("%IMG%") do set "STEM=%%~nF"
exit /b 0

:find_cmake
where cmake >nul 2>nul
if not errorlevel 1 (
  set "CMAKE=cmake"
  exit /b 0
)
if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
  set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
  exit /b 0
)
echo CMake was not found. Install it from https://cmake.org/download/ and try again.
exit /b 1

:usage
echo Usage: run.bat [command]       (no command: show a menu)
echo   demo [image]                 Interactive demo
echo   compare [image width height] Compare all algorithms
echo   dp [image width height]      DP only, with seam visualisation
echo   mincut                       Max-flow / min-cut test
echo   build                        Build everything
echo   clean                        Delete compiled files, keep downloaded OpenCV
echo   clean all                    Delete the whole build folder
exit /b 0
