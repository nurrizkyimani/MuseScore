# Development Log M1 — Sandikala / MuseScore Kanibal

**RFC:** 1  
**Status:** Brainstorm / Draft  
**Focus:** Saron Sléndro numeric notation  
**M1 estimate:** 2 sprints / 4 weeks, followed by user testing  
**Sprint duration:** 2 weeks / approximately 8 working days

## Product Goal

Extend MuseScore so a gamelan composer can create, edit, save, reopen, and
print a Saron-only Sléndro composition using numeric notation.

M1 focuses on composition and visual notation. Authentic gamelan sound,
additional instruments, Pélog, and regional tuning libraries will be added in
later milestones.

The smallest useful M1 is:

> A native Sléndro numeric staff for Saron, using real MuseScore notes but
> displaying the notes as the numbers `1`, `2`, `3`, `5`, and `6`.

## References

- Epitaph — Sandikala Ensemble:
  <https://drive.google.com/file/d/1PQy17ftW2htxpddEFRbrk4U_-3PQVNvD/view?usp=sharing>
- Riddle Machine:
  <https://drive.google.com/file/d/1acQtgSELiK5Qplfn5vnagj8KEd8dmHiF/view?usp=sharing>
- Introduction by Pak Sumarsam:
  <https://sumarsam.faculty.wesleyan.edu/files/2023/01/INTRO_THEORY_ANALYSIS-.pdf>
- Gérongan and notation examples:
  <https://gamelanbvg.com/gendhing/gerongan.html>

Note: the Epitaph and Riddle Machine files are separate references. The product
requirements should not use the Riddle Machine URL as the Epitaph source.

## Problem Statement

Current MuseScore notation is designed primarily around Western staff
notation. A gamelan composer needs to compose using numeric notation without
manually placing unrelated text objects.

The composer should be able to:

1. Create an empty score.
2. Select Saron.
3. Select the Sléndro laras.
4. Enter title and composer.
5. Select a time signature.
6. Enter numeric pitches.
7. Enter and change rhythm.
8. Add and remove gatra/measure blocks.
9. Save, close, and reopen the score without losing information.

## Working Glossary

| Term | Working definition |
|---|---|
| Kepatihan notation | Numeric or cipher notation used for Javanese gamelan |
| Laras | Tuning system; M1 supports Sléndro and a later milestone adds Pélog |
| Sléndro | Five-degree tuning system represented in M1 by `1 2 3 5 6` |
| Pathet | Modal framework; it should not be treated as only a Western key |
| Balungan | Core or skeletal melodic line |
| Gatra | Common grouping of four balungan pulses |
| Measure/bar | MuseScore time container; M1 maps one gatra to one measure |
| Saron | Metallophone instrument used for the first milestone |
| Pulse | One primary input position inside a gatra/measure |
| Rhythm line | Mark above numbers representing rhythmic subdivision |
| Gatra block | User-facing block mapped internally to one MuseScore measure |

The glossary and the musical meaning of the notation must be validated with a
domain expert.

## M1 Scope

### Included

- Create a new empty gamelan score.
- Add or select one Saron instrument.
- Enter and edit score title.
- Enter and edit composer name.
- Select `Sléndro` as the laras.
- Select and change time signature.
- Default time signature of `4/4`.
- Map one gatra to one measure for M1.
- Enter Sléndro degrees `1`, `2`, `3`, `5`, and `6`.
- Enter a dot for an empty or continuation pulse.
- Select, replace, and delete an entered number.
- Add and remove complete gatra/measure blocks.
- Shorten and lengthen the selected note duration.
- Display basic rhythm lines above numeric notes.
- Undo and redo editing operations.
- Save as MSCX.
- Close and reopen the MSCX score correctly.
- Print or export with numeric notation preserved.

### Not included

- Instruments other than Saron.
- Authentic Saron playback samples.
- Gender, Gambang, or Siter.
- Regional tuning libraries.
- Pélog.
- Full pathet behaviour.
- Arbitrary variable-length gatra blocks.
- Complex octave markers above and below numbers.
- Dynamics, ornaments, curves, and all special marks from the reference scores.
- Complete compatibility with every existing MuseScore import/export format.

## Primary Composer Workflow

```text
New empty score
       |
       v
Select instrument: Saron
       |
       v
Select laras: Sléndro
       |
       v
Enter title and composer
       |
       v
Select time signature: 4/4 by default
       |
       v
Add gatra/measure blocks
       |
       v
Enter 1, 2, 3, 5, 6, or dot
       |
       v
Shorten or lengthen note duration
       |
       v
Save MSCX -> close -> reopen
```

