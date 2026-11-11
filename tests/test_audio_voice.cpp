// test_audio_voice.cpp (issue #159): регрессии аудио-движка.
//
// Проверяем то, что не покрывают демо:
//  1) знаковый возраст голоса — реплика с пре-роллом 50 мс (startSample в
//     будущем) не должна удаляться до своего начала (#159: беззнаковое
//     вычитание now-startSample давало ~4.2e14 и реплика исчезала мгновенно);
//  2) время жизни PCM-буфера: удаление голоса не освобождает буфер, пока
//     его может читать аудиопоток (use-after-free);
//  3) таблица длительностей покрывает все VoiceType (иначе новый тип молча
//     получил бы 1 секунду и реплика обрывалась).
//
// Тест не открывает аудиоустройство (конструктор не инициализирует девайс).

#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "audio/AudioEngine.h"

namespace astro {

// Белый ящик для приватных членов AudioEngine.
struct AudioTestAccess {
  // Приватный enum доступен внутри friend-структуры; наружу отдаём алиас.
  using VoiceType = AudioEngine::VoiceType;
  using Voice = AudioEngine::Voice;
  static void useNullBackend(AudioEngine& e) {
    e.m_tried = true;
    e.m_live = false;
  }
  static uint64_t samples(AudioEngine& e) { return e.m_samples.load(); }
  static size_t voiceCount(AudioEngine& e) {
    std::lock_guard<std::mutex> lock(e.m_mutex);
    return e.m_voices.size();
  }
  static size_t retiredCount(AudioEngine& e) { return e.m_retiredPcm.size(); }
  static uint64_t epoch(AudioEngine& e) { return e.m_renderEpoch.load(); }
  static void pushVoice(AudioEngine& e, AudioTestAccess::VoiceType type,
                        std::shared_ptr<std::vector<float>> pcm,
                        uint64_t startSample) {
    std::lock_guard<std::mutex> lock(e.m_mutex);
    AudioTestAccess::Voice v;
    v.type = type;
    v.startSample = startSample;
    v.pcm = std::move(pcm);
    e.m_voices.push_back(std::move(v));
  }
  static void render(AudioEngine& e, float* out, unsigned frames) {
    e.renderFrames(out, frames);
  }
  static double durationOf(AudioEngine& e, AudioEngine::VoiceType type,
                           size_t pcmSamples) {
    return e.voiceDurationSeconds(type, pcmSamples);
  }
  static size_t durationTableCount(AudioEngine& e) {
    size_t n = 0;
    e.voiceDurationTable(n);
    return n;
  }
};

}  // namespace astro

using astro::AudioEngine;
using astro::AudioTestAccess;

namespace {

int g_fail = 0;

void check(bool ok, const char* what) {
  if (ok) {
    std::cout << "PASS: " << what << "\n";
  } else {
    std::cerr << "FAIL: " << what << "\n";
    ++g_fail;
  }
}

}  // namespace

