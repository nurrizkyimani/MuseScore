/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include <algorithm>
#include <cstdlib>

#include "draw/fontmetrics.h"
#include "draw/types/font.h"
#include "draw/types/geometry.h"
#include "engraving/types/fraction.h"
#include "global/types/string.h"

namespace mu::engraving::rendering::score {

inline double numericPulseRhythmMarkThickness(double spatium)
{
    return 0.18 * spatium;
}

inline double numericPulseRhythmMarkDistance(double spatium)
{
    return 0.40 * spatium;
}

inline double numericPulseRhythmMarkGap(double spatium)
{
    return 0.45 * spatium;
}

inline double numericPulseStemLength(double spatium, int durationMarks)
{
    const int additionalMarks = std::max(0, durationMarks - 1);
    return (2.35 + 0.16 * additionalMarks) * spatium;
}

inline double numericPulseGroupOverhang(const muse::RectF& textBbox, double spatium)
{
    // Beam anchors still originate from the Western notehead attachment point,
    // which is slightly right-biased for text glyphs such as "5".  Keep enough
    // overhang for the rhythm mark to visibly pass both edges of every digit.
    return textBbox.width() * 0.5 + 0.40 * spatium;
}

inline muse::draw::Font numericPulseFont(double scale)
{
    muse::draw::Font font(u"Edwin", muse::draw::Font::Type::Text);
    font.setPointSizeF(12.0 * scale);
    return font;
}

inline muse::RectF numericPulseTextBbox(const muse::String& text, double scale)
{
    const muse::draw::Font font = numericPulseFont(scale);
    muse::RectF bbox = muse::draw::FontMetrics(font).boundingRect(text);
    bbox.translate(-bbox.center().x(), 0.0);
    return bbox;
}

inline double numericPulseTextOriginX(const muse::String& text, double scale)
{
    return -muse::draw::FontMetrics(numericPulseFont(scale)).horizontalAdvance(text) * 0.5;
}

inline muse::RectF numericPulseEmptyPulseBbox(double spatium)
{
    // Do not use a text period here. Its glyph sits on the numeric-pulse staff
    // baseline and can disappear into the staff line. Keep the pulse dot fully
    // above that line and large enough to remain visible at normal zoom levels.
    const double diameter = 0.50 * spatium;
    const double centerY = -0.38 * spatium;
    return muse::RectF(-diameter * 0.5, centerY - diameter * 0.5, diameter, diameter);
}

inline int numericPulseMeasurePulseCount(const Fraction& measureDuration, const Fraction& timeSignature)
{
    if (!measureDuration.isValid() || !timeSignature.isValid() || timeSignature.denominator() <= 0) {
        return 1;
    }

    // The meter denominator defines the visible pulse unit: quarter-note pulses
    // in 4/4 and eighth-note pulses in 6/8. Irregular/pickup measures naturally
    // show only the pulses contained in their actual duration.
    const Fraction pulseCount = (measureDuration / Fraction(1, timeSignature.denominator())).reduced();
    return pulseCount.denominator() == 1 ? std::max(1, pulseCount.numerator()) : 1;
}

inline muse::RectF numericPulseEmptyPulseBbox(double spatium, int pulseIndex, int pulseCount, double availableWidth)
{
    muse::RectF bbox = numericPulseEmptyPulseBbox(spatium);
    if (pulseCount <= 1 || availableWidth <= 0.0) {
        return bbox;
    }

    const double paddedWidth = std::max(0.0, availableWidth - 2.0 * spatium);
    const double pulseSpacing = paddedWidth / pulseCount;
    const double centerX = (pulseIndex + 0.5) * pulseSpacing - paddedWidth * 0.5;
    bbox.translate(centerX, 0.0);
    return bbox;
}

inline muse::RectF numericPulseEmptyPulseGroupBbox(double spatium, int pulseCount, double availableWidth)
{
    muse::RectF bbox = numericPulseEmptyPulseBbox(spatium, 0, pulseCount, availableWidth);
    for (int pulseIndex = 1; pulseIndex < pulseCount; ++pulseIndex) {
        bbox = bbox.united(numericPulseEmptyPulseBbox(spatium, pulseIndex, pulseCount, availableWidth));
    }
    return bbox;
}

inline double numericPulseRegisterDotDiameter(double spatium)
{
    return 0.60 * spatium;
}

inline double numericPulseRegisterDotGap(double spatium)
{
    return 0.16 * spatium;
}

inline muse::RectF numericPulseRegisterDotBbox(const muse::RectF& textBbox, double spatium,
                                               int registerOffset, int index)
{
    const double diameter = numericPulseRegisterDotDiameter(spatium);
    const double distance = numericPulseRegisterDotGap(spatium) + diameter * 0.5
                            + index * (diameter + numericPulseRegisterDotGap(spatium));
    const double centerY = registerOffset > 0 ? textBbox.top() - distance : textBbox.bottom() + distance;
    return muse::RectF(-diameter * 0.5, centerY - diameter * 0.5, diameter, diameter);
}

inline muse::RectF numericPulseGlyphBbox(const muse::String& text, double scale, double spatium, int registerOffset)
{
    muse::RectF bbox = numericPulseTextBbox(text, scale);
    for (int i = 0; i < std::abs(registerOffset); ++i) {
        bbox = bbox.united(numericPulseRegisterDotBbox(numericPulseTextBbox(text, scale), spatium, registerOffset, i));
    }
    return bbox;
}

inline double numericPulseDurationMarkWidth(const muse::RectF& textBbox, double spatium)
{
    // Extend slightly past both sides of the numeric glyph so a standalone
    // duration mark reads clearly without looking like a full grouped beam.
    return std::max(textBbox.width() + 0.60 * spatium, 1.30 * spatium);
}

inline double numericPulseDurationMarkY(const muse::RectF& textBbox, double spatium, int index)
{
    return textBbox.top() - numericPulseRhythmMarkGap(spatium)
           - numericPulseRhythmMarkDistance(spatium) * index;
}

inline muse::RectF numericPulseNoteBbox(const muse::String& text, double scale, double spatium,
                                        int registerOffset, int durationMarks)
{
    muse::RectF bbox = numericPulseGlyphBbox(text, scale, spatium, registerOffset);
    if (durationMarks > 0) {
        const muse::RectF textBbox = numericPulseTextBbox(text, scale);
        const double width = numericPulseDurationMarkWidth(textBbox, spatium);
        const double thickness = numericPulseRhythmMarkThickness(spatium);
        const double top = numericPulseDurationMarkY(bbox, spatium, durationMarks - 1) - thickness * 0.5;
        bbox = bbox.united(muse::RectF(-width * 0.5, top, width, thickness));
    }
    return bbox;
}

}