## Proposed M1 Interface

```text
+------------------------------------------------------------------+
| Laras: [Sléndro v]  Birama: [4/4 v]  Instrumen: [Saron v]        |
+------------------------------------------------------------------+
| Input:     [1] [2] [3] [5] [6] [.]                              |
| Rhythm:    [Pendekkan nada] [Panjangkan nada]                    |
| Structure: [+ Gatra] [- Gatra]                                   |
+------------------------------------------------------------------+

Saron   |  1      2      3      5  |  6      .      3      2  |
        +--------- Gatra 1 ---------+--------- Gatra 2 ---------+
```

The terms `Gatra` and `Measure` may both appear initially:

```text
UI label:       Gatra
MuseScore data: Measure
```

## Time Signature

MuseScore's existing time-signature model should be reused.

M1 options:

```text
4/4  default
3/4  supported if existing MuseScore behaviour works correctly
2/4  supported if existing MuseScore behaviour works correctly
```

Initial mapping:

```text
1 gatra = 1 MuseScore measure = 4 primary pulses
```

This is an M1 implementation simplification. A gatra and a Western measure are
not assumed to be identical in every musical context.

## Sléndro Selection

Sléndro must be stored as a real score or staff setting, not merely as visible
text.

Conceptual model:

```text
Notation system: GAMELAN_NUMERIC
Laras:           SLENDRO
Instrument:      SARON
Valid degrees:   1, 2, 3, 5, 6
```

Selecting Sléndro controls:

- Valid input numbers.
- Numeric pitch mapping.
- Future playback mapping.
- Future tuning behaviour.
- Validation when reopening a score.

Pélog should be visible only as a later milestone or disabled option until its
rules are implemented.

## Gatra/Measure Blocks

For M1, a block means one complete gatra mapped to one MuseScore measure.

```text
Add gatra:

| 1  2  3  5 | 6  .  3  2 |

becomes:

| 1  2  3  5 | 6  .  3  2 | .  .  .  . |
```

```text
Remove gatra:

| 1  2  3  5 | 6  .  3  2 | .  .  .  . |

becomes:

| 1  2  3  5 | 6  .  3  2 |
```

Removing a non-empty gatra must use MuseScore's normal undoable command flow.
The user must be able to undo an accidental removal.

M1 should not support arbitrary resizing of a gatra to any number of primary
pulses. The composer adds or removes complete gatra blocks instead.

## Perpanjang / Perpendek

In M1, `perpanjang` and `perpendek` refer to changing the selected note's
duration, not resizing the entire gatra.

```text
Shorter:
1/2 -> 1/4 -> 1/8 -> 1/16

Longer:
1/16 -> 1/8 -> 1/4 -> 1/2
```

The internal MuseScore note/chord duration should remain the source of truth.
The gamelan renderer displays that duration using numeric rhythmic lines rather
than Western noteheads and flags.

Possible controls:

```text
[ Pendekkan nada ] [ Panjangkan nada ]
```

Existing MuseScore duration keyboard shortcuts should be reused where
practical.

## Meaning of the Lines

The references contain multiple types of line:

```text
Horizontal row separator  -> separates instrument rows visually
Vertical measure line     -> separates gatra/measure blocks
Short line above numbers  -> rhythmic subdivision
```

M1 supports these three basic categories. Curves, ties, octave markings, and
special performance symbols are later work.

## Meaning of the Dot

The exact semantics of `.` must be validated before implementation.

Open possibilities:

- Silence/rest.
- Continuation of the previous sound.
- Empty pulse whose interpretation depends on context.

The application should not permanently encode every dot as a Western rest
until this decision is confirmed.

## MuseScore Data and Rendering Design

Numeric notes must remain real MuseScore note objects:

```text
MuseScore Note
|-- musical pitch/degree
|-- duration
|-- measure position
|-- instrument: Saron
|
|-- numeric renderer -> draws "1", "2", "3", "5", or "6"
|-- MSCX writer      -> saves the musical object
`-- playback later   -> produces a Saron sound
```

Avoid:

```text
Unrelated Text object containing "3"
```

Prefer:

```text
Real Note object visually rendered as "3"
```

This preserves:

- Selection.
- Mouse interaction.
- Rhythm.
- Measures.
- Undo and redo.
- Saving and reopening.
- Printing.
- Future MIDI or sample playback.

The intended architecture is:

```text
Score DOM
    |
    +-- Standard staff rendering
    |
    +-- Tablature rendering
    |
    `-- Gamelan numeric rendering
```

## User Stories

### M1

