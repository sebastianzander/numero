add_executable(numero_test test.cpp)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(numero_test LINK_PUBLIC numero ${Boost_LIBRARIES})

add_test(NAME numero_test_module COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/numero_test)
