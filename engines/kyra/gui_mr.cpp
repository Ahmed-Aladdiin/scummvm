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

#include "kyra/gui_mr.h"
#include "kyra/kyra_mr.h"
#include "kyra/text_mr.h"
#include "kyra/wsamovie.h"
#include "kyra/resource.h"
#include "kyra/sound.h"
#include "kyra/timer.h"

#include "common/savefile.h"

namespace Kyra {

void KyraEngine_MR::loadButtonShapes() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::loadButtonShapes()");
	_res->exists("BUTTONS.SHP", true);
	uint8 *data = _res->fileData("BUTTONS.SHP", 0);
	assert(data);
	for (int i = 0; i <= 10; ++i)
		addShapeToPool(data, 0x1C7+i, i);
	delete[] data;

	Button::Callback callback1 = BUTTON_FUNCTOR(KyraEngine_MR, this, &KyraEngine_MR::callbackButton1);
	Button::Callback callback2 = BUTTON_FUNCTOR(KyraEngine_MR, this, &KyraEngine_MR::callbackButton2);
	Button::Callback callback3 = BUTTON_FUNCTOR(KyraEngine_MR, this, &KyraEngine_MR::callbackButton3);

	_gui->getScrollUpButton()->data0Callback = callback1;
	_gui->getScrollUpButton()->data1Callback = callback2;
	_gui->getScrollUpButton()->data2Callback = callback3;
	_gui->getScrollDownButton()->data0Callback = callback1;
	_gui->getScrollDownButton()->data1Callback = callback2;
	_gui->getScrollDownButton()->data2Callback = callback3;

	_mainButtonData[0].data0Callback = callback1;
	_mainButtonData[0].data1Callback = callback2;
	_mainButtonData[0].data2Callback = callback3;
}

int KyraEngine_MR::callbackButton1(Button *button) {
	const uint8 *shapePtr = 0;
	if (button->index == 1)
		shapePtr = getShapePtr(0x1CD);
	else if (button->index == 22)
		shapePtr = getShapePtr(0x1C7);
	else if (button->index == 23)
		shapePtr = getShapePtr(0x1CA);

	if (shapePtr)
		_screen->drawShape(0, shapePtr, button->x, button->y, 0, 0, 0);

	return 0;
}

int KyraEngine_MR::callbackButton2(Button *button) {
	const uint8 *shapePtr = 0;
	if (button->index == 1)
		shapePtr = getShapePtr(0x1CE);
	else if (button->index == 22)
		shapePtr = getShapePtr(0x1C9);
	else if (button->index == 23)
		shapePtr = getShapePtr(0x1CC);

	if (shapePtr)
		_screen->drawShape(0, shapePtr, button->x, button->y, 0, 0, 0);

	return 0;
}

int KyraEngine_MR::callbackButton3(Button *button) {
	const uint8 *shapePtr = 0;
	if (button->index == 1)
		shapePtr = getShapePtr(0x1CE);
	else if (button->index == 22)
		shapePtr = getShapePtr(0x1C8);
	else if (button->index == 23)
		shapePtr = getShapePtr(0x1CB);

	if (shapePtr)
		_screen->drawShape(0, shapePtr, button->x, button->y, 0, 0, 0);

	return 0;
}

void KyraEngine_MR::showMessage(const char *string, uint8 c0, uint8 c1) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::showMessage('%s', %d, %d)", string, c0, c1);
	_shownMessage = string;
	_screen->hideMouse();

	restoreCommandLine();
	_restoreCommandLine = false;

	if (string) {
		int x = _text->getCenterStringX(string, 0, 320);
		int pageBackUp = _screen->_curPage;
		_screen->_curPage = 0;
		_text->printText(string, x, _commandLineY, c0, c1, 0);
		_screen->_curPage = pageBackUp;
		_screen->updateScreen();
		setCommandLineRestoreTimer(7);
	}

	_screen->showMouse();
}

void KyraEngine_MR::showMessageFromCCode(int string, uint8 c0, int) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::showMessageFromCCode(%d, %d, -)", string, c0);
	showMessage((const char*)getTableEntry(_cCodeFile, string), c0, 0xF0);
}

void KyraEngine_MR::updateItemCommand(int item, int str, uint8 c0) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::updateItemCommand(%d, %d, %d)", item, str, c0);
	char buffer[100];
	char *src = (char*)getTableEntry(_itemFile, item);

	while (*src != ' ')
		++src;
	++src;

	*src = toupper(*src);

	strcpy(buffer, src);
	strcat(buffer, " ");
	strcat(buffer, (const char*)getTableEntry(_cCodeFile, str));

	showMessage(buffer, c0, 0xF0);
}

void KyraEngine_MR::updateCommandLine() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::updateCommandLine()");
	if (_restoreCommandLine) {
		restoreCommandLine();
		_restoreCommandLine = false;
	}
}

void KyraEngine_MR::restoreCommandLine() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::restoreCommandLine()");
	int y = _inventoryState ? 144 : 188;
	_screen->copyBlockToPage(0, 0, y, 320, 12, _interfaceCommandLine);
}

