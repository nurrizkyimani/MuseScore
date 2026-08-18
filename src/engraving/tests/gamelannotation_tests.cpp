/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */

#include <gtest/gtest.h>

#include "engraving/dom/gamelannotation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/input.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/transaction/transaction.h"

#include "utils/scorerw.h"

using namespace mu::engraving;

TEST(Engraving_GamelanNotationTests, SlendroDegreePitchMapping)
{
    const std::pair<int, int> mapping[] = {
        { 1, 60 }, { 2, 62 }, { 3, 64 }, { 5, 67 }, { 6, 69 }
    };

    for (const auto& [degree, pitch] : mapping) {
        ASSERT_TRUE(pitchForSlendroDegree(degree).has_value());
        EXPECT_EQ(*pitchForSlendroDegree(degree), pitch);
        ASSERT_TRUE(slendroDegreeForPitch(pitch).has_value());
        EXPECT_EQ(*slendroDegreeForPitch(pitch), degree);
        EXPECT_EQ(slendroDegreeText(pitch), muse::String::number(degree));
    }

    EXPECT_FALSE(pitchForSlendroDegree(4).has_value());
    EXPECT_FALSE(pitchForSlendroDegree(7).has_value());
    EXPECT_FALSE(slendroDegreeForPitch(65).has_value());
    EXPECT_EQ(slendroDegreeText(65), muse::String(u"?"));
}

TEST(Engraving_GamelanNotationTests, SlendroInputSteps)
{
    EXPECT_EQ(diatonicStepForSlendroDegree(1), 28);
    EXPECT_EQ(diatonicStepForSlendroDegree(2), 29);
    EXPECT_EQ(diatonicStepForSlendroDegree(3), 30);
    EXPECT_EQ(diatonicStepForSlendroDegree(5), 32);
    EXPECT_EQ(diatonicStepForSlendroDegree(6), 33);
    EXPECT_FALSE(diatonicStepForSlendroDegree(4).has_value());
}

TEST(Engraving_GamelanNotationTests, SlendroStaffPreset)
{
    const StaffType* staffType = StaffType::preset(StaffTypes::GAMELAN_SLENDRO);
    ASSERT_NE(staffType, nullptr);
    EXPECT_TRUE(staffType->isGamelanStaff());
    EXPECT_EQ(staffType->group(), StaffGroup::STANDARD);
    EXPECT_EQ(staffType->lines(), 1);
    EXPECT_TRUE(staffType->showBarlines());
    EXPECT_TRUE(staffType->genTimesig());
    EXPECT_FALSE(staffType->genClef());
    EXPECT_FALSE(staffType->genKeysig());
    EXPECT_FALSE(staffType->showLedgerLines());
}

TEST(Engraving_GamelanNotationTests, PulseInputIsMonophonicAndDoesNotStack)
{
    MasterScore* score = ScoreRW::readScore(u"durationtype_data/empty.mscx");
    ASSERT_NE(score, nullptr);

    const StaffType* staffType = StaffType::preset(StaffTypes::GAMELAN_SLENDRO);
    ASSERT_NE(staffType, nullptr);
    score->staff(0)->setStaffType(Fraction(0, 1), *staffType);

    InputState& inputState = score->inputState();
    inputState.setTrack(0);
    inputState.setSegment(score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest));
    inputState.setDuration(DurationType::V_QUARTER);
    inputState.setNoteEntryMode(true);

    NoteInputParams degreeOne;
    degreeOne.gamelanDegree = 1;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter Slendro pulse"), [&](Transaction& tx) {
        NoteInput::addPitch(tx, score, degreeOne, true, false);
    });

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 1u);
    EXPECT_EQ(chord->upNote()->pitch(), 60);

    inputState.setSegment(score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest));
    NoteInputParams degreeFive;
    degreeFive.gamelanDegree = 5;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Replace Slendro pulse"), [&](Transaction& tx) {
        NoteInput::addPitch(tx, score, degreeFive, true, false);
    });

    chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 1u);
    EXPECT_EQ(chord->upNote()->pitch(), 67);

    delete score;
}
