add_library(emberdeep_core
    src/campaign.cpp
    src/boss_minigames.cpp
    src/crafting.cpp
    src/dungeon.cpp
    src/formulas.cpp
    src/game_state.cpp
    src/inventory.cpp
    src/mining.cpp
    src/postgame.cpp
    src/pcg.cpp
    src/save.cpp
    src/stats.cpp
    src/crypto.cpp
    src/achievements.cpp
    src/data_loader.cpp
    src/descent/spatial.cpp
    src/descent/spawning.cpp
    src/descent/progression.cpp
    src/descent/combat_sim.cpp
)

target_include_directories(emberdeep_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(emberdeep_core PUBLIC cxx_std_20)
