# for each "test/x.cpp", generate target "x"
file(GLOB_RECURSE all_tests *.cpp)
foreach(v ${all_tests})
    string(REGEX MATCH "test/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "test/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v} ${SRC_LIST} ${PROTOBUF_LIST})
    target_link_libraries(${target_name} PRIVATE /usr/local/tinyxml/libtinyxml.a /usr/local/lib/libprotobuf.a)
endforeach()
