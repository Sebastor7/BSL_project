cmake_minimum_required(VERSION 3.10)
project(BSL_project)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUS_CPP REQUIRED sdbus-c++)

add_executable(nus_ble_client src/main.cpp)
target_include_directories(nus_ble_client PRIVATE ${SDBUS_CPP_INCLUDE_DIRS})
target_link_libraries(nus_ble_client PRIVATE ${SDBUS_CPP_LIBRARIES})
