// === Audio === (агент AU): реализация процедурного движка.
// MINIAUDIO_IMPLEMENTATION ровно здесь, в одном .cpp. Больше нигде
// miniaudio.h с имплементацией не подключать.

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4127 4201 4244 4267 4305 4310 4389 4505 4701 4702 4996)
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "audio/AudioEngine.h"

#include <cmath>
#include <cstring>

namespace astro {
namespace {

// --- Константы музыки: Am-F-C-G, тоники октавы 2-3 ---
// A2 110.0, F2 87.307, C3 130.813, G2 97.999.
constexpr double kRoots[4] = {110.0, 87.30705758, 130.81278265, 97.99885896};
// Пентатонные щипки (A-минор): время в лупе + частота.
constexpr double kPluckTimes[6] = {2.5, 7.2, 11.8, 18.3, 23.7, 29.2};
constexpr double kPluckNotes[6] = {440.0, 659.25511383, 523.2511306,
                                   783.99087196, 587.32953583, 880.0};
constexpr double kEcho1 = 0.375;  // delay-эхo щипков
constexpr double kEcho2 = 0.75;

constexpr double kPi = 3.14159265358979323846;

float whiteNoise(uint32_t& rng) {
  // xorshift32 -> [-1, 1].
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  const float u =
      static_cast<float>(rng >> 8) * (1.0f / 16777216.0f);  // [0,1)
  return u * 2.0f - 1.0f;
}

double wrapLoop(double t) {
  double l = std::fmod(t, AudioEngine::kLoopSec);
  if (l < 0.0) l += AudioEngine::kLoopSec;
  return l;
}

// Один аккорд: тоника + квинта + октава с медленным биением 0.07 Гц.
float chordTone(double tAbs, double root) {
  const double f1 = root;
  const double f2 = root * 1.5;
  const double f3 = root * 2.0;
  const double beat1 = 1.0 + 0.28 * std::sin(2.0 * kPi * 0.07 * tAbs);
  const double beat2 =
      1.0 + 0.28 * std::sin(2.0 * kPi * 0.07 * tAbs + 1.1);
  const double beat3 =
      1.0 + 0.28 * std::sin(2.0 * kPi * 0.07 * tAbs + 2.2);
  const float s1 = static_cast<float>(std::sin(2.0 * kPi * f1 * tAbs)) *
                   static_cast<float>(beat1);
  const float s2 = static_cast<float>(std::sin(2.0 * kPi * f2 * tAbs)) *
                   static_cast<float>(beat2);
  const float s3 = static_cast<float>(std::sin(2.0 * kPi * f3 * tAbs)) *
                   static_cast<float>(beat3);
  return 0.34f * s1 + 0.22f * s2 + 0.15f * s3;
}

// Щипок с экспонентой + 2-я гармоника.
float pluckTone(double freq, double age) {
  if (age < 0.0) return 0.0f;
  const float env = static_cast<float>(std::exp(-3.5 * age));
  const float s1 =
      static_cast<float>(std::sin(2.0 * kPi * freq * age));
  const float s2 =
      static_cast<float>(std::sin(2.0 * kPi * freq * 2.0 * age));
  return (s1 + 0.4f * s2) * env;
}

}  // namespace

struct AudioEngine::DeviceHolder {
  ma_device device;
  bool started = false;
};

AudioEngine::AudioEngine() : m_holder(new DeviceHolder()) {
  std::memset(&m_holder->device, 0, sizeof(m_holder->device));
}

AudioEngine::~AudioEngine() {
  if (m_live) {
    ma_device_uninit(&m_holder->device);
    m_live = false;
  }
}

float AudioEngine::dbToGain(float db) {
  return std::pow(10.0f, db / 20.0f);
}

bool AudioEngine::ensureInitialized() {
  if (m_tried) return m_live;
  m_tried = true;

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = static_cast<ma_uint32>(kChannels);
  config.sampleRate = static_cast<ma_uint32>(kSampleRate);
  config.dataCallback = [](ma_device* dev, void* out, const void* in,
                           ma_uint32 frames) {
    (void)in;
    auto* self = static_cast<AudioEngine*>(dev->pUserData);
    self->renderFrames(static_cast<float*>(out),
                       static_cast<unsigned>(frames));
  };
  config.pUserData = this;

  if (ma_device_init(nullptr, &config, &m_holder->device) != MA_SUCCESS) {
    // Null-backend: устройства нет (headless/CI) — не падаем, очередь
    // крутится виртуально в update().
    m_live = false;
    return false;
  }
  if (ma_device_start(&m_holder->device) != MA_SUCCESS) {
    ma_device_uninit(&m_holder->device);
    std::memset(&m_holder->device, 0, sizeof(m_holder->device));
    m_live = false;
    return false;
  }
  m_holder->started = true;
  m_live = true;
  return true;
}

void AudioEngine::update(float dt) {
  if (!m_tried) return;
  if (dt < 0.0f) dt = 0.0f;
  if (dt > 0.5f) dt = 0.5f;

  // Сглаживание варпа к цели (~0.3с) — дакинг музыки читает m_warpCurrent.
  const float target = m_warpTarget.load();
  float cur = m_warpCurrent.load();
  const float a = 1.0f - std::exp(-dt * 6.0f);
  cur += (target - cur) * a;
  if (cur < 0.0f) cur = 0.0f;
  if (cur > 1.0f) cur = 1.0f;
  m_warpCurrent.store(cur);

  // Гул двигателя (v3.3.0): сглаживание цели (~0.2с), читает аудиопоток.
  const float engTarget = m_engineTarget.load();
  float eng = m_engineCurrent.load();
  const float ea = 1.0f - std::exp(-dt * 5.0f);
  eng += (engTarget - eng) * ea;
  if (eng < 0.0f) eng = 0.0f;
  if (eng > 1.0f) eng = 1.0f;
  m_engineCurrent.store(eng);

  if (!m_live) {
    // Null-backend: двигаем виртуальное время, чистим голоса по нему.
    uint64_t adv = static_cast<uint64_t>(static_cast<double>(dt) * kSampleRate);
    m_samples.fetch_add(adv);
  }
  const uint64_t now = m_samples.load();
  const double rate = static_cast<double>(kSampleRate);

  std::lock_guard<std::mutex> lock(m_mutex);
  for (size_t i = 0; i < m_voices.size();) {
    const double age =
        static_cast<double>(now - m_voices[i].startSample) / rate;
    double dur = 1.0;
    switch (m_voices[i].type) {
      case VoiceType::Click: dur = 0.12; break;
      case VoiceType::Whoosh: dur = 0.7; break;
      case VoiceType::Jump: dur = 1.6; break;
      case VoiceType::Chime: dur = 2.0; break;
      case VoiceType::Blip: dur = 0.3; break;
      case VoiceType::Capture: dur = 2.4; break;
      case VoiceType::Tick: dur = 0.05; break;
      case VoiceType::File:
        dur = m_voices[i].pcm ? static_cast<double>(m_voices[i].pcm->size()) /
                                    kSampleRate
                              : 0.0;
        break;
    }
    if (age > dur + 0.1) {
      m_voices[i] = m_voices.back();
      m_voices.pop_back();
    } else {
      ++i;
    }
  }
}

void AudioEngine::setVolume(float v) {
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  m_volume.store(v);
}

void AudioEngine::toggleMute() { m_muted.store(!m_muted.load()); }

void AudioEngine::setWarpLevel(float k) {
  if (k < 0.0f) k = 0.0f;
  if (k > 1.0f) k = 1.0f;
  m_warpTarget.store(k);
}

void AudioEngine::setEngineLevel(float k) {
  if (k < 0.0f) k = 0.0f;
  if (k > 1.0f) k = 1.0f;
  m_engineTarget.store(k);
}

void AudioEngine::playClick() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Click, m_samples.load()});
}

