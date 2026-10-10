with open("client/src/app/main.cpp", "r") as f:
    content = f.read()

target = 'std::string texPath = "sprites/Materials/" + id + "_ingot.png";'
replacement = '''std::string safeId = id;
        for (char& c : safeId) if (c == '-') c = '_';
        std::string texPath = "sprites/Materials/" + safeId + "_ingot.png";'''

if target in content:
    content = content.replace(target, replacement)
    print("Fixed texPath")

with open("client/src/app/main.cpp", "w") as f:
    f.write(content)
