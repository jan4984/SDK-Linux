find_package(PkgConfig)
pkg_check_modules(OPUS REQUIRED opus)
#overwrite
set(OPUS_INCLUDE_DIRS "/rpxc/sysroot/usr/include")