// v3.0.0: тик пульсара. Короткий голос (25 мс): планировщик AppGL зовёт
// его каждые 59..945 мс, перекрытий почти нет, кап общий (32) не жмёт.
void AudioEngine::playTick() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Tick, m_samples.load()});
}

void AudioEngine::playWhoosh() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Whoosh, m_samples.load()});
}

void AudioEngine::playJump() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Jump, m_samples.load()});
}

void AudioEngine::playChime() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Chime, m_samples.load()});
}

void AudioEngine::playBlip() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Blip, m_samples.load()});
}

void AudioEngine::playCapture() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  // Тревогу не спамим: не чаще одной за раз.
  for (const auto& v : m_voices) {
    if (v.type == VoiceType::Capture) {
      const double age =
          static_cast<double>(m_samples.load() - v.startSample) / kSampleRate;
      if (age < 2.0) return;
    }
  }
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Capture, m_samples.load()});
}

// v4.0: файл-голос катсцены. Декод синхронный в вызывающем треде (UI):
// короткий wav катсцены читается за миллисекунды; audio-поток потом
// только читает готовый буфер (no lock).
void AudioEngine::playFile(const std::string& path, float db) {
  if (!m_tried) return;
  ma_decoder_config dcfg =
      ma_decoder_config_init(ma_format_f32, 1, kSampleRate);
  ma_decoder dec;
  if (ma_decoder_init_file(path.c_str(), &dcfg, &dec) != MA_SUCCESS) return;
  std::vector<float> buf;
  float chunk[4096];
  for (;;) {
    ma_uint64 got = 0;
    if (ma_decoder_read_pcm_frames(&dec, chunk, 4096, &got) != MA_SUCCESS ||
        got == 0)
      break;
    buf.insert(buf.end(), chunk, chunk + got);
  }
  ma_decoder_uninit(&dec);
  if (buf.empty()) return;
  Voice v;
  v.type = VoiceType::File;
  v.startSample = m_samples.load();
  v.pcm = std::make_shared<std::vector<float>>(std::move(buf));
  v.gainDb = db;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 24) return;
  m_voices.push_back(std::move(v));
}

