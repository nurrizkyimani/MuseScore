/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */

#include <gtest/gtest.h>

#include <array>

#include "engraving/dom/beam.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/input.h"
#include "engraving/dom/instrtemplate.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/numericpulseprofile.h"
#include "engraving/dom/part.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"
#include "engraving/editing/edittie.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/rendering/score/numericpulserendering.h"

#include "utils/scorerw.h"

using namespace mu::engraving;

namespace {

MasterScore* makeNumericPulseScore(const String& instrumentId = u"saron-pelog")
{
    MasterScore* score = ScoreRW::readScore(u"durationtype_data/empty.mscx");
    if (!score) {
        return nullptr;
    }

    const StaffType* staffType = StaffType::preset(StaffTypes::NUMERIC_PULSE);
    if (!staffType) {
        delete score;
        return nullptr;
    }

    score->staff(0)->setStaffType(Fraction(0, 1), *staffType);
    score->staff(0)->part()->instrument()->setId(instrumentId);
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
    params.numericDegree = degree;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter numeric pulse"), [&](Transaction& tx) {
        NoteInput::addPitch(tx, score, params, true, false);
    });
}

}

TEST(Engraving_GamelanNotationTests, PelogProfileDefinesSevenNumericDegrees)
{
    const InstrumentTemplate* instrumentTemplate = searchTemplate(u"saron-pelog");
    ASSERT_NE(instrumentTemplate, nullptr);
    const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
    ASSERT_TRUE(profile.isValid());

    const std::pair<int, int> mapping[] = {
        { 1, 60 }, { 2, 62 }, { 3, 64 }, { 4, 65 }, { 5, 67 }, { 6, 69 }, { 7, 71 }
    };

    for (const auto& [degree, pitch] : mapping) {
        ASSERT_NE(profile.degreeForNumber(degree), nullptr);
        EXPECT_EQ(profile.degreeForNumber(degree)->scorePitch, pitch);
        ASSERT_NE(profile.degreeForPitch(pitch), nullptr);
        EXPECT_EQ(profile.degreeForPitch(pitch)->number, degree);
        EXPECT_EQ(numericPulseDegreeText(&profile, pitch), muse::String::number(degree));
    }
}

TEST(Engraving_GamelanNotationTests, SlendroProfileRemainsAvailableForCompatibility)
{
    const InstrumentTemplate* instrumentTemplate = searchTemplate(u"saron-slendro");
    ASSERT_NE(instrumentTemplate, nullptr);
    const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
    ASSERT_TRUE(profile.isValid());
    EXPECT_EQ(profile.degrees.size(), 5u);
    EXPECT_EQ(profile.degreeForNumber(1)->inputStep, 28);
    EXPECT_EQ(profile.degreeForNumber(2)->inputStep, 29);
    EXPECT_EQ(profile.degreeForNumber(3)->inputStep, 30);
    EXPECT_EQ(profile.degreeForNumber(5)->inputStep, 32);
    EXPECT_EQ(profile.degreeForNumber(6)->inputStep, 33);
    EXPECT_EQ(profile.degreeForNumber(4), nullptr);
    EXPECT_EQ(profile.degreeForNumber(7), nullptr);
}

TEST(Engraving_GamelanNotationTests, NumericPulseProfileRejectsDuplicateDegreeDefinitions)
{
    NumericPulseProfile profile;
    profile.id = u"invalid-duplicate";
    profile.degrees = {
        { 1, 60, 28, 0, u"1", u"note-c" },
        { 1, 62, 29, 1, u"1", u"note-d" }
    };
    EXPECT_FALSE(profile.isValid());
}

TEST(Engraving_GamelanNotationTests, NumericPulseStaffPreset)
{
    const StaffType* staffType = StaffType::preset(StaffTypes::NUMERIC_PULSE);
    ASSERT_NE(staffType, nullptr);
    EXPECT_TRUE(staffType->isNumericPulseStaff());
    EXPECT_EQ(staffType->group(), StaffGroup::STANDARD);
    EXPECT_EQ(staffType->lines(), 1);
    EXPECT_TRUE(staffType->showBarlines());
    EXPECT_TRUE(staffType->genTimesig());
    EXPECT_FALSE(staffType->genClef());
    EXPECT_FALSE(staffType->genKeysig());
    EXPECT_FALSE(staffType->showLedgerLines());
}

TEST(Engraving_GamelanNotationTests, LegacySlendroStaffPresetNameResolvesToNumericPulse)
{
    const StaffType* numericPulse = StaffType::preset(StaffTypes::NUMERIC_PULSE);
    ASSERT_NE(numericPulse, nullptr);
    EXPECT_EQ(StaffType::presetFromXmlName(u"stdGamelanSlendro"), numericPulse);
}

TEST(Engraving_GamelanNotationTests, WesternInstrumentHasNoNumericPulseProfile)
{
    MasterScore* score = ScoreRW::readScore(u"durationtype_data/empty.mscx");
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(numericPulseProfile(score->staff(0), Fraction(0, 1)), nullptr);
    EXPECT_FALSE(score->staff(0)->staffType(Fraction(0, 1))->isNumericPulseStaff());
    delete score;
}

