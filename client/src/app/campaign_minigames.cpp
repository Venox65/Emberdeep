#include "campaign_minigames.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "game_actions.hpp"

namespace ed::app {

void drawCampaignMinigames(const AppState& app, const ed::BossDef& boss) {
  if (app.campaignIntro) {
    DrawRectangle(210, 165, 860, 390, Color{12, 13, 25, 235});
    DrawRectangleLinesEx({210, 165, 860, 390}, 3.0F, Color{224, 187, 106, 255});
    drawTextCentered(boss.name, 245, 40, Color{255, 224, 145, 255});
    drawTextCentered(boss.challenge == ed::BossChallenge::FrenzyDig
                             ? "GRUBNOK'S FRENZY DIG"
                                 : boss.challenge == ed::BossChallenge::BoneDuel
                                       ? "SIR RATTLEBONE'S BONE DUEL"
                                       : boss.challenge == ed::BossChallenge::CinderDance
                                             ? "MORWENNA'S CINDER DANCE"
                                             : boss.challenge == ed::BossChallenge::HeadHunt
                                                   ? "THESSALY'S HEAD HUNT"
                                                   : boss.challenge == ed::BossChallenge::ParryDuel
                                                         ? "ALDRIC'S PARRY DUEL"
                                                         : boss.challenge == ed::BossChallenge::RuneRecall
                                                               ? "ISOLDE'S RUNE RECALL"
                                                               : boss.challenge == ed::BossChallenge::StarTrace
                                                                     ? "SERAPHEL'S STAR TRACE"
                                                                           : boss.challenge == ed::BossChallenge::Heartpierce
                                                                                 ? "IGNATHIA'S HEARTPIERCE"
                                                                                 : boss.challenge == ed::BossChallenge::SoulDodge
                                                                                       ? "XAR'MAL'S SOUL DODGE"
                                                                                       : "THE HOLLOW KING'S FIVE TRIALS",
                             305, 24, Color{199, 199, 218, 255});
    drawTextCentered(boss.challenge == ed::BossChallenge::FrenzyDig
                             ? "Mine the golden throne before the frenzy ends."
                             : boss.challenge == ed::BossChallenge::BoneDuel
                                   ? "Parry the incoming marker inside the green zone."
                                   : boss.challenge == ed::BossChallenge::CinderDance
                                         ? "Keep the ember rhythm alive across four lanes."
                                         : boss.challenge == ed::BossChallenge::HeadHunt
                                               ? "Strike real hydra heads and ignore the blue decoys."
                                               : boss.challenge == ed::BossChallenge::ParryDuel
                                                     ? "Match each attack direction before it lands."
                                                     : boss.challenge == ed::BossChallenge::RuneRecall
                                                           ? "Memorize each rune sequence, then repeat it."
                                                           : boss.challenge == ed::BossChallenge::StarTrace
                                                                 ? "Touch the numbered stars in order."
                                                                 : boss.challenge == ed::BossChallenge::Heartpierce
                                                                       ? "Track the heart with the lance."
                                                                       : boss.challenge == ed::BossChallenge::SoulDodge
                                                                             ? "Survive the deterministic bullet patterns."
                                                                             : "Complete all five trials with shared hearts.",
                         375, 20, Color{227, 222, 205, 255});
    drawTextCentered("Press ENTER when ready   |   ESC to return", 470, 22, Color{122, 217, 255, 255});
    return;
  }

  if (!app.campaignAttempt) {
    DrawText("Press ENTER to view the boss title card", 300, 452, 18,
             Color{178, 178, 190, 255});
    return;
  }

  const auto& playerStats = ed::calculatePlayerStats(app.game, app.inventory);

  if (boss.challenge == ed::BossChallenge::FrenzyDig) {
    DrawCircle(640, 350, 105, Color{105, 73, 48, 255});
    DrawCircleLines(640, 350, 105, Color{235, 194, 109, 255});
    if (app.frenzyDig.nuggetVisible) {
      DrawCircle(640.0F + (app.frenzyDig.nuggetX - 0.5F) * 700.0F,
                 350.0F + (app.frenzyDig.nuggetY - 0.5F) * 260.0F, 18,
                 Color{255, 221, 72, 255});
    }
    DrawText(("Taps: " + std::to_string(app.frenzyDig.taps) + "   Time: " +
              std::to_string(static_cast<int>(10.0F - app.frenzyDig.elapsed)) + "s")
                 .c_str(),
             410, 505, 22, RAYWHITE);
    DrawText("Golden nugget gives +5 taps", 475, 545, 18, Color{255, 221, 72, 255});
  } else if (boss.challenge == ed::BossChallenge::BoneDuel) {
    DrawRectangle(360, 325, 560, 18, Color{57, 55, 70, 255});
    const float zone = ed::boneDuelZoneWidth(
        app.boneDuel, static_cast<double>(playerStats.focusBasisPoints) / 10000.0);
    DrawRectangle(640 - static_cast<int>(280 * zone), 325,
                  static_cast<int>(560 * zone), 18, Color{92, 208, 117, 255});
    DrawRectangle(static_cast<int>(360 + 560 * app.boneDuel.needle) - 5, 300, 10, 68,
                  Color{235, 235, 245, 255});
    DrawLine(640, 292, 640, 375, Color{255, 221, 72, 255});
    DrawText(("Round " + std::to_string(app.boneDuel.round) + "/8   Misses: " +
              std::to_string(app.boneDuel.misses) + "   Perfect: " +
              std::to_string(app.boneDuel.perfectHits))
                 .c_str(),
             425, 430, 21, RAYWHITE);
    DrawText("Click or press Space to parry inside the green zone", 385, 475, 18,
             Color{174, 235, 187, 255});
  } else if (boss.challenge == ed::BossChallenge::CinderDance) {
    DrawText(("Groove: " + std::to_string(app.cinderDance.groove) +
              "   Perfect: " + std::to_string(app.cinderDance.perfectHits) +
              "   Good: " + std::to_string(app.cinderDance.goodHits) +
              "   Misses: " + std::to_string(app.cinderDance.misses))
                 .c_str(),
             390, 255, 21, RAYWHITE);
    for (int lane = 0; lane < 4; ++lane) {
      const int x = 350 + lane * 150;
      DrawRectangle(x, 300, 110, 250, Color{38, 34, 53, 255});
      DrawRectangle(x, 505, 110, 12, Color{255, 197, 80, 255});
      const char* labels[] = {"D", "F", "J", "K"};
      DrawText(labels[lane], x + 43, 535, 24, Color{255, 230, 160, 255});
    }
    for (const auto& note : app.cinderDance.chart) {
      const float distance = note.time - app.cinderDance.elapsed;
      if (!note.judged && distance >= -0.15F && distance <= 2.1F) {
        const int laneX = 350 + note.lane * 150;
        const int y = 505 - static_cast<int>(distance * 100.0F);
        const int noteW = 110;
        const int noteH = 28;
        const int noteY = y - 14;
        if (noteY + noteH >= 300 && noteY <= 550) {
          DrawRectangle(laneX, noteY, noteW, noteH, Color{255, 118, 91, 255});
          DrawRectangleLinesEx(
              {static_cast<float>(laneX), static_cast<float>(noteY),
               static_cast<float>(noteW), static_cast<float>(noteH)},
              3.0F, BLACK);
        }
      }
    }
    DrawText("Hit the lane when its note reaches the gold line", 405, 580, 18,
             Color{255, 221, 150, 255});
  } else if (boss.challenge == ed::BossChallenge::HeadHunt) {
    DrawText(("Real heads: " + std::to_string(app.headHunt.realHits) +
              "   Decoys: " + std::to_string(app.headHunt.decoyHits) +
              "   Hearts lost: " + std::to_string(app.headHunt.heartsLost))
                 .c_str(),
             380, 255, 21, RAYWHITE);
    for (int cell = 0; cell < 9; ++cell) {
      const int x = 430 + (cell % 3) * 140;
      const int y = 290 + (cell / 3) * 95;
      DrawCircle(x + 45, y + 35, 32, Color{45, 38, 48, 255});
      const auto target = std::find_if(
          app.headHunt.targets.begin(), app.headHunt.targets.end(),
          [cell](const ed::HeadHuntTarget& item) { return item.cell == cell; });
      if (target != app.headHunt.targets.end()) {
        DrawCircle(x + 45, y + 35, 23,
                   target->real ? Color{79, 194, 108, 255} : Color{74, 157, 220, 255});
        DrawCircle(x + 38, y + 29, 4, Color{240, 238, 174, 255});
        DrawCircle(x + 52, y + 29, 4, Color{240, 238, 174, 255});
      }
      DrawText(std::to_string(cell + 1).c_str(), x + 40, y + 70, 16,
               Color{180, 180, 194, 255});
    }
    DrawText("Click a cell or press 1-9 to strike", 455, 585, 18,
             Color{174, 235, 187, 255});
  } else if (boss.challenge == ed::BossChallenge::ParryDuel) {
    DrawText(("Attack " + std::to_string(app.parryDuel.attack) + "/24   Parries: " +
              std::to_string(app.parryDuel.successfulParries) + "   Hearts lost: " +
              std::to_string(app.parryDuel.heartsLost))
                 .c_str(),
             390, 255, 21, RAYWHITE);
    const char* directions[] = {"LEFT", "DOWN", "RIGHT"};
    const std::string telegraph =
        app.parryDuel.feint ? "FEINT — do not press a direction"
                            : "INCOMING: " + std::string(directions[app.parryDuel.direction]);
    DrawText(telegraph.c_str(), 430, 330, 22,
             app.parryDuel.feint ? Color{255, 139, 102, 255}
                                 : Color{255, 224, 145, 255});
    const float reactionWindow =
        std::max(0.35F, 0.6F - 0.25F * app.parryDuel.attack / 23.0F);
    DrawRectangle(430, 365, 420, 10, Color{49, 45, 62, 255});
    DrawRectangle(430, 365,
                  static_cast<int>(420.0F *
                                   std::clamp(1.0F - app.parryDuel.elapsed / reactionWindow,
                                              0.0F, 1.0F)),
                  10, Color{255, 139, 102, 255});
    const char* labels[] = {"LEFT", "DOWN", "RIGHT"};
    for (int direction = 0; direction < 3; ++direction) {
      const int x = 330 + direction * 220;
      DrawRectangleRounded({static_cast<float>(x), 390, 170, 70}, 0.15F, 8,
                           direction == app.parryDuel.direction && !app.parryDuel.feint
                               ? Color{108, 78, 48, 255}
                               : Color{67, 62, 82, 255});
      DrawText(labels[direction], x + 42, 414, 18, Color{255, 224, 145, 255});
    }
    DrawText("Arrow keys or click a direction", 475, 520, 18,
             Color{190, 190, 205, 255});
  } else if (boss.challenge == ed::BossChallenge::RuneRecall) {
    DrawText(("Round " + std::to_string(app.runeRecall.round) + "/10   Mistakes: " +
              std::to_string(app.runeRecall.mistakes))
                 .c_str(),
             465, 255, 21, RAYWHITE);
    DrawText(app.runeRecall.showing ? "MEMORIZE THIS SEQUENCE"
                                    : "Repeat the rune sequence with keys 1-6",
             430, 310, 20,
             app.runeRecall.showing ? Color{255, 224, 145, 255}
                                    : Color{218, 211, 190, 255});
    if (app.runeRecall.showing) {
      std::string sequence;
      for (const int rune : app.runeRecall.sequence) {
        if (!sequence.empty()) sequence += "  ";
        sequence += std::to_string(rune + 1);
      }
      DrawText(sequence.c_str(), 500, 345, 28, Color{255, 238, 180, 255});
    } else {
      DrawText(("Progress: " + std::to_string(app.runeRecall.inputIndex) + "/" +
                std::to_string(app.runeRecall.sequence.size()))
                   .c_str(),
               500, 345, 20, Color{174, 235, 187, 255});
    }
    for (int rune = 0; rune < 6; ++rune) {
      const int x = 285 + (rune % 3) * 245;
      const int y = 370 + (rune / 3) * 100;
      DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 180, 70},
                           0.12F, 8, Color{59, 76, 103, 255});
      DrawText(std::to_string(rune + 1).c_str(), x + 82, y + 22, 24,
               Color{255, 224, 145, 255});
    }
  } else if (boss.challenge == ed::BossChallenge::StarTrace) {
    DrawText(("Time: " + std::to_string(static_cast<int>(40.0F - app.starTrace.elapsed)) +
              "s   Constellations: " + std::to_string(app.starTrace.constellation) +
              "   Hearts lost: " + std::to_string(app.starTrace.heartsLost))
                 .c_str(),
             355, 255, 21, RAYWHITE);
    for (std::size_t index = 0; index < app.starTrace.stars.size(); ++index) {
      const int x = 300 + static_cast<int>(app.starTrace.starX[index] * 600.0F);
      const int y = 280 + static_cast<int>(app.starTrace.starY[index] * 300.0F);
      DrawCircle(x, y, 30, index == static_cast<std::size_t>(app.starTrace.targetIndex)
                               ? Color{255, 224, 105, 255}
                               : Color{103, 137, 195, 255});
      DrawText(std::to_string(app.starTrace.stars[index] + 1).c_str(), x - 8, y - 10, 20,
               Color{20, 25, 43, 255});
    }
    DrawText("Tap the highlighted stars in order. Wrong stars cost a heart.", 375, 575, 18,
             Color{255, 224, 145, 255});
  } else if (boss.challenge == ed::BossChallenge::Heartpierce) {
    const char* phaseNames[] = {"Cold Scales", "Ember Heat", "Wyrmfire Fury"};
    DrawText(("Phase " + std::to_string(app.heartpierce.phase + 1) + ": " +
              phaseNames[app.heartpierce.phase] + "   Hearts: " +
              std::to_string(app.heartpierce.hearts) + "   Overlap: " +
              std::to_string(static_cast<int>(app.heartpierce.gauge * 100.0F)) + "%")
                 .c_str(),
             350, 255, 21, RAYWHITE);
    DrawText(("Time: " + std::to_string(static_cast<int>(app.heartpierce.totalTime)) +
              "s   Hearts lost: " + std::to_string(app.heartpierce.heartsLost) +
              "   Heartpierce: " +
              std::to_string(static_cast<int>(app.heartpierce.gauge * 100.0F)) + "%")
                 .c_str(), 350, 285, 18, Color{255, 224, 145, 255});
    DrawRectangle(365, 325, 290, 220, Color{45, 34, 43, 255});
    DrawCircle(510, 420, 78, Color{113, 39, 39, 255});
    DrawCircle(510, 420, 58, Color{238, 80, 42, 255});
    DrawCircle(535, 397, 13, Color{255, 220, 106, 255});
    DrawCircleLines(510, 420, 92, Color{230, 220, 190, 255});
    DrawRectangle(800, 275, 80, 300, Color{30, 33, 43, 255});
    DrawRectangle(822, 295, 36, 260, Color{64, 67, 80, 255});
    const int heartY = 555 - static_cast<int>(app.heartpierce.heartY * 240.0F);
    const int lanceY = 555 - static_cast<int>(app.heartpierce.lanceY * 240.0F);
    const float zonePixels = (0.09F + 0.04F * (2 - app.heartpierce.phase)) *
                             (1.0F + app.heartpierce.focus) * 240.0F;
    DrawRectangle(805, heartY - static_cast<int>(zonePixels),
                  70, static_cast<int>(zonePixels * 2.0F),
                  Color{255, 80, 48, 55});
    DrawCircle(840, heartY, 17, app.heartpierce.dartTelegraph > 0.0F
                                  ? Color{255, 255, 255, 255}
                                  : Color{255, 85, 45, 255});
    DrawRectangle(813, lanceY - 12, 54, 24, Color{118, 205, 210, 255});
    DrawTriangle({840, static_cast<float>(lanceY - 32)},
                 {825, static_cast<float>(lanceY - 10)},
                 {855, static_cast<float>(lanceY - 10)}, Color{211, 235, 229, 255});
    DrawRectangle(680, 350, 18, 200, Color{45, 35, 38, 255});
    DrawRectangle(680, 550 - static_cast<int>(app.heartpierce.gauge * 200.0F),
                  18, static_cast<int>(app.heartpierce.gauge * 200.0F),
                  Color{255, 194, 55, 255});
    DrawText("Hold SPACE or mouse button to rise; release to fall.", 350, 600, 18,
             Color{255, 224, 145, 255});
  } else if (boss.challenge == ed::BossChallenge::SoulDodge) {
    DrawText(("Time: " + std::to_string(static_cast<int>(60.0F - app.soulDodge.elapsed)) +
              "s   Hearts: " + std::to_string(app.soulDodge.hearts) +
              "   Hits: " + std::to_string(app.soulDodge.hits)).c_str(),
             390, 255, 21, RAYWHITE);
    DrawRectangle(300, 285, 680, 300, Color{18, 20, 39, 255});
    DrawRectangleLines(300, 285, 680, 300, Color{118, 91, 157, 255});
    for (const auto& hazard : app.soulDodge.hazards) {
      if (hazard.kind == ed::SoulHazardKind::Laser) {
        const int x = 300 + static_cast<int>(hazard.x * 680.0F);
        DrawRectangle(x - 5, 285, 10, 300,
                      hazard.telegraph > 0.0F ? Color{255, 238, 166, 150}
                                             : Color{214, 80, 255, 220});
      } else {
        const float hazardX = hazard.kind == ed::SoulHazardKind::Rain
                                  ? hazard.x
                                  : hazard.x + std::cos(hazard.angle) * hazard.radius;
        const float hazardY = hazard.kind == ed::SoulHazardKind::Rain
                                  ? hazard.y
                                  : hazard.y + std::sin(hazard.angle) * hazard.radius;
        const int x = 300 + static_cast<int>(hazardX * 680.0F);
        const int y = 285 + static_cast<int>(hazardY * 300.0F);
        DrawCircle(x, y, hazard.kind == ed::SoulHazardKind::Ring ? 12 : 8,
                   hazard.telegraph > 0.0F ? Color{255, 255, 255, 160}
                                           : Color{178, 75, 232, 230});
      }
    }
    DrawCircle(300 + static_cast<int>(app.soulDodge.playerX * 680.0F),
               285 + static_cast<int>(app.soulDodge.playerY * 300.0F), 7,
               app.soulDodge.invulnerable > 0.0F ? Color{255, 255, 255, 180}
                                                 : Color{255, 88, 132, 255});
    DrawText("Move with WASD/arrows. Hold Shift for precision movement.", 365, 615, 18,
             Color{255, 224, 145, 255});
  } else if (boss.challenge == ed::BossChallenge::FiveTrials) {
    DrawText(("TRIAL " + std::to_string(app.fiveTrials.phase) +
              " / 5   Hearts: " + std::to_string(app.fiveTrials.hearts) +
              "   Lost: " + std::to_string(app.fiveTrials.heartsLost)).c_str(),
             420, 255, 21, RAYWHITE);
    DrawRectangle(350, 320, 580, 180, Color{32, 35, 53, 255});
    if (app.fiveTrials.phase == 1) {
      DrawText(("BURST: " + std::to_string(std::max(0, 6 - static_cast<int>(app.fiveTrials.burstElapsed))) +
                " seconds remaining").c_str(), 440, 390, 24, RAYWHITE);
      DrawText(("Taps: " + std::to_string(app.fiveTrials.burstTaps) +
                " / 48").c_str(), 540, 440, 22, Color{255, 224, 145, 255});
    } else if (app.fiveTrials.phase == 2) {
      DrawText(("RHYTHM: D F J K   " +
                std::to_string(std::max(0, 30 - static_cast<int>(app.fiveTrials.phaseElapsed))) +
                " seconds").c_str(), 420, 300, 24, RAYWHITE);
      const char* laneKeys[] = {"D", "F", "J", "K"};
      for (int lane = 0; lane < 4; ++lane) {
        const int laneX = 390 + lane * 120;
        DrawRectangle(laneX, 330, 116, 160,
                      lane % 2 == 0 ? Color{37, 43, 66, 255}
                                    : Color{31, 37, 58, 255});
        DrawRectangleLines(laneX, 330, 116, 160, Color{93, 107, 145, 255});
        DrawRectangle(laneX + 5, 450, 106, 35, Color{71, 103, 132, 255});
        DrawRectangleLinesEx({static_cast<float>(laneX + 5), 450, 106, 35},
                             2.0F, Color{255, 224, 145, 255});
        DrawText(laneKeys[lane], laneX + 51, 456, 22, Color{255, 238, 180, 255});
      }
      const auto& chart = app.cinderDance.chart;
      for (std::size_t index = app.cinderDance.nextNote; index < chart.size(); ++index) {
        const auto& note = chart[index];
        const float distance = note.time - app.cinderDance.elapsed;
        if (distance > 1.2F) break;
        if (note.judged || distance < -0.15F) continue;
        const int laneX = 390 + note.lane * 120;
        const float speed = 180.0F;
        const float noteCenterY = 467.5F - distance * speed;
        const float noteH = 34.0F;
        const float noteY = noteCenterY - noteH / 2.0F;
        const float noteX = static_cast<float>(laneX + 5);
        const float noteW = 106.0F;
        if (noteY + noteH >= 330.0F && noteY <= 490.0F) {
          DrawRectangle(static_cast<int>(noteX), static_cast<int>(noteY),
                        static_cast<int>(noteW), static_cast<int>(noteH),
                        Color{255, 168, 76, 255});
          DrawRectangleLinesEx({noteX, noteY, noteW, noteH}, 3.0F, BLACK);
        }
      }
      DrawText("Hit when the note reaches the outlined receptor.", 445, 510, 17,
               Color{255, 224, 145, 255});
    } else if (app.fiveTrials.phase == 3) {
      DrawText(("TRIAL 3: PARRY DUEL — Attack " +
                std::to_string(std::min(app.parryDuel.attack + 1, app.parryDuel.maxAttacks)) +
                "/" + std::to_string(app.parryDuel.maxAttacks) +
                "   Parries: " + std::to_string(app.parryDuel.successfulParries) +
                "   Hearts: " + std::to_string(app.fiveTrials.hearts)).c_str(),
               360, 280, 21, RAYWHITE);
      const char* directions[] = {"LEFT", "DOWN", "RIGHT"};
      const std::string telegraph =
          app.parryDuel.feint ? "FEINT — do not press a direction"
                              : "INCOMING: " + std::string(directions[app.parryDuel.direction]);
      DrawText(telegraph.c_str(), 430, 315, 22,
               app.parryDuel.feint ? Color{255, 139, 102, 255}
                                   : Color{255, 224, 145, 255});
      const float reactionWindow =
          std::max(0.35F, 0.6F - 0.25F * app.parryDuel.attack / 23.0F);
      DrawRectangle(430, 348, 420, 10, Color{49, 45, 62, 255});
      DrawRectangle(430, 348,
                    static_cast<int>(420.0F *
                                     std::clamp(1.0F - app.parryDuel.elapsed / reactionWindow,
                                                0.0F, 1.0F)),
                    10, Color{255, 139, 102, 255});
      const char* labels[] = {"LEFT (A)", "DOWN (S)", "RIGHT (D)"};
      for (int direction = 0; direction < 3; ++direction) {
        const int x = 330 + direction * 220;
        DrawRectangleRounded({static_cast<float>(x), 370, 170, 70}, 0.15F, 8,
                             direction == app.parryDuel.direction && !app.parryDuel.feint
                                 ? Color{108, 78, 48, 255}
                                 : Color{67, 62, 82, 255});
        DrawRectangleLinesEx({static_cast<float>(x), 370, 170, 70}, 2.0F,
                             direction == app.parryDuel.direction && !app.parryDuel.feint
                                 ? Color{255, 224, 145, 255}
                                 : Color{93, 107, 145, 255});
        DrawText(labels[direction], x + 35, 394, 18, Color{255, 224, 145, 255});
      }
      DrawText("Use Left/Down/Right, A/S/D or click a direction", 420, 460, 18,
               Color{190, 190, 205, 255});
    } else if (app.fiveTrials.phase == 4) {
      DrawText(("TRIAL 4: SOUL DODGE — " +
                std::to_string(static_cast<int>(std::max(0.0F, 40.0F - app.soulDodge.elapsed))) +
                "s   Hearts: " + std::to_string(app.fiveTrials.hearts) +
                "   Hits: " + std::to_string(app.soulDodge.hits)).c_str(),
               380, 275, 21, RAYWHITE);
      DrawRectangle(320, 305, 640, 250, Color{18, 20, 39, 255});
      DrawRectangleLines(320, 305, 640, 250, Color{118, 91, 157, 255});
      for (const auto& hazard : app.soulDodge.hazards) {
        if (hazard.kind == ed::SoulHazardKind::Laser) {
          const int x = 320 + static_cast<int>(hazard.x * 640.0F);
          DrawRectangle(x - 5, 305, 10, 250,
                        hazard.telegraph > 0.0F ? Color{255, 238, 166, 150}
                                               : Color{214, 80, 255, 220});
        } else {
          const float hazardX = hazard.kind == ed::SoulHazardKind::Rain
                                    ? hazard.x
                                    : hazard.x + std::cos(hazard.angle) * hazard.radius;
          const float hazardY = hazard.kind == ed::SoulHazardKind::Rain
                                    ? hazard.y
                                    : hazard.y + std::sin(hazard.angle) * hazard.radius;
          const int x = 320 + static_cast<int>(hazardX * 640.0F);
          const int y = 305 + static_cast<int>(hazardY * 250.0F);
          DrawCircle(x, y, hazard.kind == ed::SoulHazardKind::Ring ? 10 : 7,
                     hazard.telegraph > 0.0F ? Color{255, 255, 255, 160}
                                             : Color{178, 75, 232, 230});
        }
      }
      DrawCircle(320 + static_cast<int>(app.soulDodge.playerX * 640.0F),
                 305 + static_cast<int>(app.soulDodge.playerY * 250.0F), 7,
                 app.soulDodge.invulnerable > 0.0F ? Color{255, 255, 255, 180}
                                                   : Color{255, 88, 132, 255});
      DrawText("WASD/Arrows to dodge   Shift for precision", 440, 565, 18,
               Color{255, 224, 145, 255});
    } else {
      DrawText("CROWN STRIKE: press SPACE in the center zone", 395, 405, 21, RAYWHITE);
      DrawRectangle(450, 455, 380, 16, Color{57, 55, 70, 255});
      DrawRectangle(635, 451, 10, 24, Color{92, 208, 117, 255});
      DrawRectangle(450 + static_cast<int>(380 * app.crownStrike.needle) - 5,
                    440, 10, 45, RAYWHITE);
    }
  }
}

