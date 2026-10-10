import re

with open("client/src/app/main.cpp", "r") as f:
    content = f.read()

target = """            st.gemHollowVeins[i]--;
            
            if (st.gemHollowVeins[i] == 0) {"""

replacement = """            st.gemHollowVeins[i]--;
            
            bool allCleared = true;
            for (int v : st.gemHollowVeins) {
              if (v > 0) { allCleared = false; break; }
            }
            if (allCleared) {
              st.gemHollowSwingsRemaining = 0;
            }
            
            if (st.gemHollowVeins[i] == 0) {"""

if target in content:
    content = content.replace(target, replacement)
    print("Patched successfully")
else:
    print("Target not found")

with open("client/src/app/main.cpp", "w") as f:
    f.write(content)
