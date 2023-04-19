cmake_minimum_required(VERSION 3.13)
project(rgbwindows)

add_executable(rgbwindows main.cpp WindowsThemeColorApi.cpp WindowsThemeColorApi.h rgbwindows.exe.manifest resources.rc)
