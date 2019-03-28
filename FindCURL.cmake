find_package(PkgConfig)
pkg_check_modules(CURL REQUIRED libcurl)
set(CURL_LIBRARIES "-lcurl -lnghttp2 -lssl -lcrypto")