# RFC-001 — M1 Sléndro Numeric Notation

**Status:** Plan only  
**Instrument:** Saron  
**Related:** `downloads/dev_log_m1_sandikala.md`

## Goal

Create a Saron Sléndro score with numbers `1 2 3 5 6`, one vertical position,
time signature, barlines, basic rhythm lines, dots, gatra blocks, duration
editing, and MSCX save/reopen. Sound and authentic tuning come later.

## Design

Keep real MuseScore objects:

```text
Measure (one gatra in M1)
└── Chord
    └── Note: real pitch + duration, drawn as a number
```

Add a `StaffGroup::STANDARD` preset named `stdGamelanSlendro`:

- One staff line.
- No clef, key signature, or ledger lines.
- Show time signature and barlines.
- Keep stems/beams for rhythm.

Initial logical mapping:

| Degree | Pitch |
|---:|---:|
| 1 | 60 |
| 2 | 62 |
| 3 | 64 |
| 5 | 67 |
| 6 | 69 |

These pitches do not define authentic Sléndro tuning.

## Code Changes

| Area | Files | Functions/change |
|---|---|---|
| Staff preset | `src/engraving/dom/stafftype.h/.cpp` | Add `StaffTypes::GAMELAN_SLENDRO`, update `StaffType::type()` and `initStaffTypes()`, add `isGamelanStaff()` |
| Staff helper | `src/engraving/dom/staff.h/.cpp` | Add `Staff::isGamelanStaff(tick)` |
| Degree mapping | New `src/engraving/dom/gamelannotation.h/.cpp` | `isValidSlendroDegree()`, `degreeForPitch()`, `pitchForDegree()`, `degreeText()` |
| Number metrics | `src/engraving/rendering/score/tlayout.cpp` | Branch in `TLayout::layoutNote()` to create the numeric bounding box |
| Flat position | `src/engraving/rendering/score/chordlayout.cpp` | Branch in `ChordLayout::layoutChords3()` to use one fixed Y |
| Number drawing | `src/engraving/rendering/score/tdraw.cpp` | Branch in `TDraw::draw(const Note*)` to draw the degree |
| Dot metrics | `src/engraving/rendering/score/restlayout.cpp` | Branch in `RestLayout::layoutRest()` |
| Dot drawing | `src/engraving/rendering/score/tdraw.cpp` | Branch in `TDraw::draw(const Rest*)` |
| Saron | `share/instruments/instruments.xml` | Add `saron-slendro` using preset `stdGamelanSlendro` |
| Input actions | `src/notationscene/notationcommands.h`, `notationactioncontroller.h/.cpp` | Add `handleGamelanDegreeAction(int)` and validation |
| Input UI | New `GamelanInputBar.qml` and `gamelaninputmodel.h/.cpp` | Buttons `[1][2][3][5][6][.]` |

The numeric bounding box must belong to the real `Note`, not an attached text
object. This preserves clicking, selection, undo, saving, and future playback.

## Reuse Existing MuseScore Features

| Feature | Existing function/path |
|---|---|
| Time signature | Existing `TimeSig` |
| Add gatra | Existing append-measure action |
| Remove gatra | `NotationInteraction::removeSelectedMeasures()` |
| Panjangkan | `doubleNoteInputDuration()` |
| Pendekkan | `halveNoteInputDuration()` |
| Rhythm | Existing `Chord.durationType`, `Stem`, and `Beam` |
| Save/reopen | `TWrite::write(StaffType*)` and `TRead::read(StaffType*)` |

For M1:

```text
1 gatra = 1 MuseScore measure = 4 primary pulses
```

Do not replace global numeric shortcuts yet. Start with clickable degree
buttons.

## MSCX

The existing writer stores the new preset name:

```xml
<StaffType group="pitched">
  <name>stdGamelanSlendro</name>
  <lines>1</lines>
</StaffType>
```

No new per-note `<degree>` field is needed because degree is derived from
`Note.pitch`.

## Minimal Harness

### Unit test

Add `src/engraving/tests/gamelannotation_tests.cpp`.

Test:

- Valid `1 2 3 5 6`.
- Reject `4` and `7`.
- Degree/pitch mapping.
- Sléndro staff preset configuration.

### Score fixture

Add `src/engraving/tests/gamelan_data/slendro-basic.mscx`:

```text
4/4
Measure 1: 1 2 3 5
Measure 2: 6 5 . 3
```

After save/reopen, verify the preset, time signature, measures, pitches,
durations, and dot.

### Layout and visual tests

- Assert all numeric notes have valid bounding boxes and the same Y.
- Assert dot, time signature, and barlines are laid out.
- Add `vtest/scores/gamelan-slendro-basic.mscx`.
- Compare number, line, dot, barline, and beam appearance with the reference.

## Development Order

1. Degree mapping and unit test.
2. Sléndro staff preset.
3. MSCX fixture.
4. Number layout/drawing and fixed Y.
5. Dot.
6. Validate existing beams as rhythm lines.
7. Saron instrument.
8. Numeric input UI.
9. Save/reopen and visual tests.

## Decide Before Coding

1. Are valid degrees exactly `1 2 3 5 6`?
2. Does `.` mean silence or continuation?
3. Is one gatra four pulses in M1?
4. How should each duration line look?
5. Are numbers above or on the horizontal line?

## Done

M1 is done when a composer can create a Saron Sléndro score, enter numeric
notes and rhythm, add/remove gatra, change duration and time signature,
save/reopen MSCX, and export the notation without affecting standard staves.
# RFC-001 — M1 Sléndro Numeric Notation for Saron

