/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include <algorithm>

#include "draw/fontmetrics.h"
#include "draw/types/font.h"
#include "draw/types/geometry.h"
#include "global/types/string.h"

namespace mu::engraving::rendering::score {

inline muse::draw::Font gamelanFont(double scale)
{
    muse::draw::Font font(u"Edwin", muse::draw::Font::Type::Text);
    font.setPointSizeF(12.0 * scale);
    return font;
}

inline muse::RectF gamelanTextBbox(const muse::String& text, double scale)
{
    const muse::draw::Font font = gamelanFont(scale);
    muse::RectF bbox = muse::draw::FontMetrics(font).boundingRect(text);
    bbox.translate(-bbox.center().x(), 0.0);
    return bbox;
}

inline double gamelanTextOriginX(const muse::String& text, double scale)
{
    return -muse::draw::FontMetrics(gamelanFont(scale)).horizontalAdvance(text) * 0.5;
}

inline double gamelanDurationMarkWidth(const muse::RectF& textBbox, double spatium)
{
    return std::max(textBbox.width(), 0.9 * spatium);
}

inline double gamelanDurationMarkY(const muse::RectF& textBbox, double spatium, int index)
{
    return textBbox.top() - (0.22 + 0.24 * index) * spatium;
}

inline muse::RectF gamelanNoteBbox(const muse::String& text, double scale, double spatium, int durationMarks)
{
    muse::RectF bbox = gamelanTextBbox(text, scale);
    if (durationMarks > 0) {
        const double width = gamelanDurationMarkWidth(bbox, spatium);
        const double top = gamelanDurationMarkY(bbox, spatium, durationMarks - 1) - 0.06 * spatium;
        bbox = bbox.united(muse::RectF(-width * 0.5, top, width, 0.12 * spatium));
    }
    return bbox;
}

}
