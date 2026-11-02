#include "ed/data_loader.hpp"

#include <cctype>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ed {

namespace {

enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonVal {
  JsonType type = JsonType::Null;
  bool boolVal = false;
  double numVal = 0.0;
  std::string strVal;
  std::vector<JsonVal> arrVal;
  std::map<std::string, JsonVal> objVal;

  const JsonVal& operator[](const std::string& key) const {
    static const JsonVal kNull;
    auto it = objVal.find(key);
    return it != objVal.end() ? it->second : kNull;
  }

  std::string getString(const std::string& key, const std::string& def = "") const {
    auto it = objVal.find(key);
    return (it != objVal.end() && it->second.type == JsonType::String) ? it->second.strVal : def;
  }

  int getInt(const std::string& key, int def = 0) const {
    auto it = objVal.find(key);
    return (it != objVal.end() && it->second.type == JsonType::Number) ? static_cast<int>(it->second.numVal) : def;
  }

  std::int64_t getInt64(const std::string& key, std::int64_t def = 0) const {
    auto it = objVal.find(key);
    return (it != objVal.end() && it->second.type == JsonType::Number) ? static_cast<std::int64_t>(it->second.numVal) : def;
  }
};

class JsonParser {
public:
  explicit JsonParser(std::string_view src) : src_(src), pos_(0) {}

  bool parse(JsonVal& out) {
    skipWs();
    if (pos_ >= src_.size()) return false;
    return parseValue(out);
  }

private:
  std::string_view src_;
  std::size_t pos_ = 0;

  void skipWs() {
    while (pos_ < src_.size()) {
      char c = src_[pos_];
      if (std::isspace(static_cast<unsigned char>(c))) {
        ++pos_;
      } else if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
        pos_ += 2;
        while (pos_ < src_.size() && src_[pos_] != '\n') ++pos_;
      } else {
        break;
      }
    }
  }

  bool parseValue(JsonVal& out) {
    skipWs();
    if (pos_ >= src_.size()) return false;
    char c = src_[pos_];
    if (c == 'n') return parseNull(out);
    if (c == 't' || c == 'f') return parseBool(out);
    if (c == '"') return parseString(out.strVal) && ((out.type = JsonType::String), true);
    if (c == '[') return parseArray(out);
    if (c == '{') return parseObject(out);
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(out);
    return false;
  }

  bool parseNull(JsonVal& out) {
    if (src_.substr(pos_, 4) == "null") {
      pos_ += 4;
      out.type = JsonType::Null;
      return true;
    }
    return false;
  }

  bool parseBool(JsonVal& out) {
    if (src_.substr(pos_, 4) == "true") {
      pos_ += 4;
      out.type = JsonType::Bool;
      out.boolVal = true;
      return true;
    }
    if (src_.substr(pos_, 5) == "false") {
      pos_ += 5;
      out.type = JsonType::Bool;
      out.boolVal = false;
      return true;
    }
    return false;
  }

  bool parseNumber(JsonVal& out) {
    std::size_t start = pos_;
    if (src_[pos_] == '-') ++pos_;
    while (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.')) {
      ++pos_;
    }
    std::string s(src_.substr(start, pos_ - start));
    try {
      out.type = JsonType::Number;
      out.numVal = std::stod(s);
      return true;
    } catch (...) {
      return false;
    }
  }

  bool parseString(std::string& out) {
    if (pos_ >= src_.size() || src_[pos_] != '"') return false;
    ++pos_;
    out.clear();
    while (pos_ < src_.size()) {
      char c = src_[pos_++];
      if (c == '"') return true;
      if (c == '\\' && pos_ < src_.size()) {
        char esc = src_[pos_++];
        if (esc == '"' || esc == '\\' || esc == '/') out.push_back(esc);
        else if (esc == 'n') out.push_back('\n');
        else if (esc == 'r') out.push_back('\r');
        else if (esc == 't') out.push_back('\t');
        else out.push_back(esc);
      } else {
        out.push_back(c);
      }
    }
    return false;
  }

  bool parseArray(JsonVal& out) {
    if (pos_ >= src_.size() || src_[pos_] != '[') return false;
    ++pos_;
    out.type = JsonType::Array;
    out.arrVal.clear();
    skipWs();
    if (pos_ < src_.size() && src_[pos_] == ']') {
      ++pos_;
      return true;
    }
    while (pos_ < src_.size()) {
      JsonVal item;
      if (!parseValue(item)) return false;
      out.arrVal.push_back(std::move(item));
      skipWs();
      if (pos_ < src_.size() && src_[pos_] == ',') {
        ++pos_;
        skipWs();
      } else if (pos_ < src_.size() && src_[pos_] == ']') {
        ++pos_;
        return true;
      } else {
        return false;
      }
    }
    return false;
  }

