cmake_minimum_required(VERSION 3.16)
project(numero LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_BUILD_TYPE Debug)

add_executable(numero)

set(source_files
    "src/numero/numero.cpp"
)

target_sources(numero PRIVATE ${source_files})

find_package(Boost REQUIRED COMPONENTS program_options regex)
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(numero ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY})
