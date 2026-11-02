#include "ed/descent/combat_sim.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <utility>
#include <vector>

namespace ed::descent {

void FlowField::resize(int w, int h) {
  width = w;
  height = h;
  walkable.assign(static_cast<std::size_t>(w * h), 1);
  // Set boundary walls as non-walkable
  for (int x = 0; x < w; ++x) {
    walkable[x] = 0;
    walkable[(h - 1) * w + x] = 0;
  }
  for (int y = 0; y < h; ++y) {
    walkable[y * w] = 0;
    walkable[y * w + (w - 1)] = 0;
  }
  directions.assign(static_cast<std::size_t>(w * h), Vec2{0.0F, 0.0F});
}

void FlowField::compute(int targetTileX, int targetTileY) {
  if (width <= 0 || height <= 0) return;
  targetTileX = std::clamp(targetTileX, 1, width - 2);
  targetTileY = std::clamp(targetTileY, 1, height - 2);
  playerTileX = targetTileX;
  playerTileY = targetTileY;

  const int totalCells = width * height;
  std::vector<int> dist(totalCells, 99999);
  std::queue<std::pair<int, int>> q;

  const int targetIdx = targetTileY * width + targetTileX;
  dist[targetIdx] = 0;
  q.push({targetTileX, targetTileY});

  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  while (!q.empty()) {
    const auto [cx, cy] = q.front();
    q.pop();
    const int curDist = dist[cy * width + cx];

    for (int i = 0; i < 4; ++i) {
      const int nx = cx + dx[i];
      const int ny = cy + dy[i];
      if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
        const int nIdx = ny * width + nx;
        if (walkable[nIdx] && dist[nIdx] > curDist + 1) {
          dist[nIdx] = curDist + 1;
          q.push({nx, ny});
        }
      }
    }
  }

  // Derive gradient directions
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int idx = y * width + x;
      if (!walkable[idx] || (x == targetTileX && y == targetTileY)) {
        directions[idx] = Vec2{0.0F, 0.0F};
        continue;
      }
      int bestDist = dist[idx];
      Vec2 bestDir{0.0F, 0.0F};

      for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
          if (ox == 0 && oy == 0) continue;
          const int nx = x + ox;
          const int ny = y + oy;
          if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            const int nIdx = ny * width + nx;
            if (walkable[nIdx] && dist[nIdx] < bestDist) {
              bestDist = dist[nIdx];
              bestDir = Vec2{static_cast<float>(ox), static_cast<float>(oy)};
            }
          }
        }
      }
      directions[idx] = bestDir.normalized();
    }
  }
}

Vec2 FlowField::getDirection(float worldX, float worldY) const {
  if (width <= 0 || height <= 0) return {0.0F, 0.0F};
  const int tx = std::clamp(static_cast<int>(worldX / DescentSimState::kTileSize), 0, width - 1);
  const int ty = std::clamp(static_cast<int>(worldY / DescentSimState::kTileSize), 0, height - 1);
  return directions[ty * width + tx];
}

void SpatialHash::init(float worldWidth, float worldHeight) {
  cols = std::max(1, static_cast<int>(std::ceil(worldWidth / kCellSize)));
  rows = std::max(1, static_cast<int>(std::ceil(worldHeight / kCellSize)));
  cells.clear();
  cells.resize(static_cast<std::size_t>(cols * rows));
  for (auto& c : cells) {
    c.reserve(8);
  }
}

void SpatialHash::clear() {
  for (auto& c : cells) {
    c.clear();
  }
}

void SpatialHash::insert(int entityIndex, Vec2 pos) {
  const int cx = std::clamp(static_cast<int>(pos.x / kCellSize), 0, cols - 1);
  const int cy = std::clamp(static_cast<int>(pos.y / kCellSize), 0, rows - 1);
  cells[cy * cols + cx].push_back(entityIndex);
}

void SpatialHash::query(Vec2 pos, float radius, std::vector<int>& outIndices) const {
  outIndices.clear();
  const int minX = std::clamp(static_cast<int>((pos.x - radius) / kCellSize), 0, cols - 1);
  const int maxX = std::clamp(static_cast<int>((pos.x + radius) / kCellSize), 0, cols - 1);
  const int minY = std::clamp(static_cast<int>((pos.y - radius) / kCellSize), 0, rows - 1);
  const int maxY = std::clamp(static_cast<int>((pos.y + radius) / kCellSize), 0, rows - 1);

  for (int cy = minY; cy <= maxY; ++cy) {
    for (int cx = minX; cx <= maxX; ++cx) {
      const auto& cell = cells[cy * cols + cx];
      outIndices.insert(outIndices.end(), cell.begin(), cell.end());
    }
  }
}

} // namespace ed::descent
