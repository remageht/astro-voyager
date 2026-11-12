// === Audio === (Р°РіРµРЅС‚ AU): СЂРµР°Р»РёР·Р°С†РёСЏ РїСЂРѕС†РµРґСѓСЂРЅРѕРіРѕ РґРІРёР¶РєР°.
// MINIAUDIO_IMPLEMENTATION СЂРѕРІРЅРѕ Р·РґРµСЃСЊ, РІ РѕРґРЅРѕРј .cpp. Р‘РѕР»СЊС€Рµ РЅРёРіРґРµ
// miniaudio.h СЃ РёРјРїР»РµРјРµРЅС‚Р°С†РёРµР№ РЅРµ РїРѕРґРєР»СЋС‡Р°С‚СЊ.

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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>  // playFile: path -> ma_decoder (GCC РЅРµ С‚СЏРЅРµС‚ <string> С‚СЂР°РЅР·РёС‚РёРІРЅРѕ)

namespace astro {
namespace {

// --- РљРѕРЅСЃС‚Р°РЅС‚С‹ РјСѓР·С‹РєРё: Am-F-C-G, С‚РѕРЅРёРєРё РѕРєС‚Р°РІС‹ 2-3 ---
// A2 110.0, F2 87.307, C3 130.813, G2 97.999.
constexpr double kRoots[4] = {110.0, 87.30705758, 130.81278265, 97.99885896};
// РџРµРЅС‚Р°С‚РѕРЅРЅС‹Рµ С‰РёРїРєРё (A-РјРёРЅРѕСЂ): РІСЂРµРјСЏ РІ Р»СѓРїРµ + С‡Р°СЃС‚РѕС‚Р°.
constexpr double kPluckTimes[6] = {2.5, 7.2, 11.8, 18.3, 23.7, 29.2};
constexpr double kPluckNotes[6] = {440.0, 659.25511383, 523.2511306,
                                   783.99087196, 587.32953583, 880.0};
constexpr double kEcho1 = 0.375;  // delay-СЌС…o С‰РёРїРєРѕРІ
constexpr double kEcho2 = 0.75;

constexpr double kPi = 3.14159265358979323846;
constexpr double kMenuFadeSec = 0.8;

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

// РћРґРёРЅ Р°РєРєРѕСЂРґ: С‚РѕРЅРёРєР° + РєРІРёРЅС‚Р° + РѕРєС‚Р°РІР° СЃ РјРµРґР»РµРЅРЅС‹Рј Р±РёРµРЅРёРµРј 0.07 Р“С†.
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

// Р©РёРїРѕРє СЃ СЌРєСЃРїРѕРЅРµРЅС‚РѕР№ + 2-СЏ РіР°СЂРјРѕРЅРёРєР°.
float pluckTone(double freq, double age) {
  if (age < 0.0) return 0.0f;
  const float env = static_cast<float>(std::exp(-3.5 * age));
  const float s1 =
      static_cast<float>(std::sin(2.0 * kPi * freq * age));
  const float s2 =
      static_cast<float>(std::sin(2.0 * kPi * freq * 2.0 * age));
  return (s1 + 0.4f * s2) * env;
}

// Absolute sample time keeps phase continuous across blocks and menu toggles.
void renderMenuAmbient(double tAbs, float& left, float& right) {
  const double phase = 2.0 * kPi * tAbs;
  const float hum =
      0.018f * static_cast<float>(std::sin(phase * 36.708)) +
      0.007f * static_cast<float>(std::sin(phase * 73.416 + 0.3));
  const float pad =
      0.011f * static_cast<float>(std::sin(phase * 146.832)) +
      0.007f * static_cast<float>(std::sin(phase * 220.248 + 0.5));
  const float airL =
      0.005f * static_cast<float>(std::sin(phase * 293.664 + 0.9)) +
      0.003f * static_cast<float>(std::sin(phase * 440.496 + 1.7)) +
      0.002f * static_cast<float>(std::sin(phase * 587.328 + 0.2));
  const float airR =
      0.005f * static_cast<float>(std::sin(phase * 293.664 + 1.6)) +
      0.003f * static_cast<float>(std::sin(phase * 440.496 + 0.4)) +
      0.002f * static_cast<float>(std::sin(phase * 587.328 + 2.1));
  // Only slow breathing (tens of seconds), without rhythmic pulses or beats.
  const float breathL =
      0.78f + 0.18f * static_cast<float>(std::sin(phase * 0.031));
  const float breathR =
      0.78f + 0.18f * static_cast<float>(std::sin(phase * 0.023 + 1.3));
  left = hum + (pad + airL) * breathL;
  right = hum + (pad + airR) * breathR;
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
    ma_device_stop(&m_holder->device);
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
    // Null-backend: СѓСЃС‚СЂРѕР№СЃС‚РІР° РЅРµС‚ (headless/CI) вЂ” РЅРµ РїР°РґР°РµРј, РѕС‡РµСЂРµРґСЊ
    // РєСЂСѓС‚РёС‚СЃСЏ РІРёСЂС‚СѓР°Р»СЊРЅРѕ РІ update().
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

  // РЎРіР»Р°Р¶РёРІР°РЅРёРµ РІР°СЂРїР° Рє С†РµР»Рё (~0.3СЃ) вЂ” РґР°РєРёРЅРі РјСѓР·С‹РєРё С‡РёС‚Р°РµС‚ m_warpCurrent.
  const float target = m_warpTarget.load();
  float cur = m_warpCurrent.load();
  const float a = 1.0f - std::exp(-dt * 6.0f);
  cur += (target - cur) * a;
  if (cur < 0.0f) cur = 0.0f;
  if (cur > 1.0f) cur = 1.0f;
  m_warpCurrent.store(cur);

  // Р“СѓР» РґРІРёРіР°С‚РµР»СЏ (v3.3.0): СЃРіР»Р°Р¶РёРІР°РЅРёРµ С†РµР»Рё (~0.2СЃ), С‡РёС‚Р°РµС‚ Р°СѓРґРёРѕРїРѕС‚РѕРє.
  const float engTarget = m_engineTarget.load();
  float eng = m_engineCurrent.load();
  const float ea = 1.0f - std::exp(-dt * 5.0f);
  eng += (engTarget - eng) * ea;
  if (eng < 0.0f) eng = 0.0f;
  if (eng > 1.0f) eng = 1.0f;
  m_engineCurrent.store(eng);

  // v4.3.0: Smooth tension level
  float tc = m_tensionCurrent.load();
  float tt = m_tensionTarget.load();
  tc += (tt - tc) * std::clamp(dt * 3.0f, 0.0f, 1.0f);
  m_tensionCurrent.store(tc);

  // v4.9.0: Smooth atmospheric roar level
  float ac = m_atmoCurrent.load();
  float at = m_atmoTarget.load();
  ac += (at - ac) * std::clamp(dt * 5.0f, 0.0f, 1.0f);
  m_atmoCurrent.store(ac);

  // v5.0.0: Smooth radio carrier static
  float rc = m_radioStaticCurrent.load();
  float rt = m_radioStaticTarget.load();
  rc += (rt - rc) * std::clamp(dt * 8.0f, 0.0f, 1.0f);
  m_radioStaticCurrent.store(rc);

  // v5.2.0: Smooth cassette tape gain
  float cg = m_cassetteGainCurrent.load();
  float cgt = m_cassetteGainTarget.load();
  cg += (cgt - cg) * std::clamp(dt * 3.5f, 0.0f, 1.0f);
  m_cassetteGainCurrent.store(cg);

  // v5.3.0: Smooth drill motor and regolith cutting sound
  float dc = m_drillCurrent.load();
  float dtarg = m_drillTarget.load();
  dc += (dtarg - dc) * std::clamp(dt * 8.0f, 0.0f, 1.0f);
  m_drillCurrent.store(dc);

  // v5.4.0: Smooth planetary weather intensity and wind speed
  float wi = m_weatherIntensityCurrent.load();
  float wit = m_weatherIntensityTarget.load();
  wi += (wit - wi) * std::clamp(dt * 3.0f, 0.0f, 1.0f);
  m_weatherIntensityCurrent.store(wi);

  float ww = m_weatherWindCurrent.load();
  float wwt = m_weatherWindTarget.load();
  ww += (wwt - ww) * std::clamp(dt * 4.0f, 0.0f, 1.0f);
  m_weatherWindCurrent.store(ww);

  if (!m_live) {
    // Null-backend: двигаем виртуальное время, чистим голоса по нему.
    uint64_t adv = static_cast<uint64_t>(static_cast<double>(dt) * kSampleRate);
    m_samples.fetch_add(adv);
  }
  const uint64_t now = m_samples.load();
  const double rate = static_cast<double>(kSampleRate);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_voices.size();) {
      // #159: возраст считаем в ЗНАКОВОМ типе. Реплики Егорки стартуют с
      // пре-роллом 50 мс (startSample лежит в будущем), поэтому беззнаковое
      // вычитание now - startSample давало ~4.2e14 секунд, и голос удалялся
      // в ближайшем же update() — реплика не успевала прозвучать.
      const double age = static_cast<double>(
                             static_cast<int64_t>(now) -
                             static_cast<int64_t>(m_voices[i].startSample)) /
                         rate;
      const double dur = voiceDurationSeconds(
          m_voices[i].type, m_voices[i].pcm ? m_voices[i].pcm->size() : 0);
      if (age > dur + 0.1) {
        if (m_voices[i].type == VoiceType::EgorkaVoice) {
          m_activeEgorkaCue.store(-1);
        }
        // #159: буфер удаляемого голоса нельзя освобождать сразу — аудиопоток
        // читает его из снимка уже без мьютекса. Откладываем освобождение.
        retireVoicePcm(m_voices[i].pcm);
        m_voices[i] = m_voices.back();
        m_voices.pop_back();
      } else {
        ++i;
      }
    }
  }
  // Освобождение отложенных буферов — вне мьютекса и только в основном
  // потоке: free() в аудиопотоке (RT-колбэк) недопустим.
  flushRetiredPcm();
}

