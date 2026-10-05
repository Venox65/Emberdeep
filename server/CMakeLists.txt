add_library(emberdeep_server
    src/save_backend.cpp
)

target_include_directories(emberdeep_server INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(emberdeep_server PUBLIC emberdeep_core)

find_package(OpenSSL REQUIRED)
option(EMBERDEEP_ENABLE_POSTGRES "Enable PostgreSQL production backend" OFF)
if(EMBERDEEP_ENABLE_POSTGRES)
    find_package(PostgreSQL REQUIRED)
endif()

add_executable(emberdeep_api
    src/api_main.cpp
)

target_link_libraries(emberdeep_api PRIVATE emberdeep_server OpenSSL::Crypto)
if(EMBERDEEP_ENABLE_POSTGRES)
    target_sources(emberdeep_api PRIVATE src/postgres_backend.cpp)
    target_compile_definitions(emberdeep_api PRIVATE EMBERDEEP_ENABLE_POSTGRES=1)
    target_link_libraries(emberdeep_api PRIVATE PostgreSQL::PostgreSQL)
endif()
