/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */

#include <gtest/gtest.h>

#include <array>

#include "engraving/dom/beam.h"
#include "engraving/dom/gamelannotation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/input.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"
#include "engraving/editing/edittie.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/transaction/transaction.h"

#include "utils/scorerw.h"

using namespace mu::engraving;

namespace {

MasterScore* makeGamelanScore()
{
    MasterScore* score = ScoreRW::readScore(u"durationtype_data/empty.mscx");
    if (!score) {
        return nullptr;
    }

    const StaffType* staffType = StaffType::preset(StaffTypes::GAMELAN_SLENDRO);
    if (!staffType) {
        delete score;
        return nullptr;
    }

    score->staff(0)->setStaffType(Fraction(0, 1), *staffType);
    InputState& inputState = score->inputState();
    inputState.setTrack(0);
    inputState.setSegment(score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest));
    inputState.setDuration(DurationType::V_QUARTER);
    inputState.setNoteEntryMode(true);
    return score;
}

void enterDegree(MasterScore* score, int degree)
{
    NoteInputParams params;
    params.gamelanDegree = degree;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter Slendro pulse"), [&](Transaction& tx) {
        NoteInput::addPitch(tx, score, params, true, false);
    });
}

}

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
    EXPECT_EQ(slendroDegreeText(65), muse::String(u"×"));
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
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);

    InputState& inputState = score->inputState();
    enterDegree(score, 1);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 1u);
    EXPECT_EQ(chord->upNote()->pitch(), 60);

    inputState.setSegment(score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest));
    enterDegree(score, 5);

    chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 1u);
    EXPECT_EQ(chord->upNote()->pitch(), 67);

    delete score;
}

TEST(Engraving_GamelanNotationTests, RejectsUnsupportedInputDegrees)
{
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);

    const std::array<int, 5> supportedSteps { 0, 1, 2, 4, 5 };
    for (int step : supportedSteps) {
        NoteInputParams params;
        EXPECT_TRUE(NoteInput::resolveNoteInputParams(score, step, false, params));
        EXPECT_NE(params.gamelanDegree, 0);
    }

    NoteInputParams params;
    EXPECT_FALSE(NoteInput::resolveNoteInputParams(score, 3, false, params));
    EXPECT_FALSE(NoteInput::resolveNoteInputParams(score, 6, false, params));
    delete score;
}