float AudioEngine::renderMusic(double tAbs, double tLoop) const {
  // Аккорд i + equal-power кроссфейд 2с в конце каждого 8с отрезка.
  const int ci = static_cast<int>(tLoop / kChordSec) % 4;
  const int cn = (ci + 1) % 4;
  const double local = tLoop - static_cast<double>(ci) * kChordSec;
  float pad;
  if (local > 6.0) {
    const double x = (local - 6.0) / 2.0;  // 0..1
    const float gA =
        static_cast<float>(std::cos(x * kPi * 0.5));  // equal-power
    const float gB =
        static_cast<float>(std::sin(x * kPi * 0.5));
    pad = gA * chordTone(tAbs, kRoots[ci]) + gB * chordTone(tAbs, kRoots[cn]);
  } else {
    pad = chordTone(tAbs, kRoots[ci]);
  }

  // Разреженные пентатонные щипки + echo-delay (бесшовно: дистанция по модулю).
  float plucks = 0.0f;
  for (int i = 0; i < 6; ++i) {
    double d = tLoop - kPluckTimes[i];
    d = std::fmod(d, kLoopSec);
    if (d < 0.0) d += kLoopSec;
    // d — возраст щипка в лупе; старше 3с почти затух — скипаем.
    if (d > 3.0) continue;
    plucks += pluckTone(kPluckNotes[i], d);
    plucks += 0.35f * pluckTone(kPluckNotes[i], d - kEcho1);
    plucks += 0.22f * pluckTone(kPluckNotes[i], d - kEcho2);
  }

  const float music = pad * 0.6f + plucks * 0.5f;
  return music * dbToGain(kMusicDb);  // тихо: -18 дБ
}