**Project codename:** MuseScore Kanibal / Sandikala  
**Status:** Proposed implementation plan  
**Milestone:** M1  
**Primary instrument:** Saron  
**Primary laras:** Sléndro  
**Implementation status:** No production implementation has been made by this RFC  
**Related product log:** `downloads/dev_log_m1_sandikala.md`

## 1. Purpose

This RFC converts the M1 product discussion into a concrete MuseScore
implementation plan.

M1 must let a composer:

1. Create a score containing a Saron staff.
2. Select the Sléndro notation preset.
3. Enter the numeric degrees `1`, `2`, `3`, `5`, and `6`.
4. See every number on one fixed vertical level.
5. Use real rhythmic durations and beams/lines.
6. Display a dot for an empty pulse/rest.
7. Add and remove gatra/measure blocks.
8. Use and change a time signature.
9. Shorten and lengthen a note.
10. Save, close, and reopen the score as MSCX without data loss.

Authentic Saron sound is explicitly deferred. The M1 data model must still use
real MuseScore notes so playback can be added later without replacing the
notation model.

## 2. Executive Technical Decision

Implement Sléndro notation as a new **standard-group staff preset**:

```text
StaffGroup::STANDARD
    |
    `-- StaffTypes::GAMELAN_SLENDRO
            xmlName: stdGamelanSlendro
            lines: 1
            clef: hidden
            key signature: hidden
            time signature: shown
            barlines: shown
            ledger lines: hidden
```

Do not add a new `StaffGroup` in M1. `StaffGroup` currently controls broad
behaviour for standard, percussion, and tablature staves. Adding another group
would create a much larger compatibility and switch-statement blast radius.

Do not implement the numbers as independent `Text` elements. The displayed
number must be the visual representation of a real `Note`.

Do not model Saron as fake guitar tablature. Tablature is a useful reference,
but its string/fret semantics are wrong for gamelan.

## 3. Reuse Versus New Behaviour

```text
Existing MuseScore object       M1 usage
---------------------------------------------------------------
Score                           Source document
Part/Instrument                 Saron
Staff                           One Saron row
StaffType                       Sléndro numeric display mode
Measure                         One gatra block in M1
TimeSig                         Birama/time signature
Chord                           Rhythmic event container
Note                            Sléndro degree and future sounding pitch
Rest                            Empty pulse, drawn as a dot
Stem/Beam                       Basic rhythmic lines above numbers
Undo stack                      All editing operations
MSCX reader/writer              Persistence
```

New M1 behaviour:

```text
New staff preset
Pitch <-> Sléndro degree helper
Numeric Note layout and drawing branch
Dot Rest layout and drawing branch
Fixed vertical position
Thin numeric input surface
Saron instrument template
Dedicated tests and visual fixture
```

## 4. User-Visible M1 Model

```text
Score: "Untitled Sléndro"
Composer: "..."
Time signature: 4/4

Saron | 1    2    3    5 | 6    .    3    2 |
      +------ Gatra 1 ----+------ Gatra 2 ----+
```

The M1 input palette is:

```text
[1] [2] [3] [5] [6] [.] [Shorter] [Longer] [+ Gatra] [- Gatra]
```

For M1, the laras choice appears as `Saron (Sléndro)` in instrument selection.
There is no benefit in displaying a dropdown containing only one enabled
choice. When Pélog is implemented, introduce a real `Laras` selector.

## 5. Domain Assumptions Requiring Validation

The following are working software assumptions, not final musicological
decisions:

1. The allowed M1 degrees are exactly `1`, `2`, `3`, `5`, and `6`.
2. One gatra maps to one MuseScore measure.
3. The default measure is `4/4` with four primary pulses.
4. A visible `.` initially maps to a MuseScore `Rest`.
5. Sléndro octave/register dots are out of M1.
6. M1 Saron entry is monophonic: one note per chord.
7. Cross-measure note duration is not a primary M1 workflow.

The dot assumption is the largest semantic risk. If a dot means continuation
rather than silence, the implementation must use a continuation-aware model
instead of permanently treating it as a rest.

## 6. Internal Data Representation

### 6.1 Keep `Note` as the source of truth

```text
Note.pitch = 60
Chord.durationType = quarter
StaffType = stdGamelanSlendro
             |
             +-- renderer draws "1"
             +-- MSCX saves a real Note
             `-- later playback resolves a Saron sound
```

The initial reference pitch mapping is:

| Degree | Stored pitch | Pitch class | M1 purpose |
|---:|---:|---:|---|
| 1 | 60 | C | Stable internal representative |
| 2 | 62 | D | Stable internal representative |
| 3 | 64 | E | Stable internal representative |
| 5 | 67 | G | Stable internal representative |
| 6 | 69 | A | Stable internal representative |

This is not a claim that Sléndro is equal-tempered. The values are stable
logical representatives for M1. A later tuning layer may add cents offsets or
sample-specific playback mappings.

### 6.2 Proposed helper API

Add:

```text
src/engraving/dom/gamelannotation.h
src/engraving/dom/gamelannotation.cpp
```

Proposed declarations:

```cpp
namespace mu::engraving::GamelanNotation {

enum class Laras {
    Slendro
};

bool isValidDegree(Laras laras, int degree);
std::optional<int> degreeForPitch(Laras laras, int pitch);
std::optional<int> pitchForDegree(Laras laras, int degree, int octave);
String degreeText(Laras laras, const Note& note);

}
```

M1 only uses `Laras::Slendro`. The enum creates a testable boundary without
adding Pélog behaviour prematurely.

