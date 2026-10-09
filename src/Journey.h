#pragma once

#include <string>
#include <vector>

namespace astro {

std::string journeySerialize(const std::vector<std::string>& stations);

bool journeyDeserialize(const std::string& text,
                        std::vector<std::string>& outStations,
                        std::string& outError);

std::string journeySanitizeFileName(const std::string& name);

}  // namespace astro