const AudioEngine::VoiceDuration* AudioEngine::voiceDurationTable(size_t& count) {
  static const VoiceDuration kTable[] = {
      {VoiceType::Click, 0.12},      {VoiceType::Whoosh, 0.70},
      {VoiceType::Jump, 1.60},       {VoiceType::Chime, 2.00},
      {VoiceType::Blip, 0.30},       {VoiceType::Capture, 2.40},
      {VoiceType::Tick, 0.05},       {VoiceType::File, -1.00},
      {VoiceType::RadioBeep, 0.08},  {VoiceType::RadioEnd, 0.06},
      {VoiceType::WarpReady, 0.55},  {VoiceType::Hazard, 0.45},
      {VoiceType::TargetLock, 0.22}, {VoiceType::LiquidDrop, 0.35},
      {VoiceType::Eureka, 3.20},     {VoiceType::Thunder, 3.50},
      {VoiceType::EgorkaVoice, -1.00}};
  // #159: добавление нового VoiceType обязано ломать сборку, пока для него
  // не задана длительность. Иначе новый тип молча получил бы «длительность
  // по умолчанию» и обрывался через 1 секунду (как с EgorkaVoice в #159).
  static_assert(sizeof(kTable) / sizeof(kTable[0]) ==
                    static_cast<size_t>(VoiceType::kCount),
                "voiceDurationTable must have one entry per VoiceType");
  count = sizeof(kTable) / sizeof(kTable[0]);
  return kTable;
}

double AudioEngine::voiceDurationSeconds(VoiceType type,
                                         size_t pcmSamples) const {
  size_t count = 0;
  const VoiceDuration* table = voiceDurationTable(count);
  for (size_t i = 0; i < count; ++i) {
    if (table[i].type != type) continue;
    if (table[i].seconds >= 0.0) return table[i].seconds;
    // File / EgorkaVoice: длительность = размер PCM-буфера.
    return static_cast<double>(pcmSamples) / static_cast<double>(kSampleRate);
  }
  return 0.0;  // недостижимо: таблица покрывает все VoiceType
}

void AudioEngine::retireVoicePcm(std::shared_ptr<std::vector<float>>& pcm) {
  if (!pcm) return;
  RetiredPcm rv;
  rv.pcm = std::move(pcm);
  rv.retireEpoch = m_renderEpoch.load(std::memory_order_acquire);
  m_retiredPcm.push_back(std::move(rv));
}

