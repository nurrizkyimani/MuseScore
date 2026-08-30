/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include <vector>

#include "global/types/string.h"

namespace mu::engraving {

class Fraction;
class Instrument;
class Staff;
class XmlReader;
class XmlWriter;

struct NumericPulseDegree {
    int number = 0;
    int scorePitch = -1;
    int inputStep = 0;
    int inputIndex = -1;
    muse::String label;
    muse::String action;
};

struct NumericPulseProfile {
    muse::String id;
    std::vector<NumericPulseDegree> degrees;
    bool fixedBaseline = true;
    bool monophonic = true;
    bool showEmptyPulses = true;
    bool durationMarksAbove = true;
    int minimumRegister = -1;
    int maximumRegister = 1;
    int defaultRegister = 0;

    bool isValid() const;
    void read(XmlReader&);
    void write(XmlWriter&) const;

    const NumericPulseDegree* degreeForNumber(int number) const;
    const NumericPulseDegree* degreeForPitch(int pitch) const;
    const NumericPulseDegree* degreeForInputIndex(int inputIndex) const;
};

const NumericPulseProfile* numericPulseProfile(const Instrument* instrument);
const NumericPulseProfile* numericPulseProfile(const Staff* staff, const Fraction& tick);
muse::String numericPulseDegreeText(const NumericPulseProfile* profile, int pitch);
int numericPulseRegister(const NumericPulseProfile* profile, int pitch);
int numericPulsePitch(const NumericPulseProfile* profile, const NumericPulseDegree* degree, int registerOffset);

}
