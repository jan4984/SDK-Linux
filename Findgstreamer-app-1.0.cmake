find_package(PkgConfig)
pkg_check_modules(GSTAPP REQUIRED gstreamer-app-1.0)
set(GSTAPP_LDFLAGS "-lgstapp-1.0 -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgmodule-2.0 -lffi -lpcre")