/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */

#include <gtest/gtest.h>

#include <array>
#include <vector>

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
#include "engraving/editing/editnote.h"
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

TEST(Engraving_GamelanNotationTests, RequestedPelogInstrumentProfilesAreAvailable)
{
    struct ExpectedProfile {
        const char16_t* instrumentId;
        std::vector<int> degrees;
    };

    const std::array<ExpectedProfile, 9> expectedProfiles { {
        { u"saron-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"gender-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"gambang-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"rebab-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"suling-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"siter-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"gong-kemodong-pelog", { 5, 6 } },
        { u"gong-ageng-pelog", { 6 } },
        { u"kempul-pelog", { 1, 3, 5, 6, 7 } }
    } };

    for (const ExpectedProfile& expected : expectedProfiles) {
        const InstrumentTemplate* instrumentTemplate = searchTemplate(muse::String(expected.instrumentId));
        ASSERT_NE(instrumentTemplate, nullptr) << muse::String(expected.instrumentId).toStdString();
        ASSERT_NE(instrumentTemplate->staffTypePreset, nullptr);
        EXPECT_TRUE(instrumentTemplate->staffTypePreset->isNumericPulseStaff());

        const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
        ASSERT_TRUE(profile.isValid()) << muse::String(expected.instrumentId).toStdString();
        ASSERT_EQ(profile.degrees.size(), expected.degrees.size());
        for (size_t i = 0; i < expected.degrees.size(); ++i) {
            EXPECT_EQ(profile.degrees[i].number, expected.degrees[i]);
        }
    }
}

TEST(Engraving_GamelanNotationTests, TwoHandedInstrumentsUseTwoNumericPulseStaves)
{
    // Gendèr, gambang and siter are played with two independent hands, so each
    // hand needs its own numeric-pulse line rather than sharing a single staff.
    const std::array<const char16_t*, 3> twoHandedInstruments { {
        u"gender-pelog",
        u"gambang-pelog",
        u"siter-pelog"
    } };

    for (const char16_t* instrumentId : twoHandedInstruments) {
        const InstrumentTemplate* instrumentTemplate = searchTemplate(muse::String(instrumentId));
        ASSERT_NE(instrumentTemplate, nullptr) << muse::String(instrumentId).toStdString();
        EXPECT_EQ(instrumentTemplate->staffCount, size_t(2)) << muse::String(instrumentId).toStdString();

        // Both hands must render as one-line numeric-pulse staves. staffLines is
        // only populated per staff index that the instrument file mentions, so a
        // missing <stafflines staff="2"> would silently leave the lower hand at
        // the five-line default.
        for (staff_idx_t staffIdx = 0; staffIdx < instrumentTemplate->staffCount; ++staffIdx) {
            EXPECT_EQ(instrumentTemplate->staffLines[staffIdx], 1)
                << muse::String(instrumentId).toStdString() << " staff " << staffIdx;
        }

        // The brace and spanning barlines are what read the two lines as one instrument.
        EXPECT_EQ(instrumentTemplate->bracket[0], BracketType::BRACE) << muse::String(instrumentId).toStdString();
        EXPECT_EQ(instrumentTemplate->bracketSpan[0], 2) << muse::String(instrumentId).toStdString();
        // barlineSpan is a per-staff bool: <barlineSpan>2</barlineSpan> marks staff 0
        // as joined to the staff below it.
        EXPECT_TRUE(instrumentTemplate->barlineSpan[0]) << muse::String(instrumentId).toStdString();

        // Widened so the lower hand can be written an octave below the baseline.
        EXPECT_LE(instrumentTemplate->minPitchA, 48) << muse::String(instrumentId).toStdString();
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

TEST(Engraving_GamelanNotationTests, NumericPulseRegisterMappingUsesProfileBounds)
{
    const InstrumentTemplate* instrumentTemplate = searchTemplate(u"saron-pelog");
    ASSERT_NE(instrumentTemplate, nullptr);
    const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
    const NumericPulseDegree* degree = profile.degreeForNumber(5);
    ASSERT_NE(degree, nullptr);

    EXPECT_EQ(numericPulsePitch(&profile, degree, -1), 55);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 0), 67);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 1), 79);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 2), 91);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 3), -1);
    EXPECT_EQ(numericPulseRegister(&profile, 55), -1);
    EXPECT_EQ(numericPulseRegister(&profile, 79), 1);
    EXPECT_EQ(numericPulseDegreeText(&profile, 79), muse::String(u"5"));
}

TEST(Engraving_GamelanNotationTests, RegisterDotsExpandGlyphAndMoveDurationMarksClear)
{
    const double spatium = 1.0;
    const muse::String text(u"5");
    const muse::RectF plain = rendering::score::numericPulseNoteBbox(text, 1.0, spatium, 0, 1);
    const muse::RectF upper = rendering::score::numericPulseNoteBbox(text, 1.0, spatium, 2, 1);
    const muse::RectF lower = rendering::score::numericPulseNoteBbox(text, 1.0, spatium, -1, 1);

    EXPECT_LT(upper.top(), plain.top());
    EXPECT_GT(lower.bottom(), plain.bottom());
    EXPECT_LT(rendering::score::numericPulseDurationMarkY(upper, spatium, 0), upper.top());
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

TEST(Engraving_GamelanNotationTests, ArrowPitchModeStepsThroughPelogDegrees)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 3);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    score->select(note);

    score->startCmd(TranslatableString::untranslatable("Numeric degree up"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 65);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(4));

    score->startCmd(TranslatableString::untranslatable("Numeric degree down"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 64);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(3));

    delete score;
}

