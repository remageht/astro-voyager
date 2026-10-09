#include "../src/Journey.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
  // Test 1: round-trip: {"sgra","ori","m31"}
  {
    std::vector<std::string> stations = {"sgra", "ori", "m31"};
    std::string json = astro::journeySerialize(stations);
    std::vector<std::string> out;
    std::string err;
    if (!astro::journeyDeserialize(json, out, err)) {
      std::cerr << "Test 1 failed: deserialize error: " << err << '\n';
      return 1;
    }
    if (out != stations) {
      std::cerr << "Test 1 failed: round-trip mismatch\n";
      return 1;
    }
  }

  // Test 2: empty vector
  {
    std::vector<std::string> stations = {};
    std::string json = astro::journeySerialize(stations);
    std::vector<std::string> out;
    std::string err;
    if (!astro::journeyDeserialize(json, out, err)) {
      std::cerr << "Test 2 failed: deserialize error: " << err << '\n';
      return 1;
    }
    if (!out.empty()) {
      std::cerr << "Test 2 failed: expected empty vector\n";
      return 1;
    }
  }

  // Test 3: empty text -> false + non-empty error
  {
    std::vector<std::string> out;
    std::string err;
    if (astro::journeyDeserialize("", out, err)) {
      std::cerr << "Test 3 failed: expected failure on empty text\n";
      return 1;
    }
    if (err.empty()) {
      std::cerr << "Test 3 failed: expected non-empty error message\n";
      return 1;
    }
  }

  // Test 4: text without version -> false
  {
    std::vector<std::string> out;
    std::string err;
    if (astro::journeyDeserialize("{\"stations\":[\"sgra\"]}", out, err)) {
      std::cerr << "Test 4 failed: expected failure when version is missing\n";
      return 1;
    }
  }

  // Test 5: version not 1 (e.g. 2) -> false
  {
    std::vector<std::string> out;
    std::string err;
    if (astro::journeyDeserialize("{\"version\":2,\"stations\":[\"sgra\"]}", out, err)) {
      std::cerr << "Test 5 failed: expected failure when version is not 1\n";
      return 1;
    }
  }

  // Test 6: malformed JSON (unclosed bracket) -> false
  {
    std::vector<std::string> out;
    std::string err;
    if (astro::journeyDeserialize("{\"version\":1,\"stations\":[\"sgra\"", out, err)) {
      std::cerr << "Test 6 failed: expected failure on malformed JSON\n";
      return 1;
    }
  }

  // Test 7: token with illegal character (e.g. "or i") -> false
  {
    std::vector<std::string> out;
    std::string err;
    if (astro::journeyDeserialize("{\"version\":1,\"stations\":[\"or i\"]}", out, err)) {
      std::cerr << "Test 7 failed: expected failure on illegal token character\n";
      return 1;
    }
  }

  // Test 8: journeySanitizeFileName
  {
    if (astro::journeySanitizeFileName("my route!!") != "my_route__") {
      std::cerr << "Test 8 failed: sanitize 'my route!!' got "
                << astro::journeySanitizeFileName("my route!!") << '\n';
      return 1;
    }
    if (astro::journeySanitizeFileName("") != "journey") {
      std::cerr << "Test 8 failed: sanitize empty got "
                << astro::journeySanitizeFileName("") << '\n';
      return 1;
    }
    std::string longName(60, 'x');
    std::string sanitizedLong = astro::journeySanitizeFileName(longName);
    if (sanitizedLong.size() != 48 || sanitizedLong != std::string(48, 'x')) {
      std::cerr << "Test 8 failed: sanitize 60 chars got length "
                << sanitizedLong.size() << '\n';
      return 1;
    }
  }

  std::cout << "journey tests: ALL PASS\n";
  return 0;
}