float AudioEngine::renderVoice(const Voice& v, double age,
                               uint32_t& rng) const {
  if (age < 0.0) return 0.0f;
  switch (v.type) {
    case VoiceType::Click: {
      if (age > 0.12) return 0.0f;
      const float s = static_cast<float>(std::sin(2.0 * kPi * 1900.0 * age));
      return s * static_cast<float>(std::exp(-age * 90.0)) * 0.7f;
    }
    case VoiceType::Whoosh: {
      if (age > 0.7) return 0.0f;
      const double e = std::sin(kPi * age / 0.6);
      const float env = static_cast<float>(std::pow(e > 0.0 ? e : 0.0, 1.2));
      const double f = 300.0 * std::pow(8.0, age / 0.6);  // 300 -> 2400
      // Фаза sweep: интеграл f(t).
      const double phase =
          2.0 * kPi * 300.0 * 0.6 / std::log(8.0) * (std::pow(8.0, age / 0.6) - 1.0);
      (void)f;
      const float sweep = static_cast<float>(std::sin(phase));
      const float n = whiteNoise(rng);
      return (n * 0.6f + sweep * 0.35f) * env * 0.8f;
    }
    case VoiceType::Jump: {
      if (age > 1.6) return 0.0f;
      // Sine-drop 200 -> 30 Гц: f(t) = 30 + 170*exp(-3.2t).
      const double phase =
          2.0 * kPi *
          (30.0 * age + 170.0 * (1.0 - std::exp(-3.2 * age)) / 3.2);
      const float tone = static_cast<float>(std::sin(phase));
      const float n = whiteNoise(rng);
      return tone * static_cast<float>(std::exp(-age * 1.8)) * 0.9f +
             n * static_cast<float>(std::exp(-age * 5.0)) * 0.35f;
    }
    case VoiceType::Chime: {
      if (age > 2.0) return 0.0f;
      static const double kNotes[3] = {659.25511383, 783.99087196, 880.0};
      static const double kAt[3] = {0.0, 0.14, 0.28};
      float s = 0.0f;
      for (int i = 0; i < 3; ++i) {
        const double a = age - kAt[i];
        if (a < 0.0) continue;
        const float env = static_cast<float>(std::exp(-a * 4.0));
        s += (static_cast<float>(std::sin(2.0 * kPi * kNotes[i] * a)) +
              0.35f * static_cast<float>(
                           std::sin(2.0 * kPi * kNotes[i] * 2.0 * a))) *
             env;
      }
      return s * 0.33f;
    }
    case VoiceType::Blip: {
      if (age > 0.3) return 0.0f;
      const double f = (age < 0.1) ? 660.0 : 880.0;
      const float attack =
          static_cast<float>(age < 0.005 ? age / 0.005 : 1.0);
      const float s = static_cast<float>(std::sin(2.0 * kPi * f * age)) +
                      0.2f * static_cast<float>(
                                 std::sin(2.0 * kPi * f * 3.0 * age));
      return s * static_cast<float>(std::exp(-age * 8.0)) * attack * 0.5f;
    }
    case VoiceType::Capture: {
      if (age > 2.4) return 0.0f;
      // Тревога захвата ЧД: чередование 523/784 Гц, пульс 5 Гц.
      const double cyc = std::fmod(age * 2.5, 1.0);
      const double f = (cyc < 0.5) ? 523.2511306 : 783.99087196;
      const float s = static_cast<float>(std::sin(2.0 * kPi * f * age)) +
                      0.3f * static_cast<float>(
                                 std::sin(2.0 * kPi * f * 3.0 * age));
      const float pulse =
          0.6f + 0.4f * static_cast<float>(std::sin(2.0 * kPi * 5.0 * age));
      float env = 1.0f;
      if (age < 0.05) env = static_cast<float>(age / 0.05);
      if (age > 2.0) env = static_cast<float>((2.4 - age) / 0.4);
      return s * 0.45f * pulse * env;
    }
    case VoiceType::Tick: {
      // Тик пульсара: пинг 2200 Гц, затухание 25 мс (радиоудар).
      if (age > 0.05) return 0.0f;
      const float s = static_cast<float>(std::sin(2.0 * kPi * 2200.0 * age));
      return s * static_cast<float>(std::exp(-age * 120.0)) * 0.5f;
    }
    case VoiceType::File: {
      // Файл-голос: линейный лерп по готовому float-буферу (моно,
      // kSampleRate, значит шаг 1 сэмпл на сэмпл; лерп страховка).
      if (!v.pcm || v.pcm->empty()) return 0.0f;
      const size_t n = v.pcm->size();
      const double idx = age * static_cast<double>(kSampleRate);
      if (idx < 0.0) return 0.0f;
      if (idx >= static_cast<double>(n)) return 0.0f;
      const size_t i0 = static_cast<size_t>(idx);
      const float f = static_cast<float>(idx - static_cast<double>(i0));
      const float a = (*v.pcm)[i0];
      const float b = (i0 + 1 < n) ? (*v.pcm)[i0 + 1] : a;
      return (a + (b - a) * f) * dbToGain(v.gainDb);
    }
  }
  return 0.0f;
}