Expected behaviour:

```text
degreeForPitch(Slendro, 60) -> 1
degreeForPitch(Slendro, 62) -> 2
degreeForPitch(Slendro, 64) -> 3
degreeForPitch(Slendro, 67) -> 5
degreeForPitch(Slendro, 69) -> 6
degreeForPitch(Slendro, 61) -> no value
```

The helper must not depend on painting, QML, or application state. That makes
it suitable for the lowest-level unit-test harness.

## 7. Existing Call Paths

### 7.1 Visual layout

Current chord layout entry:

```text
ChordLayout::layout()
    |
    +-- ChordLayout::layoutTablature()
    |
    `-- ChordLayout::layoutPitched()
            |
            +-- TLayout::layoutNote()
            `-- ChordLayout::layoutChords3()
```

Verified files:

- `src/engraving/rendering/score/chordlayout.cpp`
- `src/engraving/rendering/score/tlayout.cpp`
- `src/engraving/rendering/score/tdraw.cpp`

### 7.2 Drawing

```text
TDraw::draw(const EngravingItem*)
    |
    `-- ElementType::NOTE
            |
            `-- TDraw::draw(const Note*)
```

### 7.3 Note input

```text
NotationActionController::handleNoteAction()
    |
    `-- NotationNoteInput::addNote()
            |
            `-- NoteInput::addPitch()
```

Verified files:

- `src/notationscene/internal/notationactioncontroller.cpp`
- `src/notation/internal/notationnoteinput.cpp`
- `src/engraving/editing/noteinput.cpp`

### 7.4 Save and reopen

```text
TWrite::write(const Staff*)
    |
    `-- TWrite::write(const StaffType*)

TRead::read(Staff*)
    |
    `-- TRead::read(StaffType*)
```

Verified files:

- `src/engraving/rw/write/twrite.cpp`
- `src/engraving/rw/read500/tread.cpp`

## 8. Function-by-Function Change Plan

The table distinguishes existing functions from proposed functions.

| Area | Existing or proposed function | Planned responsibility |
|---|---|---|
| Staff type enum | `StaffTypes` | Add `GAMELAN_SLENDRO` before `STAFF_TYPES` |
| Staff preset | `StaffType::initStaffTypes()` | Register `stdGamelanSlendro` |
| Preset lookup | `StaffType::type()` | Map XML name to enum |
| Detection | proposed `StaffType::isGamelanStaff()` | True only for gamelan numeric presets |
| Staff convenience | proposed `Staff::isGamelanStaff(const Fraction&)` | Resolve staff type at a tick |
| Pitch mapping | proposed `GamelanNotation::degreeForPitch()` | Convert stored pitch to number |
| Input mapping | proposed `GamelanNotation::pitchForDegree()` | Convert number to stored pitch |
| Note metrics | `TLayout::layoutNote()` | Use text metrics and numeric bounding box |
| Note placement | `ChordLayout::layoutChords3()` | Force numeric notes to one fixed Y |
| Note drawing | `TDraw::draw(const Note*)` | Draw the numeric degree |
| Rest metrics | `RestLayout::layoutRest()` | Use a dot bounding box on gamelan staff |
| Rest drawing | `TDraw::draw(const Rest*)` | Draw `.` instead of Western rest symbol |
| Numeric action | proposed `NotationActionController::handleGamelanDegreeAction()` | Validate staff and enter mapped pitch |
| UI visibility | proposed `GamelanInputModel::isGamelanStaff()` | Show palette only on gamelan staff |
| UI input | proposed `GamelanInputModel::inputDegree()` | Dispatch degree input action |
| Duration | existing `doubleNoteInputDuration()` | Reuse for Panjangkan |
| Duration | existing `halveNoteInputDuration()` | Reuse for Pendekkan |
| Add gatra | existing `Score::appendMeasures()` path | Reuse append-measure action |
| Remove gatra | existing `NotationInteraction::removeSelectedMeasures()` | Reuse undoable measure removal |
| Time signature | existing `TimeSig` behaviour | No new time-signature domain object |
| Persistence | `TWrite::write(const StaffType*)` | Existing XML name preserves the preset |
| Persistence | `TRead::read(StaffType*)` | Existing name restores the preset |

## 9. Staff Type Changes

### 9.1 `src/engraving/dom/stafftype.h`

Add the new preset enum at the end of the real preset list:

```cpp
enum class StaffTypes : signed char {
    STANDARD,
    // existing percussion and tablature values...
    GAMELAN_SLENDRO,
    STAFF_TYPES,
};
```

Add:

```cpp
bool isGamelanStaff() const
{
    return type() == StaffTypes::GAMELAN_SLENDRO;
}
```

Do not change `StaffGroup`.

### 9.2 `src/engraving/dom/stafftype.cpp`

Update `StaffType::type()`:

```cpp
{ u"stdGamelanSlendro", StaffTypes::GAMELAN_SLENDRO },
```

Append the matching preset in `StaffType::initStaffTypes()` in exactly the same
ordinal position as the enum:

```text
group             STANDARD
xmlName           stdGamelanSlendro
display name      Gamelan numeric (Sléndro)
lines             1
step offset       0
line distance     1
generate clef     false
show barlines     true
stemless          false
generate timesig  true
generate keysig   false
ledger lines      false
invisible         false
```

`stemless` remains false because existing MuseScore `Stem` and `Beam` objects
are candidates for the M1 rhythm-line representation.

### 9.3 `src/engraving/dom/staff.h` and `staff.cpp`

Add a convenience method analogous to `isTabStaff()`:

