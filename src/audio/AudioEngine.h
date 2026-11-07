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
  void playTick(float pan = 0.0f);     // v3.0.0 / v4.3.0: тик пульсара с панорамой [-1..1]
  void playWhoosh(float pan = 0.0f);   // телепорт: sweep шума 0.6с
  void setWarpLevel(float k);  // 0..1: коричневый шум + суб-синус,
                               // сам же дакает музыку (-8 дБ при K=1)
  void setEngineLevel(float k);  // v3.3.0: гул двигателя от скорости 0..1
  void setDrillLevel(float k, float strain = 0.0f);  // v5.3.0: бур реголита
  float drillLevel() const { return m_drillCurrent.load(); }
                                 // (тон 52-82 Гц + обертон + шум-дыхание)
  void setAtmosphereRoar(float level);  // v4.9.0: гиперзвуковой аэродинамический свист и рев плазмы 0..1
  float atmosphereRoar() const { return m_atmoCurrent.load(); }
  // v4.3.0: эффект Доплера и напряжение у гравитационных аномалий
  void setDopplerShift(float factor);  // 0.5 .. 2.0 (pitch shift)
  float dopplerShift() const { return m_dopplerFactor.load(); }
  void setTensionLevel(float k);       // 0 .. 1 (near hazards)
  float tensionLevel() const { return m_tensionCurrent.load(); }
  void playJump();     // варп-прыжок: sine-drop 200->30 Гц + шум
  void playChime();    // ачивка: 3 щипка
  void playBlip();     // ошибка/тур: короткий blip
  void playCapture();  // тревога захвата ЧД (зовёт PH-хук наружу)

  // v5.0.0: Радиопереговоры мостика и авионика бортового ИИ
  void playRadioBeep(float pan = 0.0f);   // PTT squelch open chirp + click
  void playRadioEnd(float pan = 0.0f);    // PTT squelch close tail burst
  void setRadioStatic(float level);       // 0..1: фоновый радиоэфир/несущая частота
  float radioStatic() const { return m_radioStaticCurrent.load(); }
  void playWarpReadyChime();              // двухтоновый колокольчик готовности варп-ядра (523->659 Гц)
  void playHazardAlarm();                 // тревожный тактический пульсирующий зуммер (880/440 Гц)
  void playTargetLockPing(float pan = 0.0f); // тактический пинг захвата цели на МФД (1200->950 Гц)

  // v5.1.0: Эврика и синтез Zettelkasten
  void playLiquidDrop(float pan = 0.0f);  // процедурный «бульк» капли / квантовый всплеск вакуума
  void playEurekaChime();                 // небесный полифонический перезвон открытия (C5-G5-C6-E6)

  // v5.2.0: Бортовой кассетный магнитофон (Cosmic Ambient Deck)
  enum class CassetteTape : int {
    Off = 0,
    ParisRain = 1,     // Звуки Дома / Paris Rain (шум дождя + фортепианный сонный аккорд)
    PulsarBeat = 2,    // Пульс Космоса / Pulsar Beat (пульсирующий саб-бас 1.4 Гц + дилэй-щелчки)
    DeepSpaceVoid = 3  // Эфир Пустоты / Deep Space Void (космический 4-голосный пэд с LFO-фильтром)
  };

  void setCassetteTape(int tape);
  int cassetteTape() const { return m_cassetteTape.load(); }
  void setCassetteVolume(float v);
  float cassetteVolume() const { return m_cassetteVolume.load(); }

  // v4.0: файл-голос для катсцен (wav, декодируется miniaudio в float
  // моно kSampleRate синхронно при запуске; каталог с аудио рядом).
  void playFile(const std::string& path, float db);

  void setVolume(float v);  // 0..1
  float volume() const { return m_volume.load(); }
  void toggleMute();
  bool isMuted() const { return m_muted.load(); }
  float warpLevel() const { return m_warpCurrent.load(); }

 private:
  enum class VoiceType : int {
    Click, Whoosh, Jump, Chime, Blip, Capture, Tick, File,
    RadioBeep, RadioEnd, WarpReady, Hazard, TargetLock,
    LiquidDrop, Eureka
  };
  struct Voice {
    VoiceType type = VoiceType::Click;
    uint64_t startSample = 0;
    // File-голос (v4.0): декодированный float-моно PCM, kSampleRate.
    // audio-поток читает без мьютекса (после push не меняется).
    std::shared_ptr<std::vector<float>> pcm;
    float gainDb = 0.0f;
    float pan = 0.0f;  // v4.3.0: -1.0 left .. +1.0 right
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
  std::atomic<float> m_engineTarget{0.0f};
  std::atomic<float> m_engineCurrent{0.0f};
  std::atomic<float> m_drillTarget{0.0f};
  std::atomic<float> m_drillCurrent{0.0f};
  std::atomic<float> m_dopplerFactor{1.0f};
  std::atomic<float> m_tensionTarget{0.0f};
  std::atomic<float> m_tensionCurrent{0.0f};
  std::atomic<float> m_atmoTarget{0.0f};
  std::atomic<float> m_atmoCurrent{0.0f};
  std::atomic<float> m_radioStaticTarget{0.0f};
  std::atomic<float> m_radioStaticCurrent{0.0f};
  std::atomic<int> m_cassetteTape{0};
  std::atomic<float> m_cassetteVolume{0.7f};
  std::atomic<float> m_cassetteGainTarget{0.0f};
  std::atomic<float> m_cassetteGainCurrent{0.0f};
  std::atomic<uint64_t> m_samples{0};  // счётчик сэмплов (live: колбэк)

  std::mutex m_mutex;
  std::vector<Voice> m_voices;

  // Состояние шума — ТОЛЬКО аудиопоток (renderFrames), update() не трогает.
  uint32_t m_rng = 0x12345678u;
  float m_brown = 0.0f;
  float m_atmoNoise = 0.0f;
  float m_radioBandpass1 = 0.0f;
  float m_radioBandpass2 = 0.0f;
  float m_cassetteRainNoise = 0.0f;
  float m_cassetteLfoPhase = 0.0f;

  friend struct AudioCallbackBridge;
};

}  // namespace astro
