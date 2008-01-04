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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "common/system.h"

#include "parallaction/parallaction.h"
#include "parallaction/sound.h"


namespace Parallaction {

const char *introMsg1[] = {
	"INSERISCI IL CODICE",
	"ENTREZ CODE",
	"ENTER CODE",
	"GIB DEN KODE EIN"
};

const char *introMsg2[] = {
	"CODICE ERRATO",
	"CODE ERRONE",
	"WRONG CODE",
	"GIB DEN KODE EIN"
};

const char *introMsg3[] = {
	"PRESS LEFT MOUSE BUTTON",
	"TO SEE INTRO",
	"PRESS RIGHT MOUSE BUTTON",
	"TO START"
};

const char *newGameMsg[] = {
	"NUOVO GIOCO",
	"NEUF JEU",
	"NEW GAME",
	"NEUES SPIEL"
};

const char *loadGameMsg[] = {
	"GIOCO SALVATO",
	"JEU SAUVE'",
	"SAVED GAME",
	"SPIEL GESPEICHERT"
};


#define BLOCK_WIDTH 	16
#define BLOCK_HEIGHT	24

#define BLOCK_X 		112
#define BLOCK_Y 		130

#define BLOCK_SELECTION_X		  (BLOCK_X-1)
#define BLOCK_SELECTION_Y		  (BLOCK_Y-1)

#define BLOCK_X_OFFSET	(BLOCK_WIDTH+1)
#define BLOCK_Y_OFFSET	9

//	destination slots for code blocks
//
#define SLOT_X			61
#define SLOT_Y			64
#define SLOT_WIDTH		(BLOCK_WIDTH+2)

#define PASSWORD_LEN	6

#define CHAR_DINO       0
#define CHAR_DONNA      1
#define CHAR_DOUGH      2

static const uint16 _amigaKeys[][PASSWORD_LEN] = {
	{ 5, 3, 6, 2, 2, 7 },		// dino
	{ 0, 3, 6, 2, 2, 6 },		// donna
	{ 1, 3 ,7, 2, 4, 6 }		// dough
};

static const uint16 _pcKeys[][PASSWORD_LEN] = {
	{ 5, 3, 6, 1, 4, 7 },		// dino
	{ 0, 2, 8, 5, 5, 1 },		// donna
	{ 1, 7 ,7, 2, 2, 6 }		// dough
};

static const char *_charStartLocation[] = {
	"test.dino",
	"test.donna",
	"test.dough"
};

enum {
	NEW_GAME,
	LOAD_GAME
};

enum {
	START_DEMO,
	START_INTRO,
	GAME_LOADED,
	SELECT_CHARACTER
};

void Parallaction_ns::guiStart() {

	_disk->selectArchive((getFeatures() & GF_LANG_MULT) ? "disk1" : "disk0");

	guiSplash();

	_gfx->setFont(_introFont);
	_gfx->setFontShadow(true);

	_language = guiChooseLanguage();
	_disk->setLanguage(_language);

	int event;

	if (getFeatures() & GF_DEMO) {
		event = START_DEMO;
	} else {
		if (guiSelectGame() == NEW_GAME) {
			event = guiNewGame();
		} else {
			event = loadGame() ? GAME_LOADED : START_INTRO;
		}
	}

	switch (event) {
	case START_DEMO:
		strcpy(_location._name, "fognedemo.dough");
		break;

	case START_INTRO:
		strcpy(_location._name, "fogne.dough");
		break;

	case GAME_LOADED:
		// nothing to do here
		return;

	case SELECT_CHARACTER:
		selectStartLocation();
		break;

	}

	return;
}

void Parallaction_ns::selectStartLocation() {
	int character = guiSelectCharacter();
	if (character == -1)
		error("invalid character selected from menu screen");

	scheduleLocationSwitch(_charStartLocation[character]);
}


void Parallaction_ns::guiSplash() {

	showSlide("intro");
	_gfx->updateScreen();
	g_system->delayMillis(2000);

	showSlide("minintro");
	_gfx->updateScreen();
	g_system->delayMillis(2000);

}

int Parallaction_ns::guiNewGame() {

	const char **v14 = introMsg3;

    _gfx->setFont(_menuFont);
    _gfx->setFontShadow(true);

	_disk->selectArchive("disk1");

	setBackground("test", NULL, NULL);

	_gfx->swapBuffers();

	_gfx->displayCenteredString(50, v14[0]);
	_gfx->displayCenteredString(70, v14[1]);
	_gfx->displayCenteredString(100, v14[2]);
	_gfx->displayCenteredString(120, v14[3]);

	showCursor(false);

	_gfx->updateScreen();

	_mouseButtons = kMouseNone;
	do {
		readInput();
	} while (_mouseButtons != kMouseLeftUp && _mouseButtons != kMouseRightUp);

	showCursor(true);

	if (_mouseButtons != kMouseRightUp) {
		return START_INTRO;
	}

	return SELECT_CHARACTER;
}

static const Common::Rect _dosLanguageSelectBlocks[4] = {
	Common::Rect(  80, 110, 128, 180 ),	// Italian
	Common::Rect( 129,  85, 177, 155 ),	// French
	Common::Rect( 178,  60, 226, 130 ),	// English
	Common::Rect( 227,  35, 275, 105 )	// German
};

static const Common::Rect _amigaLanguageSelectBlocks[4] = {
	Common::Rect(  -1,  -1,  -1,  -1 ),	// Italian: not supported by Amiga multi-lingual version
	Common::Rect( 129,  85, 177, 155 ),	// French
	Common::Rect( 178,  60, 226, 130 ),	// English
	Common::Rect( 227,  35, 275, 105 )	// German
};


uint16 Parallaction_ns::guiChooseLanguage() {

	const Common::Rect *blocks;

	if (getPlatform() == Common::kPlatformAmiga) {
		if (!(getFeatures() & GF_LANG_MULT)) {
			if (getFeatures() & GF_DEMO) {
				return 1;		// Amiga Demo supports English
			} else {
				return 0;		// The only other non multi-lingual version just supports Italian
			}
		}

		blocks = _amigaLanguageSelectBlocks;
	} else {
		blocks = _dosLanguageSelectBlocks;
	}

	// user can choose language in dos version
	showSlide("lingua");
	_gfx->displayString(60, 30, "SELECT LANGUAGE", 1);
	setArrowCursor();

	int selection = -1;
	while (selection == -1) {
		waitUntilLeftClick();
		for (uint16 i = 0; i < 4; i++) {
			if (blocks[i].contains(_mousePos)) {
				selection = i;
				break;
			}
		}
	}

	beep();

	return selection;
}



uint16 Parallaction_ns::guiSelectGame() {
//	  printf("selectGame()\n");

	showSlide("restore");

	uint16 _si = 0;
	uint16 _di = 3;

	_mouseButtons = kMouseNone;
	while (_mouseButtons != kMouseLeftUp) {

		readInput();

		_si = (_mousePos.x > 160) ? 1 : 0;

		if (_si != _di) {
			_di = _si;

			_gfx->copyScreen(Gfx::kBitBack, Gfx::kBitFront);
			if (_si != 0) {
				// load a game
				_gfx->displayString(60, 30, loadGameMsg[_language], 1);
			} else {
				// new game
				_gfx->displayString(60, 30, newGameMsg[_language], 1);
			}

		}

		_gfx->updateScreen();
		g_system->delayMillis(30);
	}

	return _si ? LOAD_GAME : NEW_GAME;
}

static const Common::Rect codeSelectBlocks[9] = {
	Common::Rect( 111, 129, 127, 153 ),		// na
	Common::Rect( 128, 120, 144, 144 ),		// wa
	Common::Rect( 145, 111, 161, 135 ),		// ra
	Common::Rect( 162, 102, 178, 126 ),		// ri
	Common::Rect( 179, 93, 195, 117 ),		// i
	Common::Rect( 196, 84, 212, 108 ),		// ne
	Common::Rect( 213, 75, 229, 99 ),		// ho
	Common::Rect( 230, 66, 246, 90 ),		// ki
	Common::Rect( 247, 57, 263, 81 )		// ka
};

static const Common::Rect codeTrueBlocks[9] = {
	Common::Rect( 112, 130, 128, 154 ),
	Common::Rect( 129, 121, 145, 145 ),
	Common::Rect( 146, 112, 162, 136 ),
	Common::Rect( 163, 103, 179, 127 ),
	Common::Rect( 180, 94, 196, 118 ),
	Common::Rect( 197, 85, 213, 109 ),
	Common::Rect( 214, 76, 230, 100 ),
	Common::Rect( 231, 67, 247, 91 ),
	Common::Rect( 248, 58, 264, 82 )
};


int Parallaction_ns::guiGetSelectedBlock(const Common::Point &p) {

	int selection = -1;

	for (uint16 i = 0; i < 9; i++) {
		if (codeSelectBlocks[i].contains(p)) {
			selection = i;
			break;
		}
	}

	if ((selection != -1) && (getPlatform() == Common::kPlatformAmiga)) {
		_gfx->invertRect(Gfx::kBitFront, codeTrueBlocks[selection]);
		_gfx->updateScreen();
		beep();
		g_system->delayMillis(100);
		_gfx->invertRect(Gfx::kBitFront, codeTrueBlocks[selection]);
		_gfx->updateScreen();
	}

	return selection;
}


//
//	character selection and protection
//
int Parallaction_ns::guiSelectCharacter() {
	debugC(1, kDebugMenu, "Parallaction_ns::guiselectCharacter()");

	Graphics::Surface v14;
	v14.create(BLOCK_WIDTH, BLOCK_HEIGHT, 1);

	setArrowCursor();
	_soundMan->stopMusic();

	_gfx->setFont(_menuFont);
	_gfx->setFontShadow(true);

	_disk->selectArchive((getFeatures() & GF_LANG_MULT) ? "disk1" : "disk0");

	showSlide("password");	// loads background into kBitBack buffer


	const uint16 (*keys)[PASSWORD_LEN] = (getPlatform() == Common::kPlatformAmiga && (getFeatures() & GF_LANG_MULT)) ? _amigaKeys : _pcKeys;
	uint16 _di = 0;
    byte points[3] = { 0, 0, 0 };

    bool fail;

	while (true) {

        points[0] = 0;
        points[1] = 0;
        points[2] = 0;
        fail = false;

		_gfx->displayString(60, 30, introMsg1[_language], 1);			// displays message

		_di = 0;
		while (_di < PASSWORD_LEN) {

			waitUntilLeftClick();
			int _si = guiGetSelectedBlock(_mousePos);

			if (_si != -1) {
				_gfx->grabRect((byte*)v14.pixels, codeTrueBlocks[_si], Gfx::kBitFront, BLOCK_WIDTH);
				_gfx->flatBlitCnv(&v14, _di * SLOT_WIDTH + SLOT_X, SLOT_Y, Gfx::kBitFront);

                if (keys[0][_di] == _si) {
                    points[0]++;
                } else
                if (keys[1][_di] == _si) {
                    points[1]++;
                } else
                if (keys[2][_di] == _si) {
                    points[2]++;
                } else {
                    fail = true;
                }

                // build user preference
                points[0] += (keys[0][_di] == _si);
                points[1] += (keys[1][_di] == _si);
                points[2] += (keys[2][_di] == _si);

                _di++;
			}

		}

        if (!fail) {
            break;
        }

		_gfx->copyScreen(Gfx::kBitBack, Gfx::kBitFront);
		_gfx->displayString(60, 30, introMsg2[_language], 1);
		_gfx->updateScreen();

		g_system->delayMillis(2000);

		_gfx->copyScreen(Gfx::kBitBack, Gfx::kBitFront);
	}

	_gfx->setBlackPalette();
	_gfx->updateScreen();

	v14.free();


    // actually select character

	int character = -1;
	if (points[0] >= points[1] && points[0] >= points[2]) {
        character = CHAR_DINO;
	} else
	if (points[1] >= points[0] && points[1] >= points[2]) {
        character = CHAR_DONNA;
	} else
	if (points[2] >= points[0] && points[2] >= points[1]) {
        character = CHAR_DOUGH;
	} else {
        error("If you read this, either your CPU or transivity is broken (we believe the former).");
	}

    return character;
}


} // namespace Parallaction
