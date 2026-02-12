#pragma once

#include <string>
#include "Result.h"

namespace sdrtrunk {

/**
 * Get MP3 file duration using libmpg123
 *
 * This uses the battle-tested libmpg123 library which provides:
 * - Accurate frame counting and duration calculation
 * - Proper handling of VBR/CBR files
 * - Support for gapless playback metadata (LAME/Xing tags)
 * - Robust error handling for corrupted files
 *
 * @param filepath Path to the MP3 file
 * @return Duration in seconds, or error if parsing fails
 */
Result<double> getMP3Duration(const std::string& filepath);

} // namespace sdrtrunk