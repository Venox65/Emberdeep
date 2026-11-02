add_library(emberdeep_client
    src/screens/main_screen.cpp
)

target_include_directories(emberdeep_client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(emberdeep_client PUBLIC emberdeep_core)

add_executable(emberdeep_playtest
    src/playtest/main.cpp
)

target_link_libraries(emberdeep_playtest PRIVATE emberdeep_client)

find_package(raylib CONFIG REQUIRED)

add_executable(emberdeep_graphical
    src/app/main.cpp
    src/app/save_system.cpp
    src/app/game_actions.cpp
    src/app/game_updates.cpp
    src/app/campaign_minigames.cpp
    src/app/ui_screens.cpp
    src/app/ui_descent.cpp
    src/app/ui_inventory.cpp
    src/app/ui_profile.cpp
    src/app/input_handler.cpp
    src/app/input_descent.cpp
    src/app/input_inventory.cpp
    src/app/input_profile.cpp
)

target_link_libraries(emberdeep_graphical PRIVATE emberdeep_client raylib)
