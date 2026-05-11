@rem Pass argument s2 or s3 or c3 to change the target system from default esp32
@set SCRIPT_PATH=.\DICMFramework\tools\scripts
@pushd %SCRIPT_PATH%
@set SCRIPT_ABS_PATH=%CD%
@popd
@rem Add to path
@set PATH=%SCRIPT_ABS_PATH%;%PATH%
@rem Set build variables
@set CMAKE_TOOLCHAIN_FILE=%IDF_PATH%\tools\cmake\toolchain-esp32%1.cmake
@set IDF_TARGET=esp32%1
@echo Setting IDF_TARGET to %IDF_TARGET%
@echo Setting CMAKE_TOOLCHAIN_FILE to %CMAKE_TOOLCHAIN_FILE%
