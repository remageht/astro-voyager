#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Camera.h"
#include "Catalog.h"
#include "Cubemap.h"
#include "Mesh.h"
#include "PostFX.h"
#include "Shader.h"
#include "SolarSystem.h"

namespace astro {

struct RenderSettings {
  float rs = 1.0f;
  float shellRadius = 30.0f;
  float stepSize = 0.05f;
  int maxSteps = 600;
  float fovDegrees = 60.0f;
  bool diskOn = true;  // accretion disk visible by default (harness overrides)
};

// User-spawned object (flexible object editor, v3.2.0).
// v3.2.0 adds a surface palette: type/detail select the planet-shader look
// (rock / rocky planet / moon / gas giant / star), colorB is the secondary
// palette colour and ringOuter > 0 draws a scaled ring around the body.
struct CustomBody {
  std::string id;
  std::string name;
  Vec3 pos;
  Vec3 vel;
  float radius = 0.3f;
  Vec3 color{0.62, 0.58, 0.52};
  Vec3 colorB{0.37, 0.35, 0.31};  // secondary palette colour (bands/maria)
  int type = 2;                   // u_Type: 0 star, 1 gas, 2 rocky, 3 ice
  int detail = 0;                 // u_Body: 0 rock, 2 rocky, 5 moon, 7 gas, 1 star
  float ringOuter = 0.0f;         // > 0 draws a ring (world units, outer edge)
  float seed = 1.0f;
};

enum class PerfPreset {
  Low = 0,
  Med = 1,
  Ultra = 2,
  Custom = 3
};

inline RenderSettings getPresetSettings(PerfPreset preset) {
  RenderSettings s;
  switch (preset) {
    case PerfPreset::Low:
      s.rs = 1.0f;
      s.shellRadius = 20.0f;
      s.stepSize = 0.08f;
      s.maxSteps = 300;
      s.fovDegrees = 60.0f;
      break;
    case PerfPreset::Med:
    default:
      s.rs = 1.0f;
      s.shellRadius = 30.0f;
      s.stepSize = 0.05f;
      s.maxSteps = 600;
      s.fovDegrees = 60.0f;
      break;
    case PerfPreset::Ultra:
      s.rs = 1.0f;
      s.shellRadius = 45.0f;
      s.stepSize = 0.025f;
      s.maxSteps = 1200;
      s.fovDegrees = 60.0f;
      break;
  }
  return s;
}

namespace {
// Adjustable time-pace presets (sim-seconds per wall-second).
// Keys -/= step through this list; the log slider spans 1..1000.
constexpr float kTimePacePresets[] = {1.0f, 10.0f, 60.0f, 300.0f, 1000.0f};
constexpr int kTimePacePresetCount = 5;
}  // namespace

class SceneManager {
 public:
  SceneManager();
  ~SceneManager() = default;

  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;

  void teleportTo(const std::string& stationId, Camera& camera);
  void render(const Camera& camera, int fbWidth, int fbHeight, float time);

  const Station* getCurrentStation() const { return m_currentStation; }
  const std::vector<BodyState>& getBodies() const { return m_lastBodies; }

  // M31 named bright stars for close-range labels / fly-to (m31 only).
  // v3.1.0: reused as hero-star records for every galaxy (mag custom).
  struct M31BrightStar {
    glm::vec3 pos{0.0f};
    std::string name;
    std::string fact;
    float mag = 0.0f;
  };
  const std::vector<M31BrightStar>& m31BrightStars() const { return m_m31Bright; }
  // v3.1.0: hero stars for the current galaxy station (fly-to + labels);
  // empty unless a galaxy station with heroes is active.
  const std::vector<M31BrightStar>& galaxyBrightStars() const;
  // v3.1.0 Sun-link: world position of the Sun inside the Milky Way
  // station; true only there. AppGL flies the camera to it by hand and
  // auto-jumps to "sol" on proximity.
  bool sunWorldPos(glm::vec3& out) const;
  RenderSettings& getSettings() { return m_settings; }
  const RenderSettings& getSettings() const { return m_settings; }

  PerfPreset getPreset() const { return m_preset; }
  void applyPreset(PerfPreset preset);
  void setPreset(PerfPreset preset) { m_preset = preset; }

  void setExposure(float e) { m_exposure = e; }
  float getExposure() const { return m_exposure; }
  void setShowOrbits(bool on) { m_showOrbits = on; }
  bool showOrbits() const { return m_showOrbits; }
  void setShowBelt(bool on) { m_showBelt = on; }
  bool showBelt() const { return m_showBelt; }

  void togglePause() { m_paused = !m_paused; }
  bool isPaused() const { return m_paused; }
  void setTimeScale(float s) {
    m_timeScale = (s < 0.0f) ? 0.0f : (s > 1000.0f ? 1000.0f : s);
  }
  float getTimeScale() const { return m_timeScale; }

