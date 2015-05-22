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

#ifndef SHERLOCK_EVENTS_H
#define SHERLOCK_EVENTS_H

#include "common/scummsys.h"
#include "common/events.h"
#include "common/stack.h"
#include "sherlock/resources.h"

namespace Sherlock {

#define GAME_FRAME_RATE 60
#define GAME_FRAME_TIME (1000 / GAME_FRAME_RATE)

enum CursorId { ARROW = 0, MAGNIFY = 1, WAIT = 2, INVALID_CURSOR = -1 };

class SherlockEngine;

class Events {
private:
	SherlockEngine *_vm;
	uint32 _frameCounter;
	uint32 _priorFrameTime;
	ImageFile *_cursorImages;
	int _mouseButtons;

	/**
	 * Check whether it's time to display the next screen frame
	 */
	bool checkForNextFrameCounter();
public:
	CursorId _cursorId;
	bool _pressed;
	bool _released;
	bool _rightPressed;
	bool _rightReleased;
	bool _oldButtons;
	bool _oldRightButton;
	Common::Stack<Common::KeyState> _pendingKeys;
public:
	Events(SherlockEngine *vm);
	~Events();

	/**
	 * Load a set of cursors from the specified file
	 */
	void loadCursors(const Common::String &filename);

	/**
	 * Set the cursor to show
	 */
	void setCursor(CursorId cursorId);

	/**
	 * Set the cursor to show from a passed frame
	 */
	void setCursor(const Graphics::Surface &src);

	/**
	 * Show the mouse cursor
	 */
	void showCursor();

	/**
	 * Hide the mouse cursor
	 */
	void hideCursor();

	/**
	 * Returns the cursor
	 */
	CursorId getCursor() const;

	/**
	 * Returns true if the mouse cursor is visible
	 */
	bool isCursorVisible() const;

	/**
	 * Move the mouse
	 */
	void moveMouse(const Common::Point &pt);

	/**
	 * Check for any pending events
	 */
	void pollEvents();

	/**
	 * Poll for events and introduce a small delay, to allow the system to
	 * yield to other running programs
	 */
	void pollEventsAndWait();

	/**
	 * Get the current mouse position
	 */
	Common::Point mousePos() const;

	uint32 getFrameCounter() const { return _frameCounter; }

	bool kbHit() const { return !_pendingKeys.empty(); }

	/**
	 * Get a pending keypress
	 */
	Common::KeyState getKey();

	/**
	 * Clear any current keypress or mouse click
	 */
	void clearEvents();

	/**
	 * Clear any pending keyboard inputs
	 */
	void clearKeyboard();

	/**
	 * Delay for a given number of game frames, where each frame is 1/60th of a second
	 */
	void wait(int numFrames);

	/**
	 * Does a delay of the specified number of milliseconds
	 */
	bool delay(uint32 time, bool interruptable = false);

	/**
	 * Sets the pressed and released button flags on the raw button state previously set in pollEvents calls.
	 * @remarks		The events manager has separate variables for the raw immediate and old button state
	 *		versus the current buttons states for the frame. This method is expected to be called only once
	 *		per game frame
	 */
	void setButtonState();

	/**
	 * Checks to see to see if a key or a mouse button is pressed.
	 */
	bool checkInput();
};

} // End of namespace Sherlock

#endif /* SHERLOCK_EVENTS_H */