void KyraEngine_MR::updateCLState() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::updateCLState()");
	if (_inventoryState)
		_commandLineY = 145;
	else
		_commandLineY = 189;
}

void KyraEngine_MR::showInventory() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::showInventory()");
	if (!_screen->isMouseVisible())
		return;
	if (queryGameFlag(3))
		return;

	_screen->copyBlockToPage(3, 0, 0, 320, 56, _interface);
	drawMalcolmsMoodText();

	_inventoryState = true;
	updateCLState();

	redrawInventory(30);
	drawMalcolmsMoodPointer(-1, 30);
	drawScore(30, 215, 191);
	
	if (queryGameFlag(0x97))
		drawJestersStaff(1, 30);
	
	_screen->hideMouse();

	if (_itemInHand < 0) {
		_handItemSet = -1;
		_screen->setMouseCursor(0, 0, getShapePtr(0));
	}

	_screen->copyRegion(0, 188, 0, 0, 320, 12, 0, 2, Screen::CR_NO_P_CHECK);

	if (_inventoryScrollSpeed == -1) {
		uint32 endTime = _system->getMillis() + _tickLength * 15;
		int times = 0;
		while (_system->getMillis() < endTime) {
			_screen->copyRegion(0, 188, 0, 0, 320, 12, 0, 2, Screen::CR_NO_P_CHECK);
			_screen->copyRegion(0, 188, 0, 0, 320, 12, 0, 2, Screen::CR_NO_P_CHECK);
			++times;
		}

		times = MAX(times, 1);

		int speed = 60 / times;
		if (speed <= 1)
			_inventoryScrollSpeed = 1;
		else if (speed >= 8)
			_inventoryScrollSpeed = 8;
		else
			_inventoryScrollSpeed = speed;
	}

	int height = 12;
	int y = 188;
	int times = 0;
	uint32 waitTill = _system->getMillis() + _tickLength;

	while (y > 144) {
		_screen->copyRegion(0, 0, 0, y, 320, height, 2, 0, Screen::CR_NO_P_CHECK);
		_screen->updateScreen();

		++times;
		if (_inventoryScrollSpeed == 1 && times == 3) {
			while (waitTill > _system->getMillis())
				_system->delayMillis(10);
			times = 0;
			waitTill = _system->getMillis() + _tickLength;
		}
		
		height += _inventoryScrollSpeed;
		y -= _inventoryScrollSpeed;
	}

	_screen->copyRegion(0, 0, 0, 144, 320, 56, 2, 0, Screen::CR_NO_P_CHECK);
	_screen->updateScreen();

	initMainButtonList(false);

	restorePage3();
	_screen->showMouse();
}

void KyraEngine_MR::hideInventory() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::hideInventory()");
	if (queryGameFlag(3))
		return;

	_inventoryState = false;
	updateCLState();
	initMainButtonList(true);
	
	_screen->copyBlockToPage(3, 0, 0, 320, 56, _interface);
	_screen->hideMouse();
	
	restorePage3();
	flagAnimObjsForRefresh();
	drawAnimObjects();
	_screen->copyRegion(0, 144, 0, 0, 320, 56, 0, 2, Screen::CR_NO_P_CHECK);

	if (_inventoryScrollSpeed == -1) {
		uint32 endTime = _system->getMillis() + _tickLength * 15;
		int times = 0;
		while (_system->getMillis() < endTime) {
			_screen->copyRegion(0, 144, 0, 0, 320, 12, 0, 2, Screen::CR_NO_P_CHECK);
			_screen->copyRegion(0, 144, 0, 0, 320, 12, 0, 2, Screen::CR_NO_P_CHECK);
			++times;
		}

		times = MAX(times, 1);

		int speed = 60 / times;
		if (speed <= 1)
			_inventoryScrollSpeed = 1;
		else if (speed >= 8)
			_inventoryScrollSpeed = 8;
		else
			_inventoryScrollSpeed = speed;
	}

	int y = 144;
	int y2 = 144 + _inventoryScrollSpeed;
	uint32 waitTill = _system->getMillis() + _tickLength;
	int times = 0;

	while (y2 < 188) {
		_screen->copyRegion(0, 0, 0, y2, 320, 56, 2, 0, Screen::CR_NO_P_CHECK);
		_screen->copyRegion(0, y, 0, y, 320, _inventoryScrollSpeed, 2, 0, Screen::CR_NO_P_CHECK);
		_screen->updateScreen();

		++times;
		if (_inventoryScrollSpeed == 1 && times == 3) {
			while (waitTill > _system->getMillis())
				_system->delayMillis(10);
			times = 0;
			waitTill = _system->getMillis() + _tickLength;
		}
		
		y += _inventoryScrollSpeed;
		y2 += _inventoryScrollSpeed;
	}

	_screen->copyRegion(0, 0, 0, 188, 320, 56, 2, 0, Screen::CR_NO_P_CHECK);
	_screen->copyRegion(0, y, 0, y, 320, 188-y, 2, 0, Screen::CR_NO_P_CHECK);
	_screen->showMouse();
}

