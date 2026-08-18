/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "noteinputbarmodel.h"

#include "types/translatablestring.h"

#include "context/shortcutcontext.h"

#include "notation/inotationelements.h" // IWYU pragma: keep
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"

#include "internal/notationuiactions.h"
#include "notationcommands.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"

using namespace mu;
using namespace mu::notation;
using namespace muse;
using namespace muse::actions;
using namespace muse::ui;
using namespace muse::uicomponents;

static const QString TOOLBAR_NAME("noteInput");

static const ActionCode ADD_ACTION_CODE("add");
static const ActionCode CROSS_STAFF_BEAMING_CODE("cross-staff-beaming");
static const ActionCode TUPLET_ACTION_CODE("tuplet");

static bool isGamelanDegreeAction(const QString& actionId)
{
    return actionId == QStringLiteral("note-c")
           || actionId == QStringLiteral("note-d")
           || actionId == QStringLiteral("note-e")
           || actionId == QStringLiteral("note-g")
           || actionId == QStringLiteral("note-a");
}

static bool isDurationAction(const QString& actionId)
{
    return actionId == QStringLiteral("pad-note-1")
           || actionId == QStringLiteral("pad-note-2")
           || actionId == QStringLiteral("pad-note-4")
           || actionId == QStringLiteral("pad-note-8")
           || actionId == QStringLiteral("pad-note-16")
           || actionId == QStringLiteral("pad-note-32")
           || actionId == QStringLiteral("pad-note-64")
           || actionId == QStringLiteral("pad-note-128")
           || actionId == QStringLiteral("pad-note-256")
           || actionId == QStringLiteral("pad-note-512")
           || actionId == QStringLiteral("pad-note-1024");
}

static bool isAlternativeNoteInputModeAction(const ActionCode& actionCode)
{
    return actionCode.rfind("note-input-", 0) == 0;
}

NoteInputBarModel::NoteInputBarModel(QObject* parent)
    : AbstractMenuModel(parent)
{
}

QVariant NoteInputBarModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (!isIndexValid(row)) {
        return QVariant();
    }

    const MenuItem* item = items().at(row);
    switch (role) {
    case OrderRole: return row;
    case SectionRole: return item->section();
    }

    return AbstractMenuModel::data(index, role);
}

QHash<int, QByteArray> NoteInputBarModel::roleNames() const
{
    QHash<int, QByteArray> roles = AbstractMenuModel::roleNames();
    roles[OrderRole] = "order";
    roles[SectionRole] = "section";

    return roles;
}

void NoteInputBarModel::classBegin()
{
    init();
}

void NoteInputBarModel::init()
{
    subscribeOnChanges();

    const auto listenToNoteInputState = [this]() {
        m_noteInputStateReceiver = std::make_unique<muse::async::Asyncable>();
        INotationPtr notation = context()->currentNotation();
        if (!notation || !notation->interaction() || !notation->interaction()->noteInput()) {
            return;
        }
        notation->interaction()->noteInput()->stateChanged().onNotify(m_noteInputStateReceiver.get(), [this]() {
            load();
            emit isGamelanStaffChanged();
        });
    };

    uiState()->toolConfigChanged(TOOLBAR_NAME).onNotify(this, [this]() {
        load();
    });

    commandsController()->isNoteInputAllowedChanged().onReceive(this, [this](bool) {
        emit isInputAllowedChanged();
        load();
    });

    context()->currentNotationChanged().onNotify(this, [this, listenToNoteInputState]() {
        m_selectedGamelanDegreeAction.clear();
        listenToNoteInputState();
        load();
        emit isGamelanStaffChanged();
    });

    listenToNoteInputState();

    load();
}