TEST(Engraving_GamelanNotationTests, EighthPulseGroupStaysFlatAndMonophonic)
{
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDuration(DurationType::V_EIGHTH);

    for (int degree : { 1, 2, 3, 5 }) {
        enterDegree(score, degree);
    }

    score->setLayoutAll();
    score->doLayout();

    const std::array<Fraction, 4> ticks {
        Fraction(0, 1), Fraction(1, 8), Fraction(1, 4), Fraction(3, 8)
    };
    Beam* beam = nullptr;
    for (const Fraction& tick : ticks) {
        Chord* chord = score->firstMeasure()->findChord(tick, 0);
        ASSERT_NE(chord, nullptr);
        ASSERT_EQ(chord->notes().size(), 1u);
        EXPECT_EQ(chord->durationType().type(), DurationType::V_EIGHTH);
        if (!beam) {
            beam = chord->beam();
        } else {
            EXPECT_EQ(chord->beam(), beam);
        }
    }

    ASSERT_NE(beam, nullptr);
    EXPECT_TRUE(beam->up());
    EXPECT_DOUBLE_EQ(beam->slope(), 0.0);

    const double baseline = score->firstMeasure()->findChord(ticks.front(), 0)->upNote()->ldata()->pos().y();
    for (const Fraction& tick : ticks) {
        EXPECT_DOUBLE_EQ(score->firstMeasure()->findChord(tick, 0)->upNote()->ldata()->pos().y(), baseline);
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, SixteenthPulseGroupUsesTwoFlatRhythmMarks)
{
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDuration(DurationType::V_16TH);

    for (int degree : { 1, 2, 3, 5 }) {
        enterDegree(score, degree);
    }
    score->setLayoutAll();
    score->doLayout();

    Beam* beam = nullptr;
    for (const Fraction& tick : { Fraction(0, 1), Fraction(1, 16), Fraction(1, 8), Fraction(3, 16) }) {
        Chord* chord = score->firstMeasure()->findChord(tick, 0);
        ASSERT_NE(chord, nullptr);
        EXPECT_EQ(chord->durationType().type(), DurationType::V_16TH);
        EXPECT_EQ(chord->durationType().hooks(), 2);
        beam = beam ? beam : chord->beam();
        EXPECT_EQ(chord->beam(), beam);
    }
    ASSERT_NE(beam, nullptr);
    EXPECT_TRUE(beam->up());
    EXPECT_DOUBLE_EQ(beam->slope(), 0.0);
    delete score;
}

TEST(Engraving_GamelanNotationTests, DeletePulseUndoRedoDoesNotShiftNeighbors)
{
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 1);
    enterDegree(score, 2);

    Chord* firstChord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    Chord* secondChord = score->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(firstChord, nullptr);
    ASSERT_NE(secondChord, nullptr);
    const int secondPitch = secondChord->upNote()->pitch();

    InputState& inputState = score->inputState();
    inputState.setSegment(firstChord->segment());
    inputState.setDuration(DurationType::V_QUARTER);
    inputState.setRest(true);
    Position position;
    position.segment = firstChord->segment();
    position.staffIdx = 0;
    position.line = 0;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Clear Slendro pulse"), [&](Transaction& tx) {
        NoteInput::putNote(tx, score, position, true);
    });

    EXPECT_EQ(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
    Segment* emptyPulseSegment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(emptyPulseSegment, nullptr);
    EngravingItem* emptyPulse = emptyPulseSegment->element(0);
    ASSERT_NE(emptyPulse, nullptr);
    ASSERT_TRUE(emptyPulse->isRest());
    EXPECT_EQ(toRest(emptyPulse)->durationType().type(), DurationType::V_QUARTER);
    secondChord = score->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(secondChord, nullptr);
    EXPECT_EQ(secondChord->upNote()->pitch(), secondPitch);

    score->undoRedo(true, nullptr);
    EXPECT_NE(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
    score->undoRedo(false, nullptr);
    EXPECT_EQ(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
    delete score;
}

TEST(Engraving_GamelanNotationTests, NativeTieProvidesContinuation)
{
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 3);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    score->select(note);
    EditTie::cmdAddTie(score);

    ASSERT_NE(note->tieFor(), nullptr);
    ASSERT_NE(note->tieFor()->endNote(), nullptr);
    EXPECT_EQ(note->tieFor()->endNote()->pitch(), note->pitch());
    EXPECT_EQ(note->tieFor()->endNote()->tick(), Fraction(1, 4));
    delete score;
}

TEST(Engraving_GamelanNotationTests, SaveReloadPreservesStaffAndPulseData)
{
    MasterScore* score = makeGamelanScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 6);
    score->inputState().setDuration(DurationType::V_EIGHTH);
    enterDegree(score, 5);

    const String savePath = u"gamelan-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);

    EXPECT_TRUE(reopened->staff(0)->staffType(Fraction(0, 1))->isGamelanStaff());
    Chord* firstChord = reopened->firstMeasure()->findChord(Fraction(0, 1), 0);
    Chord* secondChord = reopened->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(firstChord, nullptr);
    ASSERT_NE(secondChord, nullptr);
    EXPECT_EQ(firstChord->upNote()->pitch(), 69);
    EXPECT_EQ(secondChord->upNote()->pitch(), 67);
    EXPECT_EQ(secondChord->durationType().type(), DurationType::V_EIGHTH);

    delete reopened;
    delete score;
}