bool handleActiveCampaignInput(AppState& app) {
  if (app.screen != Screen::Campaign && !app.descentMinigameActive) return false;
  if (app.campaignIntro) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
      app.campaignIntro = false;
      return true;
    }
  }
  if (!app.campaignAttempt) return false;
  if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
    if (app.descentMinigameActive) {
      finishDescentVaultMinigame(app, false);
      return true;
    }
    app.campaignAttempt = false;
    app.campaignIntro = false;
    app.campaignScore = 0;
    app.campaignHeartsLost = 0;
    setNotice(app, "Boss attempt abandoned.");
    return true;
  }

  const auto bossIdx = app.descentMinigameActive ? static_cast<std::size_t>(app.descentMinigameBossIdx)
                                                 : static_cast<std::size_t>(app.selectedBoss);
  const auto challenge = app.bosses[bossIdx].challenge;
  if (challenge == ed::BossChallenge::FrenzyDig &&
      (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    const Vector2 mouse = GetMousePosition();
    const float normalizedX = std::clamp((mouse.x - 290.0F) / 700.0F, 0.0F, 1.0F);
    const float normalizedY = std::clamp((mouse.y - 220.0F) / 260.0F, 0.0F, 1.0F);
    ed::tapFrenzyDig(app.frenzyDig, normalizedX, normalizedY, app.rng);
    return true;
  }
  if (challenge == ed::BossChallenge::BoneDuel &&
      (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
    if (!ed::strikeBoneDuel(
            app.boneDuel, static_cast<double>(stats.focusBasisPoints) / 10000.0)) {
      setNotice(app, "Miss! The needle must be inside the green zone.");
    }
    return true;
  }
  if (challenge == ed::BossChallenge::CinderDance) {
    const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
    const int keys[] = {KEY_D, KEY_F, KEY_J, KEY_K};
    for (int lane = 0; lane < 4; ++lane) {
      if (IsKeyPressed(keys[lane])) {
        ed::judgeCinderDance(
            app.cinderDance, lane,
            static_cast<double>(stats.focusBasisPoints) / 10000.0);
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= 290.0F && mouse.y <= 550.0F) {
        const int lane = static_cast<int>((mouse.x - 350.0F) / 150.0F);
        if (lane >= 0 && lane < 4) {
          ed::judgeCinderDance(
              app.cinderDance, lane,
              static_cast<double>(stats.focusBasisPoints) / 10000.0);
        }
      }
    }
    return true;
  }
  if (challenge == ed::BossChallenge::HeadHunt) {
    int cell = -1;
    const int keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
                        KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE};
    for (int index = 0; index < 9; ++index) {
      if (IsKeyPressed(keys[index])) cell = index;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.x >= 430.0F && mouse.x < 850.0F &&
          mouse.y >= 290.0F && mouse.y < 575.0F) {
        const int column = static_cast<int>((mouse.x - 430.0F) / 140.0F);
        const int row = static_cast<int>((mouse.y - 290.0F) / 95.0F);
        if (column < 3 && row < 3) cell = row * 3 + column;
      }
    }
    if (cell >= 0) ed::hitHeadHunt(app.headHunt, cell);
    return true;
  }
  if (challenge == ed::BossChallenge::ParryDuel) {
    int direction = -1;
    if (IsKeyPressed(KEY_LEFT)) direction = 0;
    if (IsKeyPressed(KEY_DOWN)) direction = 1;
    if (IsKeyPressed(KEY_RIGHT)) direction = 2;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= 390.0F && mouse.y <= 460.0F &&
          mouse.x >= 330.0F && mouse.x < 990.0F) {
        direction = std::clamp(static_cast<int>((mouse.x - 330.0F) / 220.0F), 0, 2);
      }
    }
    if (direction >= 0 && !ed::parryDuelInput(app.parryDuel, direction)) {
      setNotice(app, "Wrong parry or feint.");
    }
    return true;
  }
  if (challenge == ed::BossChallenge::RuneRecall) {
    int rune = -1;
    const int keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX};
    for (int index = 0; index < 6; ++index) {
      if (IsKeyPressed(keys[index])) rune = index;
    }
    if (rune >= 0 && !ed::inputRuneRecall(app.runeRecall, rune)) {
      setNotice(app, "Wrong rune. The sequence resets.");
    }
    return true;
  }
  if (challenge == ed::BossChallenge::StarTrace) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      for (std::size_t index = 0; index < app.starTrace.stars.size(); ++index) {
        const int x = 300 + static_cast<int>(app.starTrace.starX[index] * 600.0F);
        const int y = 280 + static_cast<int>(app.starTrace.starY[index] * 300.0F);
        if (CheckCollisionPointCircle(mouse, {static_cast<float>(x), static_cast<float>(y)}, 34.0F)) {
          ed::hitStarTrace(app.starTrace, app.starTrace.stars[index]);
          break;
        }
      }
    }
    return true;
  }
  if (challenge == ed::BossChallenge::Heartpierce) {
    return true;
  }
  if (challenge == ed::BossChallenge::SoulDodge) {
    return true;
  }
  if (challenge == ed::BossChallenge::FiveTrials) {
    if (app.fiveTrials.phase == 1 &&
        (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
      ++app.fiveTrials.burstTaps;
    } else if (app.fiveTrials.phase == 2) {
      const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
      const int keys[] = {KEY_D, KEY_F, KEY_J, KEY_K};
      for (int lane = 0; lane < 4; ++lane) {
        if (IsKeyPressed(keys[lane])) {
          ed::judgeCinderDance(app.cinderDance, lane,
                               static_cast<double>(stats.focusBasisPoints) / 10000.0);
        }
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        if (mouse.y >= 440.0F && mouse.y <= 500.0F &&
            mouse.x >= 390.0F && mouse.x < 870.0F) {
          const int lane = static_cast<int>((mouse.x - 390.0F) / 120.0F);
          if (lane >= 0 && lane < 4) {
            ed::judgeCinderDance(
                app.cinderDance, lane,
                static_cast<double>(stats.focusBasisPoints) / 10000.0);
          }
        }
      }
    } else if (app.fiveTrials.phase == 3) {
      int direction = -1;
      if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) direction = 0;
      if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) direction = 1;
      if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) direction = 2;
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        if (mouse.y >= 370.0F && mouse.y <= 440.0F &&
            mouse.x >= 330.0F && mouse.x < 990.0F) {
          direction = std::clamp(static_cast<int>((mouse.x - 330.0F) / 220.0F), 0, 2);
        }
      }
      if (direction >= 0 && !ed::parryDuelInput(app.parryDuel, direction)) {
        setNotice(app, "Wrong parry or feint.");
      }
    } else if (app.fiveTrials.phase == 5 &&
               (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
      ed::strikeCrown(app.crownStrike, 0.0F);
    }
    return true;
  }
  return true;
}

} // namespace ed::app