void NoteInputBarModel::load()
{
    MenuItemList items;
    const bool gamelanStaff = isGamelanStaff();
    const QString selectedDurationAction = gamelanStaff ? currentGamelanDurationAction() : QString();

    if (gamelanStaff) {
        struct GamelanAction {
            ActionCode code;
            const char* title;
            const char* section;
            bool textButton;
        };
        static const GamelanAction gamelanActions[] = {
            { "note-input-by-note-name", "Saron Slendro input", "gamelan-input", false },
            { "note-c", "1", "gamelan-degree", true }, { "note-d", "2", "gamelan-degree", true },
            { "note-e", "3", "gamelan-degree", true }, { "note-g", "5", "gamelan-degree", true },
            { "note-a", "6", "gamelan-degree", true },
            { "pad-note-4", "One pulse", "gamelan-pulse", true },
            { "pad-note-8", "Half pulse", "gamelan-pulse", true },
            { "pad-note-16", "Quarter pulse", "gamelan-pulse", true },
            // Unlike pad-rest, the "rest" action enters one rest immediately
            // and advances the cursor. The Gamelan renderer displays that rest
            // as an empty-pulse dot.
            { "rest", "Empty pulse", "gamelan-rhythm", true },
            { "tie", "Continue / tie", "gamelan-rhythm", true }
        };
        for (const GamelanAction& definition : gamelanActions) {
            UiAction action = uiActionsRegister()->action(definition.code);
            action.title = TranslatableString::untranslatable(definition.title);
            if (definition.textButton) {
                action.iconCode = IconCode::Code::NONE;
            }
            MenuItem* item = makeActionItem(action, QString::fromLatin1(definition.section));
            const QString actionId = QString::fromStdString(definition.code);
            if (isGamelanDegreeAction(actionId)) {
                UiActionState state = item->state();
                state.checked = actionId == m_selectedGamelanDegreeAction;
                item->setState(state);
            } else if (isDurationAction(actionId)) {
                UiActionState state = item->state();
                state.checked = actionId == selectedDurationAction;
                item->setState(state);
            }
            items << item;
        }
    }

    ToolConfig noteInputConfig = uiState()->toolConfig(TOOLBAR_NAME, NotationUiActions::defaultNoteInputBarConfig());

    int section = 0;
    for (const ToolConfig::Item& citem : noteInputConfig.items) {
        if (!citem.show) {
            continue;
        }

        // Saron has one predictable numeric input mode. Keep the original
        // editing controls, but omit Western alternate entry modes which make
        // duration buttons insert the stored/default pitch.
        if (gamelanStaff && isAlternativeNoteInputModeAction(citem.action)) {
            continue;
        }

        if (citem.action.empty()) {
            section++;
            continue;
        }

        MenuItemList subitems;
        if (citem.action == CROSS_STAFF_BEAMING_CODE) {
            subitems = makeCrossStaffBeamingItems();
        } else if (citem.action == TUPLET_ACTION_CODE) {
            subitems = makeTupletItems();
        }

        const QString sectionName = gamelanStaff
                                    ? QStringLiteral("western-%1").arg(section)
                                    : QString::number(section);
        MenuItem* item = makeActionItem(uiActionsRegister()->action(citem.action), sectionName, subitems);
        const QString actionId = QString::fromStdString(citem.action);
        if (gamelanStaff && isDurationAction(actionId)) {
            UiActionState state = item->state();
            state.checked = actionId == selectedDurationAction;
            item->setState(state);
        }
        items << item;
    }

    const QString addSection = gamelanStaff
                               ? QStringLiteral("western-%1").arg(++section)
                               : QString::number(++section);
    items << makeAddItem(addSection);
    setItems(items);
}

bool NoteInputBarModel::isInputAllowed() const
{
    return commandsController()->isNoteInputAllowed();
}

bool NoteInputBarModel::isGamelanStaff() const
{
    INotationPtr notation = context()->currentNotation();
    if (!notation || !notation->interaction() || !notation->interaction()->noteInput()) {
        return false;
    }

    const NoteInputState& state = notation->interaction()->noteInput()->state();
    const engraving::Staff* staff = state.staff();
    const engraving::StaffType* staffType = staff ? staff->staffType(state.tick()) : nullptr;
    return staffType && staffType->isGamelanStaff();
}

void NoteInputBarModel::handleMenuItem(const QString& itemId)
{
    const bool gamelanStaff = isGamelanStaff();
    const bool selectGamelanDegree = gamelanStaff && isGamelanDegreeAction(itemId);

    if (gamelanStaff && isDurationAction(itemId)) {
        INotationPtr notation = context()->currentNotation();
        INotationNoteInputPtr noteInput = notation && notation->interaction()
                                                ? notation->interaction()->noteInput()
                                                : nullptr;
        if (noteInput && noteInput->isNoteInputMode()
            && !noteInput->usingNoteInputMethod(NoteInputMethod::BY_NOTE_NAME)) {
            noteInput->setNoteInputMethod(NoteInputMethod::BY_NOTE_NAME);
        }
    }

    AbstractMenuModel::handleMenuItem(itemId);

    if (selectGamelanDegree) {
        m_selectedGamelanDegreeAction = itemId;
        load();
    }
}