void AudioEngine::flushRetiredPcm() {
  if (m_retiredPcm.empty()) return;
  const uint64_t epoch = m_renderEpoch.load(std::memory_order_acquire);
  size_t keep = 0;
  for (size_t i = 0; i < m_retiredPcm.size(); ++i) {
    if (epoch > m_retiredPcm[i].retireEpoch) {
      // Рендер-проход, который мог читать этот буфер, уже завершён —
      // освобождаем в основном потоке.
      m_retiredPcm[i].pcm.reset();
    } else {
      if (keep != i) {
        m_retiredPcm[keep] = std::move(m_retiredPcm[i]);
      }
      ++keep;
    }
  }
  m_retiredPcm.resize(keep);
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

void AudioEngine::setMenuActive(bool active) {
  m_menuTarget.store(active);
}

void AudioEngine::setDrillLevel(float k, float /*strain*/) {
  if (k < 0.0f) k = 0.0f;
  if (k > 1.0f) k = 1.0f;
  m_drillTarget.store(k);
  m_drillCurrent.store(k);
}

void AudioEngine::setDopplerShift(float factor) {
  if (factor < 0.2f) factor = 0.2f;
  if (factor > 3.0f) factor = 3.0f;
  m_dopplerFactor.store(factor);
}

void AudioEngine::setTensionLevel(float k) {
  if (k < 0.0f) k = 0.0f;
  if (k > 1.0f) k = 1.0f;
  m_tensionTarget.store(k);
}

void AudioEngine::setAtmosphereRoar(float level) {
  if (level < 0.0f) level = 0.0f;
  if (level > 1.0f) level = 1.0f;
  m_atmoTarget.store(level);
}

void AudioEngine::setRadioStatic(float level) {
  if (level < 0.0f) level = 0.0f;
  if (level > 1.0f) level = 1.0f;
  m_radioStaticTarget.store(level);
}

void AudioEngine::setCassetteTape(int tape) {
  if (tape < 0) tape = 0;
  if (tape > 3) tape = 3;
  m_cassetteTape.store(tape);
  m_cassetteGainTarget.store(tape > 0 ? 1.0f : 0.0f);
}

void AudioEngine::setCassetteVolume(float v) {
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  m_cassetteVolume.store(v);
}

void AudioEngine::playClick() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Click, m_samples.load(), nullptr, 0.0f, 0.0f});
}

// v3.0.0 / v4.3.0: тик пульсара с пространственной панорамой [-1..1].
void AudioEngine::playTick(float pan) {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Tick, m_samples.load(), nullptr, 0.0f, std::clamp(pan, -1.0f, 1.0f)});
}

void AudioEngine::playWhoosh(float pan) {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Whoosh, m_samples.load(), nullptr, 0.0f, std::clamp(pan, -1.0f, 1.0f)});
}

void AudioEngine::playJump() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Jump, m_samples.load(), nullptr, 0.0f});
}

void AudioEngine::playChime() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Chime, m_samples.load(), nullptr, 0.0f});
}

void AudioEngine::playBlip() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Blip, m_samples.load(), nullptr, 0.0f});
}

void AudioEngine::playCapture() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  // РўСЂРµРІРѕРіСѓ РЅРµ СЃРїР°РјРёРј: РЅРµ С‡Р°С‰Рµ РѕРґРЅРѕР№ Р·Р° СЂР°Р·.
  for (const auto& v : m_voices) {
    if (v.type == VoiceType::Capture) {
      const double age =
          static_cast<double>(m_samples.load() - v.startSample) / kSampleRate;
      if (age < 2.0) return;
    }
  }
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Capture, m_samples.load(), nullptr, 0.0f});
}

void AudioEngine::playRadioBeep(float pan) {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::RadioBeep, m_samples.load(), nullptr, 0.0f, std::clamp(pan, -1.0f, 1.0f)});
}

void AudioEngine::playRadioEnd(float pan) {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::RadioEnd, m_samples.load(), nullptr, 0.0f, std::clamp(pan, -1.0f, 1.0f)});
}

void AudioEngine::playWarpReadyChime() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::WarpReady, m_samples.load(), nullptr, 0.0f, 0.0f});
}

void AudioEngine::playHazardAlarm() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& v : m_voices) {
    if (v.type == VoiceType::Hazard) {
      const double age =
          static_cast<double>(m_samples.load() - v.startSample) / kSampleRate;
      if (age < 0.45) return;
    }
  }
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Hazard, m_samples.load(), nullptr, 0.0f, 0.0f});
}

void AudioEngine::playTargetLockPing(float pan) {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::TargetLock, m_samples.load(), nullptr, 0.0f, std::clamp(pan, -1.0f, 1.0f)});
}

void AudioEngine::playLiquidDrop(float pan) {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::LiquidDrop, m_samples.load(), nullptr, 0.0f, std::clamp(pan, -1.0f, 1.0f)});
}

void AudioEngine::playEurekaChime() {
  if (!m_tried) return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() > 32) return;
  m_voices.push_back({VoiceType::Eureka, m_samples.load(), nullptr, 0.0f, 0.0f});
}

// v4.0: С„Р°Р№Р»-РіРѕР»РѕСЃ РєР°С‚СЃС†РµРЅС‹. Р”РµРєРѕРґ СЃРёРЅС…СЂРѕРЅРЅС‹Р№ РІ РІС‹Р·С‹РІР°СЋС‰РµРј С‚СЂРµРґРµ (UI):
// РєРѕСЂРѕС‚РєРёР№ wav РєР°С‚СЃС†РµРЅС‹ С‡РёС‚Р°РµС‚СЃСЏ Р·Р° РјРёР»Р»РёСЃРµРєСѓРЅРґС‹; audio-РїРѕС‚РѕРє РїРѕС‚РѕРј
// С‚РѕР»СЊРєРѕ С‡РёС‚Р°РµС‚ РіРѕС‚РѕРІС‹Р№ Р±СѓС„РµСЂ (no lock).
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