void KyraEngine_MR::drawMalcolmsMoodText() {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::drawMalcolmsMoodText()");
	static const int stringId[] = { 0x32, 0x37, 0x3C };

	if (queryGameFlag(0x219))
		return;

	const char *string = (const char*)getTableEntry(_cCodeFile, stringId[_malcolmsMood]);

	Screen::FontId oldFont = _screen->setFont(Screen::FID_8_FNT);
	_screen->_charWidth = -2;

	int width = _screen->getTextWidth(string);

	_screen->_charWidth = 0;
	_screen->setFont(oldFont);

	int pageBackUp = _screen->_curPage;
	const int x = 280 - (width / 2);
	int y = 0;
	if (_inventoryState) {
		y = 189;
		_screen->_curPage = 0;
	} else {
		y = 45;
		_screen->_curPage = 2;
	}

	_screen->hideMouse();
	_screen->drawShape(_screen->_curPage, getShapePtr(432), 244, 189, 0, 0);
	_text->printText(string, x, y+1, 0xFF, 0xF0, 0x00);
	_screen->showMouse();
	_screen->_curPage = pageBackUp;
}

void KyraEngine_MR::drawMalcolmsMoodPointer(int frame, int page) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::drawMalcolmsMoodPointer(%d, %d)", frame, page);
	static const uint8 stateTable[] = {
		1, 6, 11
	};

	if (frame == -1)
		frame = stateTable[_malcolmsMood];
	if (queryGameFlag(0x219))
		frame = 13;

	if (page == 0) {
		_invWsa->setX(0);
		_invWsa->setY(0);
		_invWsa->setDrawPage(0);
		_invWsa->displayFrame(frame, 0);
		_screen->updateScreen();
	} else if (page == 30) {
		_invWsa->setX(0);
		_invWsa->setY(-144);
		_invWsa->setDrawPage(2);
		_invWsa->displayFrame(frame, 0);
	}

	_invWsaFrame = frame;
}

void KyraEngine_MR::drawJestersStaff(int type, int page) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::drawJestersStaff(%d, %d)", type, page);
	int y = 155;
	if (page == 30) {
		page = 2;
		y -= 144;
	}

	int shape = (type != 0) ? 454 : 453;
	_screen->drawShape(page, getShapePtr(shape), 217, y, 0, 0);
}

void KyraEngine_MR::drawScore(int page, int x, int y) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::drawScore(%d, %d, %d)", page, x, y);
	if (page == 30) {
		page = 2;
		y -= 144;
	}

	int shape1 = _score / 100;
	int shape2 = (_score - shape1*100) / 10;
	int shape3 = _score % 10;

	_screen->drawShape(page, getShapePtr(shape1+433), x, y, 0, 0);
	x += 8;
	_screen->drawShape(page, getShapePtr(shape2+433), x, y, 0, 0);
	x += 8;
	_screen->drawShape(page, getShapePtr(shape3+433), x, y, 0, 0);
}

void KyraEngine_MR::drawScoreCounting(int oldScore, int newScore, int drawOld, const int x) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::drawScoreCounting(%d, %d, %d, %d)", oldScore, newScore, drawOld, x);
	int y = 189;
	if (_inventoryState)
		y -= 44;

	int old100 = oldScore / 100;
	int old010 = (oldScore - old100*100) / 10;
	int old001 = oldScore % 10;

	int new100 = newScore / 100;
	int new010 = (newScore - new100*100) / 10;
	int new001 = newScore % 10;

	if (drawOld) {
		_screen->drawShape(0, getShapePtr(old100+433), x +  0, y, 0, 0);
		_screen->drawShape(0, getShapePtr(old010+433), x +  8, y, 0, 0);
		_screen->drawShape(0, getShapePtr(old001+433), x + 16, y, 0, 0);
	}

	if (old100 != new100)
		_screen->drawShape(0, getShapePtr(old100+443), x +  0, y, 0, 0);

	if (old010 != new010)
		_screen->drawShape(0, getShapePtr(old010+443), x +  8, y, 0, 0);

	_screen->drawShape(0, getShapePtr(old001+443), x + 16, y, 0, 0);

	_screen->updateScreen();

	_screen->drawShape(0, getShapePtr(new100+433), x +  0, y, 0, 0);
	_screen->drawShape(0, getShapePtr(new010+433), x +  8, y, 0, 0);
	_screen->drawShape(0, getShapePtr(new001+433), x + 16, y, 0, 0);
}

int KyraEngine_MR::getScoreX(const char *str) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::getScoreX('%s')", str);
	Screen::FontId oldFont = _screen->setFont(Screen::FID_8_FNT);
	_screen->_charWidth = -2;

	int width = _screen->getTextWidth(str);
	int x = 160 + (width / 2) - 32;

	_screen->setFont(oldFont);
	_screen->_charWidth = 0;
	return x;
}

