import re

with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

old_block = """  if (tex.id != 0) {
      DrawTexturePro(tex, {0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height)}, 
                     {static_cast<float>(blockX), static_cast<float>(blockY), static_cast<float>(blockSize), static_cast<float>(blockSize)}, 
                     {0, 0}, 0.0f, WHITE);
  }"""

new_block = """  if (tex.id != 0) {
      int frames = tex.width / 32;
      if (frames < 1) frames = 1;
      int frame = (static_cast<int>(GetTime() * 60) / 15) % frames;
      Rectangle source = {static_cast<float>(frame * 32), 0.0f, 32.0f, 32.0f};
      DrawTexturePro(tex, source, 
                     {static_cast<float>(blockX), static_cast<float>(blockY), static_cast<float>(blockSize), static_cast<float>(blockSize)}, 
                     {0, 0}, 0.0f, WHITE);
  }"""

code = code.replace(old_block, new_block)

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)