// v5.5.0: Голосовой аннонсер бортового ИИ (Егорка-6)
static const char* egorkaCueFilename(AudioEngine::EgorkaVoiceCue cue) {
  switch (cue) {
    case AudioEngine::EgorkaVoiceCue::WarpOn: return "warp_on.wav";
    case AudioEngine::EgorkaVoiceCue::WarpOff: return "warp_off.wav";
    case AudioEngine::EgorkaVoiceCue::GearDown: return "gear_down.wav";
    case AudioEngine::EgorkaVoiceCue::GearUp: return "gear_up.wav";
    case AudioEngine::EgorkaVoiceCue::DockOk: return "dock_ok.wav";
    case AudioEngine::EgorkaVoiceCue::UndockOk: return "undock_ok.wav";
    case AudioEngine::EgorkaVoiceCue::FuelLow: return "fuel_low.wav";
    case AudioEngine::EgorkaVoiceCue::ShieldLow: return "shield_low.wav";
    case AudioEngine::EgorkaVoiceCue::TerrainWarn: return "terrain_warn.wav";
    case AudioEngine::EgorkaVoiceCue::OrbitOk: return "orbit_ok.wav";
    case AudioEngine::EgorkaVoiceCue::SysOnline: return "sys_online.wav";
    case AudioEngine::EgorkaVoiceCue::DialOj287: return "dial_oj287.wav";
    case AudioEngine::EgorkaVoiceCue::DialHyperdisk: return "dial_hyperdisk.wav";
    case AudioEngine::EgorkaVoiceCue::DialGw170817: return "dial_gw170817.wav";
    case AudioEngine::EgorkaVoiceCue::DialSn1987a: return "dial_sn1987a.wav";
    case AudioEngine::EgorkaVoiceCue::DialHardLanding: return "dial_hard_landing.wav";
    case AudioEngine::EgorkaVoiceCue::DialDrillEureka: return "dial_drill_eureka.wav";
    case AudioEngine::EgorkaVoiceCue::MiaM16: return "mia_m16.wav";
    case AudioEngine::EgorkaVoiceCue::MiaM1: return "mia_m1.wav";
    case AudioEngine::EgorkaVoiceCue::MiaAt2019qiz: return "mia_at2019qiz.wav";
    case AudioEngine::EgorkaVoiceCue::MiaTouchdown: return "mia_touchdown.wav";
    case AudioEngine::EgorkaVoiceCue::ElenaSgra: return "elena_sgra.wav";
    case AudioEngine::EgorkaVoiceCue::ElenaM87: return "elena_m87.wav";
    case AudioEngine::EgorkaVoiceCue::AuroraSysOnline: return "aurora_sys_online.wav";
  }
  return nullptr;
}

void AudioEngine::stopCurrentVoiceCue() {
  m_activeEgorkaCue.store(-1);
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_voices.begin(); it != m_voices.end(); ) {
    if (it->type == VoiceType::EgorkaVoice) {
      // #159: тот же протокол, что и в update() — буфер освобождается
      // отложенно, чтобы аудиопоток не читал освобождённую память.
      retireVoicePcm(it->pcm);
      it = m_voices.erase(it);
    } else {
      ++it;
    }
  }
}

bool AudioEngine::isEgorkaVoicePlaying() const {
  if (m_activeEgorkaCue.load() < 0) return false;
  std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
  for (const auto& v : m_voices) {
    if (v.type == VoiceType::EgorkaVoice) return true;
  }
  return false;
}

void AudioEngine::playEgorkaVoice(EgorkaVoiceCue cue, float gainDb, bool force) {
  if (!m_tried || !m_egorkaVoiceEnabled.load()) return;
  const int cueId = static_cast<int>(cue);
  const uint64_t nowSample = m_samples.load();

  // Дебаунс: повтор той же реплики не чаще чем раз в 3.0 секунды (если не force)
  // Кеширование буфера в m_egorkaCache
  std::shared_ptr<std::vector<float>> pcm;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!force && m_lastEgorkaCue == cueId) {
      const double elapsed = static_cast<double>(nowSample - m_lastEgorkaSample) / kSampleRate;
      if (elapsed < 3.0) return;
    }
    auto it = m_egorkaCache.find(cueId);
    if (it != m_egorkaCache.end()) {
      pcm = it->second;
    }
  }

  if (!pcm) {
    const char* fn = egorkaCueFilename(cue);
    if (!fn) return;

    // Пути поиска: res/audio/egorka/<fn>, res/audio/crew_female/<fn>
    const std::string candidates[] = {
      std::string("res/audio/egorka/") + fn,
      std::string("../res/audio/egorka/") + fn,
      std::string("build-gl/Release/res/audio/egorka/") + fn,
      std::string("res/audio/crew_female/") + fn,
      std::string("../res/audio/crew_female/") + fn,
      std::string("build-gl/Release/res/audio/crew_female/") + fn
    };

    for (const auto& path : candidates) {
      ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 1, kSampleRate);
      ma_decoder dec;
      if (ma_decoder_init_file(path.c_str(), &dcfg, &dec) == MA_SUCCESS) {
        std::vector<float> buf;
        float chunk[4096];
        for (;;) {
          ma_uint64 got = 0;
          if (ma_decoder_read_pcm_frames(&dec, chunk, 4096, &got) != MA_SUCCESS || got == 0) break;
          buf.insert(buf.end(), chunk, chunk + got);
        }
        ma_decoder_uninit(&dec);
        if (!buf.empty()) {
          pcm = std::make_shared<std::vector<float>>(std::move(buf));
          std::lock_guard<std::mutex> lock(m_mutex);
          m_egorkaCache[cueId] = pcm;
          break;
        }
      }
    }
  }

  if (!pcm || pcm->empty()) return;

  // Останавливаем предыдущую реплику, чтобы не было наложения
  stopCurrentVoiceCue();

  // Тактический PTT squelch клик перед голосом
  playRadioBeep(0.0f);

  Voice v;
  v.type = VoiceType::EgorkaVoice;
  v.startSample = m_samples.load() + static_cast<uint64_t>(kSampleRate * 0.05);
  v.pcm = pcm;
  v.gainDb = gainDb;
  v.pan = 0.0f;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_voices.size() <= 32) {
      m_voices.push_back(std::move(v));
      m_lastEgorkaSample = nowSample;
      m_lastEgorkaCue = cueId;
      m_activeEgorkaCue.store(cueId);
    }
  }
}