void KyraEngine_MR::redrawInventory(int page) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::redrawInventory(%d)", page);
	int yOffset = 0;

	if (page == 30) {
		page = 2;
		yOffset = -144;
	}

	int pageBackUp = _screen->_curPage;
	_screen->_curPage = page;
	_screen->hideMouse();

	for (int i = 0; i < 10; ++i) {
		clearInventorySlot(i, page);
		if (_mainCharacter.inventory[i] != 0xFFFF) {
			_screen->drawShape(page, getShapePtr(_mainCharacter.inventory[i]+248), _inventoryX[i], _inventoryY[i] + yOffset, 0, 0);
			drawInventorySlot(page, _mainCharacter.inventory[i], i);
		}
	}

	_screen->showMouse();
	_screen->_curPage = pageBackUp;
	
	if (page == 0 || page == 1)
		_screen->updateScreen();	
}

void KyraEngine_MR::clearInventorySlot(int slot, int page) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::clearInventorySlot(%d, %d)", slot, page);
	int yOffset = 0;
	if (page == 30) {
		page = 2;
		yOffset = -144;
	}

	_screen->drawShape(page, getShapePtr(slot+422), _inventoryX[slot], _inventoryY[slot] + yOffset, 0, 0);
}

void KyraEngine_MR::drawInventorySlot(int page, int item, int slot) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::drawInventorySlot(%d, %d, %d)", page, item, slot);
	int yOffset = 0;
	if (page == 30) {
		page = 2;
		yOffset = -144;
	}

	_screen->drawShape(page, getShapePtr(item+248), _inventoryX[slot], _inventoryY[slot] + yOffset, 0, 0);
}

int KyraEngine_MR::buttonInventory(Button *button) {
	debugC(9, kDebugLevelMain, "KyraEngine_MR::buttonInventory(%p)", (const void*)button);
	setNextIdleAnimTimer();
	if (!_enableInventory || !_inventoryState || !_screen->isMouseVisible())
		return 0;

	const int slot = button->index - 5;
	const int16 slotItem = (int16)_mainCharacter.inventory[slot];
	if (_itemInHand == -1) {
		if (slotItem == -1)
			return 0;

		_screen->hideMouse();
		clearInventorySlot(slot, 0);
		snd_playSoundEffect(0x0B, 0xC8);
		setMouseCursor(slotItem);
		updateItemCommand(slotItem, (_lang == 1) ? getItemCommandStringPickUp(slotItem) : 0, 0xFF);
		_itemInHand = slotItem;
		_mainCharacter.inventory[slot] = 0xFFFF;
		_screen->showMouse();
	} else if (_itemInHand == 27) {
		if (_chatText)
			return 0;
		return buttonJesterStaff(&_mainButtonData[3]);
	} else {
		if (slotItem >= 0) {
			if (itemInventoryMagic(_itemInHand, slot))
				return 0;

			snd_playSoundEffect(0x0B, 0xC8);

			_screen->hideMouse();
			clearInventorySlot(slot, 0);
			drawInventorySlot(0, _itemInHand, slot);
			setMouseCursor(slotItem);
			updateItemCommand(slotItem, (_lang == 1) ? getItemCommandStringPickUp(slotItem) : 0, 0xFF);
			_mainCharacter.inventory[slot] = _itemInHand;
			_itemInHand = slotItem;
			_screen->showMouse();
		} else {
			snd_playSoundEffect(0x0C, 0xC8);
			_screen->hideMouse();
			drawInventorySlot(0, _itemInHand, slot);
			_screen->setMouseCursor(0, 0, getShapePtr(0));
			updateItemCommand(_itemInHand, (_lang == 1) ? getItemCommandStringInv(_itemInHand) : 2, 0xFF);
			_screen->showMouse();
			_mainCharacter.inventory[slot] = _itemInHand;
			_itemInHand = -1;
		}
	}

	return 0;
}

int KyraEngine_MR::buttonMoodChange(Button *button) {
	if (queryGameFlag(0x219)) {
		snd_playSoundEffect(0x0D, 0xC8);
		return 0;
	}

	static const uint8 frameTable[] = { 1, 6, 11 };

	if (_mouseX >= 245 && _mouseX <= 267 && _mouseY >= 159 && _mouseY <= 198)
		_malcolmsMood = 0;
	else if (_mouseX >= 268 && _mouseX <= 289 && _mouseY >= 159 && _mouseY <= 198)
		_malcolmsMood = 1;
	else if (_mouseX >= 290 && _mouseX <= 312 && _mouseY >= 159 && _mouseY <= 198)
		_malcolmsMood = 2;

	int direction = (_invWsaFrame > frameTable[_malcolmsMood]) ? -1 : 1;

	if (_invWsaFrame != frameTable[_malcolmsMood]) {
		_screen->hideMouse();
		setGameFlag(3);

		snd_playSoundEffect(0x2E, 0xC8);

		while (_invWsaFrame != frameTable[_malcolmsMood]) {
			uint32 endTime = _system->getMillis() + 2 * _tickLength;
			_invWsaFrame += direction;

			drawMalcolmsMoodPointer(_invWsaFrame, 0);
			_screen->updateScreen();

			while (endTime > _system->getMillis()) {
				update();
				_system->delayMillis(10);
			}
		}

		resetGameFlag(3);
		_screen->showMouse();

		drawMalcolmsMoodText();
		updateDlgIndex();
		
		EMCData data;
		EMCState state;
		memset(&data, 0, sizeof(data));
		memset(&state, 0, sizeof(state));

		_res->exists("_ACTOR.EMC", true);
		_emc->load("_ACTOR.EMC", &data, &_opcodes);
		_emc->init(&state, &data);
		_emc->start(&state, 1);

		int vocHigh = _vocHigh;
		_vocHigh = 200;
		_useActorBuffer = true;

		while (_emc->isValid(&state))
			_emc->run(&state);

		_useActorBuffer = false;
		_vocHigh = vocHigh;
		_emc->unload(&data);
	}

	return 0;
}

