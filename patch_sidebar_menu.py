with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

old_hamburger = """  } else {
    // Just a small hamburger menu button at top left
    DrawRectangleRounded({10, 10, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(18, 20, 24, 4, RAYWHITE);
    DrawRectangle(18, 28, 24, 4, RAYWHITE);
    DrawRectangle(18, 36, 24, 4, RAYWHITE);
  }"""
new_hamburger = """  } else {
    // Hamburger menu button on the right, below the sign out button
    DrawRectangleRounded({1220, 60, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(1228, 70, 24, 4, RAYWHITE);
    DrawRectangle(1228, 78, 24, 4, RAYWHITE);
    DrawRectangle(1228, 86, 24, 4, RAYWHITE);
  }"""
code = code.replace(old_hamburger, new_hamburger)

old_click = """    } else {
      if (CheckCollisionPointRec(mouse, {10, 10, 40, 40})) {
        app.sidebarOpen = true;
      }
    }"""
new_click = """    } else {
      if (CheckCollisionPointRec(mouse, {1220, 60, 40, 40})) {
        app.sidebarOpen = true;
      }
    }"""
code = code.replace(old_click, new_click)

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)