float AudioEngine::renderMusic(double tAbs, double tLoop) const {
  // РђРєРєРѕСЂРґ i + equal-power РєСЂРѕСЃСЃС„РµР№Рґ 2СЃ РІ РєРѕРЅС†Рµ РєР°Р¶РґРѕРіРѕ 8СЃ РѕС‚СЂРµР·РєР°.
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

  // Р Р°Р·СЂРµР¶РµРЅРЅС‹Рµ РїРµРЅС‚Р°С‚РѕРЅРЅС‹Рµ С‰РёРїРєРё + echo-delay (Р±РµСЃС€РѕРІРЅРѕ: РґРёСЃС‚Р°РЅС†РёСЏ РїРѕ РјРѕРґСѓР»СЋ).
  float plucks = 0.0f;
  for (int i = 0; i < 6; ++i) {
    double d = tLoop - kPluckTimes[i];
    d = std::fmod(d, kLoopSec);
    if (d < 0.0) d += kLoopSec;
    // d вЂ” РІРѕР·СЂР°СЃС‚ С‰РёРїРєР° РІ Р»СѓРїРµ; СЃС‚Р°СЂС€Рµ 3СЃ РїРѕС‡С‚Рё Р·Р°С‚СѓС… вЂ” СЃРєРёРїР°РµРј.
    if (d > 3.0) continue;
    plucks += pluckTone(kPluckNotes[i], d);
    plucks += 0.35f * pluckTone(kPluckNotes[i], d - kEcho1);
    plucks += 0.22f * pluckTone(kPluckNotes[i], d - kEcho2);
  }

  const float music = pad * 0.6f + plucks * 0.5f;
  return music * dbToGain(kMusicDb);  // С‚РёС…Рѕ: -18 РґР‘
}

float AudioEngine::renderVoice(const VoiceView& v, double age,
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
      // Р¤Р°Р·Р° sweep: РёРЅС‚РµРіСЂР°Р» f(t).
      const double phase =
          2.0 * kPi * 300.0 * 0.6 / std::log(8.0) * (std::pow(8.0, age / 0.6) - 1.0);
      (void)f;
      const float sweep = static_cast<float>(std::sin(phase));
      const float n = whiteNoise(rng);
      return (n * 0.6f + sweep * 0.35f) * env * 0.8f;
    }
    case VoiceType::Jump: {
      if (age > 1.6) return 0.0f;
      // Sine-drop 200 -> 30 Р“С†: f(t) = 30 + 170*exp(-3.2t).
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
      // РўСЂРµРІРѕРіР° Р·Р°С…РІР°С‚Р° Р§Р”: С‡РµСЂРµРґРѕРІР°РЅРёРµ 523/784 Р“С†, РїСѓР»СЊСЃ 5 Р“С†.
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
    case VoiceType::RadioBeep: {
      // PTT chirp + relay click
      if (age > 0.08) return 0.0f;
      const double f = 2400.0 - (age / 0.08) * 600.0; // 2400 -> 1800 Hz sweep
      const float tone = static_cast<float>(std::sin(2.0 * kPi * f * age));
      const float n = (age < 0.02) ? whiteNoise(rng) * 0.5f : 0.0f;
      const float env = static_cast<float>(std::sin(kPi * (age / 0.08)));
      return (tone * 0.6f + n) * env * 0.5f;
    }
    case VoiceType::RadioEnd: {
      // PTT tail squelch burst + release click
      if (age > 0.06) return 0.0f;
      const float n = whiteNoise(rng);
      const float env = static_cast<float>(std::exp(-age * 60.0));
      const float click = static_cast<float>(std::sin(2.0 * kPi * 400.0 * age)) * env;
      return (n * 0.7f + click * 0.4f) * env * 0.45f;
    }
    case VoiceType::WarpReady: {
      // Ascending two-tone chime: 523.25 Hz (C5) -> 659.25 Hz (E5)
      if (age > 0.55) return 0.0f;
      if (age < 0.18) {
        const float s = static_cast<float>(std::sin(2.0 * kPi * 523.25 * age)) +
                        0.25f * static_cast<float>(std::sin(2.0 * kPi * 1046.5 * age));
        const float env = static_cast<float>(std::exp(-age * 6.0));
        return s * env * 0.45f;
      } else {
        const double a2 = age - 0.18;
        const float s = static_cast<float>(std::sin(2.0 * kPi * 659.25 * a2)) +
                        0.25f * static_cast<float>(std::sin(2.0 * kPi * 1318.5 * a2));
        const float env = static_cast<float>(std::exp(-a2 * 5.0));
        return s * env * 0.5f;
      }
    }
    case VoiceType::Hazard: {
      // Pulsed alarm: alternates between 880 Hz and 440 Hz
      if (age > 0.45) return 0.0f;
      const double f = (std::fmod(age, 0.12) < 0.06) ? 880.0 : 440.0;
      const float s = static_cast<float>(std::sin(2.0 * kPi * f * age)) +
                      0.3f * static_cast<float>(std::sin(2.0 * kPi * f * 2.0 * age));
      const float env = static_cast<float>(std::sin(kPi * (age / 0.45)));
      return s * env * 0.5f;
    }
    case VoiceType::TargetLock: {
      // Tactical lock ping: 1200 Hz -> 950 Hz drop
      if (age > 0.22) return 0.0f;
      const double f = 950.0 + 250.0 * std::exp(-age * 18.0);
      const float s = static_cast<float>(std::sin(2.0 * kPi * f * age)) +
                      0.2f * static_cast<float>(std::sin(2.0 * kPi * 2.0 * f * age));
      const float env = static_cast<float>(std::exp(-age * 16.0));
      return s * env * 0.55f;
    }
    case VoiceType::LiquidDrop: {
      // Процедурный «бульк» капли: частотный свип 600 -> 1750 Гц за 40 мс, затухание 120 мс
      if (age > 0.16) return 0.0f;
      double f;
      if (age < 0.045) {
        f = 600.0 + (1750.0 - 600.0) * (age / 0.045);
      } else {
        f = 1750.0 - 150.0 * (age - 0.045);
      }
      const double phase = 2.0 * kPi * f * age;
      const float s = static_cast<float>(std::sin(phase)) +
                      0.3f * static_cast<float>(std::sin(phase * 2.0));
      // Всплеск поверхностного натяжения (pop transient в первые 8 мс)
      const float pop = (age < 0.008) ? static_cast<float>(std::sin(2.0 * kPi * 2800.0 * age)) * 0.4f : 0.0f;
      const float env = static_cast<float>(std::exp(-age * 28.0));
      return (s * 0.7f + pop) * env * 0.65f;
    }
    case VoiceType::Eureka: {
      // Небесный полифонический перезвон открытия: арпеджированный аккорд C5-G5-C6-E6
      if (age > 3.2) return 0.0f;
      static const double kNotes[4] = {523.251, 783.991, 1046.502, 1318.510};
      static const double kDelays[4] = {0.0, 0.06, 0.13, 0.22};
      float sum = 0.0f;
      for (int i = 0; i < 4; ++i) {
        const double a = age - kDelays[i];
        if (a < 0.0) continue;
        const double f = kNotes[i];
        // Мягкое мерцающее вибрато 4.5 Гц
        const double vib = 1.0 + 0.003 * std::sin(2.0 * kPi * 4.5 * a);
        const float tone = static_cast<float>(std::sin(2.0 * kPi * (f * vib) * a)) +
                           0.35f * static_cast<float>(std::sin(2.0 * kPi * (f * vib * 2.0) * a)) +
                           0.12f * static_cast<float>(std::sin(2.0 * kPi * (f * vib * 3.0) * a));
        const float env = static_cast<float>(std::exp(-a * 1.5));
        const float att = (a < 0.012) ? static_cast<float>(a / 0.012) : 1.0f;
        sum += tone * env * att;
      }
      return sum * 0.30f;
    }
    case VoiceType::Thunder: {
      if (age > 3.5) return 0.0f;
      // Initial crack: crisp high-energy burst within first 0.06s
      const float w = whiteNoise(rng);
      float crack = (age < 0.06) ? w * (1.0f - static_cast<float>(age / 0.06)) * 0.75f : 0.0f;
      // Low rolling thunder resonant bass (36-52 Hz)
      float rollFreq = 48.0f - 14.0f * static_cast<float>(age / 3.5);
      float sub1 = static_cast<float>(std::sin(2.0 * kPi * rollFreq * age)) * 0.45f;
      float sub2 = static_cast<float>(std::sin(2.0 * kPi * (rollFreq * 1.6f) * age + 0.4)) * 0.28f;
      float decay = static_cast<float>(std::exp(-age * 1.15));
      return (crack + (sub1 + sub2 + w * 0.20f) * decay) * 0.85f;
    }
    case VoiceType::File: {
      // Р¤Р°Р№Р»-РіРѕР»РѕСЃ: Р»РёРЅРµР№РЅС‹Р№ Р»РµСЂРї РїРѕ РіРѕС‚РѕРІРѕРјСѓ float-Р±СѓС„РµСЂСѓ (РјРѕРЅРѕ,
      // kSampleRate, Р·РЅР°С‡РёС‚ С€Р°Рі 1 СЃСЌРјРїР» РЅР° СЃСЌРјРїР»; Р»РµСЂРї СЃС‚СЂР°С…РѕРІРєР°).
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
    case VoiceType::EgorkaVoice: {
      if (!v.pcm || v.pcm->empty()) return 0.0f;
      const size_t n = v.pcm->size();
      const double idx = age * static_cast<double>(kSampleRate);
      if (idx < 0.0) return 0.0f;
      if (idx >= static_cast<double>(n)) return 0.0f;
      const size_t i0 = static_cast<size_t>(idx);
      const float f = static_cast<float>(idx - static_cast<double>(i0));
      const float a = (*v.pcm)[i0];
      const float b = (i0 + 1 < n) ? (*v.pcm)[i0 + 1] : a;
      const float vVol = m_egorkaVoiceVolume.load();
      return (a + (b - a) * f) * dbToGain(v.gainDb) * vVol;
    }
    case VoiceType::kCount:
      return 0.0f;  // служебный маркер, голосом не является
  }
  return 0.0f;
}