TEST(Engraving_GamelanNotationTests, PelogAcceptsAllSevenDegreeInputs)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    for (int inputIndex = 0; inputIndex < 7; ++inputIndex) {
        NoteInputParams params;
        EXPECT_TRUE(NoteInput::resolveNoteInputParams(score, inputIndex, false, params));
        EXPECT_EQ(params.numericDegree, inputIndex + 1);
    }

    delete score;
}

TEST(Engraving_GamelanNotationTests, PulseInputIsMonophonicAndDoesNotStack)
{
    MasterScore* score = makeNumericPulseScore();
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

TEST(Engraving_GamelanNotationTests, EmptyPulseCreatesExplicitRestAndAdvancesCursor)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    enterDegree(score, 4);
    ASSERT_EQ(score->inputState().tick(), Fraction(1, 4));

    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter empty pulse"), [&](Transaction& tx) {
        NoteInput::addRest(tx, score);
    });

    Segment* segment = score->tick2segment(Fraction(1, 4), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    const Rest* rest = toRest(item);
    EXPECT_FALSE(rest->generated());
    EXPECT_FALSE(rest->isGap());
    EXPECT_EQ(rest->durationType().type(), DurationType::V_QUARTER);
    EXPECT_EQ(score->inputState().tick(), Fraction(1, 2));

    score->setLayoutAll();
    score->doLayout();
    const muse::RectF expectedBbox = rendering::score::numericPulseEmptyPulseBbox(rest->spatium());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().x(), expectedBbox.x());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().y(), expectedBbox.y());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().width(), expectedBbox.width());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().height(), expectedBbox.height());
    EXPECT_LT(rest->ldata()->bbox().bottom(), 0.0);

    delete score;
}

TEST(Engraving_GamelanNotationTests, EmptyPulseCanSplitLongerExistingRest)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    // Reproduce the toolbar cursor shown in the bug report: it is positioned
    // inside the initial full-measure rest, so no ChordRest exists at this tick.
    InputState& inputState = score->inputState();
    Segment* cursorSegment = score->firstMeasure()->getSegment(SegmentType::ChordRest, Fraction(1, 4));
    ASSERT_NE(cursorSegment, nullptr);
    inputState.setSegment(cursorSegment);
    ASSERT_EQ(inputState.cr(), nullptr);

    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter empty pulse inside rest"),
                                              [&](Transaction& tx) { NoteInput::addRest(tx, score); });

    Segment* segment = score->tick2segment(Fraction(1, 4), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    EXPECT_FALSE(toRest(item)->generated());
    EXPECT_FALSE(toRest(item)->isGap());
    EXPECT_EQ(toRest(item)->durationType().type(), DurationType::V_QUARTER);
    EXPECT_EQ(score->inputState().tick(), Fraction(1, 2));

    delete score;
}

TEST(Engraving_GamelanNotationTests, RejectsUnsupportedInputDegrees)
{
    MasterScore* score = makeNumericPulseScore(u"saron-slendro");
    ASSERT_NE(score, nullptr);

    const std::array<int, 5> supportedSteps { 0, 1, 2, 4, 5 };
    for (int step : supportedSteps) {
        NoteInputParams params;
        EXPECT_TRUE(NoteInput::resolveNoteInputParams(score, step, false, params));
        EXPECT_NE(params.numericDegree, 0);
    }

    NoteInputParams params;
    EXPECT_FALSE(NoteInput::resolveNoteInputParams(score, 3, false, params));
    EXPECT_FALSE(NoteInput::resolveNoteInputParams(score, 6, false, params));
    delete score;
}

TEST(Engraving_GamelanNotationTests, EighthPulseGroupStaysFlatAndMonophonic)
{
    MasterScore* score = makeNumericPulseScore();
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
    EXPECT_DOUBLE_EQ(beam->beamWidth(),
                     rendering::score::numericPulseRhythmMarkThickness(beam->spatium()) * beam->mag());
    EXPECT_DOUBLE_EQ(beam->beamDist(),
                     rendering::score::numericPulseRhythmMarkDistance(beam->spatium()) * beam->mag());

    const double baseline = score->firstMeasure()->findChord(ticks.front(), 0)->upNote()->ldata()->pos().y();
    for (const Fraction& tick : ticks) {
        EXPECT_DOUBLE_EQ(score->firstMeasure()->findChord(tick, 0)->upNote()->ldata()->pos().y(), baseline);
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, SixteenthPulseGroupUsesTwoFlatRhythmMarks)
{
    MasterScore* score = makeNumericPulseScore();
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
    MasterScore* score = makeNumericPulseScore();
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
    MasterScore* score = makeNumericPulseScore();
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
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 6);
    score->inputState().setDuration(DurationType::V_EIGHTH);
    enterDegree(score, 5);

    const String savePath = u"numeric-pulse-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);

    EXPECT_TRUE(reopened->staff(0)->staffType(Fraction(0, 1))->isNumericPulseStaff());
    ASSERT_NE(numericPulseProfile(reopened->staff(0), Fraction(0, 1)), nullptr);
    EXPECT_EQ(numericPulseProfile(reopened->staff(0), Fraction(0, 1))->id, muse::String(u"saron-pelog"));
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
