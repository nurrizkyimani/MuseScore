/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include "draw/font.h"
#include "draw/fontmetrics.h"
#include "global/types/geometry.h"
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

}