  bool parseObject(JsonVal& out) {
    if (pos_ >= src_.size() || src_[pos_] != '{') return false;
    ++pos_;
    out.type = JsonType::Object;
    out.objVal.clear();
    skipWs();
    if (pos_ < src_.size() && src_[pos_] == '}') {
      ++pos_;
      return true;
    }
    while (pos_ < src_.size()) {
      skipWs();
      std::string key;
      if (!parseString(key)) return false;
      skipWs();
      if (pos_ >= src_.size() || src_[pos_] != ':') return false;
      ++pos_;
      JsonVal val;
      if (!parseValue(val)) return false;
      out.objVal[key] = std::move(val);
      skipWs();
      if (pos_ < src_.size() && src_[pos_] == ',') {
        ++pos_;
        skipWs();
      } else if (pos_ < src_.size() && src_[pos_] == '}') {
        ++pos_;
        return true;
      } else {
        return false;
      }
    }
    return false;
  }
};

std::string readFileContent(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in.is_open()) return "";
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

BossChallenge parseChallenge(std::string_view name) {
  if (name == "frenzy-dig") return BossChallenge::FrenzyDig;
  if (name == "bone-duel") return BossChallenge::BoneDuel;
  if (name == "cinder-dance") return BossChallenge::CinderDance;
  if (name == "head-hunt") return BossChallenge::HeadHunt;
  if (name == "parry-duel") return BossChallenge::ParryDuel;
  if (name == "rune-recall") return BossChallenge::RuneRecall;
  if (name == "star-trace") return BossChallenge::StarTrace;
  if (name == "heartpierce" || name == "wyrmfire-run") return BossChallenge::Heartpierce;
  if (name == "soul-dodge") return BossChallenge::SoulDodge;
  if (name == "five-trials") return BossChallenge::FiveTrials;
  return BossChallenge::FrenzyDig;
}

} // namespace

bool loadMaterialsFromJsonFile(const std::filesystem::path& path, MaterialList& out) {
  const std::string text = readFileContent(path);
  if (text.empty()) return false;
  JsonVal root;
  JsonParser parser(text);
  if (!parser.parse(root) || root.type != JsonType::Array || root.arrVal.empty()) return false;

  MaterialList list;
  for (const auto& item : root.arrVal) {
    if (item.type != JsonType::Object) continue;
    MaterialDef mat;
    mat.id = item.getString("id");
    mat.name = item.getString("name");
    mat.layer = item.getString("layer");
    mat.color = item.getString("color", "#ffffff");
    mat.hp = item.getInt("hp", 20);
    mat.hardness = item.getInt("hardness", 0);
    mat.xp = item.getInt("xp", 5);
    mat.sell = item.getInt("sell", 1);
    mat.minLevel = item.getInt("minLevel", 1);
    if (!mat.id.empty()) list.push_back(std::move(mat));
  }
  if (list.empty()) return false;
  out = std::move(list);
  return true;
}

bool loadLayersFromJsonFile(const std::filesystem::path& path, LayerList& out) {
  const std::string text = readFileContent(path);
  if (text.empty()) return false;
  JsonVal root;
  JsonParser parser(text);
  if (!parser.parse(root) || root.type != JsonType::Array || root.arrVal.empty()) return false;

  LayerList list;
  for (const auto& item : root.arrVal) {
    if (item.type != JsonType::Object) continue;
    LayerDef layer;
    layer.id = item.getString("id");
    layer.name = item.getString("name");
    layer.tier = item.getInt("tier", 1);
    layer.minLevel = item.getInt("minLevel", 1);
    layer.requiredPickaxeTier = item.getInt("requiredPickaxeTier", 0);
    if (!layer.id.empty()) list.push_back(std::move(layer));
  }
  if (list.empty()) return false;
  out = std::move(list);
  return true;
}

