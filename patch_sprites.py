import re

with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

texture_cache = """#include <map>

std::map<std::string, Texture2D> g_textures;
Texture2D getTexture(const std::string& path) {
    if (g_textures.find(path) == g_textures.end()) {
        g_textures[path] = LoadTexture(path.c_str());
    }
    return g_textures[path];
}

void drawMineScreen"""

code = code.replace("void drawMineScreen", texture_cache, 1)

old_block = """  const Color blockColor = materialColor(app.blockSpec);
  DrawRectangleRounded({static_cast<float>(blockX), static_cast<float>(blockY),
                        static_cast<float>(blockSize), static_cast<float>(blockSize)},
                       0.14F, 12, blockColor);
  DrawRectangleLinesEx({static_cast<float>(blockX), static_cast<float>(blockY),
                        static_cast<float>(blockSize), static_cast<float>(blockSize)},
                       5.0F, Color{239, 225, 189, 255});"""

new_block = """  const Color blockColor = materialColor(app.blockSpec);
  std::string baseName = app.blockSpec.materialId;
  std::replace(baseName.begin(), baseName.end(), '-', '_');
  std::string texName = "sprites/Mining_Block/" + baseName + (hpRatio <= 0.50F ? "_break.png" : "_idle.png");
  Texture2D tex = getTexture(texName);
  
  if (tex.id != 0) {
      DrawTexturePro(tex, {0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height)}, 
                     {static_cast<float>(blockX), static_cast<float>(blockY), static_cast<float>(blockSize), static_cast<float>(blockSize)}, 
                     {0, 0}, 0.0f, WHITE);
  } else {
      DrawRectangleRounded({static_cast<float>(blockX), static_cast<float>(blockY),
                            static_cast<float>(blockSize), static_cast<float>(blockSize)},
                           0.14F, 12, blockColor);
      DrawRectangleLinesEx({static_cast<float>(blockX), static_cast<float>(blockY),
                            static_cast<float>(blockSize), static_cast<float>(blockSize)},
                           5.0F, Color{239, 225, 189, 255});
  }"""

code = code.replace(old_block, new_block)

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)

