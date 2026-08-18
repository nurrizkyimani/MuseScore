/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#include "numericpulseprofile.h"

#include <set>
#include <string>

#include "instrument.h"
#include "instrtemplate.h"
#include "part.h"
#include "staff.h"
#include "rw/xmlreader.h"
#include "rw/xmlwriter.h"

namespace mu::engraving {

bool NumericPulseProfile::isValid() const
{
    if (id.empty() || degrees.empty()) {
        return false;
    }

    std::set<int> numbers;
    std::set<int> pitchClasses;
    std::set<int> inputIndexes;
    std::set<std::string> actions;
    for (const NumericPulseDegree& degree : degrees) {
        const int pitchClass = (degree.scorePitch % 12 + 12) % 12;
        if (degree.number <= 0 || degree.scorePitch < 0 || degree.scorePitch > 127
            || degree.inputIndex < 0 || degree.inputIndex > 6 || degree.label.empty() || degree.action.empty()
            || !numbers.insert(degree.number).second
            || !pitchClasses.insert(pitchClass).second
            || !inputIndexes.insert(degree.inputIndex).second
            || !actions.insert(degree.action.toStdString()).second) {
            return false;
        }
    }
    return true;
}

void NumericPulseProfile::read(XmlReader& e)
{
    id = e.attribute("id");
    degrees.clear();

    while (e.readNextStartElement()) {
        const AsciiStringView tag(e.name());
        if (tag == "fixedBaseline") {
            fixedBaseline = e.readBool();
        } else if (tag == "monophonic") {
            monophonic = e.readBool();
        } else if (tag == "showEmptyPulses") {
            showEmptyPulses = e.readBool();
        } else if (tag == "durationMarksAbove") {
            durationMarksAbove = e.readBool();
        } else if (tag == "degree") {
            NumericPulseDegree degree;
            degree.number = e.intAttribute("number");
            degree.scorePitch = e.intAttribute("pitch", -1);
            degree.inputStep = e.intAttribute("inputStep");
            degree.inputIndex = e.intAttribute("inputIndex", -1);
            degree.action = e.attribute("action");
            degree.label = e.readText();
            degrees.push_back(degree);
        } else {
            e.unknown();
        }
    }
}

void NumericPulseProfile::write(XmlWriter& xml) const
{
    if (id.empty()) {
        return;
    }

    xml.startElement("numericPulse", { { "id", id } });
    xml.tag("fixedBaseline", fixedBaseline);
    xml.tag("monophonic", monophonic);
    xml.tag("showEmptyPulses", showEmptyPulses);
    xml.tag("durationMarksAbove", durationMarksAbove);
    for (const NumericPulseDegree& degree : degrees) {
        xml.tag("degree", {
            { "number", degree.number },
            { "pitch", degree.scorePitch },
            { "inputStep", degree.inputStep },
            { "inputIndex", degree.inputIndex },
            { "action", degree.action }
        }, degree.label);
    }
    xml.endElement();
}

const NumericPulseDegree* NumericPulseProfile::degreeForNumber(int number) const
{
    for (const NumericPulseDegree& degree : degrees) {
        if (degree.number == number) {
            return &degree;
        }
    }
    return nullptr;
}

const NumericPulseDegree* NumericPulseProfile::degreeForPitch(int pitch) const
{
    const int pitchClass = (pitch % 12 + 12) % 12;
    for (const NumericPulseDegree& degree : degrees) {
        if ((degree.scorePitch % 12 + 12) % 12 == pitchClass) {
            return &degree;
        }
    }
    return nullptr;
}

const NumericPulseDegree* NumericPulseProfile::degreeForInputIndex(int inputIndex) const
{
    for (const NumericPulseDegree& degree : degrees) {
        if (degree.inputIndex == inputIndex) {
            return &degree;
        }
    }
    return nullptr;
}

const NumericPulseProfile* numericPulseProfile(const Instrument* instrument)
{
    if (!instrument) {
        return nullptr;
    }
    const InstrumentTemplate* instrumentTemplate = searchTemplate(instrument->id());
    if (!instrumentTemplate || instrumentTemplate->numericPulseProfile.id.empty()) {
        return nullptr;
    }
    return &instrumentTemplate->numericPulseProfile;
}

const NumericPulseProfile* numericPulseProfile(const Staff* staff, const Fraction& tick)
{
    return staff && staff->part() ? numericPulseProfile(staff->part()->instrument(tick)) : nullptr;
}

muse::String numericPulseDegreeText(const NumericPulseProfile* profile, int pitch)
{
    const NumericPulseDegree* degree = profile ? profile->degreeForPitch(pitch) : nullptr;
    return degree ? degree->label : muse::String(u"×");
}

}
