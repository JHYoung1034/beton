###################### cmake 环境变量设置 #####################################
cmake_minimum_required(VERSION 3.1.3)
project(Beton LANGUAGES C CXX)
#默认使用C++11
set(CMAKE_CXX_STANDARD 11)
#默认使用debug模式
set(CMAKE_BUILD_TYPE "Debug")
#默认把verbose打开，编译可看到更详尽的信息
set(CMAKE_VERBOSE_MAKEFILE ON)
#暂时不支持非unix或类Unix平台
if (NOT UNIX)
    message("Do not support not unix platform!")
    return()
endif()
message("platform: " ${CMAKE_SYSTEM_NAME})
#设置安装路径 /usr/local
set(CMAKE_INSTALL_PREFIX "/usr/local")
#编译生成路径
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/output/${CMAKE_BUILD_TYPE})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})
set(LIBRARY_OUTPUT_PATH ${OUTPUT_DIR})

############################ 编译宏设置 #######################################
#自定义宏


#设置git版本信息：hash,branch,buildtime
set(GIT_HASH "Git_Unkown_commit")
set(GIT_BRANCH "Git_Unkown_branch")
set(BUILD_TIME "")

string(TIMESTAMP BUILD_TIME "%Y-%m-%dT%H:%M:%S")

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  execute_process(
    COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.ini
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  @ONLY)

############################ 添加自定以查找某些第三方库的cmake ##################
function(update_cached name value)
  set("${name}" "${value}" CACHE INTERNAL "*** Internal ***" FORCE)
endfunction()

function(update_cached_list name)
  set(_tmp_list "${${name}}")
  list(APPEND _tmp_list "${ARGN}")
  list(REMOVE_DUPLICATES _tmp_list)
  update_cached(${name} "${_tmp_list}")
endfunction()

############################ 编译选项设置 ######################################
set(COMPILE_OPTIONS_DEFAULT
    "-g"
    "-fPIC"
    "-Wall;-Wextra"
    "-Wno-unused-function;-Wno-unused-parameter;-Wno-unused-variable"
    "-Wno-error=extra;-Wno-error=missing-field-initializers;-Wno-error=type-limits")
  
update_cached_list(BT_LINK_LIBRARIES pthread)

############################ 添加编译子路径，子路径会继承父路径的所有环境变量 #####
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(tools)
add_subdirectory(apps)
#主程序所在目录
add_subdirectory(server)

#测试
# add_subdirectory(tests)

############################ 拷贝需要安装的文件 ################################
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/config.ini"  DESTINATION ${EXECUTABLE_OUTPUT_PATH})