cmake_minimum_required(VERSION 3.13)
project(rgbwindows VERSION 1.0.0)

add_executable(rgbwindows main.cpp WindowsThemeColorApi.cpp WindowsThemeColorApi.h rgbwindows.exe.manifest resources.rc)

install(TARGETS rgbwindows)

set(CPACK_GENERATOR ZIP)
include(CPack)
