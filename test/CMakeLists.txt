# for each "test/x.cpp", generate target "x"
file(GLOB_RECURSE all_tests *.cpp)
foreach(v ${all_tests})
    string(REGEX MATCH "test/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "test/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v} ${SRC_LIST} ${PROTOBUF_LIST})
    target_link_libraries(${target_name} PRIVATE ${PROTOBUF_LIBRARIES} ${CMAKE_SOURCE_DIR}/tinyxml/libtinyxml.a)
    target_compile_options(${target_name} PRIVATE ${PROTOBUF_CFLAGS_OTHER})
    target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/include ${PROTOBUF_INCLUDE_DIRS})
endforeach()


add_executable(coroutine test_coroutine.c ../src/coroutine/coroutine.c)
