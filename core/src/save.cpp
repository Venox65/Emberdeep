#include "ed/save.hpp"
#include "ed/crypto.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace ed {

namespace {

void writeMap(std::ostringstream& out, const char* prefix, const std::map<std::string, int>& values) {
  for (const auto& [id, count] : values) {
    out << prefix << id << "=" << count << "\n";
  }
}

// Obfuscated key material for ChaCha20 cipher and HMAC-SHA256 integrity
const std::uint8_t kObfCipherKey[32] = {
    0x9E, 0x1A, 0x5C, 0x88, 0x4F, 0x21, 0x77, 0x3D,
    0xFA, 0x02, 0x8B, 0x44, 0x6C, 0xE9, 0x11, 0x82,
    0x34, 0xA7, 0x55, 0xC1, 0x69, 0xD4, 0x2E, 0xB8,
    0x0F, 0x73, 0x61, 0x98, 0xAD, 0x2C, 0x5F, 0x1E
};

const std::uint8_t kCipherMask[32] = {
    0xDF, 0x7B, 0x38, 0xEA, 0x2A, 0x43, 0x19, 0x58,
    0x9E, 0x67, 0xEF, 0x20, 0x0D, 0x88, 0x73, 0xE1,
    0x55, 0xC3, 0x30, 0xA5, 0x08, 0xB7, 0x4D, 0xD9,
    0x6B, 0x12, 0x05, 0xFE, 0xCC, 0x49, 0x3A, 0x7B
};

const std::uint8_t kObfHmacKey[32] = {
    0x72, 0x84, 0x1C, 0xE5, 0x93, 0x50, 0x2A, 0xB7,
    0x4E, 0xD1, 0x66, 0x38, 0xA9, 0x7F, 0x04, 0xBC,
    0x58, 0x31, 0xE2, 0x19, 0x8B, 0x6D, 0x40, 0x9F,
    0x23, 0xC7, 0x78, 0x5A, 0x0D, 0x8E, 0x3B, 0x65
};

const std::uint8_t kHmacMask[32] = {
    0x37, 0xC1, 0x79, 0x84, 0xD2, 0x3F, 0x4E, 0xD6,
    0x2B, 0x95, 0x03, 0x5D, 0xCC, 0x1A, 0x61, 0xD9,
    0x39, 0x50, 0x87, 0x7C, 0xEF, 0x0E, 0x25, 0xFA,
    0x46, 0xA2, 0x1D, 0x3F, 0x68, 0xEB, 0x5E, 0x0A
};

std::vector<std::uint8_t> getCipherKey() {
  std::vector<std::uint8_t> key(32);
  for (int i = 0; i < 32; ++i) key[i] = kObfCipherKey[i] ^ kCipherMask[i];
  return key;
}

std::vector<std::uint8_t> getHmacKey() {
  std::vector<std::uint8_t> key(32);
  for (int i = 0; i < 32; ++i) key[i] = kObfHmacKey[i] ^ kHmacMask[i];
  return key;
}

std::string buildInnerPlaintext(const SaveSnapshot& snapshot) {
  std::ostringstream out;
  out << "emberdeep_save_version=1\n";
  out << serializeGameState(snapshot.game);
  writeMap(out, "ore.", snapshot.inventory.ore);
  writeMap(out, "ingot.", snapshot.inventory.ingots);
  writeMap(out, "gem.", snapshot.inventory.gems);
  writeMap(out, "trophy.", snapshot.inventory.trophies);
  out << "campaign_size=" << snapshot.campaign.cleared.size() << "\n";
  for (std::size_t i = 0; i < snapshot.campaign.cleared.size(); ++i) {
    const int stars = (i < snapshot.campaign.bestStars.size()) ? snapshot.campaign.bestStars[i] : 0;
    out << "campaign." << i << "=" << (snapshot.campaign.cleared[i] ? 1 : 0) << ","
        << stars << "\n";
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

bool parseInnerPlaintext(const std::string& text, SaveSnapshot& snapshot) {
  std::istringstream input(text);
  std::ostringstream gameText;
  std::string line;
  std::vector<std::string> lines;

  while (std::getline(input, line)) {
    if (line.rfind("hash=", 0) == 0) continue;
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

  snapshot.inventory.ore.clear();
  snapshot.inventory.ingots.clear();
  snapshot.inventory.gems.clear();
  snapshot.inventory.trophies.clear();
  snapshot.inventory.gear.clear();
  snapshot.campaign.cleared.clear();
  snapshot.campaign.bestStars.clear();

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

} // namespace

void sanitizeSaveSnapshot(SaveSnapshot& snapshot) {
  auto& p = snapshot.game.player;

  // Level & progression constraints
  p.level = std::clamp(p.level, 1, 100);
  if (p.xp < 0) p.xp = 0;
  if (p.gold < 0) p.gold = std::numeric_limits<int>::max();
  if (p.arcaneDust < 0) p.arcaneDust = 0;
  if (p.bossEssence < 0) p.bossEssence = 0;
  if (p.echoSigils < 0) p.echoSigils = 0;
  if (p.aetherShards < 0) p.aetherShards = 0;
  if (p.emberMarks < 0) p.emberMarks = 0;
  if (p.vowPoints < 0) p.vowPoints = 0;
  if (p.blocksBroken < 0) p.blocksBroken = std::numeric_limits<int>::max();
  if (p.clicks < 0) p.clicks = std::numeric_limits<int>::max();
  p.bossesCleared = std::clamp(p.bossesCleared, 0, 100);
  if (p.ascensions < 0) p.ascensions = 0;
  if (p.echoRank < 0) p.echoRank = 0;
  if (p.deepestDepth < 0) p.deepestDepth = 0;
  if (p.goldEarnedThisRun < 0) p.goldEarnedThisRun = std::numeric_limits<int>::max();
  p.pickaxeTier = std::clamp(p.pickaxeTier, 1, 10);
  if (p.abyssBestFloor < 0) p.abyssBestFloor = 0;
  if (p.weakSpotsHit < 0) p.weakSpotsHit = 0;
  if (p.chronoCores < 0) p.chronoCores = 0;
  if (p.autoClickerLevel < 0) p.autoClickerLevel = 0;
  if (p.lastSaveTime < 0) p.lastSaveTime = 0;

  // Hollow Descent stats
  if (p.descentRuns < 0) p.descentRuns = 0;
  if (p.descentVictories < 0) p.descentVictories = 0;
  if (p.descentBestChamber < 0) p.descentBestChamber = 0;
  if (p.descentVaultsCracked < 0) p.descentVaultsCracked = 0;
  if (p.descentMaxRunLevel < 0) p.descentMaxRunLevel = 0;
  if (p.descentEndlessBestChamber < 0) p.descentEndlessBestChamber = 0;

  // Ember Perks
  p.emberPerks.power = std::clamp(p.emberPerks.power, 0, 20);
  p.emberPerks.wealth = std::clamp(p.emberPerks.wealth, 0, 20);
  p.emberPerks.wisdom = std::clamp(p.emberPerks.wisdom, 0, 20);
  p.emberPerks.potential = std::clamp(p.emberPerks.potential, 0, 24);
  p.emberPerks.armory = std::clamp(p.emberPerks.armory, 0, 1);
  p.emberPerks.smelting = std::clamp(p.emberPerks.smelting, 0, 5);

  // Miner's Vows
  p.vows.strongArms = std::clamp(p.vows.strongArms, 0, 10);
  p.vows.keenEye = std::clamp(p.vows.keenEye, 0, 10);
  p.vows.prospector = std::clamp(p.vows.prospector, 0, 10);
  p.vows.deepBreath = std::clamp(p.vows.deepBreath, 0, 5);
  p.vows.shatterer = std::clamp(p.vows.shatterer, 0, 1);
  p.vows.stoutHeart = std::clamp(p.vows.stoutHeart, 0, 10);
  p.vows.whetstone = std::clamp(p.vows.whetstone, 0, 10);
  p.vows.ironskin = std::clamp(p.vows.ironskin, 0, 10);
  p.vows.swiftBoots = std::clamp(p.vows.swiftBoots, 0, 5);
  p.vows.secondWind = std::clamp(p.vows.secondWind, 0, 1);
  p.vows.quickStudy = std::clamp(p.vows.quickStudy, 0, 10);
  p.vows.haggler = std::clamp(p.vows.haggler, 0, 10);
  p.vows.steadyHand = std::clamp(p.vows.steadyHand, 0, 10);
  p.vows.dustCollector = std::clamp(p.vows.dustCollector, 0, 5);
  p.vows.gemWhisperer = std::clamp(p.vows.gemWhisperer, 0, 1);

  // Inventory quantities (clean negative quantities)
  for (auto it = snapshot.inventory.ore.begin(); it != snapshot.inventory.ore.end();) {
    if (it->second < 0) it = snapshot.inventory.ore.erase(it);
    else ++it;
  }
  for (auto it = snapshot.inventory.ingots.begin(); it != snapshot.inventory.ingots.end();) {
    if (it->second < 0) it = snapshot.inventory.ingots.erase(it);
    else ++it;
  }
  for (auto it = snapshot.inventory.gems.begin(); it != snapshot.inventory.gems.end();) {
    if (it->second < 0) it = snapshot.inventory.gems.erase(it);
    else ++it;
  }
  for (auto it = snapshot.inventory.trophies.begin(); it != snapshot.inventory.trophies.end();) {
    if (it->second < 0) it = snapshot.inventory.trophies.erase(it);
    else ++it;
  }

  // Gear piece bounds
  for (auto& gear : snapshot.inventory.gear) {
    gear.tier = std::clamp(gear.tier, 1, 10);
    gear.upgradeLevel = std::clamp(gear.upgradeLevel, 0, 20);
    gear.sockets = std::clamp(gear.sockets, 0, 6);
    if (gear.socketedGems.size() > static_cast<std::size_t>(gear.sockets)) {
      gear.socketedGems.resize(static_cast<std::size_t>(gear.sockets));
    }
    if (gear.enchantRerolls < 0) gear.enchantRerolls = 0;
  }

  // Campaign star bounds
  if (snapshot.campaign.cleared.size() > 20) {
    snapshot.campaign.cleared.resize(20);
  }
  if (snapshot.campaign.bestStars.size() < snapshot.campaign.cleared.size()) {
    snapshot.campaign.bestStars.resize(snapshot.campaign.cleared.size(), 0);
  } else if (snapshot.campaign.bestStars.size() > snapshot.campaign.cleared.size()) {
    snapshot.campaign.bestStars.resize(snapshot.campaign.cleared.size());
  }
  for (int& stars : snapshot.campaign.bestStars) {
    stars = std::clamp(stars, 0, 3);
  }
}

bool validateSaveSnapshot(const SaveSnapshot& snapshot) {
  const auto& p = snapshot.game.player;

  // Level & progression constraints (spec level cap 100)
  if (p.level < 1 || p.level > 100) {
    std::cerr << "[SaveValidation] Invalid level: " << p.level << "\n";
    return false;
  }
  if (p.xp < 0) {
    std::cerr << "[SaveValidation] Negative xp: " << p.xp << "\n";
    return false;
  }
  if (p.gold < 0 || p.arcaneDust < 0 || p.bossEssence < 0) {
    std::cerr << "[SaveValidation] Negative core resources: gold=" << p.gold << " dust=" << p.arcaneDust << "\n";
    return false;
  }
  if (p.echoSigils < 0 || p.aetherShards < 0 || p.emberMarks < 0 || p.vowPoints < 0) {
    std::cerr << "[SaveValidation] Negative currencies\n";
    return false;
  }
  if (p.blocksBroken < 0 || p.clicks < 0) {
    std::cerr << "[SaveValidation] Negative blocks/clicks\n";
    return false;
  }
  if (p.bossesCleared < 0 || p.bossesCleared > 100) {
    std::cerr << "[SaveValidation] Invalid bossesCleared: " << p.bossesCleared << "\n";
    return false;
  }
  if (p.ascensions < 0 || p.echoRank < 0 || p.deepestDepth < 0 || p.abyssBestFloor < 0) {
    std::cerr << "[SaveValidation] Negative postgame progression\n";
    return false;
  }
  if (p.goldEarnedThisRun < 0 || p.weakSpotsHit < 0) {
    std::cerr << "[SaveValidation] Negative run stats\n";
    return false;
  }
  if (p.chronoCores < 0 || p.autoClickerLevel < 0 || p.lastSaveTime < 0) {
    std::cerr << "[SaveValidation] Negative chrono/autoclicker stats\n";
    return false;
  }
  if (p.descentRuns < 0 || p.descentVictories < 0 || p.descentBestChamber < 0 ||
      p.descentVaultsCracked < 0 || p.descentMaxRunLevel < 0 || p.descentEndlessBestChamber < 0) {
    std::cerr << "[SaveValidation] Negative descent stats\n";
    return false;
  }
  if (p.pickaxeTier < 1 || p.pickaxeTier > 10) {
    std::cerr << "[SaveValidation] Invalid pickaxeTier: " << p.pickaxeTier << "\n";
    return false;
  }

  // Ember Perks constraints
  if (p.emberPerks.power < 0 || p.emberPerks.power > 20) return false;
  if (p.emberPerks.wealth < 0 || p.emberPerks.wealth > 20) return false;
  if (p.emberPerks.wisdom < 0 || p.emberPerks.wisdom > 20) return false;
  if (p.emberPerks.potential < 0 || p.emberPerks.potential > 24) return false;
  if (p.emberPerks.armory < 0 || p.emberPerks.armory > 1) return false;
  if (p.emberPerks.smelting < 0 || p.emberPerks.smelting > 5) return false;

  // Miner's Vows constraints (Section 7.2)
  if (p.vows.strongArms < 0 || p.vows.strongArms > 10) return false;
  if (p.vows.keenEye < 0 || p.vows.keenEye > 10) return false;
  if (p.vows.prospector < 0 || p.vows.prospector > 10) return false;
  if (p.vows.deepBreath < 0 || p.vows.deepBreath > 5) return false;
  if (p.vows.shatterer < 0 || p.vows.shatterer > 1) return false;
  if (p.vows.stoutHeart < 0 || p.vows.stoutHeart > 10) return false;
  if (p.vows.whetstone < 0 || p.vows.whetstone > 10) return false;
  if (p.vows.ironskin < 0 || p.vows.ironskin > 10) return false;
  if (p.vows.swiftBoots < 0 || p.vows.swiftBoots > 5) return false;
  if (p.vows.secondWind < 0 || p.vows.secondWind > 1) return false;
  if (p.vows.quickStudy < 0 || p.vows.quickStudy > 10) return false;
  if (p.vows.haggler < 0 || p.vows.haggler > 10) return false;
  if (p.vows.steadyHand < 0 || p.vows.steadyHand > 10) return false;
  if (p.vows.dustCollector < 0 || p.vows.dustCollector > 5) return false;
  if (p.vows.gemWhisperer < 0 || p.vows.gemWhisperer > 1) return false;

  // Inventory quantities must be non-negative
  for (const auto& [id, count] : snapshot.inventory.ore) {
    if (count < 0) return false;
  }
  for (const auto& [id, count] : snapshot.inventory.ingots) {
    if (count < 0) return false;
  }
  for (const auto& [id, count] : snapshot.inventory.gems) {
    if (count < 0) return false;
  }
  for (const auto& [id, count] : snapshot.inventory.trophies) {
    if (count < 0) return false;
  }

  // Gear piece bounds (tier 10 mythic legendary can reach 5 sockets)
  for (const auto& gear : snapshot.inventory.gear) {
    if (gear.tier < 1 || gear.tier > 10) {
      std::cerr << "[SaveValidation] Invalid gear tier: " << gear.tier << "\n";
      return false;
    }
    if (gear.upgradeLevel < 0 || gear.upgradeLevel > 20) {
      std::cerr << "[SaveValidation] Invalid gear upgradeLevel: " << gear.upgradeLevel << "\n";
      return false;
    }
    if (gear.sockets < 0 || gear.sockets > 6) {
      std::cerr << "[SaveValidation] Invalid gear sockets: " << gear.sockets << "\n";
      return false;
    }
    if (gear.socketedGems.size() > static_cast<std::size_t>(std::max(gear.sockets, 6))) {
      std::cerr << "[SaveValidation] Too many socketed gems: " << gear.socketedGems.size() << " for sockets " << gear.sockets << "\n";
      return false;
    }
    if (gear.enchantRerolls < 0) {
      std::cerr << "[SaveValidation] Negative enchantRerolls: " << gear.enchantRerolls << "\n";
      return false;
    }
  }

  // Campaign star bounds
  if (snapshot.campaign.bestStars.size() > 20) {
    std::cerr << "[SaveValidation] Too many campaign bestStars entries: " << snapshot.campaign.bestStars.size() << "\n";
    return false;
  }
  for (int stars : snapshot.campaign.bestStars) {
    if (stars < 0 || stars > 3) {
      std::cerr << "[SaveValidation] Invalid campaign star: " << stars << "\n";
      return false;
    }
  }

  return true;
}

std::string serializeSaveSnapshot(const SaveSnapshot& snapshot) {
  const std::string inner = buildInnerPlaintext(snapshot);
  const auto nonce = crypto::randomBytes(12);
  const auto cipherKey = getCipherKey();

  std::string ciphertext(inner.size(), '\0');
  crypto::chacha20(cipherKey.data(), nonce.data(), 1,
                   reinterpret_cast<const std::uint8_t*>(inner.data()),
                   reinterpret_cast<std::uint8_t*>(ciphertext.data()),
                   inner.size());

  // Container layout:
  // [4B Magic: 'EDSV'] [1B Version: 0x02] [12B Nonce] [NB Ciphertext] [32B HMAC-SHA256]
  std::string container;
  container.reserve(4 + 1 + 12 + ciphertext.size() + 32);
  container.append("EDSV", 4);
  container.push_back('\x02');
  container.append(reinterpret_cast<const char*>(nonce.data()), 12);
  container.append(ciphertext);

  const auto hmacKey = getHmacKey();
  const auto tag = crypto::hmacSha256(hmacKey.data(), hmacKey.size(),
                                      reinterpret_cast<const std::uint8_t*>(container.data()),
                                      container.size());
  container.append(reinterpret_cast<const char*>(tag.data()), 32);
  return container;
}

bool deserializeSaveSnapshot(const std::string& text, SaveSnapshot& snapshot) {
  // Check for Version 2 Authenticated Binary Encrypted container ('EDSV\x02')
  if (text.size() >= 49 && text.compare(0, 4, "EDSV") == 0 && text[4] == '\x02') {
    const std::size_t headerAndCipherLen = text.size() - 32;
    const std::string receivedTag = text.substr(headerAndCipherLen, 32);

    const auto hmacKey = getHmacKey();
    const auto expectedTag = crypto::hmacSha256(hmacKey.data(), hmacKey.size(),
                                                reinterpret_cast<const std::uint8_t*>(text.data()),
                                                headerAndCipherLen);

    if (!crypto::constantTimeCompare(reinterpret_cast<const std::uint8_t*>(receivedTag.data()),
                                     expectedTag.data(), 32)) {
      return false; // HMAC verification failure (tampered save)
    }

    const std::uint8_t* nonce = reinterpret_cast<const std::uint8_t*>(text.data() + 5);
    const std::size_t cipherLen = headerAndCipherLen - 17;
    const std::uint8_t* cipherBytes = reinterpret_cast<const std::uint8_t*>(text.data() + 17);

    const auto cipherKey = getCipherKey();
    std::string decrypted(cipherLen, '\0');
    crypto::chacha20(cipherKey.data(), nonce, 1,
                     cipherBytes,
                     reinterpret_cast<std::uint8_t*>(decrypted.data()),
                     cipherLen);

    if (!parseInnerPlaintext(decrypted, snapshot)) return false;
    sanitizeSaveSnapshot(snapshot);
    return validateSaveSnapshot(snapshot);
  }

  // Fallback: Legacy v1 plaintext format support for seamless migration
  if (text.find("emberdeep_save_version=1") != std::string::npos) {
    std::istringstream input(text);
    std::string line;
    std::string dataWithoutHash;
    std::string expectedHashStr;

    while (std::getline(input, line)) {
      if (line.rfind("hash=", 0) == 0) {
        expectedHashStr = line.substr(5);
        continue;
      }
      dataWithoutHash += line + "\n";
    }

    if (!expectedHashStr.empty()) {
      auto actualHash = std::hash<std::string>{}(dataWithoutHash);
      if (std::to_string(actualHash) != expectedHashStr) {
        return false; // Legacy hash mismatch
      }
    }

    if (!parseInnerPlaintext(text, snapshot)) return false;
    sanitizeSaveSnapshot(snapshot);
    return validateSaveSnapshot(snapshot);
  }

  return false;
}

bool writeSaveAtomic(const std::filesystem::path& path, const SaveSnapshot& snapshot) {
  if (path.empty()) {
    std::cerr << "[SaveAtomic] Save path is empty!\n";
    return false;
  }
  SaveSnapshot cleanSnap = snapshot;
  sanitizeSaveSnapshot(cleanSnap);
  if (!validateSaveSnapshot(cleanSnap)) {
    std::cerr << "[SaveAtomic] Save validation rejected for: " << path << "\n";
    return false;
  }
  if (!path.parent_path().empty()) {
    std::error_code dirErr;
    std::filesystem::create_directories(path.parent_path(), dirErr);
  }

  static std::atomic<std::uint64_t> s_tempCounter{0};
  const auto tempSuffix = "." + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) +
                          "_" + std::to_string(++s_tempCounter) + ".tmp";
  const auto temporary = path.string() + tempSuffix;
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
      std::cerr << "[SaveAtomic] Failed to create temp save file: " << temporary << "\n";
      return false;
    }
    const std::string payload = serializeSaveSnapshot(cleanSnap);
    output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (!output) {
      std::cerr << "[SaveAtomic] Failed to write payload to: " << temporary << "\n";
      std::error_code rmErr;
      std::filesystem::remove(temporary, rmErr);
      return false;
    }
  }

  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    std::error_code cpErr;
    std::filesystem::copy_file(temporary, path, std::filesystem::copy_options::overwrite_existing, cpErr);
    if (!cpErr) {
      error.clear();
    }
  }
  std::error_code rmErr;
  std::filesystem::remove(temporary, rmErr);
  if (error) {
    std::cerr << "[SaveAtomic] Rename/Copy failed: " << error.message() << " (" << temporary << " -> " << path << ")\n";
    return false;
  }
  return true;
}

bool readSave(const std::filesystem::path& path, SaveSnapshot& snapshot) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string data = buffer.str();

  if (!deserializeSaveSnapshot(data, snapshot)) {
    return false;
  }

  // If the loaded file was in legacy format (not starting with 'EDSV'),
  // transparently upgrade and save it in the new secure binary format.
  if (data.size() < 4 || data.compare(0, 4, "EDSV") != 0) {
    writeSaveAtomic(path, snapshot);
  }

  return true;
}

} // namespace ed
