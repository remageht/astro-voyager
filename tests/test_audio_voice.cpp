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
  static void setActiveCue(AudioEngine& e, int cue) {
    e.m_activeEgorkaCue.store(cue);
  }
  // These callback-owned fields are inspected only when no render is running.
  static double menuBlend(const AudioEngine& e) { return e.m_menuBlend; }
  static void setSamples(AudioEngine& e, uint64_t sample) {
    e.m_samples.store(sample);
  }
  static void enableGameplayLayers(AudioEngine& e) {
    e.m_warpCurrent.store(0.8f);
    e.m_engineCurrent.store(0.7f);
    e.m_tensionCurrent.store(0.6f);
    e.m_atmoCurrent.store(0.5f);
    e.m_radioStaticCurrent.store(0.4f);
    e.setCassetteTape(static_cast<int>(AudioEngine::CassetteTape::PulsarBeat));
    e.setCassetteVolume(0.4f);
    e.m_cassetteGainCurrent.store(1.0f);
    e.m_drillCurrent.store(0.8f);
    e.m_weatherType.store(1);
    e.m_weatherIntensityCurrent.store(0.7f);
    e.m_weatherWindCurrent.store(0.015f);
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

std::vector<float> renderAudio(AudioEngine& eng, unsigned frames) {
  std::vector<float> out(frames * AudioEngine::kChannels);
  AudioTestAccess::render(eng, out.data(), frames);
  return out;
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

  // 6. Отслеживание статуса воспроизведения реплик экипажа и мгновенный stop.
  {
    AudioEngine eng;
    AudioTestAccess::useNullBackend(eng);
    check(!eng.isEgorkaVoicePlaying(), "initially no voice cue is playing");
    check(eng.currentEgorkaVoiceCue() < 0, "initially currentEgorkaVoiceCue is -1");

    auto pcm = std::make_shared<std::vector<float>>(44100, 0.2f);
    AudioTestAccess::pushVoice(eng, AudioTestAccess::VoiceType::EgorkaVoice, pcm,
                               AudioTestAccess::samples(eng));
    AudioTestAccess::setActiveCue(eng, static_cast<int>(AudioEngine::EgorkaVoiceCue::SysOnline));

    check(eng.isEgorkaVoicePlaying(), "isEgorkaVoicePlaying returns true when voice is active");
    check(eng.currentEgorkaVoiceCue() == static_cast<int>(AudioEngine::EgorkaVoiceCue::SysOnline),
          "currentEgorkaVoiceCue returns active cue ID");

    eng.stopCurrentVoiceCue();
    check(!eng.isEgorkaVoicePlaying(), "isEgorkaVoicePlaying is false after stopCurrentVoiceCue()");
    check(eng.currentEgorkaVoiceCue() < 0, "currentEgorkaVoiceCue is -1 after stopCurrentVoiceCue()");
    check(AudioTestAccess::voiceCount(eng) == 0, "all EgorkaVoice instances erased by stop");
  }

  // 7. Menu fades are callback-owned, sample-wise, and reversible mid-fade.
  {
    AudioEngine eng;
    eng.setVolume(1.0f);
    bool smooth = true;
    float last[2] = {};
    auto renderSmooth = [&](unsigned frames) {
      const auto out = renderAudio(eng, frames);
      for (unsigned i = 0; i < frames; ++i) {
        for (int ch = 0; ch < 2; ++ch) {
          const float sample = out[i * 2 + ch];
          smooth = smooth && std::isfinite(sample) &&
                   std::abs(sample - last[ch]) < 0.01f;
          last[ch] = sample;
        }
      }
    };
    renderSmooth(2048);
    check(AudioTestAccess::menuBlend(eng) == 0.0,
          "menu ambient is inactive by default");
    eng.setMenuActive(true);
    check(!eng.isInitialized() && AudioTestAccess::menuBlend(eng) == 0.0,
          "setMenuActive is safe before init and only changes the target");
    renderSmooth(8820);  // 0.2 s, one quarter of the 0.8 s ramp
    check(std::abs(AudioTestAccess::menuBlend(eng) - 0.25) < 1e-9,
          "menu fade advances per sample over 0.8 seconds");
    eng.setMenuActive(true);  // repeated audioUpdate calls must not restart it
    renderSmooth(8820);
    check(std::abs(AudioTestAccess::menuBlend(eng) - 0.5) < 1e-9,
          "repeated menu target does not reset the fade");
    eng.setMenuActive(false);
    check(std::abs(AudioTestAccess::menuBlend(eng) - 0.5) < 1e-9,
          "leaving menu does not abruptly switch the blend");
    renderSmooth(4410);
    check(std::abs(AudioTestAccess::menuBlend(eng) - 0.375) < 1e-9,
          "mid-fade reversal continues from the current blend");
    eng.setMenuActive(true);
    renderSmooth(22051);
    check(AudioTestAccess::menuBlend(eng) == 1.0,
          "menu fade reaches full ambient");
    eng.setMenuActive(false);
    renderSmooth(17640);
    check(std::abs(AudioTestAccess::menuBlend(eng) - 0.5) < 1e-9,
          "menu fade-out has the same 0.8 second duration");
    renderSmooth(17641);
    check(AudioTestAccess::menuBlend(eng) == 0.0,
          "menu fade-out fully restores gameplay");
    check(smooth, "fades and reversals have no abrupt sample or block jumps");
  }

  // 8. Settled menu audio is quiet, finite, nonzero, and genuinely stereo.
  {
    AudioEngine eng;
    eng.setVolume(1.0f);
    eng.setMenuActive(true);
    renderAudio(eng, AudioEngine::kSampleRate);
    const auto out = renderAudio(eng, AudioEngine::kSampleRate);
    bool bounded = true;
    double energyL = 0.0, energyR = 0.0, stereoEnergy = 0.0;
    for (size_t i = 0; i < out.size(); i += 2) {
      bounded = bounded && std::isfinite(out[i]) && std::isfinite(out[i + 1]) &&
                std::abs(out[i]) < 0.03f && std::abs(out[i + 1]) < 0.03f;
      energyL += out[i] * out[i];
      energyR += out[i + 1] * out[i + 1];
      const double side = out[i] - out[i + 1];
      stereoEnergy += side * side;
    }
    check(bounded, "menu ambient is finite and bounded below 0.03 at full volume");
    check(energyL > 0.1 && energyR > 0.1 && stereoEnergy > 0.01,
          "menu ambient has nonzero left/right energy and stereo separation");
    check(eng.cassetteTape() == 0, "menu ambient does not enable a cassette");

    // Neither the 32 s music loop nor callback block boundaries reset its phase.
    AudioTestAccess::setSamples(eng, 32ULL * AudioEngine::kSampleRate - 128);
    const auto boundary = renderAudio(eng, 256);
    bool continuous = true;
    for (size_t i = 2; i < boundary.size(); ++i) {
      continuous = continuous && std::abs(boundary[i] - boundary[i - 2]) < 0.004f;
    }
    check(continuous, "menu oscillator phase is continuous across the music loop");
  }

  // 9. Fade and oscillator results must not depend on callback block sizes.
  {
    AudioEngine whole, split;
    const unsigned chunks[] = {1, 17, 127, 512};
    bool identical = true;
    for (bool active : {true, false, true}) {
      whole.setMenuActive(active);
      split.setMenuActive(active);
      const auto expected = renderAudio(whole, AudioEngine::kSampleRate);
      std::vector<float> actual(expected.size());
      unsigned done = 0, chunk = 0;
      while (done < AudioEngine::kSampleRate) {
        unsigned frames = chunks[chunk++ % 4];
        const unsigned remaining = AudioEngine::kSampleRate - done;
        if (frames > remaining) frames = remaining;
        AudioTestAccess::render(split, actual.data() + done * 2, frames);
        done += frames;
      }
      identical = identical && expected == actual &&
                  AudioTestAccess::menuBlend(whole) == AudioTestAccess::menuBlend(split);
    }
    check(identical, "menu entry/exit/re-entry are independent of callback block size");
    check(AudioTestAccess::samples(whole) == 3ULL * AudioEngine::kSampleRate &&
              AudioTestAccess::samples(split) == AudioTestAccess::samples(whole),
          "menu transitions never reset the absolute sample clock");
  }

  // 10. Preserve existing master gain and soft clipping, including exact silence.
  {
    AudioEngine full, half, zero, muted;
    full.setVolume(1.0f);
    half.setVolume(0.5f);
    zero.setVolume(0.0f);
    muted.setVolume(1.0f);
    muted.toggleMute();
    bool scaled = true, silent = true;
    double energy = 0.0;
    for (bool active : {true, true, false}) {
      for (AudioEngine* eng : {&full, &half, &zero, &muted}) eng->setMenuActive(active);
      const auto a = renderAudio(full, AudioEngine::kSampleRate);
      const auto b = renderAudio(half, AudioEngine::kSampleRate);
      const auto c = renderAudio(zero, AudioEngine::kSampleRate);
      const auto d = renderAudio(muted, AudioEngine::kSampleRate);
      for (size_t i = 0; i < a.size(); ++i) {
        // Master gain precedes the existing tanh limiter, so undo it to compare.
        const double expected = std::tanh(std::atanh(static_cast<double>(a[i])) * 0.5);
        scaled = scaled && std::isfinite(a[i]) && std::abs(b[i] - expected) < 1e-6;
        silent = silent && c[i] == 0.0f && d[i] == 0.0f;
        energy += a[i] * a[i];
      }
    }
    check(energy > 0.1 && scaled, "master volume scales menu and fading gameplay gain");
    check(silent, "mute and zero master volume give exact silence during all fades");
    muted.toggleMute();
    zero.setVolume(1.0f);
    const auto resumed = renderAudio(full, 256);
    check(renderAudio(muted, 256) == resumed && renderAudio(zero, 256) == resumed,
          "silent rendering keeps phase continuous when master audio resumes");
  }

  // 11. Crossfade every continuous layer, without altering cassette controls/state.
  {
    AudioEngine ambient, busy, gameplay;
    for (AudioEngine* eng : {&ambient, &busy, &gameplay}) eng->setVolume(1.0f);
    AudioTestAccess::enableGameplayLayers(busy);
    AudioTestAccess::enableGameplayLayers(gameplay);
    ambient.setMenuActive(true);
    busy.setMenuActive(true);
    for (AudioEngine* eng : {&ambient, &busy, &gameplay}) renderAudio(*eng, AudioEngine::kSampleRate);
    const auto menu = renderAudio(ambient, 512);
    check(renderAudio(busy, 512) == menu,
          "full menu blend suppresses music, warp, engine, tension, atmosphere, radio, cassette, drill, weather");
    renderAudio(gameplay, 512);
    check(busy.cassetteTape() == static_cast<int>(AudioEngine::CassetteTape::PulsarBeat) &&
              busy.cassetteVolume() == 0.4f,
          "entering menu preserves cassette selection and volume");
    busy.setMenuActive(false);
    renderAudio(busy, AudioEngine::kSampleRate);
    renderAudio(gameplay, AudioEngine::kSampleRate);
    check(renderAudio(busy, 512) == renderAudio(gameplay, 512),
          "leaving menu restores unchanged gameplay and continuously running cassette state");
    check(busy.cassetteTape() == static_cast<int>(AudioEngine::CassetteTape::PulsarBeat) &&
              busy.cassetteVolume() == 0.4f,
          "leaving menu preserves cassette selection and volume");
  }

  // 12. UI clicks and PCM voice cues remain outside the menu/gameplay crossfade.
  {
    AudioEngine ambient, click, voice;
    for (AudioEngine* eng : {&ambient, &click, &voice}) {
      eng->setVolume(1.0f);
      eng->setMenuActive(true);
      renderAudio(*eng, AudioEngine::kSampleRate);
    }
    AudioTestAccess::useNullBackend(click);
    click.playClick();
    auto pcm = std::make_shared<std::vector<float>>(4096, 0.2f);
    AudioTestAccess::pushVoice(voice, AudioTestAccess::VoiceType::EgorkaVoice, pcm,
                               AudioTestAccess::samples(voice));
    const auto base = renderAudio(ambient, 256);
    const auto ui = renderAudio(click, 256);
    const auto speech = renderAudio(voice, 256);
    constexpr double pi = 3.14159265358979323846;
    bool clickIntact = true, voiceIntact = true;
    for (size_t i = 0; i < base.size(); ++i) {
      const double age = static_cast<double>(i / 2) / AudioEngine::kSampleRate;
      const double expectedClick = std::sin(2.0 * pi * 1900.0 * age) *
                                   std::exp(-age * 90.0) * 0.7 * std::sqrt(0.5) * 1.1;
      clickIntact = clickIntact &&
          std::abs(std::atanh(ui[i]) - std::atanh(base[i]) - expectedClick) < 2e-6;
      voiceIntact = voiceIntact &&
          std::abs(std::atanh(speech[i]) - std::atanh(base[i]) -
                   0.2 * std::sqrt(0.5) * 1.1) < 2e-6;
    }
    check(clickIntact, "UI click SFX retain full gain in menu");
    check(voiceIntact, "PCM voice cues retain full gain in menu");
  }

  if (g_fail > 0) {
    std::cerr << g_fail << " check(s) FAILED\n";
    return 1;
  }
  std::cout << "audio voice tests: ALL PASS\n";
  return 0;
}
