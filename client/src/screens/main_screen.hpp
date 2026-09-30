#pragma once

#include <string>

namespace ed::client {

struct MainScreenState {
  std::string currentLayer = "ashen-warrens";
  int currentHp = 0;
  int maxHp = 0;
  int gold = 0;
  int level = 1;
  bool gemHollowReady = true;
  bool dirty = false;
};

class MainScreen {
public:
  explicit MainScreen(MainScreenState initialState = {});

  void setCurrentLayer(const std::string& layerId);
  void setBlockHealth(int current, int max);
  void setGold(int value);
  void setLevel(int value);
  void setGemHollowReady(bool ready);
  const MainScreenState& state() const;

private:
  MainScreenState state_;
};

} // namespace ed::client
