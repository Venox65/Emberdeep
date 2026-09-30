add_library(emberdeep_client
    src/screens/main_screen.cpp
)

target_include_directories(emberdeep_client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(emberdeep_client PUBLIC emberdeep_core)
