# cmake/MyCustomSettings.cmake

# NOTICE: This is a template file for necessary PATH variables required for linux cross-compiling and VCPKG
# Copy this file, rename to `local-env.cmake` and update with the appropriate local
# environment paths. *Do not check the local-env.cmake file into git*

# Check `linux-cross-compile` folder for more information

# LLVM MINGW used for directxtk linux custom port (eg /opt/llvm-mingw/bin)
set(LLVM_MINGW_BIN "/path/to/llvm-mingw/bin" CACHE PATH "")

# XWIN SYSROOT holding the necessary Windows SDK/CRT data for cross-compiling (eg ~/.xwin-out/)
# This will be set when using xwin from CLIB-NG guide, should contain `sdk` and `crt` within
set(XWIN_SYSROOT "/path/to/xwin-out/" CACHE PATH "")

# CLIB-NG path (eg ~/SKSEMods/CommonLibSSE-NG/)
set(CLIB_PATH "/path/to/CommonLibSSE-NG/" CACHE PATH "")

# VCPKG ROOT (eg ~/.local/share/vcpkg)
set(ENV{VCPKG_ROOT} "/path/to/vcpkg/")
