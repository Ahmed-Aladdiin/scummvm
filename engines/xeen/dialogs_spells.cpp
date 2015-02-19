/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "xeen/dialogs_spells.h"
#include "xeen/dialogs_query.h"
#include "xeen/resources.h"
#include "xeen/spells.h"
#include "xeen/sprites.h"
#include "xeen/xeen.h"

namespace Xeen {

Character *SpellsDialog::show(XeenEngine *vm, Character *c, int isCasting) {
	SpellsDialog *dlg = new SpellsDialog(vm);
	Character *result = dlg->execute(c, isCasting);
	delete dlg;

	return result;
}

Character *SpellsDialog::execute(Character *c, int isCasting) {
	EventsManager &events = *_vm->_events;
	Interface &intf = *_vm->_interface;
	Party &party = *_vm->_party;
	Screen &screen = *_vm->_screen;
	SoundManager &sound = *_vm->_sound;
	Spells &spells = *_vm->_spells;
	bool isDarkCc = _vm->_files->_isDarkCc;
	loadButtons();

	int castingCopy = isCasting;
	isCasting &= 0x7f;
	int selection = -1;
	uint topIndex = 0;
	int newSelection;
	screen._windows[25].open();

	do {
		if (!isCasting) {
			if (!c->guildMember()) {
				sound.playSample(nullptr, 0);
				intf._overallFrame = 5;
				File f(isDarkCc ? "skull1.voc" : "guild11.voc");
				sound.playSample(&f, 1);
				break;
			}

			Common::String title = Common::String::format(BUY_SPELLS, c->_name.c_str());
			Common::String msg = Common::String::format(GUILD_OPTIONS,
				title.c_str(), XeenEngine::printMil(party._gold));
			screen._windows[10].writeString(msg);

			warning("TODO: Sprite draw using previously used button sprites");
		}

		_spells.clear();
		const char *errorMsg = setSpellText(c, castingCopy);
		screen._windows[25].writeString(Common::String::format(SPELLS_FOR,
			errorMsg == nullptr ? SPELL_LINES_0_TO_9 : "",
			c->_name.c_str()));

		// Setup and write out spell list
		const char *names[10];
		int colors[10];
		Common::String emptyStr = "";
		Common::fill(&names[0], &names[10], emptyStr.c_str());
		Common::fill(&colors[0], &colors[10], 9);

		for (int idx = 0; idx < 10; ++idx) {
			if ((topIndex + idx) < _spells.size()) {
				names[idx] = _spells[topIndex + idx]._name.c_str();
				colors[idx] = _spells[topIndex + idx]._color;
			}
		}

		if (_spells.size() == 0)
			names[0] = errorMsg;

		screen._windows[37].writeString(Common::String::format(SPELLS_DIALOG_SPELLS,
			colors[0], names[0], colors[1], names[1], colors[2], names[2],
			colors[3], names[3], colors[4], names[4], colors[5], names[5],
			colors[6], names[6], colors[7], names[7], colors[8], names[8],
			colors[9], names[9],
			isCasting ? SPELL_PTS : GOLD,
			isCasting ? c->_currentSp : party._gold
		));

		_scrollSprites.draw(screen, 4, Common::Point(39, 26));
		_scrollSprites.draw(screen, 0, Common::Point(187, 26));
		_scrollSprites.draw(screen, 2, Common::Point(187, 111));
		if (isCasting)
			_scrollSprites.draw(screen._windows[25], 5, Common::Point(132, 123));

		screen._windows[25].update();

		do {
			events.pollEventsAndWait();
			checkEvents(_vm);
		} while (!_vm->shouldQuit() && !_buttonValue);

		switch (_buttonValue) {
		case Common::KEYCODE_F1:
		case Common::KEYCODE_F2:
		case Common::KEYCODE_F3:
		case Common::KEYCODE_F4:
		case Common::KEYCODE_F5:
		case Common::KEYCODE_F6:
			if (_vm->_mode != MODE_COMBAT) {
				_buttonValue -= Common::KEYCODE_F1;
				if (_buttonValue < (int)party._activeParty.size()) {
					c = &party._activeParty[_buttonValue];
					spells._lastCaster = _buttonValue;
					intf.highlightChar(_buttonValue);

					if (_vm->_mode == MODE_17) {
						screen._windows[10].writeString(Common::String::format(GUILD_OPTIONS,
							XeenEngine::printMil(party._gold).c_str(), GUILD_TEXT, c->_name.c_str()));
					} else {
						int category;
						switch (c->_class) {
						case CLASS_ARCHER:
						case CLASS_SORCERER:
							category = 1;
							break;
						case CLASS_DRUID:
						case CLASS_RANGER:
							category = 2;
							break;
						default:
							category = 0;
							break;
						}

						int spellIndex = (c->_currentSpell == -1) ? 39 : c->_currentSpell;
						int spellId = SPELLS_ALLOWED[category][spellIndex];
						screen._windows[10].writeString(Common::String::format(SPELL_DETAILS,
							c->_name.c_str(), spells._spellNames[spellId].c_str(),
							spells.calcSpellPoints(spellId, c->getCurrentLevel()),
							SPELL_GEM_COST[spellId], c->_currentSp));
					}

					drawButtons(&screen);
					screen._windows[10].update();
				}
			}
			break;

		case Common::KEYCODE_RETURN:
		case Common::KEYCODE_KP_ENTER:
		case Common::KEYCODE_s:
			if (selection != -1)
				_buttonValue = Common::KEYCODE_ESCAPE;
			break;

		case Common::KEYCODE_ESCAPE:
			selection = -1;
			_buttonValue = Common::KEYCODE_ESCAPE;
			break;

		case Common::KEYCODE_0:
		case Common::KEYCODE_1:
		case Common::KEYCODE_2:
		case Common::KEYCODE_3:
		case Common::KEYCODE_4:
		case Common::KEYCODE_5:
		case Common::KEYCODE_6:
		case Common::KEYCODE_7:
		case Common::KEYCODE_8:
		case Common::KEYCODE_9:
			newSelection = topIndex + (_buttonValue == Common::KEYCODE_0) ? 9 :
				(_buttonValue - Common::KEYCODE_1);

			if (newSelection < (int)_spells.size()) {
				int expenseFactor = 0;
				int category = 0;

				switch (c->_class) {
				case CLASS_PALADIN:
					expenseFactor = 1;
					category = 0;
					break;
				case CLASS_ARCHER:
					expenseFactor = 1;
					category = 1;
					break;
				case CLASS_CLERIC:
					category = 0;
					break;
				case CLASS_SORCERER:
					category = 1;
					break;
				case CLASS_DRUID:
					category = 2;
					break;
				case CLASS_RANGER:
					expenseFactor = 1;
					category = 2;
					break;
				default:
					break;
				}

				int spellId = _spells[newSelection]._spellId;
				int spellIndex = _spells[newSelection]._spellIndex;
				int spellCost = spells.calcSpellCost(spellId, expenseFactor);
				if (isCasting) {
					// TODO: Confirm this refactoring against against original
					selection = _buttonValue;
				} else {
					Common::String spellName = _spells[_buttonValue]._name;
					Common::String msg = (castingCopy & 0x80) ?
						Common::String::format(SPELLS_PRESS_A_KEY, msg.c_str()) :
						Common::String::format(SPELLS_PURCHASE, msg.c_str(), spellCost);

					if (Confirm::show(_vm, msg, castingCopy + 1)) {
						if (party.subtract(0, spellCost, 0, WT_FREEZE_WAIT)) {
							++c->_spells[spellIndex];
							sound.playSample(nullptr, 0);
							intf._overallFrame = 0;
							File f(isDarkCc ? "guild12.voc" : "parrot2.voc");
							sound.playSample(&f, 1);
						} else {
							sound.playFX(21);
						}
					}
				}
			}
			break;

		case Common::KEYCODE_PAGEUP:
		case Common::KEYCODE_KP9:
			topIndex = MAX((int)topIndex - 10, 0);
			break;

		case Common::KEYCODE_PAGEDOWN:
		case Common::KEYCODE_KP3:
			topIndex = MIN(topIndex + 10, ((_spells.size() - 1) / 10) * 10);
			break;

		case Common::KEYCODE_UP:
		case Common::KEYCODE_KP8:
			if (topIndex > 0)
				--topIndex;
			break;

		case Common::KEYCODE_DOWN:
		case Common::KEYCODE_KP2:
			if (topIndex < (_spells.size() - 10))
				++topIndex;
			break;
		}
	} while (!_vm->shouldQuit() && _buttonValue != Common::KEYCODE_ESCAPE);

	screen._windows[25].close();
	if (isCasting && selection != -1)
		c->_currentSpell = _spells[selection]._spellIndex;

	return c;
}

void SpellsDialog::loadButtons() {
	_iconSprites.load("main.icn");
	_scrollSprites.load("scroll.icn");
	addButton(Common::Rect(187, 26, 198, 36), Common::KEYCODE_UP, &_scrollSprites, true);
	addButton(Common::Rect(187, 111, 198, 121), Common::KEYCODE_DOWN, &_scrollSprites, true);
	addButton(Common::Rect(40, 28, 187, 36), Common::KEYCODE_1);
	addButton(Common::Rect(40, 37, 187, 45), Common::KEYCODE_2);
	addButton(Common::Rect(40, 46, 187, 54), Common::KEYCODE_3);
	addButton(Common::Rect(40, 55, 187, 63), Common::KEYCODE_4);
	addButton(Common::Rect(40, 64, 187, 72), Common::KEYCODE_5);
	addButton(Common::Rect(40, 73, 187, 81), Common::KEYCODE_6);
	addButton(Common::Rect(40, 82, 187, 90), Common::KEYCODE_7);
	addButton(Common::Rect(40, 91, 187, 99), Common::KEYCODE_8);
	addButton(Common::Rect(40, 100, 187, 108), Common::KEYCODE_9);
	addButton(Common::Rect(40, 109, 187, 117), Common::KEYCODE_0);
	addButton(Common::Rect(174, 123, 198, 133), Common::KEYCODE_ESCAPE);
	addButton(Common::Rect(187, 35, 198, 73), Common::KEYCODE_PAGEUP);
	addButton(Common::Rect(187, 74, 198, 112), Common::KEYCODE_PAGEDOWN);
	addButton(Common::Rect(132, 123, 168, 133), Common::KEYCODE_s);
	addPartyButtons(_vm);
}

const char *SpellsDialog::setSpellText(Character *c, int v2) {
	Party &party = *_vm->_party;
	Spells &spells = *_vm->_spells;
	bool isDarkCc = _vm->_files->_isDarkCc;
	int expenseFactor = 0;
	int currLevel = c->getCurrentLevel();
	int category;

	if ((v2 & 0x7f) == 0) {
		switch (c->_class) {
		case CLASS_PALADIN:
			expenseFactor = 1;
			category = 0;
			break;
		case CLASS_ARCHER:
			expenseFactor = 1;
			category = 1;
			break;
		case CLASS_CLERIC:
			category = 0;
			break;
		case CLASS_SORCERER:
			category = 1;
			break;
		case CLASS_DRUID:
			category = 2;
			break;
		case CLASS_RANGER:
			expenseFactor = 1;
			category = 2;
			break;
		default:
			category = -1;
			break;
		}

		if (category != -1) {
			if (party._mazeId == 49 || party._mazeId == 37) {
				for (uint spellId = 0; spellId < 76; ++spellId) {
					int idx = 0;
					while (idx < MAX_SPELLS_PER_CLASS && SPELLS_ALLOWED[category][idx] == spellId)
						++idx;

					// Handling if the spell is appropriate for the character's class
					if (idx < MAX_SPELLS_PER_CLASS) {
						if (!c->_spells[idx] || (v2 & 0x80)) {
							int cost = spells.calcSpellCost(SPELLS_ALLOWED[category][idx], expenseFactor);
							_spells.push_back(SpellEntry(Common::String::format("\x3l%s\x3r\x9""000%u",
								spells._spellNames[SPELLS_ALLOWED[category][idx]], cost), 
								idx, spellId));
						}
					}
				}
			} else if (isDarkCc) {
				int groupIndex = (party._mazeId - 29) / 2;
				for (int spellId = DARK_SPELL_RANGES[groupIndex][0];
						spellId < DARK_SPELL_RANGES[groupIndex][1]; ++spellId) {
					int idx = 0;
					while (idx < 40 && SPELLS_ALLOWED[category][idx] ==
						DARK_SPELL_OFFSETS[category][spellId]);

					if (idx < 40) {
						if (!c->_spells[idx] || (v2 & 0x80)) {
							int cost = spells.calcSpellCost(SPELLS_ALLOWED[category][idx], expenseFactor);
							_spells.push_back(SpellEntry(Common::String::format("\x3l%s\x3r\x9""000%u",
								spells._spellNames[SPELLS_ALLOWED[category][idx]], cost), 
								idx, spellId));
						}
					}
				}
			} else {
				for (int spellId = 0; spellId < 20; ++spellId) {
					int idx = 0;
					while (CLOUDS_SPELL_OFFSETS[party._mazeId - 29][spellId] !=
						(int)SPELLS_ALLOWED[category][idx] && idx < 40) ;

					if (idx < 40) {
						if (!c->_spells[idx] || (v2 & 0x80)) {
							int cost = spells.calcSpellCost(SPELLS_ALLOWED[category][idx], expenseFactor);
							_spells.push_back(SpellEntry(Common::String::format("\x3l%s\x3r\x9""000%u",
								spells._spellNames[SPELLS_ALLOWED[category][idx]], cost), 
								idx, spellId));
						}
					}
				}
			}
		}

		if (c->getMaxSP() == 0)
			return NOT_A_SPELL_CASTER;

	} else if ((v2 & 0x7f) == 1) {
		switch (c->_class) {
		case 0:
		case 12:
			category = 0;
			break;
		case 1:
		case 3:
			category = 1;
			break;
		case 7:
		case 8:
			category = 2;
			break;
		default:
			category = 0;
			break;
		}

		if (c->getMaxSP() == 0) {
			return NOT_A_SPELL_CASTER;
		} else {
			for (int spellIndex = 0; spellIndex < (MAX_SPELLS_PER_CLASS - 1); ++spellIndex) {
				if (c->_spells[spellIndex]) {
					int spellId = SPELLS_ALLOWED[category][spellIndex];
					int gemCost = SPELL_GEM_COST[spellId];
					int spCost = spells.calcSpellPoints(spellId, currLevel);

					Common::String msg = Common::String::format("\x3l%s\x3r\x9""000%u/%u",
						spells._spellNames[spellId].c_str(), spCost, gemCost);
					_spells.push_back(SpellEntry(msg, spellIndex, spellId));
				}
			}
		}
	}

	return nullptr;
}

/*------------------------------------------------------------------------*/

int CastSpell::show(XeenEngine *vm, int mode) {
	Combat &combat = *vm->_combat;
	Interface &intf = *vm->_interface;
	Party &party = *vm->_party;
	Spells &spells = *vm->_spells;
	int charNum;

	// Get which character is doing the casting
	if (mode == MODE_COMBAT) {
		charNum = combat._whosTurn;
	} else if (spells._lastCaster >= 0 && spells._lastCaster < (int)party._activeParty.size()) {
		charNum = spells._lastCaster;
	} else {
		for (charNum = (int)party._activeParty.size() - 1; charNum >= 0; --charNum) {
			if (party._activeParty[charNum]._hasSpells) {
				spells._lastCaster = charNum;
				break;
			}
		}
	}

	Character *c = &party._activeParty[charNum];
	intf.highlightChar(charNum);

	CastSpell *dlg = new CastSpell(vm);
	int spellId = dlg->execute(c, mode);
	delete dlg;

	return spellId;
}

int CastSpell::show(XeenEngine *vm, Character *&c, int mode) {
	CastSpell *dlg = new CastSpell(vm);
	int spellId = dlg->execute(c, mode);
	delete dlg;

	return spellId;
}

int CastSpell::execute(Character *&c, int mode) {
	EventsManager &events = *_vm->_events;
	Interface &intf = *_vm->_interface;
	Party &party = *_vm->_party;
	Screen &screen = *_vm->_screen;
	Spells &spells = *_vm->_spells;
	Window &w = screen._windows[10];

	Mode oldMode = (Mode)mode;
	int category = c->getClassCategory();

	w.open();
	loadButtons();

	int spellId = -1;
	bool redrawFlag = true;
	do {
		if (redrawFlag) {
			int spellIndex = c->_currentSpell != -1 ? c->_currentSpell : 39;
			spellId = SPELLS_ALLOWED[category][spellIndex];
			int gemCost = SPELL_GEM_COST[spellId];
			int spCost = spells.calcSpellPoints(spellId, c->getCurrentLevel());

			w.writeString(Common::String::format(CAST_SPELL_DETAILS,
				c->_name.c_str(), spells._spellNames[spellId].c_str(), 
				spCost, gemCost, c->_currentSp));
			drawButtons(&screen);
			w.update();

			_vm->_mode = MODE_3;
			redrawFlag = false;
		}

		events.updateGameCounter();
		intf.draw3d(true);

		// Wait for event or time expiry
		do {
			events.pollEventsAndWait();
			checkEvents(_vm);
		} while (!_vm->shouldQuit() && events.timeElapsed() < 1 && !_buttonValue);

		switch (_buttonValue) {
		case Common::KEYCODE_F1:
		case Common::KEYCODE_F2:
		case Common::KEYCODE_F3:
		case Common::KEYCODE_F4:
		case Common::KEYCODE_F5:
		case Common::KEYCODE_F6:
			// Only allow changing character if the party is not in combat
			if (oldMode != MODE_COMBAT) {
				_vm->_mode = oldMode;
				_buttonValue -= Common::KEYCODE_F1;

				if (_buttonValue < (int)party._activeParty.size()) {
					c = &party._activeParty[_buttonValue];
					intf.highlightChar(_buttonValue);
					redrawFlag = true;
					break;
				}
			}
			break;

		case Common::KEYCODE_ESCAPE:
			spellId = -1;
			break;

		case Common::KEYCODE_c:
			// Cast spell - return the selected spell Id to be cast
			if (c->_currentSpell != -1 && !c->noActions())
				_buttonValue = Common::KEYCODE_ESCAPE;
			break;

		case Common::KEYCODE_n:
			// Select new spell
			_vm->_mode = oldMode;
			c = SpellsDialog::show(_vm, c, 1);
			redrawFlag = true;
			break;

		default:
			break;
		}
	} while (!_vm->shouldQuit() && _buttonValue != Common::KEYCODE_ESCAPE);

	w.close();
	intf.unhighlightChar();

	if (_vm->shouldQuit())
		spellId = -1;

	return spellId;
}

void CastSpell::loadButtons() {
	_iconSprites.load("cast.icn");
	addButton(Common::Rect(234, 108, 259, 128), Common::KEYCODE_c, &_iconSprites);
	addButton(Common::Rect(261, 108, 285, 128), Common::KEYCODE_n, &_iconSprites);
	addButton(Common::Rect(288, 108, 312, 128), Common::KEYCODE_ESCAPE, &_iconSprites);
	addPartyButtons(_vm);
}

} // End of namespace Xeen
