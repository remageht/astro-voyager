#pragma once

#include <string>
#include <vector>
#include <utility>

namespace astro {

enum class LoadingPhase : int {
  None = 0,
  ColdBoot = 1,
  NewGameWarp = 2,
  ContinueWarp = 3
};

class LoadingScreen {
 public:
  LoadingScreen();

  void startColdBoot();
  void startNewGameWarp(const std::string& directive = "");
  void startContinueWarp(const std::string& destination = "");

  void update(float dt);
  void draw(float screenW, float screenH);

  bool isActive() const { return m_phase != LoadingPhase::None; }
  LoadingPhase phase() const { return m_phase; }
  float progress() const { return m_progress; }
  void skip();

  bool justFinished() const { return m_justFinished; }
  void clearFinishedFlag() { m_justFinished = false; }

  const std::string& quote() const { return m_quote; }
  const std::string& quoteAuthor() const { return m_quoteAuthor; }

 private:
  LoadingPhase m_phase = LoadingPhase::None;
  float m_timer = 0.0f;
  float m_duration = 2.2f;
  float m_progress = 0.0f;
  bool m_justFinished = false;

  std::string m_title;
  std::string m_subtitle;
  std::string m_quote;
  std::string m_quoteAuthor;
  std::vector<std::pair<float, std::string>> m_telemetrySteps;

  float m_reticleAngle = 0.0f;
  float m_pulseTimer = 0.0f;

  void pickRandomQuote();
};

}  // namespace astro