- As a gamelan composer, I can create an empty score.
- As a gamelan composer, I can add or select Saron.
- As a gamelan composer, I can enter a title and composer name.
- As a gamelan composer, I can select Sléndro.
- As a gamelan composer, I can select a time signature.
- As a gamelan composer, I can enter the numeric pitches `1 2 3 5 6`.
- As a gamelan composer, I can enter a dot into an empty pulse.
- As a gamelan composer, I can shorten or lengthen a note.
- As a gamelan composer, I can add and remove gatra/measure blocks.
- As a gamelan composer, I can undo and redo edits.
- As a gamelan composer, I can save, close, and reopen an MSCX score without
  losing my notation.
- As a gamelan composer, I can print or export the numeric score.

### M2

- As a gamelan composer, I can add Gender.
- As a gamelan composer, I can add Gambang.
- As a gamelan composer, I can add Siter.

### M3

- As a gamelan composer, I can assign and play gamelan sound samples for Saron,
  Gender, Gambang, and Siter.

### M4

- As a gamelan composer, I can select or customize a gamelan tuning using a
  library of tunings from different regions.

## Acceptance Criteria

### Positive cases

1. Create a Saron Sléndro score.
2. Enter `1 2 3 5` in the first gatra.
3. Add a second gatra.
4. Enter `6 . 3 2`.
5. Lengthen and shorten selected notes.
6. Change the time signature.
7. Undo and redo each operation.
8. Save the score as MSCX.
9. Close MuseScore.
10. Reopen the score.
11. Confirm that metadata, Sléndro mode, numbers, durations, dots, time
    signature, and gatra blocks are preserved.
12. Print or export and confirm that numeric notation is visible.

### Negative and edge cases

- Reject or warn about `4` and `7` in Sléndro mode.
- Do not crash when deleting the first or final gatra.
- Do not silently delete a non-empty gatra.
- Undo must restore a removed non-empty gatra.
- Prevent a note duration from becoming zero or invalid.
- Define behaviour when a duration crosses a measure boundary.
- Define what happens when changing the time signature of existing material.
- Preserve Sléndro mode when reopening MSCX.
- Handle an empty title or composer without corrupting the score.
- Decide what a dot means before mapping it permanently to a rest.

## Delivery Phases

```text
Local development
    |
    +-- developer testing
    +-- automated tests
    `-- sample MSCX files
         |
         v
Beta / production test
    |
    +-- shared test build
    +-- composer testing
    `-- feedback and adjustments
```

Proposed beta starts with one tester, then expands after the save/reopen path is
stable.

## Sprint Proposal

### Sprint 1 — Numeric notation foundation

- Confirm notation rules with domain expert.
- Define Sléndro staff/score properties.
- Add Saron score or instrument preset.
- Add Sléndro notation mode.
- Render degrees `1`, `2`, `3`, `5`, and `6`.
- Position numbers on one fixed baseline.
- Add numeric input and validation.
- Add dot/empty-pulse prototype.

### Sprint 2 — Composition workflow

- Reuse or expose time-signature controls.
- Add/remove complete gatra/measure blocks.
- Add shorten/lengthen duration controls.
- Render basic rhythmic lines.
- Add title and composer workflow.
- Integrate undo/redo.
- Persist all M1 properties in MSCX.
- Add save, close, reopen, print, and export tests.

## Weekly Working Rhythm

- Standup: 08:30 WIB.
- Weekly discussion:
  - Progress.
  - Action items.
  - Blockers.
- Each sprint contains approximately 8 working days.
- M1 target: 2 sprints / 4 weeks.
- After M1: presentation, user testing, feedback, and adjustment.

## Open Decisions

These must be resolved before the implementation becomes authoritative:

1. Is the valid Sléndro input exactly `1 2 3 5 6`?
2. Does a dot mean silence, continuation, or context-dependent empty pulse?
3. Which rhythmic lines are required for the first usable version?
4. Is one gatra always mapped to one `4/4` measure in M1?
5. Are octave/register dots above and below numbers required in M1 or later?
6. Should the UI say `Gatra`, `Measure`, or `Gatra (Measure)`?
7. What should happen when a note is lengthened across a gatra boundary?
8. Does M1 need only visual Sléndro selection, or must it also establish the
   future pitch mapping for playback?

## M1 Definition of Done

M1 is complete when a composer can:

> Create a Saron-only Sléndro score, enter numeric notes and basic rhythms,
> manage gatra/measure blocks, change note duration and time signature, add
> title and composer information, save the score as MSCX, close it, reopen it
> without data loss, and print or export the same numeric notation.
