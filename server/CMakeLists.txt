add_library(emberdeep_server INTERFACE)

target_include_directories(emberdeep_server INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(emberdeep_server INTERFACE emberdeep_core)
