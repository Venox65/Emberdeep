with open("client/src/app/main.cpp", "r") as f:
    content = f.read()

# Fix transparent bar
content = content.replace("DrawRectangle(0, 685, 1280, 35, Color{28, 27, 39, 0});", "DrawRectangle(0, 685, 1280, 35, Color{28, 27, 39, 200});")

# Fix swings
content = content.replace("st.gemHollowSwingsRemaining = 25 + st.gemHollowStreak;", "st.gemHollowSwingsRemaining = 40 + st.gemHollowStreak;")

with open("client/src/app/main.cpp", "w") as f:
    f.write(content)