```cpp
bool Staff::isGamelanStaff(const Fraction& tick) const
{
    return staffType(tick)->isGamelanStaff();
}
```

This avoids repeating XML-name or enum checks throughout layout, drawing, and
UI state code.

## 10. Saron Instrument Template

### 10.1 `share/instruments/instruments.xml`

Add a pitched-percussion/world instrument:

```xml
<Instrument id="saron-slendro">
  <family>keyboard-percussion</family>
  <trackName>Saron</trackName>
  <longName>Saron</longName>
  <shortName>Sar.</shortName>
  <description>Javanese Saron using Sléndro numeric notation.</description>
  <stafftype staffTypePreset="stdGamelanSlendro">pitched</stafftype>
  <barlineSpan>1</barlineSpan>
  <aPitchRange>60-69</aPitchRange>
  <pPitchRange>60-69</pPitchRange>
  <genre>world</genre>
</Instrument>
```

The exact family, range, name, and MusicXML ID require review.

Sound is not an M1 acceptance requirement. If MuseScore requires a fallback
channel, use an explicitly documented temporary General MIDI metallophone or
vibraphone program. Do not call that authentic Saron playback.

### 10.2 Generated instrument data warning

`share/instruments/instruments.xml` and
`share/instruments/instrumentsxml.h` are generated by:

```text
share/instruments/update_instruments_xml.py
```

For a local prototype, the fork can edit the generated resources deliberately.
For an upstream-quality change, the canonical instrument spreadsheet and
generation workflow must be followed. Running the generator may require network
access and may replace unrelated generated content, so it must not be run
casually during the first renderer spike.

## 11. Numeric Note Layout

### 11.1 `TLayout::layoutNote()`

Current behaviour chooses between tablature fret text and a standard SMuFL
notehead.

Add a gamelan branch before the standard notehead branch:

```text
if tablature:
    existing tablature metrics
else if gamelan:
    resolve degree text
    calculate font metrics
    set numeric bounding box
else:
    existing standard notehead metrics
```

Candidate implementation responsibilities:

1. Resolve the staff type using `staffTypeForElement()`.
2. Call `GamelanNotation::degreeText()`.
3. Use a stable music/text font selected by design review.
4. Store enough cached layout data to avoid measuring differently in draw.
5. Set `Note::LayoutData::bbox` to the number glyph bounds.
6. Ensure the bounding box is selectable with the mouse.
7. Ensure horizontal spacing uses the numeric width.

Avoid mutating `Note.pitch` during layout. Layout must be deterministic and
read-only with respect to musical data.

### 11.2 Numeric width helpers

Existing `Note::headWidth()`, `headBodyWidth()`, and `headHeight()` assume a
SMuFL notehead except for tablature.

Two possible implementations:

**Preferred M1 approach**

Add gamelan-aware branches in:

```cpp
Note::headWidth()
Note::headBodyWidth()
Note::headHeight()
```

The branches should return the already-computed numeric bounding-box metrics.

**Alternative**

Change only the relevant chord layout calculations to use
`note->ldata()->bbox().width()`.

The preferred approach has a broader but more coherent contract. It must be
covered by regression tests because these helpers are used by stems, ties,
spacing, and collision logic.

## 12. Fixed Vertical Placement

`ChordLayout::layoutChords3()` currently calculates:

```cpp
double ny = (note->line() + stepOffset) * stepDistance;
```

For a gamelan staff, use a fixed staff-relative Y:

```cpp
double ny = isGamelanStaff
    ? gamelanNumberBaselineY
    : (note->line() + stepOffset) * stepDistance;
```

The first layout constant should be local and explicit, for example:

```cpp
static constexpr Spatium GAMELAN_NUMBER_Y = -0.75_sp;
```

The exact visual value must be selected using a rendered fixture, not guessed
from unit tests.

All Sléndro pitches must produce the same `Note::pos().y()`:

```text
pitch 60 -> y = fixed
pitch 62 -> y = fixed
pitch 64 -> y = fixed
pitch 67 -> y = fixed
pitch 69 -> y = fixed
```

M1 is monophonic. If a chord contains multiple simultaneous Sléndro notes, the
renderer must avoid crashing but may display a warning or overlapping degrees
until a later policy is defined.

## 13. Numeric Note Drawing

### 13.1 `TDraw::draw(const Note*)`

Add a branch before normal notehead drawing:

```text
if gamelan:
    draw degree text using the same font and metrics as layout
    return
```

Required behaviour:

- Respect `item->curColor(opt)` for normal, selected, and printing states.
- Respect score/staff magnification.
- Draw at the layout origin used to construct the bounding box.
- Do not draw a Western notehead.
- Keep generated stems and beams visible.
- Do not change sound/playback behaviour.

The clicked number remains a `Note`, so the existing path:

```text
NotationViewInputController::handleLeftClick()
    -> hitElement->isPlayable()
    -> PlaybackController::playElements()
```

continues to work when playback is added.

## 14. Rhythm Lines

M1 should first reuse existing:

```text
Chord.durationType
Stem
Beam
BeamMode
```

Reason:

- Duration editing already exists.
- Beam grouping already exists.
- Horizontal spacing already accounts for beams.
- Undo, save, reopen, and printing already work.

Initial renderer spike:

1. Keep stems and beams enabled.
2. Force the numeric note Y position.
3. Verify that upward stems place beams above the numbers.
4. Compare the result with the reference notation.

Only add a new generated `GamelanDurationSymbol` if existing stems/beams cannot
represent the required notation after the spike.

