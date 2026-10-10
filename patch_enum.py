with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

code = code.replace("piece.itemName", "piece.name")
code = code.replace("piece.level", "piece.tier")

old_enum = """enum class Screen {
  Mine,
  Inventory,
  Crafting,
  Campaign,
  Dungeon,
  Profile,
};"""
new_enum = """enum class Screen {
  Mine,
  Inventory,
  Crafting,
  Campaign,
  Dungeon,
  Profile,
  GemHollow,
  Enchanting,
};"""
code = code.replace(old_enum, new_enum)

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)