int KyraEngine_MR::buttonShowScore(Button *button) {
	strcpy(_stringBuffer, (const char*)getTableEntry(_cCodeFile, 18));

	char *buffer = _stringBuffer;

	while (*buffer != '%')
		++buffer;

	buffer[0] = (_score / 100) + '0';
	buffer[1] = ((_score % 100) / 10) + '0';
	buffer[2] = (_score % 10) + '0';

	while (*buffer != '%')
		++buffer;

	buffer[0] = (_scoreMax / 100) + '0';
	buffer[1] = ((_scoreMax % 100) / 10) + '0';
	buffer[2] = (_scoreMax % 10) + '0';

	showMessage(_stringBuffer, 0xFF, 0xF0);
	return 0;
}

int KyraEngine_MR::buttonJesterStaff(Button *button) {
	makeCharFacingMouse();
	if (_itemInHand == 27) {
		_screen->hideMouse();
		removeHandItem();
		snd_playSoundEffect(0x0C, 0xC8);
		drawJestersStaff(1, 0);
		updateItemCommand(27, 2, 0xFF);
		setGameFlag(0x97);
		_screen->showMouse();
	} else if (_itemInHand == -1) {
		if (queryGameFlag(0x97)) {
			_screen->hideMouse();
			snd_playSoundEffect(0x0B, 0xC8);
			setHandItem(27);
			drawJestersStaff(0, 0);
			updateItemCommand(27, 0, 0xFF);
			resetGameFlag(0x97);
			_screen->showMouse();
		} else {
			if (queryGameFlag(0x2F))
				objectChat((const char*)getTableEntry(_cCodeFile, 20), 0, 204, 20);
			else
				objectChat((const char*)getTableEntry(_cCodeFile, 25), 0, 204, 25);
		}
	} else {
		objectChat((const char*)getTableEntry(_cCodeFile, 30), 0, 204, 30);
	}
	return 0;
}

#pragma mark -

GUI_MR::GUI_MR(KyraEngine_MR *vm) : GUI_v2(vm), _vm(vm), _screen(vm->_screen) {
}

void GUI_MR::flagButtonEnable(Button *button) {
	if (!button)
		return;

	if (button->flags & 8) {
		button->flags &= ~8;
		processButton(button);
	}
}

void GUI_MR::flagButtonDisable(Button *button) {
	if (!button)
		return;

	if (!(button->flags & 8)) {
		button->flags |= 8;
		processButton(button);
	}
}

void GUI_MR::getInput() {
	_vm->musicUpdate(0);
	GUI_v2::getInput();
}

const char *GUI_MR::getMenuTitle(const Menu &menu) {
	if (!menu.menuNameId)
		return 0;

	return (const char *)_vm->getTableEntry(_vm->_optionsFile, menu.menuNameId);
}

const char *GUI_MR::getMenuItemTitle(const MenuItem &menuItem) {
	if (!menuItem.itemId)
		return 0;

	return (const char *)_vm->getTableEntry(_vm->_optionsFile, menuItem.itemId);
}

const char *GUI_MR::getMenuItemLabel(const MenuItem &menuItem) {
	if (!menuItem.labelId)
		return 0;

	return (const char *)_vm->getTableEntry(_vm->_optionsFile, menuItem.labelId);
}

char *GUI_MR::getTableString(int id) {
	return (char *)_vm->getTableEntry(_vm->_optionsFile, id);
}

int GUI_MR::redrawButtonCallback(Button *button) {
	if (!_displayMenu)
		return 0;

	_screen->hideMouse();
	_screen->drawBox(button->x + 1, button->y + 1, button->x + button->width - 1, button->y + button->height - 1, 0xD0);
	_screen->showMouse();

	return 0;
}

