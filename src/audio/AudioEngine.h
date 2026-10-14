#pragma once

// === Audio === (агент AU): процедурный звук без бинарных ассетов.
// Класс AudioEngine: float32 stereo 44100, ленивый init при первом вводе,
// null-backend при отсутствии устройства (не падать), update(dt) крутит
// музыку и SFX-очередь. MINIAUDIO_IMPLEMENTATION — ровно в одном .cpp
// (AudioEngine.cpp). Core-таргет звук не тянет: файл подключается только
// к GL-таргету (см. CMakeLists `# === Audio ===`).

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace astro {

class AudioEngine {
 public:
  static constexpr int kSampleRate = 44100;
  static constexpr int kChannels = 2;
  // Бесшовный луп музыки: 4 аккорда по 8с.
  static constexpr double kLoopSec = 32.0;
  static constexpr double kChordSec = 8.0;
  // Тихая музыка: -18 дБ от полной шкалы.
  static constexpr float kMusicDb = -18.0f;
  // Дакинг музыки в варпе: -8 дБ при K=1.
  static constexpr float kDuckDb = -8.0f;

  AudioEngine();
  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  // Ленивая инициализация устройства. Вызывать при ПЕРВОМ вводе
  // пользователя (клик/клавиша). Возвращает true, если устройство живое.
  // false = null-backend (звука нет, но API безопасно, падений нет).
  // Повторные вызовы дешёвые (флаг m_tried).
  bool ensureInitialized();
  bool isInitialized() const { return m_tried; }
  bool isDeviceLive() const { return m_live; }
  bool isNullBackend() const { return m_tried && !m_live; }

  // Крутить каждый кадр из AppGL::run (только из Audio-якоря).
  // В live-режиме чистит отыгравшие голоса и сглаживает варп;
  // в null-режиме двигает виртуальное время, чтобы очередь не висела.
  void update(float dt);

  // --- SFX (все синтезом, без вавок) ---
  void playClick();    // UI-клики
  void playTick();     // v3.0.0: тик пульсара (короткий пинг 25 мс)
  void playWhoosh();   // телепорт: sweep шума 0.6с
  void setWarpLevel(float k);  // 0..1: коричневый шум + суб-синус,
                               // сам же дакает музыку (-8 дБ при K=1)
  void playJump();     // варп-прыжок: sine-drop 200->30 Гц + шум
  void playChime();    // ачивка: 3 щипка
  void playBlip();     // ошибка/тур: короткий blip
  void playCapture();  // тревога захвата ЧД (зовёт PH-хук наружу)

  void setVolume(float v);  // 0..1
  float volume() const { return m_volume.load(); }
  void toggleMute();
  bool isMuted() const { return m_muted.load(); }
  float warpLevel() const { return m_warpCurrent.load(); }

 private:
  enum class VoiceType : int { Click, Whoosh, Jump, Chime, Blip, Capture, Tick };
  struct Voice {
    VoiceType type = VoiceType::Click;
    uint64_t startSample = 0;
  };

  // Рендер одного музыкального сэмпла (моно; стерео дублируется).
  // tAbs — абсолютное время (фаза синусов непрерывна),
  // tLoop — время в лупе 0..32 (выбор аккорда/щипков, бесшовно).
  float renderMusic(double tAbs, double tLoop) const;
  // Рендер одного голоса; rng — xorshift-состояние аудиопотока.
  float renderVoice(const Voice& v, double age, uint32_t& rng) const;
  // Вызывается из аудиоколбэка (.cpp) блоками.
  void renderFrames(float* out, unsigned frames);

  static float dbToGain(float db);

  struct DeviceHolder;
  std::unique_ptr<DeviceHolder> m_holder;

  bool m_tried = false;  // ensureInitialized уже звали (для UI-гарда)
  bool m_live = false;   // устройство реально играет

  std::atomic<float> m_volume{0.7f};
  std::atomic<bool> m_muted{false};
  std::atomic<float> m_warpTarget{0.0f};
  std::atomic<float> m_warpCurrent{0.0f};
  std::atomic<uint64_t> m_samples{0};  // счётчик сэмплов (live: колбэк)

  std::mutex m_mutex;
  std::vector<Voice> m_voices;

  // Состояние шума — ТОЛЬКО аудиопоток (renderFrames), update() не трогает.
  uint32_t m_rng = 0x12345678u;
  float m_brown = 0.0f;

  friend struct AudioCallbackBridge;
};

}  // namespace astro