Do not reuse `TabDurationSymbol` directly in production M1. Its API and data
members explicitly encode tablature assumptions. Its grid-beam implementation
in `stafftype.h`, `stafftype.cpp`, `TLayout::layoutTabDurationSymbol()`, and
`TDraw::draw(const TabDurationSymbol*)` is valuable reference code.

## 15. Dot/Rest Layout and Drawing

### 15.1 `RestLayout::layoutRest()`

Add an early gamelan branch:

```text
if gamelan staff:
    compute "." text or bullet metrics
    position at the numeric baseline
    set the rest bbox
    skip the Western rest symbol path
```

### 15.2 `TDraw::draw(const Rest*)`

Add:

```text
if gamelan staff:
    draw "."
    return
```

The layout and drawing code must share the same glyph, font, scale, and origin.

Open semantic gate:

```text
If "." means rest:
    Rest is valid.

If "." means continuation:
    Do not merge continuation semantics into Rest.
    Introduce an explicit continuation representation or rendering rule.
```

Implementation of dot entry must wait until this rule is validated.

## 16. Numeric Input

### 16.1 Do not steal global duration shortcuts initially

MuseScore already uses number-key-related actions for durations. M1 should not
globally remap `1`, `2`, `3`, `5`, and `6` until shortcut context behaviour is
specified.

First provide clickable numeric buttons visible only for a gamelan staff.

### 16.2 Proposed command definitions

In `src/notationscene/notationcommands.h`, add fixed commands:

```cpp
ENTER_GAMELAN_1_COMMAND
ENTER_GAMELAN_2_COMMAND
ENTER_GAMELAN_3_COMMAND
ENTER_GAMELAN_5_COMMAND
ENTER_GAMELAN_6_COMMAND
```

Fixed commands integrate more simply with the existing action registry than
passing a complex `NoteInputParams` object from QML.

### 16.3 `NotationActionController`

Add:

```cpp
void handleGamelanDegreeAction(int degree);
bool isGamelanDegreeInputAllowed() const;
```

Planned algorithm:

```text
1. Resolve current notation and note-input staff.
2. Verify Staff::isGamelanStaff(current tick).
3. Validate degree with GamelanNotation::isValidDegree().
4. Convert degree to a representative pitch.
5. Build NoteInputParams.
6. Call existing handleNoteAction(params, NextChord).
7. Allow existing NotationNoteInput::addNote() and NoteInput::addPitch().
```

Do not duplicate transaction or undo logic. The new command must join the
existing note-input path before `NotationNoteInput::addNote()`.

### 16.4 Proposed QML/model files

Add:

```text
src/notationscene/qml/MuseScore/NotationScene/GamelanInputBar.qml
src/notationscene/qml/MuseScore/NotationScene/gamelaninputmodel.h
src/notationscene/qml/MuseScore/NotationScene/gamelaninputmodel.cpp
```

Register them in:

```text
src/notationscene/qml/MuseScore/NotationScene/CMakeLists.txt
```

Proposed model API:

```cpp
Q_PROPERTY(bool isGamelanStaff READ isGamelanStaff NOTIFY isGamelanStaffChanged)

Q_INVOKABLE void inputDegree(int degree);
Q_INVOKABLE void inputDot();
Q_INVOKABLE void appendGatra();
Q_INVOKABLE void removeSelectedGatra();
Q_INVOKABLE void shortenDuration();
Q_INVOKABLE void lengthenDuration();
```

The model should inject `muse::actions::IActionsDispatcher` and dispatch
existing commands whenever possible:

```text
Shorten     -> "half-duration"
Lengthen    -> "double-duration"
Add gatra   -> "append-measure"
Remove      -> existing selected-measure removal action
```

The QML view should contain text buttons for `1`, `2`, `3`, `5`, `6`, and `.`
rather than inventing toolbar icons.

## 17. Time Signature

No new time-signature model is required.

The new staff preset sets:

```text
genTimesig = true
```

Reuse:

- Existing `TimeSig` DOM object.
- Existing palette or properties UI.
- Existing measure tick calculations.
- Existing MSCX read/write.

Harness requirements:

1. A `4/4` time signature appears on the one-line Sléndro staff.
2. Changing to `3/4` updates measure duration.
3. Numeric notes remain on the fixed Y position.
4. Save/reopen preserves the new time signature.

Do not relabel the underlying `TimeSig` object as `Gatra`. The user-facing
workflow may say `Gatra`, but MuseScore's structural object remains `Measure`.

## 18. Add/Remove Gatra

Reuse existing measure operations:

```text
Add gatra
    -> existing append-measure action
    -> Score::appendMeasures(1)

Remove gatra
    -> select complete measure
    -> NotationInteraction::removeSelectedMeasures()
```

The gamelan UI may label these actions:

```text
+ Gatra
- Gatra
```

No new `Gatra` DOM class is required in M1.

Negative case:

- Removing a non-empty gatra must remain undoable.
- The UI must not silently remove only part of a measure.
- Removing the last remaining measure must follow existing MuseScore safety
  behaviour.

## 19. Perpanjang/Perpendek

Reuse existing functions in
`src/notationscene/internal/notationactioncontroller.cpp`:

```cpp
NotationActionController::doubleNoteInputDuration()
NotationActionController::halveNoteInputDuration()
```

These already choose between:

- Changing the active note-input duration.
- Changing the selected note/rest duration through
  `NotationInteraction::increaseDecreaseDuration()`.

The gamelan UI adds translated labels only:

```text
Panjangkan -> double-duration
Pendekkan  -> half-duration
```

Do not create a second duration system.

## 20. MSCX Persistence

The existing writer already serializes:

```xml
<StaffType group="pitched">
  <name>stdGamelanSlendro</name>
  ...
</StaffType>
```

The existing reader calls:

```cpp
TRead::read(StaffType*)
```

and reads the `name` into `StaffType::xmlName`.

M1 therefore does not require a new per-note `<degree>` tag because:

```text
degree = Sléndro mapping of Note.pitch
laras  = staff preset stdGamelanSlendro
```

Expected M1 MSCX fragment:

```xml
<StaffType group="pitched">
  <name>stdGamelanSlendro</name>
  <lines>1</lines>
  <clef>0</clef>
  <keysig>0</keysig>
  <ledgerlines>0</ledgerlines>
</StaffType>
```

Exact output must be generated by `TWrite`; tests should not depend on
handwritten tag ordering unless using a golden file.

Forward-looking note:

When multiple laras values or custom tuning libraries are added, introduce an
explicit versioned property such as:

```xml
<gamelanLaras>slendro</gamelanLaras>
```

Do not add that field in M1 unless there is more than one meaningful value or
the preset name proves insufficient.

## 21. Source File Change Matrix

### Required for the first renderer vertical slice

| File | Change |
|---|---|
| `src/engraving/dom/stafftype.h` | Add preset enum and `isGamelanStaff()` |
| `src/engraving/dom/stafftype.cpp` | Map XML name and register preset |
| `src/engraving/dom/staff.h` | Declare convenience detector |
| `src/engraving/dom/staff.cpp` | Implement convenience detector |
| `src/engraving/dom/gamelannotation.h` | Add mapping API |
| `src/engraving/dom/gamelannotation.cpp` | Implement mapping |
| `src/engraving/CMakeLists.txt` or relevant source list | Compile new helper |
| `src/engraving/rendering/score/tlayout.cpp` | Numeric bbox |
| `src/engraving/rendering/score/chordlayout.cpp` | Fixed Y |
| `src/engraving/rendering/score/tdraw.cpp` | Draw number and dot |
| `src/engraving/rendering/score/restlayout.cpp` | Dot/rest bbox |

### Required for Saron selection

| File | Change |
|---|---|
| `share/instruments/instruments.xml` | Add Saron Sléndro template |
| `share/instruments/instrumentsxml.h` | Generated translation strings if required |

### Required for the numeric input palette

| File | Change |
|---|---|
| `src/notationscene/notationcommands.h` | Add degree commands |
| `src/notationscene/internal/notationcommandsregister.cpp` | Register command metadata |
| `src/notationscene/internal/notationactioncontroller.h` | Declare handlers |
| `src/notationscene/internal/notationactioncontroller.cpp` | Map degree actions to note input |
| `src/notationscene/internal/notationuiactions.cpp` | User-facing action names if needed |
| `src/notationscene/qml/MuseScore/NotationScene/GamelanInputBar.qml` | Numeric controls |
| `src/notationscene/qml/MuseScore/NotationScene/gamelaninputmodel.h` | Model interface |
| `src/notationscene/qml/MuseScore/NotationScene/gamelaninputmodel.cpp` | State and command dispatch |
| `src/notationscene/qml/MuseScore/NotationScene/CMakeLists.txt` | Register new source/QML |
| `src/appshell/qml/MuseScore/AppShell/NotationPage/NotationPage.qml` | Place conditional input bar if required |

### Expected reuse without domain modification

| Feature | Existing implementation |
|---|---|
| Time signature | `TimeSig` and existing palette/properties |
| Add measure | Existing insert/append-measure actions |
| Remove measure | `NotationInteraction::removeSelectedMeasures()` |
| Shorter/longer | `halveNoteInputDuration()` / `doubleNoteInputDuration()` |
| Undo/redo | Existing transaction and undo stack |
| Title/composer | Existing score text/meta workflow |
| Click selection | `NotationViewInputController` hit testing |
| Future click playback | `PlaybackController::playElements()` |

## 22. Test Harness Strategy

The harness must isolate four different risks:

```text
Mapping correctness
        |
        v
DOM + MSCX persistence
        |
        v
Layout/drawing correctness
        |
        v
Full application workflow
```

One test type cannot reliably cover all four.

### 22.1 Harness A — Pure mapping unit tests

Add:

```text
src/engraving/tests/gamelannotation_tests.cpp
```

Register it in:

```text
src/engraving/tests/CMakeLists.txt
```

Test cases:

```cpp
TEST(GamelanNotationTests, acceptsSlendroDegrees)
TEST(GamelanNotationTests, rejectsFourAndSeven)
TEST(GamelanNotationTests, mapsDegreeToPitch)
TEST(GamelanNotationTests, mapsPitchToDegree)
TEST(GamelanNotationTests, preservesDegreeAcrossOctaves)
TEST(GamelanNotationTests, rejectsUnsupportedPitchClass)
```

This test must not create a GUI, painter, or complete score.

### 22.2 Harness B — Staff preset unit tests

Test:

```text
StaffType::preset(GAMELAN_SLENDRO) is not null
xmlName == stdGamelanSlendro
group == STANDARD
lines == 1
genClef == false
genTimesig == true
genKeysig == false
showBarlines == true
showLedgerLines == false
isGamelanStaff == true
```

Also verify that all existing preset enum ordinals still map to the intended
preset. This protects the enum/vector synchronization requirement documented in
`stafftype.h`.

### 22.3 Harness C — MSCX round-trip fixture

Add:

```text
src/engraving/tests/gamelan_data/slendro-basic.mscx
src/engraving/tests/gamelan_data/slendro-basic-ref.mscx
```