bool loadBossesFromJsonFile(const std::filesystem::path& path, std::vector<BossDef>& out) {
  const std::string text = readFileContent(path);
  if (text.empty()) return false;
  JsonVal root;
  JsonParser parser(text);
  if (!parser.parse(root) || root.type != JsonType::Array || root.arrVal.empty()) return false;

  std::vector<BossDef> list;
  for (const auto& item : root.arrVal) {
    if (item.type != JsonType::Object) continue;
    BossDef boss;
    boss.id = item.getString("id");
    boss.name = item.getString("name");
    boss.layerId = item.getString("layer");
    boss.challenge = parseChallenge(item.getString("challenge"));
    boss.minLevel = item.getInt("minLevel", 1);
    boss.recommendedGearTier = boss.minLevel == 1 ? 1 : std::min(10, (boss.minLevel + 5) / 7);
    boss.clearScore = item.getInt("clearScore", 5);
    boss.twoStarScore = item.getInt("twoStarScore", 7);
    boss.threeStarScore = item.getInt("threeStarScore", 9);
    boss.firstClearGold = 250 * (1 << std::min(9, static_cast<int>(boss.challenge)));
    boss.firstClearXp = 100 * (1 << std::min(9, static_cast<int>(boss.challenge)));
    boss.legendaryId = item.getString("legendary");
    boss.maxHp = item.getInt64("maxHp", 100);
    boss.attack = item.getInt64("attack", 10);
    boss.defense = item.getInt64("defense", 0);
    boss.speed = item.getInt64("speed", 10);
    boss.abilityName = item.getString("abilityName", "Strike");
    if (!boss.id.empty()) list.push_back(std::move(boss));
  }
  if (list.empty()) return false;
  out = std::move(list);
  return true;
}

bool loadDungeonZonesFromJsonFile(const std::filesystem::path& path, std::vector<DungeonZoneDef>& out) {
  const std::string text = readFileContent(path);
  if (text.empty()) return false;
  JsonVal root;
  JsonParser parser(text);
  if (!parser.parse(root) || root.type != JsonType::Array || root.arrVal.empty()) return false;

  std::vector<DungeonZoneDef> list;
  for (const auto& item : root.arrVal) {
    if (item.type != JsonType::Object) continue;
    DungeonZoneDef zone;
    zone.id = item.getString("id");
    zone.name = item.getString("name");
    zone.tier = item.getInt("tier", 1);
    for (const auto& defZone : defaultDungeonZones()) {
      if (defZone.id == zone.id || defZone.tier == zone.tier) {
        zone.enemies = defZone.enemies;
        zone.miniBoss = defZone.miniBoss;
        break;
      }
    }
    if (!zone.id.empty()) list.push_back(std::move(zone));
  }
  if (list.empty()) return false;
  out = std::move(list);
  return true;
}

bool loadGemsFromJsonFile(const std::filesystem::path& path, GemList& out) {
  const std::string text = readFileContent(path);
  if (text.empty()) return false;
  JsonVal root;
  JsonParser parser(text);
  if (!parser.parse(root) || root.type != JsonType::Array || root.arrVal.empty()) return false;

  GemList list;
  for (const auto& item : root.arrVal) {
    if (item.type != JsonType::Object) continue;
    GemDef gem;
    gem.id = item.getString("id");
    gem.name = item.getString("name");
    gem.rarity = item.getString("rarity", "common");
    gem.effect = item.getString("effect", "");
    gem.priority = item.getInt("priority", 1);
    if (!gem.id.empty()) list.push_back(std::move(gem));
  }
  if (list.empty()) return false;
  out = std::move(list);
  return true;
}

LoadedGameData loadGameData(const std::filesystem::path& dataDir) {
  LoadedGameData data;
  data.materials = defaultMaterials();
  data.layers = defaultLayers();
  data.bosses = defaultBosses();
  data.zones = defaultDungeonZones();
  data.gems = defaultGems();

  std::filesystem::path dir = dataDir;
  if (!std::filesystem::exists(dir)) {
    if (std::filesystem::exists("../" + dataDir.string())) {
      dir = "../" + dataDir.string();
    } else if (std::filesystem::exists("../../" + dataDir.string())) {
      dir = "../../" + dataDir.string();
    }
  }

  if (std::filesystem::exists(dir)) {
    bool anySuccess = false;
    if (loadMaterialsFromJsonFile(dir / "materials.json", data.materials)) anySuccess = true;
    if (loadLayersFromJsonFile(dir / "layers.json", data.layers)) anySuccess = true;
    if (loadBossesFromJsonFile(dir / "bosses.json", data.bosses)) anySuccess = true;
    if (loadDungeonZonesFromJsonFile(dir / "dungeon_zones.json", data.zones)) anySuccess = true;
    if (loadGemsFromJsonFile(dir / "gems.json", data.gems)) anySuccess = true;
    data.loadedFromDisk = anySuccess;
  }

  return data;
}

} // namespace ed
