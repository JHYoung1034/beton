#列举需要编译的文件
file(GLOB SERVER_SRC_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

#生成可执行文件
add_executable(server ${SERVER_SRC_LIST})

#设置需要链接的库文件
set(LINK_LIBRARIES ${BT_LINK_LIBRARIES})
#设置编译宏
set(COMPILE_DEFINITIONS ${BT_COMPILE_DEFINITIONS})

#设置编译宏，编译选项，编译链接库，头文件路径
target_compile_definitions(server PUBLIC ${COMPILE_DEFINITIONS})
target_compile_options(server PUBLIC ${COMPILE_OPTIONS_DEFAULT})
target_link_libraries(server PUBLIC ${LINK_LIBRARIES})
target_include_directories(server
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")