int GUI_MR::redrawShadedButtonCallback(Button *button) {
	if (!_displayMenu)
		return 0;

	_screen->hideMouse();
	_screen->drawShadedBox(button->x, button->y, button->x + button->width, button->y + button->height, 0xD1, 0xCF);
	_screen->showMouse();

	return 0;
}
void GUI_MR::resetState(int item) {
	_vm->_timer->resetNextRun();
	_vm->setNextIdleAnimTimer();
	_isDeathMenu = false;
	if (!_loadedSave) {
		_vm->setHandItem(item);
	} else {
		_vm->setHandItem(_vm->_itemInHand);
		_vm->setCommandLineRestoreTimer(7);
		_vm->_shownMessage = " ";
		_vm->_restoreCommandLine = false;
	}
	_buttonListChanged = true;
}

int GUI_MR::quitGame(Button *caller) {
	updateMenuButton(caller);
	if (choiceDialog(0x0F, 1)) {
		_displayMenu = false;
		_vm->_runFlag = false;
		_vm->fadeOutMusic(60);
		_screen->fadeToBlack(60);
		_screen->clearCurPage();
	}

	if (_vm->_runFlag) {
		initMenu(*_currentMenu);
		updateAllMenuButtons();
	}

	return 0;
}

int GUI_MR::optionsButton(Button *button) {
	_vm->musicUpdate(0);

	_screen->hideMouse();
	updateButton(&_vm->_mainButtonData[0]);
	_screen->showMouse();

	if (!_vm->_inventoryState && button && !_vm->_menuDirectlyToLoad)
		return 0;

	_restartGame = false;
	_reloadTemporarySave = false;

	if (!_screen->isMouseVisible() && button && !_vm->_menuDirectlyToLoad)
		return 0;

	_vm->showMessage(0, 0xF0, 0xF0);

	if (_vm->_handItemSet < -1) {
		_vm->_handItemSet = -1;
		_screen->hideMouse();
		_screen->setMouseCursor(1, 1, _vm->getShapePtr(0));
		_screen->showMouse();
		return 0;
	}

	int oldHandItem = _vm->_itemInHand;
	_screen->setMouseCursor(0, 0, _vm->getShapePtr(0));
	_vm->musicUpdate(0);

	_displayMenu = true;
	for (int i = 0; i < 4; ++i) {
		if (_vm->_musicSoundChannel != i)
			_vm->_soundDigital->stopSound(i);
	}

	for (uint i = 0; i < ARRAYSIZE(_menuButtons); ++i) {
		_menuButtons[i].data0Val1 = _menuButtons[i].data1Val1 = _menuButtons[i].data2Val1 = 4;
		_menuButtons[i].data0Callback = _redrawShadedButtonFunctor;
		_menuButtons[i].data1Callback = _menuButtons[i].data2Callback = _redrawButtonFunctor;
	}

	initMenuLayout(_mainMenu);
	initMenuLayout(_gameOptions);
	initMenuLayout(_audioOptions);
	initMenuLayout(_choiceMenu);
	_loadMenu.numberOfItems = 6;
	initMenuLayout(_loadMenu);
	initMenuLayout(_saveMenu);
	initMenuLayout(_savenameMenu);
	initMenuLayout(_deathMenu);
	
	_currentMenu = &_mainMenu;

	_vm->musicUpdate(0);

	if (_vm->_menuDirectlyToLoad) {
		backUpPage1(_vm->_screenBuffer);

		_loadedSave = false;
		
		--_loadMenu.numberOfItems;
		loadMenu(0);
		++_loadMenu.numberOfItems;

		if (_loadedSave) {
			if (_restartGame)
				_vm->_itemInHand = -1;
		} else {
			restorePage1(_vm->_screenBuffer);
		}

		resetState(-1);
		_vm->_menuDirectlyToLoad = false;
		return 0;
	}

	if (!button) {
		_currentMenu = &_deathMenu;
		_isDeathMenu = true;
	} else {
		_isDeathMenu = false;
	}

	_vm->musicUpdate(0);
	backUpPage1(_vm->_screenBuffer);
	initMenu(*_currentMenu);
	_madeSave = false;
	_loadedSave = false;
	_vm->_itemInHand = -1;
	updateAllMenuButtons();

	if (_isDeathMenu) {
		while (!_screen->isMouseVisible())
			_screen->showMouse();
	}

	while (_displayMenu) {
		processHighlights(*_currentMenu, _vm->_mouseX, _vm->_mouseY);
		getInput();
	}

	if (_vm->_runFlag && !_loadedSave && !_madeSave) {
		restorePalette();
		restorePage1(_vm->_screenBuffer);
	}

	if (_vm->_runFlag)
		updateMenuButton(&_vm->_mainButtonData[0]);

	resetState(oldHandItem);

	if (!_loadedSave && _reloadTemporarySave) {
		_vm->_unkSceneScreenFlag1 = true;
		_vm->loadGame(_vm->getSavegameFilename(999));
		_vm->_saveFileMan->removeSavefile(_vm->getSavegameFilename(999));
		_vm->_unkSceneScreenFlag1 = false;
	}

	return 0;
}

