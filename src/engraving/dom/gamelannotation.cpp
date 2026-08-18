/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#include "gamelannotation.h"

namespace mu::engraving {

std::optional<int> slendroDegreeForPitch(int pitch)
{
    switch ((pitch % 12 + 12) % 12) {
    case 0: return 1;
    case 2: return 2;
    case 4: return 3;
    case 7: return 5;
    case 9: return 6;
    default: return std::nullopt;
    }
}

std::optional<int> pitchForSlendroDegree(int degree)
{
    switch (degree) {
    case 1: return 60;
    case 2: return 62;
    case 3: return 64;
    case 5: return 67;
    case 6: return 69;
    default: return std::nullopt;
    }
}

std::optional<int> diatonicStepForSlendroDegree(int degree)
{
    switch (degree) {
    case 1: return 28;
    case 2: return 29;
    case 3: return 30;
    case 5: return 32;
    case 6: return 33;
    default: return std::nullopt;
    }
}

muse::String slendroDegreeText(int pitch)
{
    const std::optional<int> degree = slendroDegreeForPitch(pitch);
    return degree ? muse::String::number(*degree) : muse::String(u"?");
}

}
