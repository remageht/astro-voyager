#include <iostream>
#include <string>
#include "Catalog.h"
#include "Camera.h"
#include "Geodesic.h"
#include "Version.h"

#ifdef ASTROVOYAGER_ENABLE_GL
#include "AppGL.h"
#endif

namespace {
void printUsage() {
  std::cout << "Usage: astro-voyager [--list | --info <id> | "
               "--demo-geodesic | --version";
#ifdef ASTROVOYAGER_ENABLE_GL
  std::cout << " | --gl | --screenshot-all";
#endif
  std::cout << "]\n";
}
}  // namespace

int main(int argc, char** argv) {
  const std::string arg = argc > 1 ? argv[1] : "";
  if (arg == "--version") {
    std::cout << astro::kAppName << " " << astro::kVersion << "\n";
    return 0;
  }
  if (arg == "--list") {
    for (const auto& s : astro::builtinCatalog()) {
      std::cout << s.id << " [" << astro::typeToString(s.type) << "] "
                << s.name << "\n";
    }
    return 0;
  }
  if (arg == "--info" && argc > 2) {
    const astro::Station* st = astro::findStation(argv[2]);
    if (!st) {
      std::cerr << "unknown station: " << argv[2] << "\n";
      return 1;
    }
    std::cout << st->id << " [" << astro::typeToString(st->type) << "] "
              << st->name << "\n" << st->description << "\nspawn Rs: "
              << st->spawnDistanceRs << "\n";
    return 0;
  }
  if (arg == "--demo-geodesic") {
    // Radial in-fall must be captured; tangential far ray must escape.
    astro::SchwState inward;
    inward.r = 10.0;
    inward.theta = 1.5707;
    inward.phi = 0.0;
    inward.dt = 1.0;
    inward.dr = -1.0;
    bool cap = false;
    const int n1 =
        astro::traceRay(inward, 1.0, 30.0, 0.02, 2000, cap);
    std::cout << "inward: steps=" << n1 << " captured=" << cap << "\n";

    astro::SchwState tangent;
    tangent.r = 10.0;
    tangent.theta = 1.5707;
    tangent.phi = 0.0;
    tangent.dt = 1.0;
    tangent.dphi = 0.09;
    bool cap2 = false;
    const int n2 =
        astro::traceRay(tangent, 1.0, 30.0, 0.02, 2000, cap2);
    std::cout << "tangent: steps=" << n2 << " captured=" << cap2 << "\n";

    astro::Camera cam;
    cam.clampNearHorizon(1.0);
    std::cout << "camera z=" << cam.position.z << "\n";
    return (cap && !cap2) ? 0 : 2;
  }

#ifdef ASTROVOYAGER_ENABLE_GL
  if (arg == "--screenshot-all") {
    return astro::AppGL::captureAllScreenshots("docs/screens");
  }
  if (arg == "--gl" || arg.empty()) {
    try {
      astro::AppGL app;
      return app.run();
    } catch (const std::exception& e) {
      std::cerr << "GL Error: " << e.what() << "\n";
      return 1;
    }
  }
#endif

  printUsage();
  return arg.empty() ? 0 : 1;
}