int GUI_MR::loadMenu(Button *caller) {
	updateSaveList();

	if (!_vm->_menuDirectlyToLoad) {
		updateMenuButton(caller);
		restorePage1(_vm->_screenBuffer);
		backUpPage1(_vm->_screenBuffer);
	}

	_savegameOffset = 0;
	setupSavegameNames(_loadMenu, 5);
	initMenu(_loadMenu);
	_isLoadMenu = true;
	_noLoadProcess = false;
	_vm->_gameToLoad = -1;
	updateAllMenuButtons();

	_screen->updateScreen();
	while (_isLoadMenu) {
		processHighlights(_loadMenu, _vm->_mouseX, _vm->_mouseY);
		getInput();
	}

	if (_noLoadProcess) {
		if (!_vm->_menuDirectlyToLoad) {
			restorePage1(_vm->_screenBuffer);
			backUpPage1(_vm->_screenBuffer);
			initMenu(*_currentMenu);
			updateAllMenuButtons();
		}
	} else if (_vm->_gameToLoad >= 0) {
		restorePage1(_vm->_screenBuffer);
		restorePalette();
		_vm->_menuDirectlyToLoad = false;
		_vm->loadGame(_vm->getSavegameFilename(_vm->_gameToLoad));
		if (_vm->_gameToLoad == 0) {
			_restartGame = true;
			_vm->runStartupScript(1, 1);
		}
		_displayMenu = false;
		_loadedSave = true;
	}

	return 0;
}

int GUI_MR::loadSecondChance(Button *button) {
	updateMenuButton(button);

	_vm->_gameToLoad = 999;
	restorePage1(_vm->_screenBuffer);
	_vm->loadGame(_vm->getSavegameFilename(_vm->_gameToLoad));
	_displayMenu = false;
	_loadedSave = true;
	return 0;
}

int GUI_MR::gameOptions(Button *caller) {
	updateMenuButton(caller);
	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);
	bool textEnabled = _vm->textEnabled();
	int lang = _vm->_lang;

	setupOptionsButtons();
	initMenu(_gameOptions);
	_isOptionsMenu = true;

	while (_isOptionsMenu) {
		processHighlights(_gameOptions, _vm->_mouseX, _vm->_mouseY);
		getInput();
	}

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);

	if (textEnabled && !_vm->textEnabled() && !_vm->speechEnabled()) {
		_vm->_configVoice = 1;
		_vm->setVolume(KyraEngine::kVolumeSpeech, 75);
		choiceDialog(0x1E, 0);
	}

	if (_vm->_lang != lang) {
		_reloadTemporarySave = true;
		_vm->saveGame(_vm->getSavegameFilename(999), "Temporary Kyrandia 3 Savegame");
		if (!_vm->loadLanguageFile("ITEMS.", _vm->_itemFile))
			error("Couldn't load ITEMS");
		if (!_vm->loadLanguageFile("SCORE.", _vm->_scoreFile))
			error("Couldn't load SCORE");
		if (!_vm->loadLanguageFile("C_CODE.", _vm->_cCodeFile))
			error("Couldn't load C_CODE");
		if (!_vm->loadLanguageFile("SCENES.", _vm->_scenesFile))
			error("Couldn't load SCENES");
		if (!_vm->loadLanguageFile("OPTIONS.", _vm->_optionsFile))
			error("Couldn't load OPTIONS");
		if (!_vm->loadLanguageFile("_ACTOR.", _vm->_actorFile))
			error("couldn't load _ACTOR");
	}

	_vm->writeSettings();

	initMenu(*_currentMenu);
	updateAllMenuButtons();
	return 0;
}

void GUI_MR::setupOptionsButtons() {
	_vm->musicUpdate(0);
	if (_vm->_configWalkspeed == 3)
		_gameOptions.item[0].itemId = 28;
	else
		_gameOptions.item[0].itemId = 27;

	if (_vm->textEnabled())
		_gameOptions.item[4].itemId = 18;
	else
		_gameOptions.item[4].itemId = 17;

	switch (_vm->_lang) {
	case 0:
		_gameOptions.item[1].itemId = 31;
		break;
	
	case 1:
		_gameOptions.item[1].itemId = 32;
		break;

	case 2:
		_gameOptions.item[1].itemId = 33;
		break;

	default:
		break;
	}

	if (_vm->_configStudio)
		_gameOptions.item[2].itemId = 18;
	else
		_gameOptions.item[2].itemId = 17;

	if (_vm->_configSkip)
		_gameOptions.item[3].itemId = 18;
	else
		_gameOptions.item[3].itemId = 17;
}

int GUI_MR::changeLanguage(Button *caller) {
	updateMenuButton(caller);
	if (!_vm->queryGameFlag(0x1B2)) {
		++_vm->_lang;
		_vm->_lang %= 3;
		setupOptionsButtons();
		renewHighlight(_gameOptions);
	}
	return 0;
}

int GUI_MR::toggleStudioSFX(Button *caller) {
	updateMenuButton(caller);
	_vm->_configStudio ^= 1;
	setupOptionsButtons();
	renewHighlight(_gameOptions);
	return 0;
}