The fixture contains:

```text
Title: Sléndro Harness
Composer: Test
Instrument: Saron
Staff preset: stdGamelanSlendro
Time signature: 4/4
Measure 1: quarter 1, quarter 2, quarter 3, quarter 5
Measure 2: eighth 6, eighth 5, quarter rest/dot, half 3
```

Add a test using existing utilities:

```cpp
MasterScore* score = ScoreRW::readScore(...);
EXPECT_TRUE(score);
EXPECT_TRUE(score->staff(0)->isGamelanStaff(Fraction(0, 1)));
EXPECT_TRUE(ScoreComp::saveCompareScore(score, ..., ...));
```

Assertions after reload:

- Staff preset remains gamelan.
- Time signature remains `4/4`.
- Measure count remains two.
- Pitches remain correct.
- Durations remain correct.
- Rest remains present.
- Title and composer remain present.

### 22.4 Harness D — Layout assertions

Using the same fixture:

1. Run score layout.
2. Resolve notes for degrees `1`, `2`, `3`, `5`, and `6`.
3. Assert every note has a valid bbox.
4. Assert every note has the same Y position within a small tolerance.
5. Assert numeric widths are positive.
6. Assert the rest/dot has a valid bbox.
7. Assert the time signature has a valid bbox.
8. Assert measure barlines are present.

Do not assert exact pixel coordinates in a normal unit test. Font and platform
differences make exact pixels fragile.

### 22.5 Harness E — Draw/visual regression

Add:

```text
vtest/scores/gamelan-slendro-basic.mscx
```

Use the repository's existing `vtest` workflow, whose purpose is comparing
current draw data with reference data.

The visual reference must cover:

- One-line staff.
- Numeric degrees `1 2 3 5 6`.
- Fixed vertical alignment.
- A visible `4/4`.
- Measure barline.
- Quarter, eighth, and sixteenth durations.
- A beamed group.
- A dot/rest.
- Selected-state appearance checked manually, because print draw data will not
  necessarily contain UI selection colour.

Never approve a new visual reference merely because the output changed.
Compare it against the RFC screenshot and design intent first.

### 22.6 Harness F — Input/controller tests

Add tests around the action controller or input model:

```text
Degree 1 dispatches representative pitch 60
Degree 5 dispatches representative pitch 67
Degree 4 is rejected
Degree action is disabled on standard staff
Degree action is enabled on stdGamelanSlendro
Shorter dispatches half-duration
Longer dispatches double-duration
Add gatra dispatches append-measure
```

Candidate test location:

```text
src/notationscene/qml/MuseScore/NotationScene/tests/
```

Reuse the existing notation-scene mocks where possible rather than booting the
full application.

### 22.7 Harness G — Manual application script

Manual acceptance sequence:

```text
1. Launch the locally built Debug application.
2. Create a new score.
3. Search for and add Saron (Sléndro).
4. Enter title and composer.
5. Confirm the time signature is visible.
6. Enter 1 2 3 5.
7. Add a second gatra.
8. Enter 6 5 . 3.
9. Shorten one note.
10. Lengthen another note.
11. Undo and redo both edits.
12. Save as MSCX.
13. Close the score.
14. Reopen the MSCX file.
15. Confirm numbers, dot, durations, title, composer, barlines, and time
    signature.
16. Export PDF and inspect the result.
```

Negative manual cases:

```text
Try entering 4
Try entering 7
Try using numeric input on a piano staff
Try deleting a non-empty gatra and undoing
Try changing 4/4 to 3/4 with existing notes
Try opening the MSCX in an unmodified MuseScore build
```

## 23. Proposed Implementation Slices

Each slice must end in a runnable, testable state.

### Slice 0 — Test fixture and mapping contract

Files:

```text
gamelannotation.h/.cpp
gamelannotation_tests.cpp
gamelan_data/slendro-basic.mscx
```

Exit gate:

- Mapping tests pass.
- Musical expert confirms allowed numbers and preliminary dot meaning.

### Slice 1 — Staff preset and numeric note renderer

Files:

```text
stafftype.h/.cpp
staff.h/.cpp
tlayout.cpp
chordlayout.cpp
tdraw.cpp
```

Exit gate:

- Existing score fixture renders `1 2 3 5 6`.
- All numbers share one Y position.
- Numbers are clickable/selectable as `Note`.
- Existing standard and tablature fixtures remain unchanged.

### Slice 2 — Rhythm line and dot

Files:

```text
chordlayout.cpp
stem/beam code only if required
restlayout.cpp
tdraw.cpp
```

Exit gate:

- Eighth and sixteenth durations produce accepted upper rhythm lines.
- Dot/rest has correct alignment.
- Print output matches the visual reference.

### Slice 3 — Saron creation workflow

Files:

```text
instruments.xml
instrumentsxml.h if generated
instrument tests/fixture
```

Exit gate:

- New score instrument search finds Saron.
- Adding Saron produces `stdGamelanSlendro`.
- Time signature is visible.

### Slice 4 — Numeric input palette

Files:

```text
notationcommands.h
notationcommandsregister.cpp
notationactioncontroller.h/.cpp
GamelanInputBar.qml
gamelaninputmodel.h/.cpp
NotationScene CMakeLists.txt
NotationPage.qml if placement is external
```

Exit gate:

- Buttons enter only valid degrees.
- Buttons disappear or disable on non-gamelan staves.
- Entry uses normal MuseScore undo/redo.

### Slice 5 — Gatra and duration controls

Mostly reuse:

```text
append-measure
removeSelectedMeasures()
double-duration
half-duration
```

