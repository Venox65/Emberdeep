include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(emberdeep_core_tests
    core/test_campaign.cpp
    core/test_boss_minigames.cpp
    core/test_crafting.cpp
    core/test_dungeon.cpp
    core/test_formulas.cpp
    core/test_inventory.cpp
    core/test_mining.cpp
    core/test_postgame.cpp
    core/test_progression.cpp
    core/test_stats_save.cpp
)

target_link_libraries(emberdeep_core_tests PRIVATE emberdeep_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(emberdeep_core_tests)