int GUI_MR::toggleSkipSupport(Button *caller) {
	updateMenuButton(caller);
	_vm->_configSkip ^= 1;
	setupOptionsButtons();
	renewHighlight(_gameOptions);
	return 0;
}

int GUI_MR::audioOptions(Button *caller) {
	updateMenuButton(caller);

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);

	//if (_configHelium)
	//	_audioOptions.item[3].itemId = 18;
	//else
		_audioOptions.item[3].itemId = 17;

	initMenu(_audioOptions);

	const int menuX = _audioOptions.x;
	const int menuY = _audioOptions.y;

	const int maxButton = 3;	// 2 if voc is disabled

	for (int i = 0; i < maxButton; ++i) {
		int x = menuX + _sliderBarsPosition[i*2+0];
		int y = menuY + _sliderBarsPosition[i*2+1];
		_screen->drawShape(0, _vm->getShapePtr(0x1CF), x, y, 0, 0);
		drawSliderBar(i, _vm->getShapePtr(0x1D0));
		_sliderButtons[0][i].buttonCallback = _sliderHandlerFunctor;
		_sliderButtons[0][i].x = x;
		_sliderButtons[0][i].y = y;
		_menuButtonList = addButtonToList(_menuButtonList, &_sliderButtons[0][i]);
		_sliderButtons[2][i].buttonCallback = _sliderHandlerFunctor;
		_sliderButtons[2][i].x = x + 10;
		_sliderButtons[2][i].y = y;
		_menuButtonList = addButtonToList(_menuButtonList, &_sliderButtons[2][i]);
		_sliderButtons[1][i].buttonCallback = _sliderHandlerFunctor;
		_sliderButtons[1][i].x = x + 120;
		_sliderButtons[1][i].y = y;
		_menuButtonList = addButtonToList(_menuButtonList, &_sliderButtons[1][i]);
	}

	_isOptionsMenu = true;
	updateAllMenuButtons();
	bool speechEnabled = _vm->speechEnabled();
	while (_isOptionsMenu) {
		processHighlights(_audioOptions, _vm->_mouseX, _vm->_mouseY);
		getInput();
	}

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);
	if (speechEnabled && !_vm->textEnabled() && (!_vm->speechEnabled() || _vm->getVolume(KyraEngine::kVolumeSpeech) == 2)) {
		_vm->_configVoice = 0;
		choiceDialog(0x1D, 0);
	}

	_vm->writeSettings();

	initMenu(*_currentMenu);
	updateAllMenuButtons();
	return 0;
}

int GUI_MR::sliderHandler(Button *caller) {
	int button = 0;
	if (caller->index >= 24 && caller->index <= 27)
		button = caller->index - 24;
	else if (caller->index >= 28 && caller->index <= 31)
		button = caller->index - 28;
	else
		button = caller->index - 32;

	assert(button >= 0 && button <= 3);

	int oldVolume = _vm->getVolume(KyraEngine::kVolumeEntry(button));
	int newVolume = oldVolume;

	if (caller->index >= 24 && caller->index <= 27)
		newVolume -= 10;
	else if (caller->index >= 28 && caller->index <= 31)
		newVolume += 10;
	else
		newVolume = _vm->_mouseX - caller->x - 7;

	newVolume = MAX(2, newVolume);
	newVolume = MIN(97, newVolume);

	if (newVolume == oldVolume)
		return 0;

	int lastMusicCommand = -1;
	bool playSoundEffect = false;

	drawSliderBar(button, _vm->getShapePtr(0x1D1));

	if (button == 2) {
		if (_vm->textEnabled())
			_vm->_configVoice = 2;
		else
			_vm->_configVoice = 1;
	}

	_vm->setVolume(KyraEngine::kVolumeEntry(button), newVolume);

	switch (button) {
	case 0:
		lastMusicCommand = _vm->_lastMusicCommand;
		break;

	case 1:
		playSoundEffect = true;
		break;

	case 2:
		if (_vm->_voiceSoundChannel != _vm->_musicSoundChannel)
			_vm->_soundDigital->stopSound(_vm->_voiceSoundChannel);
		_vm->playVoice(200, 943);
		break;

	default:
		return 0;
	}

	drawSliderBar(button, _vm->getShapePtr(0x1D0));
	if (playSoundEffect)
		_vm->snd_playSoundEffect(0x18, 0xC8);
	else if (lastMusicCommand >= 0)
		_vm->snd_playWanderScoreViaMap(lastMusicCommand, 1);

	_screen->updateScreen();
	return 0;
}

void GUI_MR::drawSliderBar(int slider, const uint8 *shape) {
	const int menuX = _audioOptions.x;
	const int menuY = _audioOptions.y;
	int x = menuX + _sliderBarsPosition[slider*2+0] + 10;
	int y = menuY + _sliderBarsPosition[slider*2+1];

	int position = _vm->getVolume(KyraEngine::kVolumeEntry(slider));

	position = MAX(2, position);
	position = MIN(97, position);
	_screen->drawShape(0, shape, x+position, y, 0, 0);
}

} // end of namespace Kyra

