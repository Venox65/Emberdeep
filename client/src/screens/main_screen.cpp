#include "screens/main_screen.hpp"

#include <utility>

namespace ed::client {

MainScreen::MainScreen(MainScreenState initialState) : state_(std::move(initialState)) {
}

void MainScreen::setCurrentLayer(const std::string& layerId) {
  state_.currentLayer = layerId;
  state_.dirty = true;
}

void MainScreen::setBlockHealth(int current, int max) {
  state_.currentHp = current;
  state_.maxHp = max;
  state_.dirty = true;
}

void MainScreen::setGold(int value) {
  state_.gold = value;
  state_.dirty = true;
}

void MainScreen::setLevel(int value) {
  state_.level = value;
  state_.dirty = true;
}

void MainScreen::setGemHollowReady(bool ready) {
  state_.gemHollowReady = ready;
  state_.dirty = true;
}

const MainScreenState& MainScreen::state() const {
  return state_;
}

} // namespace ed::client