void AudioEngine::renderFrames(float* out, unsigned frames) {
  // Снимок голосов под коротким мьютексом (без аллокаций в колбэке).
  // #159: снимок владеет копией shared_ptr на PCM-буфер — основной поток
  // не может освободить буфер, пока его читает рендер.
  VoiceView snap[40];
  size_t nSnap = 0;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    nSnap = m_voices.size() > 40 ? 40 : m_voices.size();
    for (size_t i = 0; i < nSnap; ++i) {
      snap[i].type = m_voices[i].type;
      snap[i].startSample = m_voices[i].startSample;
      snap[i].pcm = m_voices[i].pcm;
      snap[i].gainDb = m_voices[i].gainDb;
      snap[i].pan = m_voices[i].pan;
    }
  }
  const float vol = m_muted.load() ? 0.0f : m_volume.load();
  const float warpK = m_warpCurrent.load();
  const float duck = dbToGain(kDuckDb * warpK);  // -8 РґР‘ РїСЂРё K=1
  const double rate = static_cast<double>(kSampleRate);
  const double menuStep = (m_menuTarget.load() ? 1.0 : -1.0) /
                          (kMenuFadeSec * rate);
  uint64_t base = m_samples.load();

  for (unsigned i = 0; i < frames; ++i) {
    // Keep the fade and oscillator phase continuous when the target reverses.
    m_menuBlend = std::clamp(m_menuBlend + menuStep, 0.0, 1.0);
    const float menuGain = static_cast<float>(
        m_menuBlend * m_menuBlend * (3.0 - 2.0 * m_menuBlend));
    const float gameGain = 1.0f - menuGain;
    const double tAbs = static_cast<double>(base + i) / rate;
    float ambientL = 0.0f;
    float ambientR = 0.0f;
    if (menuGain > 0.0f) renderMenuAmbient(tAbs, ambientL, ambientR);
    const double tLoop = wrapLoop(tAbs);
    float music = renderMusic(tAbs, tLoop) * duck;

    float sfxL = 0.0f;
    float sfxR = 0.0f;
    for (size_t v = 0; v < nSnap; ++v) {
      // #159: возраст в знаковом типе — реплика с пре-роллом 50 мс имеет
      // startSample в будущем, и беззнаковое вычитание давало огромный
      // возраст вместо отрицательного.
      const double age =
          static_cast<double>(static_cast<int64_t>(base + i) -
                              static_cast<int64_t>(snap[v].startSample)) /
          rate;
      if (age < 0.0) continue;
      const float s = renderVoice(snap[v], age, m_rng);
      const float pan01 = std::clamp((snap[v].pan + 1.0f) * 0.5f, 0.0f, 1.0f);
      const float gL = static_cast<float>(std::cos(static_cast<double>(pan01) * kPi * 0.5));
      const float gR = static_cast<float>(std::sin(static_cast<double>(pan01) * kPi * 0.5));
      sfxL += s * gL;
      sfxR += s * gR;
    }

    // WarpRumble: РєРѕСЂРёС‡РЅРµРІС‹Р№ С€СѓРј + СЃСѓР±-СЃРёРЅСѓСЃ, РіСЂРѕРјРєРѕСЃС‚СЊ РѕС‚ СѓСЂРѕРІРЅСЏ РІР°СЂРїР°.
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

    // Engine hum (v3.3.0): С‚РѕРЅ 52-82 Р“С† РїРѕ СЃРєРѕСЂРѕСЃС‚Рё + РѕР±РµСЂС‚РѕРЅ + РґС‹С…Р°РЅРёРµ С€СѓРјР°.
    const float doppler = std::clamp(m_dopplerFactor.load(), 0.4f, 2.5f);
    float engine = 0.0f;
    if (engK > 0.001f) {
      const float f = (52.0f + 30.0f * engK) * doppler;
      const float tone =
          static_cast<float>(std::sin(2.0 * kPi * f * tAbs)) +
          0.35f * static_cast<float>(std::sin(2.0 * kPi * 2.0 * f * tAbs + 0.7));
      engine = (tone * 0.16f + m_brown * 1.2f) * engK;
    }

    const float tension = m_tensionCurrent.load();
    float drone = 0.0f;
    if (tension > 0.001f) {
      const float sub = static_cast<float>(std::sin(2.0 * kPi * 38.0 * tAbs)) +
                        0.5f * static_cast<float>(std::sin(2.0 * kPi * 41.5 * tAbs + 0.4));
      drone = sub * tension * 0.22f;
    }

    // Atmospheric roar (v4.9.0): Hypersonic air-shearing & plasma ionization roar
    const float atmoK = m_atmoCurrent.load();
    float atmoRoar = 0.0f;
    if (atmoK > 0.001f) {
      const float w = whiteNoise(m_rng);
      m_atmoNoise = (m_atmoNoise + 0.06f * w) / 1.06f;
      float flutter = 1.0f + 0.28f * static_cast<float>(std::sin(2.0 * kPi * 22.0 * tAbs));
      atmoRoar = (m_atmoNoise * 3.2f + m_brown * 1.4f) * flutter * atmoK * 0.40f;
    }

    // Radio carrier static (v5.0.0): Deep-space shortwave static & ionization hiss
    const float radioK = m_radioStaticCurrent.load();
    float radioStaticSound = 0.0f;
    if (radioK > 0.001f) {
      const float w = whiteNoise(m_rng);
      m_radioBandpass1 += (w - m_radioBandpass1) * 0.35f;
      m_radioBandpass2 += (m_radioBandpass1 - m_radioBandpass2) * 0.10f;
      float bandpass = m_radioBandpass1 - m_radioBandpass2;
      float crackle = ((m_rng & 0x7ff) == 0) ? (whiteNoise(m_rng) * 0.5f) : 0.0f;
      radioStaticSound = (bandpass * 1.8f + crackle) * radioK * 0.20f;
    }

    // Cassette Tape Player (v5.2.0: Cosmic Ambient Deck)
    const float cassK = m_cassetteGainCurrent.load();
    const float cassVol = m_cassetteVolume.load();
    const int tapeId = m_cassetteTape.load();
    float tapeL = 0.0f;
    float tapeR = 0.0f;
    if (cassK > 0.001f && tapeId > 0) {
      if (tapeId == 1) {
        // Tape 1: Paris Rain (Звуки Дома)
        const float w = whiteNoise(m_rng);
        m_cassetteRainNoise = (m_cassetteRainNoise + 0.045f * w) / 1.045f;
        float patter = 1.0f + 0.35f * static_cast<float>(std::sin(2.0 * kPi * 13.0 * tAbs) * std::cos(2.0 * kPi * 4.3 * tAbs));
        float rainL = m_cassetteRainNoise * 3.0f * patter;
        float rainR = m_cassetteRainNoise * 2.7f * (1.8f - patter);

        // Electric piano / warm chord progression (Fm -> Cm -> Eb -> Bb)
        const int chordIdx = static_cast<int>(tAbs / 6.0) % 4;
        float f1 = 174.61f, f2 = 207.65f, f3 = 261.63f, f4 = 311.13f; // Fm
        if (chordIdx == 1) { // Cm
          f1 = 130.81f; f2 = 196.00f; f3 = 261.63f; f4 = 311.13f;
        } else if (chordIdx == 2) { // Eb
          f1 = 155.56f; f2 = 233.08f; f3 = 311.13f; f4 = 392.00f;
        } else if (chordIdx == 3) { // Bb
          f1 = 116.54f; f2 = 174.61f; f3 = 261.63f; f4 = 349.23f;
        }
        float wow = 1.0f + 0.0035f * static_cast<float>(std::sin(2.0 * kPi * 0.35 * tAbs));
        float pTone = 0.28f * static_cast<float>(std::sin(2.0 * kPi * f1 * wow * tAbs)) +
                      0.24f * static_cast<float>(std::sin(2.0 * kPi * f2 * wow * tAbs + 0.4)) +
                      0.22f * static_cast<float>(std::sin(2.0 * kPi * f3 * wow * tAbs + 0.8)) +
                      0.18f * static_cast<float>(std::sin(2.0 * kPi * f4 * wow * tAbs + 1.2));
        tapeL = (rainL * 0.45f + pTone * 0.55f) * cassK * cassVol;
        tapeR = (rainR * 0.45f + pTone * 0.55f) * cassK * cassVol;
      } else if (tapeId == 2) {
        // Tape 2: Pulsar Beat (Пульс Космоса)
        double beatT = std::fmod(tAbs * 1.4, 1.0);
        float kickEnv = static_cast<float>(std::exp(-beatT * 12.0));
        float subFreq = 42.0f + 26.0f * kickEnv;
        float kick = static_cast<float>(std::sin(2.0 * kPi * subFreq * tAbs)) * kickEnv;

        double clickT = std::fmod(tAbs * 2.8, 1.0);
        float clickEnv = static_cast<float>(std::exp(-clickT * 42.0));
        const float w = whiteNoise(m_rng);
        float clickSound = w * clickEnv * 0.30f;

        float subDrone = 0.20f * static_cast<float>(std::sin(2.0 * kPi * 55.0 * tAbs));
        tapeL = (kick * 0.70f + clickSound * 0.60f + subDrone) * cassK * cassVol;
        tapeR = (kick * 0.70f + clickSound * 0.25f + subDrone) * cassK * cassVol;
      } else if (tapeId == 3) {
        // Tape 3: Deep Space Void (Эфир Пустоты)
        float lfo = 0.5f + 0.5f * static_cast<float>(std::sin(2.0 * kPi * 0.07 * tAbs));
        float s1 = static_cast<float>(std::sin(2.0 * kPi * 58.27 * tAbs));
        float s2 = static_cast<float>(std::sin(2.0 * kPi * 87.31 * tAbs + 0.5));
        float s3 = static_cast<float>(std::sin(2.0 * kPi * 116.54 * tAbs + 1.1));
        float s4 = static_cast<float>(std::sin(2.0 * kPi * 146.83 * tAbs + 1.8));

        float padL = s1 * 0.35f + s2 * (0.20f + 0.15f * lfo) + s4 * 0.25f;
        float padR = s1 * 0.35f + s3 * (0.20f + 0.15f * (1.0f - lfo)) + s4 * 0.25f;
        tapeL = padL * 0.75f * cassK * cassVol;
        tapeR = padR * 0.75f * cassK * cassVol;
      }
      music *= (1.0f - 0.60f * cassK);
    }

    // Drill sound (v5.3.0): Motor drone (95-160 Hz) + rock grinding noise modulated by torque/strain
    const float drillK = m_drillCurrent.load();
    float drillSound = 0.0f;
    if (drillK > 0.001f) {
      const float w = whiteNoise(m_rng);
      m_drillGrind = (m_drillGrind + 0.12f * w) / 1.12f;
      float dmotor = static_cast<float>(std::sin(2.0 * kPi * (95.0f + 65.0f * drillK) * tAbs)) +
                     0.4f * static_cast<float>(std::sin(2.0 * kPi * (190.0f + 130.0f * drillK) * tAbs + 0.5));
      drillSound = (dmotor * 0.15f + m_drillGrind * 0.22f) * drillK * 0.45f;
    }

    // Dynamic planetary weather audio (v5.4.0): howling wind + rain/grit patter
    const float weaIntensity = m_weatherIntensityCurrent.load();
    const float weaWind = m_weatherWindCurrent.load();
    const int weaType = m_weatherType.load();
    float weatherSoundL = 0.0f;
    float weatherSoundR = 0.0f;
    if (weaIntensity > 0.001f && weaType > 0) {
      const float w = whiteNoise(m_rng);
      m_weatherWindNoise = (m_weatherWindNoise + 0.035f * w) / 1.035f;
      float gustMod = 1.0f + 0.35f * static_cast<float>(std::sin(2.0 * kPi * 0.25 * tAbs));
      float windSound = m_weatherWindNoise * (weaWind / 0.025f) * gustMod * 0.32f;

      float patterSound = 0.0f;
      if (weaType == 1 || weaType == 2 || weaType == 3 || weaType == 4) {
        m_weatherPatterNoise = (m_weatherPatterNoise + 0.12f * w) / 1.12f;
        float patterFlutter = 1.0f + 0.4f * static_cast<float>(std::sin(2.0 * kPi * 18.0 * tAbs));
        patterSound = m_weatherPatterNoise * patterFlutter * weaIntensity * 0.22f;
      }
      weatherSoundL = (windSound + patterSound * 0.9f) * weaIntensity;
      weatherSoundR = (windSound * 0.95f + patterSound * 1.05f) * weaIntensity;
    }

    // Preserve cassette phase and leave UI/voice SFX outside the crossfade.
    const float gameplay = music + rumble + engine + drone + atmoRoar +
                           radioStaticSound + drillSound;
    float mixL = ((gameplay + tapeL + weatherSoundL) * gameGain +
                  ambientL * menuGain + sfxL) * vol;
    float mixR = ((gameplay + tapeR + weatherSoundR) * gameGain +
                  ambientR * menuGain + sfxR) * vol;
    mixL = std::tanh(mixL * 1.1f);
    mixR = std::tanh(mixR * 1.1f);
    out[i * 2 + 0] = mixL;
    out[i * 2 + 1] = mixR;
  }
  m_samples.fetch_add(frames);
  // #159: сначала отпускаем снимок (PCM-буферы), и только потом публикуем
  // новый epoch. Порядок обязателен: основной поток освобождает отложенные
  // буферы лишь увидев epoch > retireEpoch, а к этому моменту аудиопоток
  // уже не читает эти буферы.
  for (size_t i = 0; i < nSnap; ++i) {
    snap[i].pcm.reset();
  }
  m_renderEpoch.fetch_add(1, std::memory_order_release);
}

void AudioEngine::setWeatherAmbience(int weatherType, float intensity, float windKps) {
  m_weatherType.store(weatherType);
  m_weatherIntensityTarget.store(std::clamp(intensity, 0.0f, 1.0f));
  m_weatherWindTarget.store(std::clamp(windKps, 0.0f, 0.2f));
}

void AudioEngine::playThunderCrack(float pan) {
  if (!m_tried) return;
  Voice v;
  v.type = VoiceType::Thunder;
  v.startSample = m_samples.load();
  v.gainDb = 0.0f;
  v.pan = std::clamp(pan, -1.0f, 1.0f);
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_voices.size() <= 32) {
    m_voices.push_back(v);
  }
}

}  // namespace astro
