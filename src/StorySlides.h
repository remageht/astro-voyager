#pragma once

#include <string>
#include <vector>

struct ImDrawList;
struct ImVec2;  // global forward decl (defined in imgui.h); do NOT redeclare
                // inside namespace astro or it becomes a distinct type

// StorySlides (v3.5.0): declarative data + procedural art for the expedition
// intro cutscene ("Слайды + арт"): each slide is a story text plus an id of
// a small vector-scene drawn with ImDrawList primitives (house at night,
// toy rocket, dimming sun, launch pad, Earth departure...). Skippable
// (Esc) page-by-page (Space/Enter/click), language picked by the HUD
// (uiRu); pixel-independent layout.
struct StorySlide {
  std::string textRu;
  std::string textEn;
  int art = 0;  // procedural scene id in StorySlides.cpp
  // v4.0: optional photographic illustration (res/-relative path, e.g.
  // "res/story/paris_peace.jpg"). Non-empty wins over the vector art;
  // the drawing side lazy-loads it into a GL texture (stb_image).
  std::string image;
  // v4.0: катсцена-слайд: каталог с frame_%03d.jpg (fps=12) + audio.wav.
  // Непустой перекрывает и арт, и фото; слайд играет сам и листается по
  // окончании кадров (Space/Esc работают как раньше).
  std::string videoDir;
};

namespace astro {

// The fixed intro campaign: a girl dreams of space, the Sun starts dying,
// humanity chooses to find a new planet, the expedition sets off (starts
// at the Solar System station, Earth first).
const std::vector<StorySlide>& storyIntroSlides();

// Shared finish-of-chapter slides; art ids reuse the same table as intro.
const std::vector<StorySlide>& storyChapterSlides();

// v4.0 "Новая игра / New Game" prologue: Egorka's own story (dream
// revelation, dying planet, warp drive on antimatter, pilot's path) with
// two photographic slides (res/story/*.jpg); hands off to the expedition
// at Zemlya ("Дом").
const std::vector<StorySlide>& storyPrologueSlides();

// Procedural vector art for slide id (dark glass drawing list style).
void drawStoryArt(ImDrawList* dl, int art, const ImVec2& pos, float size);

// Returns slide index in storyChapterSlides() corresponding to the given finale choice.
int slideIndexForFinale(int choice);

}  // namespace astro
