# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# -----------------------------------------------------------------------------
# 产品变体选择（必须在 include(project.cmake) 之前设置：
# IDF 在 include 时就会读取 SDKCONFIG / SDKCONFIG_DEFAULTS 来推断芯片目标）
#
#   P4  ：idf.py -B build_p4  -DPRODUCT=p4  build
#   P4X ：idf.py -B build_p4x -DPRODUCT=p4x build
#
# 变体差异（芯片 revision / CPU 主频 / 产品标识）分别写在
#   sdkconfig.defaults.p4
#   sdkconfig.defaults.p4x
# 两个变体使用各自的 sdkconfig.p4 / sdkconfig.p4x，互不覆盖。
# -----------------------------------------------------------------------------
set(PRODUCT "p4" CACHE STRING "Product variant: p4 (ESP32-P4 rev<3.0) or p4x (ESP32-P4 rev>=3.0)")
set_property(CACHE PRODUCT PROPERTY STRINGS p4 p4x)

if(PRODUCT STREQUAL "p4")
    set(_product_defaults "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults.p4")
elseif(PRODUCT STREQUAL "p4x")
    set(_product_defaults "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults.p4x")
else()
    message(FATAL_ERROR "Invalid PRODUCT '${PRODUCT}'. Expected 'p4' or 'p4x'.")
endif()

set(_sdkconfig_defaults "")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
    list(APPEND _sdkconfig_defaults "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
endif()
list(APPEND _sdkconfig_defaults "${_product_defaults}")

set(SDKCONFIG_DEFAULTS "${_sdkconfig_defaults}")
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.${PRODUCT}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(Unit-PoE-P4-UserDemo)