Exit gate:

- UI labels dispatch existing commands.
- No duplicate measure or duration engine exists.

### Slice 6 — Persistence and acceptance

Exit gate:

- Save/reopen round-trip passes.
- Visual regression passes.
- PDF export is accepted.
- Negative cases do not crash or corrupt the score.

## 24. Regression Surface

Files such as `Note::headWidth()`, `TLayout::layoutNote()`,
`ChordLayout::layoutChords3()`, and `TDraw::draw(const Note*)` affect many
notation types.

Required regression fixtures:

- Standard five-line pitched staff.
- One-line percussion staff.
- Tablature staff.
- Chords with multiple notes.
- Beamed eighth and sixteenth notes.
- Grace notes.
- Notes with ties.
- Notes with augmentation dots.
- Printing/export.

Every new branch must be guarded by `isGamelanStaff()` and leave the existing
path byte-for-byte or visually unchanged where possible.

## 25. Compatibility Behaviour

### New build opening old scores

Expected: no change. Existing scores do not contain `stdGamelanSlendro`.

### Old build opening new M1 score

Risk: an older build will not recognize the new preset name and may fall back
to a standard staff while preserving pitches and durations.

Required test:

1. Save M1 score.
2. Open it in an unmodified build.
3. Confirm it does not crash.
4. Document that numeric rendering requires the modified build.

### Import/export

M1 guarantees:

- MSCX save/reopen in the modified build.
- PDF/print visual export.

M1 does not guarantee semantic MusicXML export of Sléndro notation.

## 26. Build and Run Loop

After the local Qt kit is configured:

```bash
cd /Users/nurrizky/dev/MuseScore

cmake -P build.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -G Ninja

cmake -P build.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -G Ninja \
  run
```

Current environment observation at RFC-writing time:

```text
cmake installed
ninja installed
qmake/qmake6 not found in PATH
```

The build harness cannot run until Qt's `qmake` is available through `PATH` or
`QTDIR`.

## 27. Logging During Development

Temporary diagnostic logging may be added around the gamelan-only branches:

```text
staff preset detected
degree resolved
representative pitch entered
numeric bbox calculated
fixed Y assigned
MSCX preset restored
```

Do not leave per-note layout/draw logging enabled by default. Layout and drawing
run frequently and can produce excessive output.

Prefer assertions in tests over permanent logs.

## 28. Risks and Mitigations

| Risk | Consequence | Mitigation |
|---|---|---|
| Dot semantics are wrong | Corrupted musical meaning | Domain validation gate before final rest implementation |
| `headWidth()` change affects standard notation | Layout regressions | Strict gamelan branch and visual regressions |
| Beam appearance differs from reference | M1 looks Western | Renderer spike before new duration element |
| New `StaffGroup` causes broad changes | Large unstable patch | Keep `StaffGroup::STANDARD` |
| Fake tablature leaks string semantics | Incorrect editing model | Dedicated staff preset, not TAB |
| Numeric keys conflict with durations | Broken shortcuts | Clickable palette first |
| Generated instruments file overwritten | Lost Saron template | Document generator/canonical source |
| Old MuseScore cannot render preset | Compatibility limitation | Preserve real notes and document fallback |
| Pitch mapping implies equal temperament | Wrong future sound | Treat pitches as logical representatives only |
| Multiple notes in a chord overlap | Bad rendering | M1 monophonic rule and defensive handling |

## 29. Open Decisions Before Coding

Blocking:

1. Confirm valid Sléndro degrees.
2. Confirm the exact meaning of `.`.
3. Confirm whether one gatra equals four M1 pulses.
4. Confirm the required rhythm-line convention for quarter, eighth, and
   sixteenth durations.
5. Confirm whether the number sits above the horizontal staff line or replaces
   the line at its position.

Non-blocking for the first renderer spike:

1. Final Saron sound.
2. Regional tuning.
3. Pélog.
4. Pathet.
5. Octave dots.
6. Other instruments.

## 30. Definition of Ready

Implementation may begin when:

- Product scope in `dev_log_m1_sandikala.md` is accepted.
- The five blocking musical decisions above are answered.
- A minimal `.mscx` fixture is agreed upon.
- The Qt build environment runs a clean Debug build.
- The first visual reference image is selected.

## 31. M1 Definition of Done

M1 is done when:

1. `Saron (Sléndro)` is available during score creation.
2. The resulting staff shows time signatures and barlines but no clef or key
   signature.
3. Degrees `1`, `2`, `3`, `5`, and `6` are entered as real MuseScore notes.
4. All numeric pitches appear at one vertical position.
5. Basic duration lines are visually accepted.
6. A validated dot representation works.
7. Add/remove gatra uses normal measure operations.
8. Shorter/longer uses normal duration operations.
9. Undo and redo work.
10. Save, close, and reopen preserves the score.
11. PDF/print output preserves numeric notation.
12. Mapping, persistence, layout, input, and visual harnesses pass.
13. Existing standard, percussion, and tablature notation do not regress.

## 32. Recommended First Coding Task

Do not start with the complete UI.

Start with the smallest test-driven vertical slice:

```text
1. Add GamelanNotation mapping helper and tests.
2. Add stdGamelanSlendro staff preset.
3. Create a two-measure MSCX fixture using pitches 60, 62, 64, 67, and 69.
4. Make TLayout calculate numeric bounding boxes.
5. Make ChordLayout force one Y.
6. Make TDraw draw numbers.
7. Run unit and visual checks.
```

Only after the renderer is stable should the Saron template and numeric input
palette be added.