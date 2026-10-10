import re

with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

# 1. Update screenName
code = re.sub(
    r'(case Screen::Dungeon: return "DUNGEON";)',
    r'\1\n    case Screen::GemHollow: return "GEM HOLLOW";\n    case Screen::Enchanting: return "ENCHANTING";',
    code
)

# 2. Update top bar text
code = re.sub(
    r'"1 Mine   2 Inventory   3 Crafting   4 Campaign   5 Dungeon   6 Profile   F5 Save", 26, 55, 15,\n           Color\{156, 156, 173, 255\}',
    r'"1 Mine   2 Inventory   3 Crafting   4 Campaign   5 Dungeon   6 Gem Hollow   7 Enchant   8 Profile   F5 Save", 16, 55, 14,\n           Color{156, 156, 173, 255}',
    code
)

# 3. Update drawNavigation
old_nav = """void drawNavigation(const AppState& app) {
  DrawRectangle(0, 665, kWidth, 55, Color{28, 27, 39, 255});
  for (int i = 0; i < 6; ++i) {
    const int x = 18 + i * 210;
    const Screen screen = static_cast<Screen>(i);"""
new_nav = """void drawNavigation(const AppState& app) {
  DrawRectangle(0, 665, kWidth, 55, Color{28, 27, 39, 255});
  const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Profile};
  for (int i = 0; i < 7; ++i) {
    const int x = 10 + i * 180;
    const Screen screen = navScreens[i];"""
code = code.replace(old_nav, new_nav)

code = re.sub(
    r'DrawRectangleRounded\(\{static_cast<float>\(x\), 675.0F, 170.0F, 34.0F\}, 0.25F, 8,',
    r'DrawRectangleRounded({static_cast<float>(x), 675.0F, 160.0F, 34.0F}, 0.25F, 8,',
    code
)

# 4. Update mouse click tabs
old_mouse = """    for (int i = 0; i < 6; ++i) {
      const Rectangle navigationButton = {
          18.0F + static_cast<float>(i * 210),
          675.0F,
          170.0F,
          34.0F,
      };
      if (CheckCollisionPointRec(mouse, navigationButton)) {
        app.screen = static_cast<Screen>(i);"""
new_mouse = """    for (int i = 0; i < 7; ++i) {
      const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Profile};
      const Rectangle navigationButton = {
          10.0F + static_cast<float>(i * 180),
          675.0F,
          160.0F,
          34.0F,
      };
      if (CheckCollisionPointRec(mouse, navigationButton)) {
        app.screen = navScreens[i];"""
code = code.replace(old_mouse, new_mouse)

# 5. Update hotkeys
old_keys = """  if (IsKeyPressed(KEY_SIX)) app.screen = Screen::Profile;"""
new_keys = """  if (IsKeyPressed(KEY_SIX)) app.screen = Screen::GemHollow;
  if (IsKeyPressed(KEY_SEVEN)) app.screen = Screen::Enchanting;
  if (IsKeyPressed(KEY_EIGHT)) app.screen = Screen::Profile;"""
code = code.replace(old_keys, new_keys)

# 6. Update main switch
old_switch = """        case Screen::Profile: drawProfileScreen(app); break;
      }"""
new_switch = """        case Screen::Profile: drawProfileScreen(app); break;
        case Screen::GemHollow: drawGemHollowScreen(app); break;
        case Screen::Enchanting: drawEnchantingScreen(app); break;
      }"""
code = code.replace(old_switch, new_switch)

# 7. Insert new screens
with open("/Users/kacperdurak/.gemini/antigravity-cli/brain/1247fbbc-a3ca-481a-a95a-16cb1be2b52c/scratch/new_screens.cpp", "r") as f:
    new_screens = f.read()

code = code.replace("void drawProfileScreen(const AppState& app) {", new_screens + "\nvoid drawProfileScreen(const AppState& app) {")

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)

