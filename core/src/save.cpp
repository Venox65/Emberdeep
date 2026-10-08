#include "ed/save.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace ed {

namespace {

void writeMap(std::ostringstream& out, const char* prefix, const std::map<std::string, int>& values) {
  for (const auto& [id, count] : values) {
    out << prefix << id << "=" << count << "\n";
  }
}

} // namespace

std::string serializeSaveSnapshot(const SaveSnapshot& snapshot) {
  std::ostringstream out;
  out << "emberdeep_save_version=1\n";
  out << serializeGameState(snapshot.game);
  writeMap(out, "ore.", snapshot.inventory.ore);
  writeMap(out, "ingot.", snapshot.inventory.ingots);
  writeMap(out, "gem.", snapshot.inventory.gems);
  writeMap(out, "trophy.", snapshot.inventory.trophies);
  out << "campaign_size=" << snapshot.campaign.cleared.size() << "\n";
  for (std::size_t i = 0; i < snapshot.campaign.cleared.size(); ++i) {
    out << "campaign." << i << "=" << (snapshot.campaign.cleared[i] ? 1 : 0) << ","
        << snapshot.campaign.bestStars[i] << "\n";
  }
  for (const auto& piece : snapshot.inventory.gear) {
    out << "gear=" << piece.slot << "|" << piece.itemId << "|" << piece.name << "|"
        << piece.tier << "|" << piece.upgradeLevel << "|" << piece.sockets << "|"
        << (piece.legendary ? 1 : 0) << "|";
    for (std::size_t i = 0; i < piece.socketedGems.size(); ++i) {
      if (i > 0) out << ",";
      out << piece.socketedGems[i];
    }
    out << "|" << piece.enchantment << "|" << piece.enchantRerolls << "\n";
  }
  out << "equipped.pickaxe=" << snapshot.inventory.equipped.pickaxeId << "\n";
  out << "equipped.helm=" << snapshot.inventory.equipped.helmId << "\n";
  out << "equipped.chestplate=" << snapshot.inventory.equipped.chestplateId << "\n";
  out << "equipped.gauntlets=" << snapshot.inventory.equipped.gauntletsId << "\n";
  out << "equipped.boots=" << snapshot.inventory.equipped.bootsId << "\n";
  out << "equipped.amulet=" << snapshot.inventory.equipped.amuletId << "\n";
  return out.str();
}

bool deserializeSaveSnapshot(const std::string& text, SaveSnapshot& snapshot) {
  std::istringstream input(text);
  std::ostringstream gameText;
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(input, line)) {
    lines.push_back(line);
    if (line.rfind("ore.", 0) == 0 || line.rfind("ingot.", 0) == 0 ||
        line.rfind("gem.", 0) == 0 || line.rfind("trophy.", 0) == 0 ||
        line.rfind("campaign", 0) == 0 || line.rfind("gear=", 0) == 0 ||
        line.rfind("equipped.", 0) == 0 || line.rfind("emberdeep_save_version", 0) == 0) {
      continue;
    }
    gameText << line << "\n";
  }
  if (!deserializeGameState(gameText.str(), snapshot.game)) return false;

  for (const auto& savedLine : lines) {
    line = savedLine;
    const auto equal = line.find('=');
    if (equal == std::string::npos) continue;
    const auto key = line.substr(0, equal);
    const auto value = line.substr(equal + 1);
    const auto parseCount = [&]() {
      try {
        return std::stoi(value);
      } catch (...) {
        return 0;
      }
    };
    if (key.rfind("ore.", 0) == 0) snapshot.inventory.ore[key.substr(4)] = parseCount();
    if (key.rfind("ingot.", 0) == 0) snapshot.inventory.ingots[key.substr(6)] = parseCount();
    if (key.rfind("gem.", 0) == 0) snapshot.inventory.gems[key.substr(4)] = parseCount();
    if (key.rfind("trophy.", 0) == 0) snapshot.inventory.trophies[key.substr(7)] = parseCount();
    if (key == "gear") {
      std::istringstream gearInput(value);
      std::string field;
      std::vector<std::string> fields;
      while (std::getline(gearInput, field, '|')) fields.push_back(field);
      if (fields.size() >= 7) {
        try {
          GearPiece piece{fields[0], fields[1], fields[2], std::stoi(fields[3]),
                          std::stoi(fields[4]), std::stoi(fields[5]),
                          std::stoi(fields[6]) != 0};
          if (fields.size() >= 8) {
            std::istringstream gemsInput(fields[7]);
            while (std::getline(gemsInput, field, ',')) {
              if (!field.empty()) piece.socketedGems.push_back(field);
            }
          }
          if (fields.size() >= 10) {
            piece.enchantment = fields[8];
            piece.enchantRerolls = std::stoi(fields[9]);
          }
          snapshot.inventory.gear.push_back(std::move(piece));
        } catch (...) {
          return false;
        }
      }
    }
    if (key.rfind("campaign.", 0) == 0) {
      try {
        const auto index = static_cast<std::size_t>(std::stoul(key.substr(9)));
        const auto comma = value.find(',');
        if (comma == std::string::npos) continue;
        if (snapshot.campaign.cleared.size() <= index) {
          snapshot.campaign.cleared.resize(index + 1, false);
          snapshot.campaign.bestStars.resize(index + 1, 0);
        }
        snapshot.campaign.cleared[index] = std::stoi(value.substr(0, comma)) != 0;
        snapshot.campaign.bestStars[index] = std::stoi(value.substr(comma + 1));
      } catch (...) {
        return false;
      }
    }
    if (key == "equipped.pickaxe") snapshot.inventory.equipped.pickaxeId = value;
    if (key == "equipped.helm") snapshot.inventory.equipped.helmId = value;
    if (key == "equipped.chestplate") snapshot.inventory.equipped.chestplateId = value;
    if (key == "equipped.gauntlets") snapshot.inventory.equipped.gauntletsId = value;
    if (key == "equipped.boots") snapshot.inventory.equipped.bootsId = value;
    if (key == "equipped.amulet") snapshot.inventory.equipped.amuletId = value;
  }
  return true;
}

bool writeSaveAtomic(const std::filesystem::path& path, const SaveSnapshot& snapshot) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  const auto temporary = path.string() + ".tmp";
  {
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) return false;
    output << serializeSaveSnapshot(snapshot);
    if (!output) return false;
  }
  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(temporary, path, error);
  }
  return !error;
}

bool readSave(const std::filesystem::path& path, SaveSnapshot& snapshot) {
  std::ifstream input(path);
  if (!input) return false;
  std::ostringstream text;
  text << input.rdbuf();
  return deserializeSaveSnapshot(text.str(), snapshot);
}

} // namespace ed
