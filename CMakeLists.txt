cmake_minimum_required(VERSION 3.16)
project(numero LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX d)
#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(numero)

set(source_files
    "src/numero/numero.cpp"
)

target_sources(numero PRIVATE ${source_files})
target_include_directories(numero PUBLIC include)

find_package(Boost REQUIRED COMPONENTS program_options regex)
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(numero ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY})

add_subdirectory(demo)