void AudioEngine::renderFrames(float* out, unsigned frames) {
  // Снимок голосов под коротким мьютексом (без аллокаций в колбэке).
  Voice snap[40];
  size_t nSnap = 0;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    nSnap = m_voices.size() > 40 ? 40 : m_voices.size();
    for (size_t i = 0; i < nSnap; ++i) snap[i] = m_voices[i];
  }
  const float vol = m_muted.load() ? 0.0f : m_volume.load();
  const float warpK = m_warpCurrent.load();
  const float duck = dbToGain(kDuckDb * warpK);  // -8 дБ при K=1
  const double rate = static_cast<double>(kSampleRate);
  uint64_t base = m_samples.load();

  for (unsigned i = 0; i < frames; ++i) {
    const double tAbs = static_cast<double>(base + i) / rate;
    const double tLoop = wrapLoop(tAbs);
    float music = renderMusic(tAbs, tLoop) * duck;

    float sfx = 0.0f;
    for (size_t v = 0; v < nSnap; ++v) {
      const double age =
          static_cast<double>(base + i - snap[v].startSample) / rate;
      if (age < 0.0) continue;
      sfx += renderVoice(snap[v], age, m_rng);
    }

    // WarpRumble: коричневый шум + суб-синус, громкость от уровня варпа.
    const float engK = m_engineCurrent.load();
    float rumble = 0.0f;
    if (warpK > 0.001f || engK > 0.001f) {
      const float w = whiteNoise(m_rng);
      m_brown = (m_brown + 0.02f * w) / 1.02f;
    }
    if (warpK > 0.001f) {
      const float sub =
          static_cast<float>(std::sin(2.0 * kPi * 43.0 * tAbs)) * 0.5f +
          static_cast<float>(std::sin(2.0 * kPi * 86.5 * tAbs + 1.1)) * 0.25f;
      rumble = (m_brown * 3.0f * 0.8f + sub * 0.6f) * warpK * 0.5f;
    }

    // Engine hum (v3.3.0): тон 52-82 Гц по скорости + обертон + дыхание шума.
    float engine = 0.0f;
    if (engK > 0.001f) {
      const float f = 52.0f + 30.0f * engK;
      const float tone =
          static_cast<float>(std::sin(2.0 * kPi * f * tAbs)) +
          0.35f * static_cast<float>(std::sin(2.0 * kPi * 2.0 * f * tAbs + 0.7));
      engine = (tone * 0.16f + m_brown * 1.2f) * engK;
    }

    float mix = (music + sfx + rumble + engine) * vol;
    // Мягкий лимитер tanh на мастере.
    mix = std::tanh(mix * 1.1f);
    out[i * 2 + 0] = mix;
    out[i * 2 + 1] = mix;
  }
  m_samples.fetch_add(frames);
}

}  // namespace astro
