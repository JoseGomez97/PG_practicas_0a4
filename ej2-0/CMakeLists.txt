cmake_minimum_required(VERSION 2.8)

project(ej2-0)

add_executable(ej2-0 main.cpp)
target_link_libraries(ej2-0 PGUPV)

include(../PGUPV/pgupv.cmake)

set_target_properties( ej2-0 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
)

install(TARGETS ej2-0 DESTINATION ${PG_SOURCE_DIR}/bin)