TEST(Engraving_GamelanNotationTests, OctavePitchModeMovesAndClampsNumericRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 1);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    score->select(note);

    EditNote::upDown(score, true, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 72);
    EXPECT_EQ(numericPulseRegister(numericPulseProfile(note->staff(), note->tick()), note->pitch()), 1);

    EditNote::upDown(score, true, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 84);
    EditNote::upDown(score, true, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 84);

    EditNote::upDown(score, false, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 72);
    EditNote::setNumericPulseRegister(score, -1);
    EXPECT_EQ(note->pitch(), 48);

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericDegreeInputKeepsSelectedRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setNotes({ NoteInput::noteVal(score, 72, 0, false) });

    enterDegree(score, 5);
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_NE(chord->upNote(), nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 79);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(chord->staff(), chord->tick()), chord->upNote()->pitch()),
              muse::String(u"5"));

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowPitchModeSkipsUnsupportedSlendroDegreesAndClamps)
{
    MasterScore* score = makeNumericPulseScore(u"saron-slendro");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 3);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    score->select(note);

    score->startCmd(TranslatableString::untranslatable("Numeric degree up"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 67);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(5));

    score->startCmd(TranslatableString::untranslatable("Numeric degree up"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 69);

    score->startCmd(TranslatableString::untranslatable("Numeric degree upper boundary"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 69);

    score->startCmd(TranslatableString::untranslatable("Numeric degree down"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 67);

    score->startCmd(TranslatableString::untranslatable("Numeric degree down"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 64);

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowUpReplacesEmptyPulseWithFirstProfileDegree)
{
    MasterScore* score = makeNumericPulseScore(u"gong-kemodong-pelog");
    ASSERT_NE(score, nullptr);

    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter empty pulse"), [&](Transaction& tx) {
        NoteInput::addRest(tx, score);
    });

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    Rest* rest = toRest(item);
    const Fraction originalDuration = rest->durationTypeTicks();
    score->select(rest);

    score->startCmd(TranslatableString::untranslatable("Fill empty pulse"));
    Note* note = EditNote::replaceRestWithFirstNumericPulseDegree(score, rest);
    score->endCmd();

    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->pitch(), 43);
    EXPECT_EQ(note->chord()->durationTypeTicks(), originalDuration);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(5));

    score->undoRedo(true, nullptr);
    segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    item = segment->element(0);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isRest());

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowUpReplacesFullMeasureRestWithoutCrashing)
{
    MasterScore* score = makeNumericPulseScore(u"saron-pelog");
    ASSERT_NE(score, nullptr);

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    Rest* rest = toRest(item);
    ASSERT_TRUE(rest->durationType().isMeasure());
    const Fraction originalDuration = rest->ticks();
    ASSERT_FALSE(originalDuration.isZero());
    score->select(rest);

    score->startCmd(TranslatableString::untranslatable("Fill full-measure empty pulse"));
    Note* note = EditNote::replaceRestWithFirstNumericPulseDegree(score, rest);
    score->endCmd();

    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->pitch(), 60);
    EXPECT_EQ(note->chord()->durationTypeTicks(), originalDuration);

    score->undoRedo(true, nullptr);
    segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    item = segment->element(0);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isRest());

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowDownReplacesFirstProfileDegreeWithEmptyPulse)
{
    MasterScore* score = makeNumericPulseScore(u"gong-kemodong-pelog");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 5);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    const Fraction originalDuration = chord->durationTypeTicks();
    score->select(note);

    score->startCmd(TranslatableString::untranslatable("Empty first numeric degree"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    Rest* rest = toRest(item);
    EXPECT_EQ(rest->durationTypeTicks(), originalDuration);

    score->undoRedo(true, nullptr);
    segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isChord());
    note = toChord(item)->upNote();
    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->pitch(), 43);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(5));

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

TEST(Engraving_GamelanNotationTests, AdditionalInstrumentProfileSurvivesSaveReload)
{
    MasterScore* score = makeNumericPulseScore(u"kempul-pelog");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 7);

    const String savePath = u"kempul-pelog-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);

    const NumericPulseProfile* profile = numericPulseProfile(reopened->staff(0), Fraction(0, 1));
    ASSERT_NE(profile, nullptr);
    EXPECT_EQ(profile->id, muse::String(u"kempul-pelog"));
    Chord* chord = reopened->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 59);
    EXPECT_EQ(numericPulseDegreeText(profile, chord->upNote()->pitch()), muse::String(u"7"));

    delete reopened;
    delete score;
}
