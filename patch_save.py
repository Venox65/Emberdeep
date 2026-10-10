with open("emberdeep.sav", "r") as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if line.startswith("level="):
        new_lines.append("level=20\n")
    elif line.startswith("gold="):
        new_lines.append("gold=10000\n")
    elif line.startswith("arcane_dust="):
        new_lines.append("arcane_dust=1000\n")
    else:
        new_lines.append(line)

new_lines.append("gear=Pickaxe|iron-pickaxe|Iron Pickaxe|2|0|1|0|||0\n")
new_lines.append("gear=Helm|aether-helm|Aether Helm|10|0|3|1|||0\n")
new_lines.append("gear=Chestplate|void-chestplate|Void Chestplate|8|5|2|0|||0\n")

with open("emberdeep.sav", "w") as f:
    f.writelines(new_lines)