void NoteInputBarModel::onActionsStateChanges(const ActionCodeList& codes)
{
    AbstractMenuModel::onActionsStateChanges(codes);

    if (!isGamelanStaff()) {
        return;
    }

    const QString selectedDurationAction = currentGamelanDurationAction();
    for (MenuItem* item : items()) {
        if (!item) {
            continue;
        }

        UiActionState state = item->state();
        if (item->section() == QStringLiteral("gamelan-degree")) {
            state.checked = item->id() == m_selectedGamelanDegreeAction;
            item->setState(state);
        } else if (isDurationAction(item->id())) {
            state.checked = item->id() == selectedDurationAction;
            item->setState(state);
        }
    }
}

QString NoteInputBarModel::currentGamelanDurationAction() const
{
    INotationPtr notation = context()->currentNotation();
    if (!notation || !notation->interaction() || !notation->interaction()->noteInput()) {
        return {};
    }

    switch (notation->interaction()->noteInput()->state().duration().type()) {
    case engraving::DurationType::V_WHOLE: return QStringLiteral("pad-note-1");
    case engraving::DurationType::V_HALF: return QStringLiteral("pad-note-2");
    case engraving::DurationType::V_QUARTER: return QStringLiteral("pad-note-4");
    case engraving::DurationType::V_EIGHTH: return QStringLiteral("pad-note-8");
    case engraving::DurationType::V_16TH: return QStringLiteral("pad-note-16");
    case engraving::DurationType::V_32ND: return QStringLiteral("pad-note-32");
    case engraving::DurationType::V_64TH: return QStringLiteral("pad-note-64");
    case engraving::DurationType::V_128TH: return QStringLiteral("pad-note-128");
    case engraving::DurationType::V_256TH: return QStringLiteral("pad-note-256");
    case engraving::DurationType::V_512TH: return QStringLiteral("pad-note-512");
    case engraving::DurationType::V_1024TH: return QStringLiteral("pad-note-1024");
    default: return {};
    }
}

MenuItem* NoteInputBarModel::makeActionItem(const UiAction& action, const QString& section,
                                            const muse::uicomponents::MenuItemList& subitems)
{
    MenuItem* item = new MenuItem(action, this);
    // "add" is a local toolbar menu action, not an action registered in
    // UiActionsRegister. Querying its state produces a noisy error on every
    // toolbar reload.
    if (action.code != ADD_ACTION_CODE) {
        item->setState(uiActionsRegister()->actionState(action.code));
    }
    item->setSection(section);
    item->setSubitems(subitems);
    return item;
}

MenuItem* NoteInputBarModel::makeAddItem(const QString& section)
{
    static const UiAction addAction(ADD_ACTION_CODE, UiCtxAny, mu::context::CTX_ANY,
                                    TranslatableString("global", "Add"),
                                    IconCode::Code::PLUS);

    return makeActionItem(addAction, section, makeAddItems());
}

