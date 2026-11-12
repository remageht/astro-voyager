#include "LoadingScreen.h"

#include <cmath>
#include <algorithm>
#include <random>

#ifndef ASTRO_NO_IMGUI
#include <imgui.h>
#endif

namespace astro {

namespace {
struct QuoteEntry {
  const char* text;
  const char* author;
};

static const QuoteEntry kQuotes[] = {
  {
    "«Внутри чёрной дыры само пространство падает быстрее света, увлекая за собой время.»",
    "Кип Торн, «Интерстеллар: наука за кадром»"
  },
  {
    "«Гравитация — это не невидимая сила, а искривление ткани самого пространства-времени.»",
    "Альберт Эйнштейн"
  },
  {
    "«Где-то там в космосе что-то невероятное ждёт, чтобы мы открыли его.»",
    "Карл Саган, «Космос»"
  },
  {
    "«Чёрные дыры не так безнадёжно черны, как нам казалось. Из них есть путь наружу.»",
    "Стивен Хокинг"
  },
  {
    "«Кольцо Алькубьерре сжимает пространство спереди и расширяет сзади, создавая пузырь сингулярности.»",
    "Мигель Алькубьерре (1994)"
  },
  {
    "«Астронавигатор Егорка готов к расчёту гравитационных линз и геодезических Керра.»",
    "Бортовой ИИ Егорка v5.0.8"
  }
};
static const size_t kNumQuotes = sizeof(kQuotes) / sizeof(kQuotes[0]);
}  // namespace

LoadingScreen::LoadingScreen() {
  pickRandomQuote();
}

void LoadingScreen::pickRandomQuote() {
  static unsigned int s_counter = 0;
  size_t idx = s_counter++ % kNumQuotes;
  m_quote = kQuotes[idx].text;
  m_quoteAuthor = kQuotes[idx].author;
}

void LoadingScreen::startColdBoot() {
  m_phase = LoadingPhase::ColdBoot;
  m_timer = 0.0f;
  m_duration = 2.2f;
  m_progress = 0.0f;
  m_justFinished = false;
  m_reticleAngle = 0.0f;
  m_pulseTimer = 0.0f;

  m_title = "ASTRO-VOYAGER // ИНИЦИАЛИЗАЦИЯ БОРТОВЫХ СИСТЕМ";
  m_subtitle = "КАЛИБРОВКА КВАНТОВОГО ЯДРА И ГРАВИТАЦИОННОЙ МАТРИЦЫ КЕРРА";

  m_telemetrySteps.clear();
  m_telemetrySteps.push_back({0.10f, "[ OK ] ТЕНЗОР КРИВИЗНЫ РИМАНА: МЕТРИКА КЕРРА a/M = 0.95"});
  m_telemetrySteps.push_back({0.30f, "[ OK ] АСТРОНОМИЧЕСКИЙ РЕЕСТР: 22 НЕБЕСНЫХ ТЕЛА И СТАНЦИИ"});
  m_telemetrySteps.push_back({0.50f, "[ OK ] ПОСАДОЧНЫЙ ВЫСОТОМЕР: ЛАЗЕРНАЯ ТОПОЛОГИЯ 16 РУМБОВ"});
  m_telemetrySteps.push_back({0.70f, "[ OK ] СИСТЕМА СВЯЗИ ЭКИПАЖА: МИА САТО, ЕЛЕНА РОСТОВА, ЕГОРКА"});
  m_telemetrySteps.push_back({0.90f, "[ OK ] ПУЗЫРЬ АЛЬКУБЬЕРРЕ: ВАРП-ПРИВОД ГОТОВ К МАКСИМАЛЬНОЙ ТЯГЕ"});

  pickRandomQuote();
}

void LoadingScreen::startNewGameWarp(const std::string& directive) {
  m_phase = LoadingPhase::NewGameWarp;
  m_timer = 0.0f;
  m_duration = 2.4f;
  m_progress = 0.0f;
  m_justFinished = false;
  m_reticleAngle = 0.0f;
  m_pulseTimer = 0.0f;

  m_title = "ГИПЕРПРОСТРАНСТВЕННЫЙ ПЕРЕХОД // ВХОД В ЧЕРВОВОТИНУ";
  if (!directive.empty()) {
    m_subtitle = "ДИРЕКТИВА ЭКСПЕДИЦИИ: " + directive;
  } else {
    m_subtitle = "КУРС: ТОЧКА ЛАГРАНЖА L1 // СВЕРХМАССИВНАЯ ЧЁРНАЯ ДЫРА ГАРГАНТЮА";
  }

  m_telemetrySteps.clear();
  m_telemetrySteps.push_back({0.15f, "[ WARP ] ДЕФОРМАЦИЯ ПРОСТРАНСТВА-ВРЕМЕНИ: РЕЛЯТИВИСТСКИЙ РАЗГОН"});
  m_telemetrySteps.push_back({0.45f, "[ WARP ] ГОРЛОВИНА ЧЕРВОВОТИНЫ: СТАБИЛИЗАЦИЯ ЭКЗОТИЧЕСКОЙ МАТЕРИЕЙ"});
  m_telemetrySteps.push_back({0.75f, "[ WARP ] СМЕЩЕНИЕ ЧЕРЕНКОВА: ФОТОННАЯ ВСПЫШКА ГОРИЗОНТА"});
  m_telemetrySteps.push_back({0.92f, "[ WARP ] ВЫХОД ИЗ ПОДПРОСТРАНСТВА В СЕКТОРЕ НАЗНАЧЕНИЯ"});

  pickRandomQuote();
}

void LoadingScreen::startContinueWarp(const std::string& destination) {
  m_phase = LoadingPhase::ContinueWarp;
  m_timer = 0.0f;
  m_duration = 1.6f;
  m_progress = 0.0f;
  m_justFinished = false;
  m_reticleAngle = 0.0f;
  m_pulseTimer = 0.0f;

  m_title = "СИНХРОНИЗАЦИЯ ТЕЛЕМЕТРИИ СУДНА СМК-01";
  if (!destination.empty()) {
    m_subtitle = "ВОССТАНОВЛЕНИЕ КООРДИНАТ: " + destination;
  } else {
    m_subtitle = "ВОССТАНОВЛЕНИЕ ОРБИТАЛЬНОГО ВЕКТОРА И ПОЛЕТНЫХ ЛОГОВ";
  }

  m_telemetrySteps.clear();
  m_telemetrySteps.push_back({0.20f, "[ SYNC ] БОРТОВОЙ ЖУРНАЛ И СОСТОЯНИЕ РЕСУРСОВ: ЗАГРУЖЕНО"});
  m_telemetrySteps.push_back({0.55f, "[ SYNC ] ТОПОЛОГИЯ СЕКТОРА И СТАТУС ДИРЕКТИВ: СИНХРОНИЗИРОВАНО"});
  m_telemetrySteps.push_back({0.85f, "[ SYNC ] ГОТОВНОСТЬ СИСТЕМ УПРАВЛЕНИЯ К ПОЛЁТУ: 100%"});

  pickRandomQuote();
}

void LoadingScreen::update(float dt) {
  if (m_phase == LoadingPhase::None) return;

  m_timer += dt;
  m_reticleAngle += dt * 1.8f;
  m_pulseTimer += dt;

  if (m_duration > 0.001f) {
    m_progress = std::clamp(m_timer / m_duration, 0.0f, 1.0f);
  } else {
    m_progress = 1.0f;
  }

  if (m_timer >= m_duration) {
    m_phase = LoadingPhase::None;
    m_justFinished = true;
    m_progress = 1.0f;
  }
}

void LoadingScreen::skip() {
  if (m_phase != LoadingPhase::None) {
    m_progress = 1.0f;
    m_phase = LoadingPhase::None;
    m_justFinished = true;
  }
}

void LoadingScreen::draw(float screenW, float screenH) {
  (void)screenW;
  (void)screenH;
#ifndef ASTRO_NO_IMGUI
  if (m_phase == LoadingPhase::None) return;

  ImDrawList* dl = ImGui::GetForegroundDrawList();
  if (!dl) return;

  float cx = screenW * 0.5f;
  float cy = screenH * 0.5f;

  // 1. Полноэкранный фон со звёздной бездной и затемнением
  ImU32 bgCol = IM_COL32(3, 5, 10, 245);
  dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, screenH), bgCol);

  // 2. Радиальные варп-линии при прыжке или тонкая сетка при инициализации
  if (m_phase == LoadingPhase::NewGameWarp || m_phase == LoadingPhase::ContinueWarp) {
    // Векторные полосы гиперпространственного перехода (Warp Streaks)
    const int kNumStreaks = 72;
    for (int i = 0; i < kNumStreaks; ++i) {
      float angle = (static_cast<float>(i) / kNumStreaks) * 6.2831853f + m_pulseTimer * 0.3f;
      float speedMult = 1.0f + static_cast<float>((i * 17) % 10) * 0.2f;
      float warpLen = (120.0f + 320.0f * m_progress) * speedMult;
      float r0 = 40.0f + std::fmod(m_pulseTimer * 400.0f * speedMult + i * 35.0f, cx * 1.2f);
      float r1 = r0 + warpLen * (0.3f + 0.7f * m_progress);

      float x0 = cx + std::cos(angle) * r0;
      float y0 = cy + std::sin(angle) * r0;
      float x1 = cx + std::cos(angle) * r1;
      float y1 = cy + std::sin(angle) * r1;

      float alphaNorm = std::clamp(1.0f - (r0 / (cx * 1.2f)), 0.05f, 0.9f);
      ImU32 streakCol = (i % 3 == 0)
          ? IM_COL32(110, 210, 255, static_cast<int>(alphaNorm * 220))
          : ((i % 3 == 1)
              ? IM_COL32(180, 230, 255, static_cast<int>(alphaNorm * 240))
              : IM_COL32(70, 140, 255, static_cast<int>(alphaNorm * 180)));

      dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), streakCol, 1.5f + (i % 2));
    }

    // Вспышка на финале варп-перехода (> 85%)
    if (m_progress > 0.85f) {
      float flashAlpha = (m_progress - 0.85f) / 0.15f;
      dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, screenH),
                        IM_COL32(230, 245, 255, static_cast<int>(flashAlpha * 240)));
    }
  } else {
    // Режим ColdBoot: тонкая технологическая сетка и частицы
    ImU32 gridCol = IM_COL32(25, 45, 75, 45);
    for (float x = 0.0f; x < screenW; x += 64.0f) {
      dl->AddLine(ImVec2(x, 0.0f), ImVec2(x, screenH), gridCol, 1.0f);
    }
    for (float y = 0.0f; y < screenH; y += 64.0f) {
      dl->AddLine(ImVec2(0.0f, y), ImVec2(screenW, y), gridCol, 1.0f);
    }
  }

  // 3. Центральный вращающийся гироскопический компас / ретикул
  float baseR = std::min(screenW, screenH) * 0.16f;
  if (baseR > 110.0f) baseR = 110.0f;
  if (baseR < 60.0f) baseR = 60.0f;

  float ringCenterY = cy - 45.0f;

  // Внешнее кольцо
  dl->AddCircle(ImVec2(cx, ringCenterY), baseR, IM_COL32(50, 130, 210, 90), 64, 1.5f);
  // Среднее кольцо с делениями
  dl->AddCircle(ImVec2(cx, ringCenterY), baseR * 0.75f, IM_COL32(80, 180, 255, 140), 48, 1.8f);
  // Внутреннее пульсирующее кольцо
  float pulse = 1.0f + 0.08f * std::sin(m_pulseTimer * 4.0f);
  dl->AddCircle(ImVec2(cx, ringCenterY), baseR * 0.45f * pulse, IM_COL32(110, 220, 255, 200), 36, 2.0f);
  // Центральное квантовое ядро
  dl->AddCircleFilled(ImVec2(cx, ringCenterY), 5.0f, IM_COL32(220, 245, 255, 240));

  // Вращающиеся риски компаса
  for (int i = 0; i < 8; ++i) {
    float a = m_reticleAngle + static_cast<float>(i) * (6.2831853f / 8.0f);
    float r1 = baseR * 0.75f;
    float r2 = baseR * 0.95f;
    dl->AddLine(ImVec2(cx + std::cos(a) * r1, ringCenterY + std::sin(a) * r1),
                ImVec2(cx + std::cos(a) * r2, ringCenterY + std::sin(a) * r2),
                IM_COL32(140, 220, 255, 180), 2.0f);
  }

  // 4. Текстовый заголовок и статусная строка
  ImVec2 titleSize = ImGui::CalcTextSize(m_title.c_str());
  ImVec2 subSize = ImGui::CalcTextSize(m_subtitle.c_str());

  float textY = ringCenterY + baseR + 25.0f;
  dl->AddText(ImVec2(cx - titleSize.x * 0.5f, textY),
              IM_COL32(235, 245, 255, 255), m_title.c_str());

  dl->AddText(ImVec2(cx - subSize.x * 0.5f, textY + 24.0f),
              IM_COL32(120, 190, 255, 210), m_subtitle.c_str());

  // 5. Сегментированная высокотехнологичная шкала прогресса (Progress Bar)
  float barW = std::min(screenW * 0.55f, 620.0f);
  float barH = 14.0f;
  float barX = cx - barW * 0.5f;
  float barY = textY + 58.0f;

  // Рамка шкалы
  dl->AddRect(ImVec2(barX - 2.0f, barY - 2.0f),
              ImVec2(barX + barW + 2.0f, barY + barH + 2.0f),
              IM_COL32(60, 130, 200, 160), 3.0f, 0, 1.5f);
  dl->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH),
                    IM_COL32(10, 20, 35, 220), 2.0f);

  // Сегменты
  const int kSegments = 36;
  float segSpacing = 2.0f;
  float segWidth = (barW - (kSegments - 1) * segSpacing) / static_cast<float>(kSegments);
  int activeSegments = static_cast<int>(m_progress * static_cast<float>(kSegments));

  for (int s = 0; s < kSegments; ++s) {
    float sx0 = barX + s * (segWidth + segSpacing);
    float sx1 = sx0 + segWidth;
    if (s < activeSegments) {
      ImU32 sCol = (s == activeSegments - 1)
          ? IM_COL32(220, 250, 255, 255)
          : IM_COL32(70, 180, 255, 220);
      dl->AddRectFilled(ImVec2(sx0, barY + 1.0f), ImVec2(sx1, barY + barH - 1.0f), sCol, 1.0f);
    } else {
      dl->AddRectFilled(ImVec2(sx0, barY + 1.0f), ImVec2(sx1, barY + barH - 1.0f),
                        IM_COL32(20, 40, 60, 80), 1.0f);
    }
  }

  // Проценты
  char pctBuf[32];
  std::snprintf(pctBuf, sizeof(pctBuf), "%3.0f%%", m_progress * 100.0f);
  ImVec2 pctSize = ImGui::CalcTextSize(pctBuf);
  dl->AddText(ImVec2(barX + barW + 12.0f, barY - 2.0f),
              IM_COL32(130, 210, 255, 240), pctBuf);

  // 6. Пошаговые логи телеметрии
  float logY = barY + 30.0f;
  for (const auto& step : m_telemetrySteps) {
    if (m_progress >= step.first) {
      ImVec2 lSize = ImGui::CalcTextSize(step.second.c_str());
      dl->AddText(ImVec2(cx - lSize.x * 0.5f, logY),
                  IM_COL32(100, 220, 160, 220), step.second.c_str());
      logY += 18.0f;
    }
  }

  // 7. Нижний блок: цитата и подсказка по пропуску
  if (!m_quote.empty()) {
    ImVec2 qSize = ImGui::CalcTextSize(m_quote.c_str());
    float quoteY = screenH - 85.0f;
    dl->AddText(ImVec2(cx - qSize.x * 0.5f, quoteY),
                IM_COL32(200, 220, 240, 190), m_quote.c_str());

    if (!m_quoteAuthor.empty()) {
      std::string authorStr = "— " + m_quoteAuthor;
      ImVec2 aSize = ImGui::CalcTextSize(authorStr.c_str());
      dl->AddText(ImVec2(cx - aSize.x * 0.5f, quoteY + 20.0f),
                  IM_COL32(130, 175, 215, 160), authorStr.c_str());
    }
  }

  // Подсказка мгновенного пропуска
  const char* skipHint = "[ ПРОБЕЛ / ENTER / ESC ] — ПРОПУСТИТЬ ЗАГРУЗКУ";
  ImVec2 skipSize = ImGui::CalcTextSize(skipHint);
  dl->AddText(ImVec2(cx - skipSize.x * 0.5f, screenH - 32.0f),
              IM_COL32(110, 145, 180, 150), skipHint);

#endif  // ASTRO_NO_IMGUI
}

}  // namespace astro
