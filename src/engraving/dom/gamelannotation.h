/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include <optional>

#include "global/types/string.h"

namespace mu::engraving {

std::optional<int> slendroDegreeForPitch(int pitch);
std::optional<int> pitchForSlendroDegree(int degree);
std::optional<int> diatonicStepForSlendroDegree(int degree);
muse::String slendroDegreeText(int pitch);

}
