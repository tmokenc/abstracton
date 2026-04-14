cmake_minimum_required(VERSION 4.3)

find_package(jsoncpp QUIET CONFIG)
if (NOT jsoncpp_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

add_library(parsers STATIC
    DodoParser.cpp
)

target_include_directories(parsers PUBLIC mata)
target_include_directories(parsers PUBLIC ${PROJECT_SOURCE_DIR}/benchmarks)
target_include_directories(parsers PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(parsers abstracton)
if (jsoncpp_FOUND)
    if (TARGET jsoncpp_lib)
        target_link_libraries(parsers jsoncpp_lib)
    else()
        target_link_libraries(parsers jsoncpp)
    endif()
else()
    target_include_directories(parsers PUBLIC ${JSONCPP_INCLUDE_DIRS})
    target_link_directories(parsers PUBLIC ${JSONCPP_LIBRARY_DIRS})
    target_link_libraries(parsers ${JSONCPP_LIBRARIES})
endif()

add_executable(solve_dodo solve_dodo.cpp)
target_link_libraries(solve_dodo parsers)
target_link_libraries(solve_dodo mata)
target_link_libraries(solve_dodo abstracton)

add_executable(solve_dodo_tree solve_dodo_tree.cpp)
target_link_libraries(solve_dodo_tree parsers)
target_link_libraries(solve_dodo_tree mata)
target_link_libraries(solve_dodo_tree abstracton)