int main() {
  // 1. Пре-ролл 50 мс: голос не должен исчезать до своего начала.
  {
    AudioEngine eng;
    AudioTestAccess::useNullBackend(eng);
    auto pcm = std::make_shared<std::vector<float>>(44100, 0.25f);  // 1.0 с
    // startSample в будущем — как у реплик Егорки (пре-ролл под PTT-клик).
    const uint64_t start = AudioTestAccess::samples(eng) + 2205;  // 50 мс
    AudioTestAccess::pushVoice(eng, AudioTestAccess::VoiceType::EgorkaVoice, pcm,
                               start);

    // Три кадра по 16 мс — раньше голос удалялся в первом же update().
    for (int i = 0; i < 3; ++i) {
      eng.update(0.016f);
    }
    check(AudioTestAccess::voiceCount(eng) == 1,
          "voice with future startSample survives pre-roll window");

    // Прокручиваем время past 1.0 с длительности + запас 0.1 с.
    for (int i = 0; i < 80; ++i) {
      eng.update(0.016f);
    }
    check(AudioTestAccess::voiceCount(eng) == 0,
          "expired voice is removed after its full duration");
  }

  // 2. Таблица длительностей: покрывает все типы, EgorkaVoice — из PCM.
  {
    AudioEngine eng;
    AudioTestAccess::useNullBackend(eng);
    const size_t count = AudioTestAccess::durationTableCount(eng);
    check(count == static_cast<size_t>(AudioTestAccess::VoiceType::kCount),
          "duration table covers every VoiceType");
    check(std::abs(AudioTestAccess::durationOf(eng, AudioTestAccess::VoiceType::Click, 0) - 0.12) < 1e-12,
          "Click duration = 0.12 s");
    check(std::abs(AudioTestAccess::durationOf(eng, AudioTestAccess::VoiceType::Thunder, 0) - 3.50) < 1e-12,
          "Thunder duration = 3.50 s");
    check(std::abs(AudioTestAccess::durationOf(eng, AudioTestAccess::VoiceType::EgorkaVoice, 44100) - 1.0) < 1e-12,
          "EgorkaVoice duration = pcm size / sample rate");
    check(std::abs(AudioTestAccess::durationOf(eng, AudioTestAccess::VoiceType::File, 22050) - 0.5) < 1e-12,
          "File duration = pcm size / sample rate");
  }

  // 3. Время жизни буфера: после erase буфер не освобождается мгновенно,
  //    освобождается только когда аудиопоток опубликует новый epoch.
  {
    AudioEngine eng;
    AudioTestAccess::useNullBackend(eng);
    auto pcm = std::make_shared<std::vector<float>>(44100, 0.25f);
    std::weak_ptr<std::vector<float>> weak = pcm;
    AudioTestAccess::pushVoice(eng, AudioTestAccess::VoiceType::File, pcm, 0);
    pcm.reset();  // остаётся только владелец — голос в движке

    for (int i = 0; i < 80; ++i) {
      eng.update(0.016f);
    }
    check(AudioTestAccess::voiceCount(eng) == 0, "expired File voice removed");
    check(!weak.expired(),
          "PCM buffer stays alive right after erase (no use-after-free)");
    check(AudioTestAccess::retiredCount(eng) == 1,
          "erased voice PCM is parked in the retire list");

    // Аудиопоток отрабатывает проход и публикует новый epoch.
    float buf[128];
    AudioTestAccess::render(eng, buf, 64);
    check(AudioTestAccess::epoch(eng) >= 1, "render pass publishes an epoch");

    // Основной поток освобождает отложенный буфер.
    eng.update(0.016f);
    check(weak.expired(), "retired PCM buffer released after epoch advanced");
    check(AudioTestAccess::retiredCount(eng) == 0, "retire list drained");
  }

  // 4. Рендер голоса с пре-роллом не даёт мусора (только конечные значения).
  {
    AudioEngine eng;
    AudioTestAccess::useNullBackend(eng);
    auto pcm = std::make_shared<std::vector<float>>(44100, 0.25f);
    AudioTestAccess::pushVoice(eng, AudioTestAccess::VoiceType::EgorkaVoice, pcm,
                               AudioTestAccess::samples(eng) + 2205);
    float buf[256];
    bool finite = true;
    for (int b = 0; b < 8; ++b) {
      AudioTestAccess::render(eng, buf, 128);
      for (int i = 0; i < 256; ++i) {
        if (!std::isfinite(buf[i])) finite = false;
      }
    }
    check(finite, "render output stays finite with a pre-roll voice");
  }

  // 5. Стресс: конкурентный erase во время рендера (буфер не должен умереть
  //    под ногами аудиопотока).
  {
    AudioEngine eng;
    AudioTestAccess::useNullBackend(eng);
    std::atomic<bool> stop{false};
    std::atomic<long> renders{0};
    std::atomic<bool> bad{false};
    std::thread audio([&] {
      float buf[256];
      while (!stop.load(std::memory_order_acquire)) {
        AudioTestAccess::render(eng, buf, 128);
        for (int i = 0; i < 256; ++i) {
          if (!std::isfinite(buf[i])) bad.store(true);
        }
        renders.fetch_add(1, std::memory_order_relaxed);
      }
    });
    for (int i = 0; i < 400; ++i) {
      auto pcm = std::make_shared<std::vector<float>>(4410, 0.1f);
      AudioTestAccess::pushVoice(eng, AudioTestAccess::VoiceType::File, pcm, 0);
      eng.update(0.02f);
    }
    stop.store(true);
    audio.join();
    check(renders.load() > 0, "audio thread rendered while main thread pushed voices");
    check(!bad.load(), "no non-finite samples under concurrent push/erase");
  }

  if (g_fail > 0) {
    std::cerr << g_fail << " check(s) FAILED\n";
    return 1;
  }
  std::cout << "audio voice tests: ALL PASS\n";
  return 0;
}
