/* Copyright (C) 1994-2003 Revolution Software Ltd
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

//=============================================================================
//
//	Filename	:	driver96.h
//	Created		:	6th August 1996
//	By			:	P.R.Porter
//
//	Summary		:	This include file defines all interfaces to the Revolution
//					driver96 system.  All game code which requires driver
//					functions should simlply include this file.
//
//	Functions
//	---------
//
//	---------------------------------------------------------------------------
//	------------------------------- d_draw.c ----------------------------------
//	---------------------------------------------------------------------------
//
//	int32 InitialiseDisplay(int32 width, int32 height, int32 colourDepth, int32 windowType)
//
//	Initialises the directDraw display with the sizes and colour depths passed
//	in.  The windowType is either RD_FULLSCREEN or RD_WINDOWED depending upon
//	whether the app is to run in a window or not.  If RD_WINDOWED is selected,
//	the runction may returnRDERR_GOFULLSCREEN which implies that the window
//	size and colour depth requested is not compatible with the current
//	settings.
//	If the required display cannot be set up, then an error code is
//	returned, otherwise zero.
//
//	---------------------------------------------------------------------------
//	
//	int32 ResetDisplay(void)
//
//	Closes down the directDraw sub-system and resets the display back to it's
//	original size.  Returns an RD code.
//
//	---------------------------------------------------------------------------
//
//	int32 EraseBackBuffer(void)
//
//	Fills the back buffer with palette colour zero.  Returns an RD code.
//
//	---------------------------------------------------------------------------
//
//	void InterpretDirectDrawError(int32 error)
//
//	This function is passed the pointer to a direct draw error code, and
//	translates this into a revolution driver error code.  It also reports the
//	error.
//
//	---------------------------------------------------------------------------
//
//	int32 SetBltFx(void)
//
//	Sets the edge blend and arithmetic stretching effects.
//
//	---------------------------------------------------------------------------
//
//	int32 ClearBltFx(void)
//
//	Clears the edge blend and arithmetic stretching effects.
//
//	---------------------------------------------------------------------------
//
//	int32 RenderHard(void);
//
//	Turns on the hardware renderer.  Returns an error if the
//	hardware is not capable of rendering.
//
//	---------------------------------------------------------------------------
//
//	int32 RenderSoft(void);
//
//	Turns on the software renderer.  Returns an error if it
//	is already on.
//
//	---------------------------------------------------------------------------
//
//	int32 GetRenderType(void)
//
//	Returns the type of rendering currently being used.
//	0 = H/W rendering,	1 = S/W Rendering + BltFxOFF,  2 = S/W Rendering + BltFxON
//
//	---------------------------------------------------------------------------
//
//	int32 PlaySmacker(char *filename)
//
//	Plays the smacker file, filename.
//
//	--------------------------- rdwin.c ---------------------------------------
//	---------------------------------------------------------------------------
//
//	int32 ServiceWindows(void)
//
//	This function should be called at a high rate ( > 20 per second) to service
//	windows and the interfaces it provides.
//
//	---------------------------------------------------------------------------
//
//	int32 CloseAppWindow(void)
//
//	Removes all windows hooks from the application.
//
//	---------------------------------------------------------------------------
//
//	void SetWindowName(const char *windowName)
//
//	Set the window name to windowName and stores this name in gameName for future
//  use.
//
//	---------------------------------------------------------------------------
//	--------------------------------- language.c ------------------------------
//	---------------------------------------------------------------------------
//
//	int32 GetLanguageVersion(uint8 *version)
//
//	This function modifies the 'version' passed in to be the current language.
//	The first time this function is called, it gets the language from the 
//	version.inf file distributed on the game CD.  It returns an RD error code
//	if this file cannot be opened, or the version cannot be obtained from it.
//
//	---------------------------------------------------------------------------
//	
//	int32 SetLanguageVersion(uint8 version)
//
//	This function is useful for debugging.  It sets the version to the one
//	passed in.
//
//	---------------------------------------------------------------------------
//	
//	int32 GetGameName(uint8 *name);
//
//	Fills the string pointed to by name with the title of the game, depending
//	upon what the current language version is.
//
//	--------------------------------------------------------------------------
//	------------------------------- palette.c --------------------------------
//	--------------------------------------------------------------------------
//
//	void BS2_SetPalette(int32 startEntry, int32 noEntries, uint8 *colourTable, uint8 setNow)
//
//	Sets the palette from position startEntry for noEntries, to the data 
//	pointed to by colourTable.  To set the palette immediately pass
//	RDPAL_INSTANT.  If you want to fade later, pass RDPAL_FADE.
//
//	--------------------------------------------------------------------------
//
//	int32 UpdatePaletteMatchTable(uint8 *data)
//
//	Uses the current palCopy to create a table of palette indeces which will
//	be searched later for a quick palette match - only if NULL is passed in
//	as the data.  If a pointer to valid data is passed in, the palette match
//	table is copied from that data.
//
//	--------------------------------------------------------------------------
//
//	uint8 QuickMatch(uint8 r, uint8 g, uint8 b)
//
//	Returns the palette index of the closest matching colour in the palette
//	to these RGB values.
//
//	--------------------------------------------------------------------------
//
//	int32 FadeUp(float time)
//
//	Fades the palette up from black to the current palette in time.
//
//	--------------------------------------------------------------------------
//
//	int32 FadeDown(float time)
//
//	Fades the palette down to black from the current palette in time.
//
//	--------------------------------------------------------------------------
//
//	uint8 GetFadeStatus(void)
//
//	Returns the fade status which can be one of RDFADE_UP, RDFADE_DOWN or
//	RDFADE_NONE.
//
//
//	--------------------------------------------------------------------------
//	-------------------------------- mouse.c ---------------------------------
//	--------------------------------------------------------------------------
//
//  _mouseEvent *MouseEvent(void)
//
//	If there is a mouse event in the queue, a valid pointer is returned.
//	Otherwise, NULL.
//
//	--------------------------------------------------------------------------
//
//	int32 SetMouseAnim(uint8 *ma, int32 size, int32 mouseFlash)
//
//	A pointer to a valid mouse animation is passed in, along with the size of
//	the header plus sprite data.  Remember to check that the function has 
//	successfully completed, as memory allocation is required.  When the mouse
//	animation has been set, the mouse sprite does not need to be kept in the
//	memory manager.
//	Pass NULL in to clear the mouse sprite.
//	mouseFlash should be either RDMOUSE_FLASH or RDMOUSE_NOFLASH
//	defining whether to pulse the mouse or not.
//
//	--------------------------------------------------------------------------
//
//	int32 SetLuggageAnim(uint8 *la, int32 size)
//
//	A pointer to a valid luggage animation is passed in, along with the size of
//	the header plus sprite data.  Remember to check that the function has 
//	successfully completed, as memory allocation is required.
//	Pass NULL in to clear the luggage sprite.  Luggage sprites are of the same
//	format as mouse sprites.
//
//	--------------------------------------------------------------------------
//
//	int32 AnimateMouse(void)
//
//	This function animates the current mouse pointer.  If no pointer is 
//	currently defined, an error code is returned.
//
//
//	--------------------------------------------------------------------------
//	------------------------------ keyboard.c --------------------------------
//	--------------------------------------------------------------------------
//
//  BOOL KeyWaiting(void)
//
//	This function returns TRUE if there is an unprocessed key waiting in the
//	queue, FALSE otherwise.
//
//	--------------------------------------------------------------------------
//
//	int32 ReadKey(_keyboardEvent *key)
//
//	Sets the value of key passed in to the current waiting key.  If there is
//	no key waiting, an error code is returned.
//
//
//	--------------------------------------------------------------------------
//	------------------------------- sprite.c ---------------------------------
//	--------------------------------------------------------------------------
//
//	int32 DrawSprite(_spriteInfo *s)
//
//	Draws a sprite onto the screen of the type defined in the _spriteInfo
//	structure.  The _spriteInfo structure consists of the following elements:
//
//	int16	x;				// coords for top-left of sprite
//	int16	y;
//	uint16	w;				// dimensions of sprite (before scaling)
//	uint16	h;
//	uint16	scale;			// scale at which to draw, given in 256ths 
//							   ['0' or '256' MEANS DON'T SCALE]
//	uint16	scaledWidth;	// new dimensions
//	uint16	scaledHeight;	//
//	uint16	blend			// blending value.
//	uint16	type;			// combination of the bits below
//	uint8	*data;			// pointer to the sprite data
//	uint8	*colourTable;	// pointer to 16-byte colour table - only 
//							   applicable to 16-col compression type
//
//	WARNING:  Sprites will only be drawn onto the background.  Sprites will not
//		appear over the menubar areas.  The mouse and menu drawing is treated
//		as a special case.
//
//	The type of the sprite can be any of the following:
//
//	if (RDSPR_TRANS)
//		The sprite has a transparent colour zero
//		if (RDSPR_NOCOMPRESSION)
//			The sprite data must not be compressed (slow to draw)
//			The RDSPR_DISPLAYALIGN bit may be set to draw the sprite
//				at coordinates relative to the top left corner of the
//				monitor.
//		else
//			Compression must be set as one of the following:
//				RDSPR_RLE16
//				RDSPR_RLE256
//				RDSPR_LAYERCOMPRESSION
//	else
//		The sprite has an opaque colour zero
//		RDSPR_NOCOMPRESSION must be set!
//		RDSPR_DISPLAYALIGN may be set to align the coordinates of the sprite
//			to the top left corner of the monitor.
//
//	---------------------------------------------------------------------------
//
//	int32 CreateSurface(_spriteInfo *s, uint32 *surface)
//
//	Creates a sprite surface in video memory (if possible) and returns it's
//	handle in surface.
//
//	---------------------------------------------------------------------------
//
//	void DrawSurface(_spriteInfo *s, uint32 surface, ScummVM::Rect *clipRect)
//
//	Draws the sprite surface created earlier.
//
//	---------------------------------------------------------------------------
//
//	void DeleteSurface(uint32 surface)
//
//	Deletes a surface from video memory.
//
//	---------------------------------------------------------------------------
//
//	int32 OpenLightMask(_spriteInfo *s)
//
//	Opens the light masking sprite for a room.
//
//	---------------------------------------------------------------------------
//
//	int32 CloseLightMask(void)
//
//	Closes the light masking sprite for a room.
//
//	--------------------------------------------------------------------------
//	------------------------------- render.c ---------------------------------
//	--------------------------------------------------------------------------
//	
//	int32 RenderParallax(_parallax *p)
//
//	Draws a parallax layer at the current position determined by the scroll.
//	A parallax can be either foreground, background or the main screen.
//
//	---------------------------------------------------------------------------
//
//	int32 CopyScreenBuffer(void)
//
//	Copies the screen buffer to screen memory.  This function should be called
//	when the drawing should be done to the back buffer.  It only does this
//	when we are using a software screen buffer.
//
//	---------------------------------------------------------------------------
//
//	int32 SetScrollTarget(int16 sx, int16 sy)
//
//	Sets the scroll target position for the end of the game cycle.  The drivers
//	will then automatically scroll as many times as it can to reach this 
//	position in the allotted time.
//
//	--------------------------------------------------------------------------
//
//	int32 InitialiseRenderCycle(void)
//
//	Initialises the timers before the render loop is entered.
//
//	--------------------------------------------------------------------------
//
//	int32 StartRenderCycle(void)
//
//	This function should be called when the game engine is ready to start
//	the render cycle.
//
//	--------------------------------------------------------------------------
//
//	int32 EndRenderCycle(BOOL *end)
//
//	This function should be called at the end of the render cycle.  If the
//	render cycle is to be terminated, the function sets *end to 1.  Otherwise,
//	the render cycle should continue.
//
//	--------------------------------------------------------------------------
//
//	int32 SetLocationMetrics(uint16 w, uint16 h)
//
//	This function tells the drivers the size of the background screen for the
//	current location.
//
//	--------------------------------------------------------------------------
//
//	int32 PlotPoint(uint16 x, uint16 y, uint8 colour)
//
//	Plots the point x,y in relation to the top left corner of the background.
//
//	--------------------------------------------------------------------------
//
//	int32 DrawLine(int16 x1, int16 y1, int16 x2, int16 y2, uint8 colour)
//
//	Draws a line from the point x1,y1 to x2,y2 of the specified colour.
//
//	--------------------------------------------------------------------------
//
//	int32 InitialiseBackgroundLayer(_parallax *p)
//
//	This function should be called five times with either the parallax layer
//	or a NULL pointer in order of background parallax to foreground parallax.
//
//	--------------------------------------------------------------------------
//
//	int32 CloseBackgroundLayer(void)
//
//	Should be called once after leaving a room to free up video memory.
//
//	--------------------------------------------------------------------------
//
//	int32 PlotDots(int16 x, int16 y, int16 count)
//
//	Plots 'count' dots at the position x,y.
//
//	--------------------------------------------------------------------------
//	---------------------------- menu.c --------------------------------------
//	--------------------------------------------------------------------------
//
//	int32 ProcessMenu(void)
//
//	This function should be called regularly to process the menuber system.
//	The rate at which this function is called will dictate how smooth the menu
//	system is.  The menu cannot be drawn at a higher rate than the system
//	vbl rate.
//
//	--------------------------------------------------------------------------
//
//	int32 ShowMenu(uint8 menu)
//
//	This function brings the menu in to view.  The choice of top or bottom menu
//	is defined by the parameter menu being either RDMENU_TOP or RDMENU_BOTTOM.
//	An error code is returned if the menu is already shown.
//
//	--------------------------------------------------------------------------
//
//	int32 HideMenu(uint8 menu)
//
//	This function hides the menu defined by the parameter menu.  If the menu is
//	already hidden, an error code is returned.
//
//	--------------------------------------------------------------------------
//
//	int32 SetMenuIcon(uint8 menu, uint8 pocket, uint8 *icon)
//
//	This function sets a menubar icon to that passed in.  If icon is NULL, the
//	pocket is cleared, otherwise, that icon is placed into pocket.  The menu is
//	either RDMENU_TOP or RDMENU_BOTTOM.  Valid error codes include
//	RDERR_INVALIDPOCKET if the pocket number does not exist.  Initially, there
//	are 15 pockets.
//
//	--------------------------------------------------------------------------
//
//	uint8 GetMenuStatus(uint8 menu)
//
//	This function returns the status of the menu passed in menu.  Return values
//	are RDMENU_OPENING, RDMENU_CLOSING, RDMENU_HIDDEN and RDMENU_SHOWN.
//
//
//
//	--------------------------------------------------------------------------
//	--------------------------- d_sound.c ------------------------------------
//	--------------------------------------------------------------------------
//
//	int32 PreFetchCompSpeech(const char *filename, uint32 speechid, uint8 *wave)
//
//	This function loads and decompresses speech sample 'speechid' from the
//	cluster 'filename' into 'wave'.  'wave' should contain the address of
//	preallocated memory large enough for speech to fit into 
//	(see GetCompSpeechSize).
//
//	--------------------------------------------------------------------------
//
//	int32 GetCompSpeechSize(const char *filename, uint32 speechid);
//
//	This function returns the size that speech sample 'speechid' from cluster
//	'filename' will be after it has been decompressed and had a wav header
//	added.  Returns 0 for any error.
//
//	--------------------------------------------------------------------------
//
//	int32 PlayCompSpeech(const char *filename, uint32 id, uint8 vol, int8 pan)
//
//	This function loads, decompresses and plays the wav 'id' from the cluster
// 'filename'.  An error occurs if speech is already playing, or directSound
//  comes accross problems. 'volume' can be from 0 to 16. 'pan' can be from
//  -16 (full left) to 16 (full right).
//  id is the text line id used to reference the speech within the speech
//  cluster.
//
//	--------------------------------------------------------------------------
//
//	int32 StopSpeechSword2(void)
//
//	Stops the speech from playing.
//
//	--------------------------------------------------------------------------
//
//	int32 GetSpeechStatus(void)
//
//	Returns either RDSE_SAMPLEPLAYING or RDSE_SAMPLEFINISHED
//
//	--------------------------------------------------------------------------
//
//	int32 AmISpeaking(void)
//
//	Returns either RDSE_QUIET or RDSE_SPEAKING
//
//	--------------------------------------------------------------------------
//
//	int32 PauseSpeech(void)
//
//	Stops the speech dead in it's tracks.
//
//	--------------------------------------------------------------------------
//
//	int32 UnpauseSpeech(void)
//
//	Re-starts the speech from where it was stopped.
//
//	--------------------------------------------------------------------------
//
//	int32 OpenFx(int32 id, uint8 *data)
//
//	This function opens a sound effect ready for playing.  A unique id should
//	be passed in so that each effect can be referenced individually.
//
//	WARNING: Zero is not a valid ID.
//
//	--------------------------------------------------------------------------
//
//	int32 PlayFx(int32 id, uint8 *data, uint8 vol, int8 pan, uint8 type)
//
//	This function plays a sound effect.  If the effect has already been opened
//	then *data should be NULL, and the sound effect will simply be obtained 
//	from the id passed in.  If the effect has not been opened, then the wav
//	data should be passed in data.  The sound effect will be closed when it
//	has finished playing.
//	volume can be from 0 to 16.
//	pan can be from -16 (full left) to 16 (full right).
//	type is either RDSE_FXSPOT or RDSE_FXLOOP
//
//	WARNING: Zero is not a valid ID
//
//	--------------------------------------------------------------------------
//
//	int32 CloseFx(int32 id)
//
//	This function closes a sound effect which has been previously opened for
//	playing.  Sound effects must be closed when they are finished with,
//	otherwise you will run out of sound effect buffers.
//
//	--------------------------------------------------------------------------
//
//	int32 ClearAllFx(void)
//
//	This function clears all of the sound effects which are currently open or
//	playing, irrespective of type.
//
//	--------------------------------------------------------------------------
//
//	int32 StreamMusic(uint8 *filename, int32 loopFlag)
//
//	Streams music from the file defined by filename.  The loopFlag should
//	be set to RDSE_FXLOOP if the music is to loop back to the start.
//	Otherwise, it should be RDSE_FXSPOT.
//	The return value must be checked for any problems.
//
//	--------------------------------------------------------------------------
//
//	int32 StreamCompMusic(uint8 *filename, uint32 id, int32 loopFlag)
//
//	Streams music 'id' from the cluster file 'filename'.  The loopFlag should
//	be set to RDSE_FXLOOP if the music is to loop back to the start.
//	Otherwise, it should be RDSE_FXSPOT.
//	The return value must be checked for any problems.
//
//  StreamCompMusic should not be used inconjunction with StreamMusic.
//
//	--------------------------------------------------------------------------
//
//	void StopMusic(void)
//
//	Fades out and stops the music.
//
//	--------------------------------------------------------------------------
//
//	int32 PauseMusic(void)
//
//	Stops the music dead in it's tracks.
//
//	--------------------------------------------------------------------------
//
//	int32 UnpauseMusic(void)
//
//	Re-starts the music from where it was stopped.
//
//	---------------------------------------------------------------------------
//
//	void SetMusicVolume(uint8 vol)
//
//	Set the volume of any future as well as playing (but not fading) music to
//  vol.  vol is in the range of 0 to 16 with 0 being silent.
//
//	---------------------------------------------------------------------------
//
//	uint8 GetMusicVolume(void)
//
//	Returns the volume setting for music.
//
//	---------------------------------------------------------------------------
//
//	void SetFxVolume(uint8 vol)
//
//	Set the master volume of all fx' to vol.  The fx' still have there own
//  volume setting as well as the master volume.  vol is in the range 0 to 14
//  with 0 being silent.
//
//	---------------------------------------------------------------------------
//
//	uint8 GetFxVolume(void)
//
//	Returns the master volume setting for fx'.
//
//	---------------------------------------------------------------------------
//
//	void SetSpeechVolume(uint8 vol)
//
//	Set the volume of any future as well as playing speech samples to vol.
//  vol is in the range of 0 to 14 with 0 being silent.
//
//	---------------------------------------------------------------------------
//
//	uint8 GetSpeechVolume(void)
//
//	Returns the volume setting for speech.
//
//	---------------------------------------------------------------------------
//
//	void MuteMusic(uint8 mute)
//
//	If mute is 0, the music volume is restored to the last set master level.
//  otherwise the music is muted (volume 0).
//
//	---------------------------------------------------------------------------
//
//	void MuteFx(uint8 mute)
//
//  See MuteMusic(uint8).
//
//	---------------------------------------------------------------------------
//
//	void MuteSpeech(uint8 mute)
//
//  See MuteMusic(uint8).
//
//	---------------------------------------------------------------------------
//
//	uint8 IsMusicMute(void)
//
//	Returns the music's mute state, 1 if mute, 0 if not mute.
//
//	---------------------------------------------------------------------------
//
//	uint8 IsFxMute(void)
//
//  See IsMusicMute().
//
//	---------------------------------------------------------------------------
//
//	uint8 IsMusicMute(void)
//
//  See IsMusicMute().
//
//	---------------------------------------------------------------------------
//
//	int32 MusicTimeRemaining(void)
//
//  Returns the time left for the current tune.
//
//	---------------------------------------------------------------------------
//
//	int32 ReverseStereo(void)
//
//  Returns the time left for the current tune.
//
//	---------------------------------------------------------------------------
//
//	int32 SetFxVolumePan(int32 id, uint8 vol, uint8 pan)
//
//	Sets the volume and pan of the sample which is currently playing (id)
//
//=============================================================================

#ifndef DRIVER96_H
#define DRIVER96_H

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "common/scummsys.h"
#include "base/engine.h" // for warning()
#include "common/system.h"
#include "common/rect.h"

#ifdef __cplusplus
extern "C" {
#endif

//
//	Defines
//	-------
//

// defines specific to windows headers...
#ifndef WIN32

#define SEM_FAILCRITICALERRORS 1
#define FILE_ATTRIBUTE_NORMAL 0x80
#define _MAX_PATH 260

#endif
	
//Generic error codes
#define RD_OK				0x00000000
#define RDERR_UNKNOWN			0x00000001
#define RDERR_INVALIDPOINTER		0x00000002
#define RDERR_OUTOFMEMORY		0x00000003
#define RDERR_INVALIDFILENAME		0x00000004
#define RDERR_READERROR			0x00000005
#define RDERR_WRITEERROR		0x00000006
#define RDERR_NOEMULATION		0x00000007
#define RDERR_LOCKFAILED		0x00000008

//Drawing error codes
#define RDERR_VIDEOMODE			0x00010000
#define RDERR_COLOURDEPTH		0x00010001
#define RDERR_CANNOTFLIP		0x00010002
#define RDERR_RESTORELAYERS		0x00010003
#define RDERR_DDRAWNOEMULATION		0X00010004
#define RDERR_NOHARDWARE		0x00010005
#define RDERR_ALREADYON			0x00010006
#define RDERR_DECOMPRESSION		0x00010007

//Operating system error codes
#define RDERR_CREATEWINDOW		0x00020000
#define RDERR_APPCLOSED			0x00020001
#define RDERR_GOFULLSCREEN		0x00020002

//Language and version error codes
#define RDERR_OPENVERSIONFILE		0x00030000
#define RDERR_INVALIDVERSION		0x00030001

//Keyboard error codes
#define RDERR_NOKEYWAITING		0x00040000

//Sprite drawing error codes
#define RDERR_NOCLIPPING		0x00050000
#define RDERR_NOTIMPLEMENTED		0x00050001
#define RDERR_UNKNOWNTYPE		0x00050002
#define RDERR_INVALIDSCALING		0x00050003
#define RDERR_SURFACELOST		0x00050004
#define RDERR_NOTCLOSED			0x00050005
#define RDERR_NOTOPEN			0x00050006
#define RDERR_ALREADYCLOSED		0x00050007 // added for _console.cpp by khalek

//Menubar error codes
#define RDERR_INVALIDMENU		0x00060000
#define RDERR_INVALIDPOCKET		0x00060001
#define RDERR_INVALIDCOMMAND		0x00060002

//Palette fading error codes
#define RDERR_FADEINCOMPLETE		0x00070000

//Sound engine error codes
#define RDERR_DSOUNDCREATE		0x00080000
#define RDERR_DSOUNDCOOPERATE		0x00080001
#define RDERR_DSOUNDPBUFFER		0x00080002
#define RDERR_PRIMARYFORMAT		0x00080003
#define RDERR_SPEECHPLAYING		0x00080004
#define RDERR_SPEECHNOTPLAYING		0x00080005
#define RDERR_INVALIDWAV		0x00080006
#define RDERR_CREATESOUNDBUFFER		0x00080007
#define RDERR_LOCKSPEECHBUFFER		0x00080008
#define RDERR_FXALREADYOPEN		0x00080009
#define RDERR_NOFREEBUFFERS		0x0008000A
#define RDERR_FXNOTOPEN			0x0008000B
#define RDERR_FXFUCKED			0x0008000C
#define RDERR_INVALIDID			0x0008000D




// Language codes
#define ENGLISH				0x00
#define AMERICAN			0x01
#define GERMAN				0x02
#define FRENCH				0x03
#define SPANISH				0x04
#define ITIALIAN			0x05
#define JAPANESE			0x06
#define SLOVAK				0x07

// Key codes
#define RDKEY_ESCAPE			27


// Mouse button defines
#define RD_LEFTBUTTONDOWN		0x01
#define RD_LEFTBUTTONUP			0x02
#define RD_RIGHTBUTTONDOWN		0x04
#define RD_RIGHTBUTTONUP		0x08


//Sprite defines
#define RDSPR_TRANS			0x0001
#define RDSPR_BLEND			0x0004
#define RDSPR_FLIP			0x0008
#define RDSPR_SHADOW			0x0010
#define RDSPR_DISPLAYALIGN		0x0020
#define RDSPR_NOCOMPRESSION		0x0040
#define RDSPR_EDGEBLEND			0x0080
//This is the high byte part of the sprite type which defines what type of
//  compression is used, as long as RDSPR_NOCOMPRESSION is not defined.
#define RDSPR_RLE16			0x0000
#define RDSPR_RLE256			0x0100
#define RDSPR_RLE256FAST		0x0200


//Rendering defines
#define RD_SOFTWARESCREENBUFFER		0x01


//Windows defines
#define RD_FULLSCREEN			0x01000000
#define RD_WINDOWED			0x01000001


//Fading defines
#define RDFADE_NONE			0x00
#define RDFADE_UP			0x01
#define RDFADE_DOWN			0x02
#define RDFADE_BLACK			0x03

//Mouse defines
#define RDMOUSE_NOFLASH			0x00
#define RDMOUSE_FLASH			0x01

//Menubar defines.
#define RDMENU_TOP			0x00
#define RDMENU_BOTTOM			0x01
									
#define RDMENU_HIDDEN			0x00
#define RDMENU_SHOWN			0x01
#define RDMENU_OPENING			0x02
#define RDMENU_CLOSING			0x03

#define RDMENU_ICONWIDE			35
#define RDMENU_ICONDEEP			30
#define RDMENU_ICONSTART		24
#define RDMENU_ICONSPACING		5
#define RDMENU_MAXPOCKETS		15
#define RDMENU_MENUDEEP			40

#define RDSE_SAMPLEFINISHED		0
#define RDSE_SAMPLEPLAYING		1
#define RDSE_FXTOCLEAR			0
#define RDSE_FXCACHED			1
#define RDSE_FXSPOT			0
#define RDSE_FXLOOP			1
#define RDSE_FXLEADIN			2
#define RDSE_FXLEADOUT			3
#define RDSE_QUIET			1
#define RDSE_SPEAKING          		0


#define RDPAL_FADE			0
#define RDPAL_INSTANT			1

//Blitting FX defines
#define RDBLTFX_MOUSEBLT		0x01
#define RDBLTFX_FGPARALLAX		0x02
#define RDBLTFX_ARITHMETICSTRETCH	0x04
#define RDBLTFX_EDGEBLEND		0x08
#define RDBLTFX_SHADOWBLEND		0x10
#define RDBLTFX_FLATALPHA		0x20
#define RDBLTFX_GRADEDALPHA		0x40
#define RDBLTFX_ALLHARDWARE		0x80

// Max number of sound fx
#define MAXFX 16
#define MAXMUS 2

// Key buffer size
#define MAX_KEY_BUFFER 32

//
//	Structure definitions
//	---------------------
//

typedef struct {
	uint16 buttons;
} _mouseEvent;

typedef struct {
	uint16 ascii;
	int keycode;
	int modifiers;
} _keyboardEvent;

#if !defined(__GNUC__)
	#pragma START_PACK_STRUCTS
#endif

typedef struct {
	uint16 w;
	uint16 h;
	uint32 offset[2];	// 2 is arbitrary
} GCC_PACK _parallax;

#if !defined(__GNUC__)
	#pragma END_PACK_STRUCTS
#endif


// The _spriteInfo structure is used to tell the driver96 code what attributes
// are linked to a sprite for drawing.  These include position, scaling and
// compression.

typedef struct {
	int16 x;		// coords for top-left of sprite
	int16 y;
	uint16 w;		// dimensions of sprite (before scaling)
	uint16 h;
	uint16 scale;		// scale at which to draw, given in 256ths ['0' or '256' MEANS DON'T SCALE]
	uint16 scaledWidth;	// new dimensions (we calc these for the mouse area, so may as well pass to you to save time)
	uint16 scaledHeight;	//
	uint16 type;		// mask containing 'RDSPR_' bits specifying compression type, flip, transparency, etc
	uint16 blend;		// holds the blending values.
	uint8 *data;		// pointer to the sprite data
	uint8 *colourTable;	// pointer to 16-byte colour table, only applicable to 16-col compression type
} _spriteInfo;


// This is the format of a .WAV file.  Somewhere after this header is the string
// 'DATA' followed by an int32 size which is the size of the data.  Following
// the size of the data is the data itself.
typedef struct {
	uint32 riff;
	uint32 fileLength;
	uint32 wavID;
	uint32 format;
	uint32 formatLen;
	uint16 formatTag;
	uint16 channels;
	uint16 samplesPerSec;
	uint16 avgBytesPerSec;
	uint16 blockAlign;
	uint16 unknown1;
	uint16 unknown2;
	uint16 bitsPerSample;
} _wavHeader;


//  This is the structure which is passed to the sequence player.
//	It includes the smack to play, and any text lines which are
//	to be displayed over the top of the sequence.

typedef struct {
	uint16 startFrame;
	uint16 endFrame;
	_spriteInfo *textSprite;
	uint32 speechBufferSize;
	uint16 *speech;
} _movieTextObject;

//
//	Function Prototypes
//	-------------------
//

//-----------------------------------------------------------------------------
//  Display functions - from d_draw.c
//-----------------------------------------------------------------------------
extern int32 InitialiseDisplay(int16 width, int16 height, int16 colourDepth, int32 windowType);
extern int32 EraseBackBuffer(void);
extern void SetTransFx(void);
extern void ClearTransFx(void);
extern void SetBltFx(void);
extern void ClearBltFx(void);
extern void ClearShadowFx(void);
extern void SetShadowFx(void);
extern int32 GetRenderType(void);
extern int32 PlaySmacker(char *filename, _movieTextObject *textObjects[], uint8 *musicOut);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Windows OS functions - from rdwin.c
//-----------------------------------------------------------------------------
extern int32 CloseAppWindow(void);
extern int32 ServiceWindows(void);
extern void  SetWindowName(const char *windowName);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Language functions - from language.c
//-----------------------------------------------------------------------------
extern int32 GetLanguageVersion(uint8 *version);
extern int32 SetLanguageVersion(uint8 version);
extern int32 GetGameName(uint8 *name);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Palette functions - from palette.c
//-----------------------------------------------------------------------------
extern int32 BS2_SetPalette(int16 startEntry, int16 noEntries, uint8 *palette, uint8 setNow);
extern int32 UpdatePaletteMatchTable(uint8 *data);
extern uint8 QuickMatch(uint8 r, uint8 g, uint8 b);
extern int32 FadeUp(float time);
extern int32 FadeDown(float time);
extern uint8 GetFadeStatus(void);
extern int32 DimPalette(void);
extern void WaitForFade(void);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Mouse functions - from mouse.c
//-----------------------------------------------------------------------------
extern _mouseEvent *MouseEvent(void);
extern int32 SetMouseAnim(uint8 *ma, int32 size, int32 mouseFlash);
extern int32 SetLuggageAnim(uint8 *la, int32 size);
extern int32 AnimateMouse(void);
uint8 CheckForMouseEvents(void);		// (James23july97)
extern void ResetRenderEngine(void);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Keyboard functions - from keyboard.c
//-----------------------------------------------------------------------------
extern bool KeyWaiting(void);
extern int32 ReadKey(_keyboardEvent *ke);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Sprite functions - from sprite.c
//-----------------------------------------------------------------------------
extern int32 DrawSprite(_spriteInfo *s);
extern int32 CreateSurface(_spriteInfo *s, uint8 **surface);
extern void DrawSurface(_spriteInfo *s, uint8 *surface, ScummVM::Rect *clipRect = NULL);
extern void DeleteSurface(uint8 *surface);
extern int32 OpenLightMask(_spriteInfo *s);
extern int32 CloseLightMask(void);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Screen drawing and scrolling function - from render.c
//-----------------------------------------------------------------------------
extern int32 SetScrollTarget(int16 sx, int16 sy);
extern int32 InitialiseRenderCycle(void);
extern int32 StartRenderCycle(void);
extern int32 EndRenderCycle(bool *end);
extern int32 RenderParallax(_parallax *p, int16 layer);
extern int32 SetLocationMetrics(uint16 w, uint16 h);
extern int32 CopyScreenBuffer(void);
extern int32 PlotPoint(uint16 x, uint16 y, uint8 colour);
extern int32 DrawLine(int16 x1, int16 y1, int16 x2, int16 y2, uint8 colour);
extern int32 InitialiseBackgroundLayer(_parallax *p);
extern int32 CloseBackgroundLayer(void);
extern int32 PlotDots(int16 x, int16 y, int16 count);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//	Menubar control and drawing functions - from menu.c
//-----------------------------------------------------------------------------
extern int32 ProcessMenu(void);
extern int32 ShowMenu(uint8 menu);
extern int32 HideMenu(uint8 menu);
extern int32 SetMenuIcon(uint8 menu, uint8 pocket, uint8 *icon);
extern uint8 GetMenuStatus(uint8 menu);
extern int32 CloseMenuImmediately(void);

//-----------------------------------------------------------------------------
//	Misc functions - from misc.cpp
//-----------------------------------------------------------------------------
extern uint32 SVM_timeGetTime(void);
extern void SVM_SetFileAttributes(char *file, uint32 atrib);
extern void SVM_DeleteFile(char *file);
extern int32 SVM_GetVolumeInformation(char *cdPath, char *sCDName, uint32 maxPath, uint8 *, uint32 *dwMaxCompLength, uint32 *dwFSFlags, uint8 *, uint32 a);
extern void scumm_mkdir(const char *pathname);
extern void SVM_GetModuleFileName(void *module, char *destStr, uint32 maxLen);

//-----------------------------------------------------------------------------
extern int16 screenWide;	// Width of the screen display
extern int16 screenDeep;	// Height of the screen display
extern int16 mousex;		// Mouse screen x coordinate
extern int16 mousey;		// Mouse screen y coordinate
extern int32 renderCaps;	// Flags which determine how to render the scene.
extern uint8 palCopy[256][4];	// Current palette.

#ifdef __cplusplus
}
#endif

#endif