MenuItemList NoteInputBarModel::makeCrossStaffBeamingItems()
{
    MenuItemList items {
        makeMenuItem("move-up"),
        makeMenuItem("move-down")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeTupletItems()
{
    MenuItemList items {
        makeMenuItem(ADD_DUPLET_COMMAND),
        makeMenuItem(ADD_TRIPLET_COMMAND),
        makeMenuItem(ADD_QUADRUPLET_COMMAND),
        makeMenuItem(ADD_QUINTUPLET_COMMAND),
        makeMenuItem(ADD_SEXTUPLET_COMMAND),
        makeMenuItem(ADD_SEPTUPLET_COMMAND),
        makeMenuItem(ADD_OCTUPLET_COMMAND),
        makeMenuItem(ADD_NONUPLET_COMMAND),
        makeMenuItem(SHOW_TUPLET_CONFIGURE_COMMAND)
    };

    return items;
}

MenuItemList NoteInputBarModel::makeAddItems()
{
    MenuItemList items {
        makeMenu(TranslatableString("notation", "Notes"), makeNotesItems()),
        makeMenu(TranslatableString("notation", "Intervals"), makeIntervalsItems()),
        makeMenu(TranslatableString("notation", "Measures"), makeMeasuresItems()),
        makeMenu(TranslatableString("notation", "Frames"), makeFramesItems()),
        makeMenu(TranslatableString("notation", "Text"), makeTextItems()),
        makeMenu(TranslatableString("notation", "Lines"), makeLinesItems()),
        makeMenu(TranslatableString("notation", "Chords and fretboard diagrams"), makeChordAndFretboardDiagramsItems()),
    };

    return items;
}

MenuItemList NoteInputBarModel::makeNotesItems()
{
    MenuItemList items {
        makeMenuItem("command://notation/enter-note-c"),
        makeMenuItem("command://notation/enter-note-d"),
        makeMenuItem("command://notation/enter-note-e"),
        makeMenuItem("command://notation/enter-note-f"),
        makeMenuItem("command://notation/enter-note-g"),
        makeMenuItem("command://notation/enter-note-a"),
        makeMenuItem("command://notation/enter-note-b"),
        makeSeparator(),
        makeMenuItem("command://notation/add-note-c"),
        makeMenuItem("command://notation/add-note-d"),
        makeMenuItem("command://notation/add-note-e"),
        makeMenuItem("command://notation/add-note-f"),
        makeMenuItem("command://notation/add-note-g"),
        makeMenuItem("command://notation/add-note-a"),
        makeMenuItem("command://notation/add-note-b")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeIntervalsItems()
{
    MenuItemList items {
        makeMenuItem("interval1"),
        makeMenuItem("interval2"),
        makeMenuItem("interval3"),
        makeMenuItem("interval4"),
        makeMenuItem("interval5"),
        makeMenuItem("interval6"),
        makeMenuItem("interval7"),
        makeMenuItem("interval8"),
        makeMenuItem("interval9"),
        makeMenuItem("interval10"),
        makeSeparator(),
        makeMenuItem("interval-2"),
        makeMenuItem("interval-3"),
        makeMenuItem("interval-4"),
        makeMenuItem("interval-5"),
        makeMenuItem("interval-6"),
        makeMenuItem("interval-7"),
        makeMenuItem("interval-8"),
        makeMenuItem("interval-9"),
        makeMenuItem("interval-10")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeMeasuresItems()
{
    MenuItemList items {
        makeMenuItem("insert-measure"),
        makeMenuItem("append-measure"),
        makeSeparator(),
        makeMenuItem("insert-measures"),
        makeMenuItem("insert-measures-after-selection"),
        makeSeparator(),
        makeMenuItem("insert-measures-at-start-of-score"),
        makeMenuItem("append-measures")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeFramesItems()
{
    MenuItemList items {
        makeMenuItem("insert-hbox"),
        makeMenuItem("insert-vbox"),
        makeMenuItem("insert-textframe"),
        makeMenuItem("insert-fretframe"),
        makeSeparator(),
        makeMenu(TranslatableString("notation", "Insert at end of score"), makeFramesAppendItems())
    };

    return items;
}

MenuItemList NoteInputBarModel::makeFramesAppendItems()
{
    MenuItemList items {
        makeMenuItem("append-hbox"),
        makeMenuItem("append-vbox"),
        makeMenuItem("append-textframe"),
        makeMenuItem("append-fretframe")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeTextItems()
{
    MenuItemList items {
        makeMenuItem("title-text"),
        makeMenuItem("subtitle-text"),
        makeMenuItem("composer-text"),
        makeMenuItem("poet-text"),
        makeMenuItem("part-text"),
        makeSeparator(),
        makeMenuItem("system-text"),
        makeMenuItem("staff-text"),
        makeMenuItem("add-dynamic"),
        makeMenuItem("expression-text"),
        makeMenuItem("rehearsalmark-text"),
        makeMenuItem("instrument-change-text"),
        makeMenuItem("fingering-text"),
        makeSeparator(),
        makeMenuItem("sticking-text"),
        makeMenuItem("chord-text"),
        makeMenuItem("roman-numeral-text"),
        makeMenuItem("nashville-number-text"),
        makeMenuItem("lyrics"),
        makeMenuItem("figured-bass"),
        makeMenuItem("tempo")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeLinesItems()
{
    MenuItemList items {
        makeMenuItem("command://notation/add-slur"),
        makeMenuItem("add-hairpin"),
        makeMenuItem("add-hairpin-reverse"),
        makeMenuItem("add-8va"),
        makeMenuItem("add-8vb"),
        makeMenuItem("add-noteline")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeChordAndFretboardDiagramsItems()
{
    MenuItemList items {
        makeMenuItem("chord-text"),
        makeMenuItem("add-fretboard-diagram"),
        makeSeparator(),
        makeMenuItem("insert-fretframe", TranslatableString("notation", "Fretboard diagram legend"))
    };

    return items;
}
