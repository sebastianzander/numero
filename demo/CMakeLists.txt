add_executable(demo demo.cpp)
set_target_properties(demo PROPERTIES OUTPUT_NAME numero)
target_link_libraries(demo LINK_PUBLIC numero)