  // Object editor (custom bodies, sol station only).
  void spawnRockAt(const Vec3& p, const Vec3& v);
  // v3.2.0 editor: spawn from a full template (id/name auto-numbered by
  // palette prefix), live mutable access for the selected object.
  void spawnCustom(const CustomBody& templ, const Vec3& p, const Vec3& v);
  CustomBody* findCustom(const std::string& id);
  void deleteSelected();
  void clearCustom();
  void setCollisions(bool on) { m_collisions = on; }
  bool collisionsOn() const { return m_collisions; }
  void updateCustom(float dt);
  void setSelected(const std::string& id) { m_selectedId = id; }
  const std::string& getSelected() const { return m_selectedId; }
  const std::vector<CustomBody>& getCustoms() const { return m_custom; }

 private:
  void initConstellationOrion();
  void initConstellationUma();
  // S-star swarm (sgra only): deterministic Kepler-ish orbit params,
  // advected on CPU by render time, drawn as a separate additive pass.
  struct SStarParam {
    float r = 1.0f;
    float phase = 0.0f;
    float omega = 0.0f;
    glm::vec3 u{1.0f, 0.0f, 0.0f};
    glm::vec3 v{0.0f, 0.0f, 1.0f};
    float mag = 1.5f;
  };
  void initSStars();
  std::vector<SStarParam> m_sStarParams;
  bool m_sStarsInit = false;
  void initM31Disk();
  // v3.1.0: shared deterministic disk generator (initM31Disk calls it with
  // the legacy constants, so m31 output is bit-identical); cluster ball.
  static void genDiskPoints(uint32_t seed, int count, float rMin, float rMax,
                            float thick, float armPull,
                            const std::vector<M31BrightStar>& heroes,
                            std::vector<glm::vec3>& pos,
                            std::vector<float>& mags);
  void initGalaxyDisks();
  void initCluster();

  // Advances m_simTime from wall-clock time (pause/timeScale aware,
  // dt clamped to 0.5s). Returns the clamped dt. The harness passes a
  // constant time so simTime stays 0 and frames stay deterministic.
  float tickSimTime(float time);

  const Station* m_currentStation = nullptr;
  RenderSettings m_settings;
  PerfPreset m_preset = PerfPreset::Med;
  float m_exposure = 1.0f;
  bool m_showOrbits = true;
  bool m_showBelt = true;

  // Simulation clock for animated stations (solar system orbits).
  // Advanced from wall time inside render(); harness passes constant
  // time so screenshots stay deterministic.
  float m_simTime = 0.0f;
  float m_lastTime = -1.0f;
  float m_timeScale = 1.0f;
  bool m_paused = false;

  // === Relativity === скорость наблюдателя (дельта позиции камеры за кадр,
  // сглаженная). Харнесс передаёт константный time => dt == 0 => beta == 0
  // => шейдерные ветки выключены, PNG байт-в-байт.
  Vec3 m_relVel{0.0, 0.0, 0.0};
  Vec3 m_relPos{0.0, 0.0, 0.0};
  Vec3 m_relDir{0.0, 0.0, 1.0};
  float m_relTime = -1.0f;
  float m_relBeta = 0.0f;

  std::unique_ptr<ScreenQuad> m_screenQuad;
  // === PostFX === scene FBO + композит (см. PostFX.h).
  PostFX m_postFx;
  std::unique_ptr<Shader> m_bhShader;
  std::unique_ptr<Shader> m_quasarShader;
  std::unique_ptr<Shader> m_linesShader;
  std::unique_ptr<Shader> m_starShader;
  std::unique_ptr<Shader> m_m31StarShader;
  std::unique_ptr<Shader> m_galaxyShader;
  std::unique_ptr<Shader> m_nebulaShader;
  std::unique_ptr<Shader> m_pulsarShader;
  std::unique_ptr<Shader> m_planetShader;
  std::unique_ptr<Shader> m_ringShader;
  std::unique_ptr<Shader> m_cometShader;
  std::unique_ptr<Shader> m_skyShader;
  std::unique_ptr<Cubemap> m_skyboxDeep;
  std::unique_ptr<Cubemap> m_skyboxLocal;

  LineMesh m_orionMesh;
  LineMesh m_umaMesh;
  StarPoints m_orionStars;
  StarPoints m_umaStars;
  StarPoints m_bgStars;
  StarPoints m_sStars;
  // M31 volumetric disk starfield (m31 NEAR LOD only, alpha = near).
  StarPoints m_m31Stars;
  std::vector<M31BrightStar> m_m31Bright;
  // v3.1.0 Deep Field: approach LOD disks for the other galaxies
  // (same StarPoints infra, per-galaxy seed) + hero stars for fly-to.
  StarPoints m_m51Stars;
  StarPoints m_m104Stars;
  StarPoints m_mwStars;
  std::vector<M31BrightStar> m_m51Bright;
  std::vector<M31BrightStar> m_m104Bright;
  std::vector<M31BrightStar> m_mwBright;
  // v3.1.0: globular cluster ball (m13), deterministic Plummer sphere.
  StarPoints m_clusterStars;

  SolarSystem m_sol;
  SphereMesh m_sphere;
  RingMesh m_saturnRing;
  BeltPoints m_belt;
  LineMesh m_orbits;
  std::vector<BodyState> m_lastBodies;

  // Object editor state (empty at spawn, only UI spawns).
  std::vector<CustomBody> m_custom;
  bool m_collisions = true;
  int m_customSeq = 0;
  std::string m_selectedId;
};

}  // namespace astro
