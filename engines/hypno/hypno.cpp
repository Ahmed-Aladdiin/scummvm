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

#include "audio/audiostream.h"
#include "audio/decoders/raw.h"
#include "common/archive.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/debug.h"
#include "common/error.h"
#include "common/events.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/str.h"
#include "common/system.h"
#include "common/timer.h"
#include "common/tokenizer.h"
#include "common/memstream.h"
#include "engines/util.h"
#include "image/bmp.h"


#include "hypno/hypno.h"
#include "hypno/grammar.h"

namespace Hypno {


Hotspots *g_parsedHots;
ArcadeShooting g_parsedArc;

MVideo::MVideo(Common::String _path, Common::Point _position, bool _transparent, bool _scaled) {
	path = _path;
	position = _position;
	scaled = _scaled;
	transparent = _transparent;
	videoDecoder = nullptr;
	currentFrame = nullptr;
	finishBeforeEnd = 0;
}

const static char* levelVariables[] = {
	"GS_NONE",
	"GS_SCTEXT",
	"GS_AMBIENT",
	"GS_MUSIC",
	"GS_VOLUME",
	"GS_MOUSESPEED",
	"GS_MOUSEON",
	"GS_LEVELCOMPLETE",
	"GS_LEVELWON",
	"GS_CHEATS",
	"GS_SWITCH0",
	"GS_SWITCH1",
	"GS_SWITCH2",
	"GS_SWITCH3",
	"GS_SWITCH4",
	"GS_SWITCH5",
	"GS_SWITCH6",
	"GS_SWITCH7",
	"GS_SWITCH8",
	"GS_SWITCH9",
	"GS_SWITCH10",
	"GS_SWITCH11",
	"GS_SWITCH12",
	"GS_COMBATJSON",
	"GS_COMBATLEVEL",
	"GS_PUZZLELEVEL",
	NULL
};

extern int parse_mis(const char *);
extern int parse_arc(const char *);

HypnoEngine::HypnoEngine(OSystem *syst, const ADGameDescription *gd)
	: Engine(syst), _gameDescription(gd), _image(nullptr),
	  _compositeSurface(nullptr), _transparentColor(0), 
	  _nextHotsToAdd(nullptr), _nextHotsToRemove(nullptr),
	  _screenW(640), _screenH(480) {
	_rnd = new Common::RandomSource("hypno");
}

HypnoEngine::~HypnoEngine() {
	// Dispose your resources
	delete _rnd;
}

void HypnoEngine::initializePath(const Common::FSNode &gamePath) {
	SearchMan.addDirectory(gamePath.getPath(), gamePath, 0, 10);
}

void HypnoEngine::parseLevel(Common::String filename) {
    filename = convertPath(filename);
	Common::File *test = new Common::File();
	assert(isDemo());
    assert(test->open(filename.c_str()));

	const uint32 fileSize = test->size();
	char *buf = (char *)malloc(fileSize + 1);
	test->read(buf, fileSize);
	buf[fileSize] = '\0';
	parse_mis(buf);
	Level level;
	level.hots = *g_parsedHots; 
	_levels[filename] = level;
	debug("Loaded hots size: %d", g_parsedHots->size());
}

LibData HypnoEngine::loadLib(char *filename) {
	Common::File libfile;
    assert(libfile.open(filename));
	uint32 i = 0;
	Common::String entry = "<>";
	LibData r;
	FileData f;
	f.name = "<>";
	byte b;
	uint32 start;
	uint32 size;
	uint32 pos;

	while (f.name.size() > 0) {
		f.name.clear();
		f.data.clear();
		for (i = 0; i < 12; i++) {
			b = libfile.readByte();
			if (b != 0x96 && b != 0x0)
				f.name += tolower(char(b));
		}
		debug("name: %s", f.name.c_str());
		start = libfile.readUint32LE();
		size = libfile.readUint32LE();
		debug("field: %x", libfile.readUint32LE());

		pos = libfile.pos();
		libfile.seek(start);

		for (i = 0; i < size; i++) {
			b = libfile.readByte();
			if (b != '\n')
				b = b ^ 0xfe;
			f.data.push_back(b);
		}
		debug("size: %d", f.data.size());
		libfile.seek(pos);
		r.push_back(f);

	}
	return r;
}

void HypnoEngine::resetLevelState() {
	uint32 i = 0;
	while (levelVariables[i]) {
		_levelState[levelVariables[i]] = 0;
		i++;
	}
}

bool HypnoEngine::checkLevelCompleted() {
	return _levelState["GS_LEVELCOMPLETE"];
}

void HypnoEngine::parseShootList(Common::String name, Common::String data) {
	Common::StringTokenizer tok(data, " S,\t\n");

	Common::String t;
	Common::String n;
	ShootInfo si;
	while(!tok.empty()) {
		t = tok.nextToken();
		n = tok.nextToken();
		if (t == "Z")
			break;
		si.name = n;
		si.timestamp = atoi(t.c_str());
		_shootInfos.push_back(si);
		debug("%d -> %s", si.timestamp, si.name.c_str());
	}

}

void HypnoEngine::parseArcadeShooting(Common::String name, Common::String data) {
	parse_arc(data.c_str());
	Level level;
	level.arcade = g_parsedArc;  
	_levels[name] = level;
	g_parsedArc.background.clear();
	g_parsedArc.player.clear();
	g_parsedArc.shoots.clear();
}

void HypnoEngine::loadAssets() {

	
	LibData files = loadLib("C_MISC/MISSIONS.LIB");
	uint32 i = 0;
	uint32 j = 0;

	Common::String arc;
	Common::String list;

	debug("Splitting file: %s",files[0].name.c_str());
	for (i = 0; i < files[0].data.size(); i++) {
		arc += files[0].data[i];
		if (files[0].data[i] == 'X') {
			i++;
			for (j = i; j < files[0].data.size(); j++) {
				if (files[0].data[j] == 'Y')
					break;
				list += files[0].data[j];
			}
			break; // No need to keep parsing, no more files are used in the demo
		}
	}

	parseArcadeShooting(files[0].name, arc);
	parseShootList(files[0].name, list);

	//files = loadLib("C_MISC/FONTS.LIB");
	files = loadLib("C_MISC/SOUND.LIB");
	//ByteArray *raw = new ByteArray(files[3].data);

	//Common::MemorySeekableReadWriteStream *mstream = new Common::MemorySeekableReadWriteStream(raw->data(), raw->size());
	//Audio::LoopingAudioStream *stream;
	//stream = new Audio::LoopingAudioStream(Audio::makeRawStream(mstream, 22050, Audio::FLAG_UNSIGNED, DisposeAfterUse::NO), 1);
	//_mixer->playStream(Audio::Mixer::kSFXSoundType, &_soundHandle, stream, -1, Audio::Mixer::kMaxChannelVolume);

	// quit level
	Hotspot q;
	q.type = MakeMenu;
	Action *a = new Quit();
	q.actions.push_back(a);
	Level level;
	Hotspots quit;
	quit.push_back(q);
	level.hots = quit;  
	_levels["mis/quit.mis"] = level;

	// Read assets from mis files
	parseLevel("mis/demo.mis");
	_levels["mis/demo.mis"].intros.push_back(MVideo("demo/dcine1.smk", Common::Point(0, 0), false, true));
	_levels["mis/demo.mis"].intros.push_back(MVideo("demo/dcine2.smk", Common::Point(0, 0), false, true));
	_levels["mis/demo.mis"].hots[1].setting = "c1.mi_";
	_levels["mis/demo.mis"].hots[2].setting = "mis/alley.mis";
	_levels["mis/demo.mis"].hots[5].setting = "mis/order.mis";

	parseLevel("mis/order.mis");
	_levels["mis/order.mis"].hots[1].setting = "mis/quit.mis";
	parseLevel("mis/alley.mis");
	_levels["mis/alley.mis"].intros.push_back(MVideo("demo/aleyc01s.smk", Common::Point(0, 0), false, true));

	//loadMis("mis/shoctalk.mis");

}

Common::Error HypnoEngine::run() {
	_language = Common::parseLanguage(ConfMan.get("language"));
	_platform = Common::parsePlatform(ConfMan.get("platform"));

	Graphics::ModeList modes;
	modes.push_back(Graphics::Mode(640, 480));
	modes.push_back(Graphics::Mode(320, 200));
    initGraphicsModes(modes);

	// Initialize graphics
	initGraphics(_screenW, _screenH, nullptr);
	_pixelFormat = g_system->getScreenFormat();
	if (_pixelFormat == Graphics::PixelFormat::createFormatCLUT8())
		return Common::kUnsupportedColorMode;

	//screenRect = Common::Rect(0, 0, _screenW, _screenH);
	//changeCursor("default");
	_compositeSurface = new Graphics::ManagedSurface();
	_compositeSurface->create(_screenW, _screenH, _pixelFormat);

	// Main event loop
	Common::Event event;
	Common::Point mousePos;
	/*int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot >= 0) { // load the savegame
		loadGameState(saveSlot);
	} else {
		_nextSetting = getGoIntroSetting();
	}*/
	loadAssets();
	_nextSetting = "mis/demo.mis";
	while (!shouldQuit()) {
		resetLevelState();
		_videosPlaying.clear();
		if (!_nextSetting.empty()) {
			debug("Executing setting %s", _nextSetting.c_str());
			_currentSetting = _nextSetting;
			_nextSetting = "";
			runLevel(_currentSetting);
		}
		_levels["mis/demo.mis"].intros.clear();
			
	}
	return Common::kNoError;
}

void HypnoEngine::runLevel(Common::String name) {
	assert(_levels.contains(name));
	if (_levels[name].hots.size() == 0) {
		changeScreenMode("arcade");
		runArcade(_levels[name].arcade);
	} else {
		changeScreenMode("scene");
		runScene(_levels[name].hots, _levels[name].intros);
	}

}

void HypnoEngine::shootSpiderweb(Common::Point target) {
	uint32 c = _pixelFormat.RGBToColor(255, 255, 255);
	_compositeSurface->drawLine(0, 300, target.x, target.y+1, c);
	_compositeSurface->drawLine(0, 300, target.x, target.y  , c);
	_compositeSurface->drawLine(0, 300, target.x, target.y-1, c);

	drawScreen();
	g_system->delayMillis(2);
}

void HypnoEngine::runArcade(ArcadeShooting arc) {
	Common::Event event;
	Common::Point mousePos;
	Common::List<uint32> videosToRemove;

	//_nextParallelVideoToPlay.push_back(MVideo(arc.background, Common::Point(0, 0), false, false));
	MVideo background = MVideo(arc.background, Common::Point(0, 0), false, false);	
	Graphics::Surface *sp = decodeFrame(arc.player, 2);

	changeCursor("mouse/cursor1.smk", 0);
	playVideo(background);

	while (!shouldQuit()) {
		
		while (g_system->getEventManager()->pollEvent(event)) {
			mousePos = g_system->getEventManager()->getMousePos();
			// Events
			switch (event.type) {

			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				break;

			case Common::EVENT_LBUTTONDOWN:
				shootSpiderweb(mousePos);
				clickedShoot(mousePos);
				break;

			case Common::EVENT_MOUSEMOVE:
				break;

			default:
				break;
			}
		}

		// Movies
		for (Videos::iterator it = _nextParallelVideoToPlay.begin(); it != _nextParallelVideoToPlay.end(); ++it) {
			playVideo(*it);
			_videosPlaying.push_back(*it);
		}

		if (_nextParallelVideoToPlay.size() > 0)
			_nextParallelVideoToPlay.clear();

		if (background.videoDecoder->endOfVideo()) {
			skipVideo(background);
			_nextSetting = "mis/demo.mis";
			return;
		}


		if (background.videoDecoder->needsUpdate())
			updateScreen(background);

		if (_shootInfos.size() > 0) {
			ShootInfo si = _shootInfos.front();
			if (si.timestamp <= background.videoDecoder->getCurFrame()) {
				_shootInfos.pop_front();
				for (Shoots::iterator it = arc.shoots.begin(); it != arc.shoots.end(); ++it) {
					if (it->name == si.name) {
						_nextParallelVideoToPlay.push_back(MVideo(it->animation, it->position , true, false));
						_nextParallelVideoToPlay[0].finishBeforeEnd = 24;
					}
				}
			}
		}

		//drawImage(*sp, 60, 129, true);
		uint32 i = 0;
		videosToRemove.clear();

		for (Videos::iterator it = _videosPlaying.begin(); it != _videosPlaying.end(); ++it) {
			if (it->videoDecoder) {
				if (it->videoDecoder-> getCurFrame() > 0 && it->videoDecoder-> getCurFrame() >= it->videoDecoder->getFrameCount() - it->finishBeforeEnd) {
				delete it->videoDecoder;
				it->videoDecoder = nullptr;
				videosToRemove.push_back(i);

				} else if (it->videoDecoder->needsUpdate()) {
					updateScreen(*it);
				}
				i++;
			}
		}

		if (videosToRemove.size() > 0) {
			for(Common::List<uint32>::iterator it = videosToRemove.begin(); it != videosToRemove.end(); ++it) {
				debug("removing %d from %d size", *it, _videosPlaying.size()); 
				_videosPlaying.remove_at(*it);
			}
		}

		drawScreen();
		g_system->delayMillis(10);
	}
}

void HypnoEngine::runScene(Hotspots hots, Videos intros) {
	Common::Event event;
	Common::Point mousePos;
	Common::List<uint32> videosToRemove;
	
	stack.clear();
	_nextHotsToAdd = &hots;
	_nextSequentialVideoToPlay = intros;	
	changeCursor("mouse/cursor1.smk", 0);

	while (!shouldQuit() && _nextSetting.empty()) {
		
		while (g_system->getEventManager()->pollEvent(event)) {
			mousePos = g_system->getEventManager()->getMousePos();
			// Events
			switch (event.type) {
			case Common::EVENT_KEYDOWN:
				if (event.kbd.keycode == Common::KEYCODE_ESCAPE) {
					for (Videos::iterator it = _videosPlaying.begin(); it != _videosPlaying.end(); ++it) {
						if (it->videoDecoder)
							skipVideo(*it);
					}
					_videosPlaying.clear();

					if (!stack.empty()) { 
						runMenu(*stack.back());
						drawScreen();
					}
				}

				break;

			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				break;

			case Common::EVENT_LBUTTONDOWN:
				if (stack.empty())
					break;
				if (!_nextHotsToAdd || !_nextHotsToRemove)
				 	clickedHotspot(mousePos);
				break;

			case Common::EVENT_MOUSEMOVE:
				// Reset cursor to default
				//changeCursor("default");
				// The following functions will return true
				// if the cursor is changed
				if (hoverHotspot(mousePos)) {
				} else
					changeCursor("mouse/cursor1.smk", 0);
				break;

			default:
				break;
			}
		}

		// Movies
		if (_nextSequentialVideoToPlay.size() > 0 && _videosPlaying.empty()) {
			playVideo(*_nextSequentialVideoToPlay.begin());
			_videosPlaying.push_back(*_nextSequentialVideoToPlay.begin());
			_nextSequentialVideoToPlay.remove_at(0);
		}
		uint32 i = 0;
		videosToRemove.clear();
		for (Videos::iterator it = _videosPlaying.begin(); it != _videosPlaying.end(); ++it) {

			if (it->videoDecoder) {
				if (it->videoDecoder->endOfVideo()) {
				it->videoDecoder->close();
				delete it->videoDecoder;
				it->videoDecoder = nullptr;
				videosToRemove.push_back(i);

				} else if (it->videoDecoder->needsUpdate()) {
					updateScreen(*it);
				}
			}
			i++;

		}
		if (videosToRemove.size() > 0) {

			for(Common::List<uint32>::iterator it = videosToRemove.begin(); it != videosToRemove.end(); ++it) {
				debug("removing %d from %d size", *it, _videosPlaying.size()); 
				_videosPlaying.remove_at(*it);
			}

			// Nothing else to play
			if (_videosPlaying.size() == 0 && _nextSequentialVideoToPlay.size() == 0){
				if (!stack.empty()) { 
					runMenu(*stack.back());
					drawScreen();
				}
			}

		}

		if (_videosPlaying.size() > 0 || _nextSequentialVideoToPlay.size() > 0) {
			drawScreen();
			continue;
		} 

		if (_nextHotsToRemove) {
			debug("Removing a hotspot list!");
			stack.pop_back();
			runMenu(*stack.back());
			_nextHotsToRemove = NULL;
			drawScreen();
		} else if (_nextHotsToAdd) {
			debug("Adding a hotspot list!");
			//clearAreas();
			stack.push_back(_nextHotsToAdd);
			runMenu(*stack.back());
			_nextHotsToAdd = NULL;
			drawScreen();
		}

		if (checkLevelCompleted())
			_nextSetting = "mis/demo.mis";

		g_system->updateScreen();
		g_system->delayMillis(10);
	}
}

//Actions

void HypnoEngine::runMenu(Hotspots hs) {
	const Hotspot h = *hs.begin();
	assert(h.type == MakeMenu);

	debug("hotspot actions size: %d", h.actions.size());
	for (Actions::const_iterator itt = h.actions.begin(); itt != h.actions.end(); ++itt) {
		Action *action = *itt;
		if (typeid(*action) == typeid(Quit))
			runQuit(h, (Quit*) action);
		else if (typeid(*action) == typeid(Background))
			runBackground(h, (Background*) action);
		else if (typeid(*action) == typeid(Overlay))
			runOverlay(h, (Overlay*) action);
		//else if (typeid(*action) == typeid(Mice))
		//	runMice(h, (Mice*) action);
	}

	//if (h.stype == "SINGLE_RUN")
	//	loadImage("int_main/mainbutt.smk", 0, 0);
	if (h.stype == "AUTO_BUTTONS")
		loadImage("int_main/resume.smk", 0, 0, true);
}

void HypnoEngine::runBackground(const Hotspot h, Background *a) {
	if (a->condition.size() > 0 && !_levelState[a->condition])
		return;
	Common::Point origin = a->origin;
	loadImage(a->path, origin.x, origin.y, false);
}

void HypnoEngine::runOverlay(const Hotspot h, Overlay *a) {
	Common::Point origin = a->origin;
	loadImage(a->path, origin.x, origin.y, false);
}

void HypnoEngine::runMice(const Hotspot h, Mice *a) {
    changeCursor(a->path, a->index);
}

void HypnoEngine::runEscape(const Hotspot h, Escape *a) {
    _nextHotsToRemove = stack.back();
}

void HypnoEngine::runCutscene(const Hotspot h, Cutscene *a) {
	_nextSequentialVideoToPlay.push_back(MVideo(a->path, Common::Point(0, 0), false, true));
}

void HypnoEngine::runGlobal(const Hotspot h, Global *a) {
    if (a->command == "TURNON")
		_levelState[a->variable] = 1;
	else if (a->command == "TURNOFF")
		_levelState[a->variable] = 0;
	else
		error("Invalid command %s", a->command.c_str());
}

void HypnoEngine::runPlay(const Hotspot h, Play *a) {
	if (a->condition.size() > 0 && !_levelState[a->condition])
		return;
	Common::Point origin = a->origin;

	if (a->flag == "BITMAP")
			loadImage(a->path, origin.x, origin.y, false);
	else {
		_nextSequentialVideoToPlay.push_back(MVideo(a->path, a->origin, false, false));
	}
}

void HypnoEngine::runWalN(const Hotspot h, WalN *a) {
	if (a->condition.size() > 0 && !_levelState[a->condition])
		return;
	Common::Point origin = a->origin;
	if (a->flag == "BITMAP")
			loadImage(a->path, origin.x, origin.y, false);
	else { 
		_nextSequentialVideoToPlay.push_back(MVideo(a->path, a->origin, false, false));
	}
}

void HypnoEngine::runQuit(const Hotspot h, Quit *a) {
    quitGame();
}

// Shoots

bool HypnoEngine::clickedShoot(Common::Point mousePos) {
	bool found = false;
	int x;
	int y;

	Videos::iterator it = _videosPlaying.begin();
	//it++;
	for (; it != _videosPlaying.end(); ++it) {
		x = mousePos.x - it->position.x;
		y = mousePos.y - it->position.y;
		//debug("%d %d %d %d", x, y, it->videoDecoder->getWidth(), it->videoDecoder->getHeight());
		//assert(it->currentFrame->w == it->videoDecoder->getWidth());
		//assert(it->currentFrame->h == it->videoDecoder->getHeight());
		if (it->videoDecoder && x >= 0 && y >= 0 && x < it->videoDecoder->getWidth() && y < it->videoDecoder->getHeight()) {
			uint32 c = it->currentFrame->getPixel(x, y);
			debug("inside %x", c); 
			if (c > 0) {
				it->videoDecoder->rewind();
				it->videoDecoder->start();
			}
		}
	}
	return found;
}

// Hotspots

void HypnoEngine::clickedHotspot(Common::Point mousePos) {
	debug("clicked in %d %d", mousePos.x, mousePos.y);
	Hotspots *hots = stack.back();
	Hotspot selected;
	bool found = false;
	int rs = 100000000;
	int cs = 0;
	for (Hotspots::const_iterator it = hots->begin(); it != hots->end(); ++it) {
		const Hotspot h = *it;
		if (h.type != MakeHotspot)
			continue;

		cs = h.rect.width() * h.rect.height();
		if (h.rect.contains(mousePos)) {
			if (cs < rs) {
				selected = h;
				found = true;
				rs = cs;
			}
		}
	}
	if (found) {
		//debug("Hotspot found! %x", selected.smenu);
		if (selected.smenu) {
			debug("SMenu found!");
			assert(selected.smenu->size() > 0);
			_nextHotsToAdd = selected.smenu;
		}

		if (!selected.setting.empty())
			_nextSetting = selected.setting;

		debug("hotspot clicked actions size: %d", selected.actions.size());
		for (Actions::const_iterator itt = selected.actions.begin(); itt != selected.actions.end(); ++itt) {
			Action *action = *itt;
			if (typeid(*action) == typeid(Escape))
				runEscape(selected, (Escape*) action);
			if (typeid(*action) == typeid(Cutscene))
				runCutscene(selected, (Cutscene*) action);
			if (typeid(*action) == typeid(Play))
				runPlay(selected, (Play*) action);
			if (typeid(*action) == typeid(WalN))
				runWalN(selected, (WalN*) action);
			if (typeid(*action) == typeid(Global))
				runGlobal(selected, (Global*) action);
		}

	}
}

bool HypnoEngine::hoverHotspot(Common::Point mousePos) {
	if (stack.empty())
		return false;

	Hotspots *hots = stack.back();
	Hotspot selected;
	bool found = false;
	int rs = 100000000;
	int cs = 0;
	for (Hotspots::const_iterator it = hots->begin(); it != hots->end(); ++it) {
		const Hotspot h = *it;
		if (h.type != MakeHotspot)
			continue;

		cs = h.rect.width() * h.rect.height();
		if (h.rect.contains(mousePos)) {
			if (cs < rs) {
				selected = h;
				found = true;
				rs = cs;
			}
		}
	}
	if (found) {
		//debug("Hovered over %d %d %d %d!", selected.rect.left, selected.rect.top, selected.rect.bottom, selected.rect.right);

		//debug("hotspot actions size: %d", h.actions.size());
		for (Actions::const_iterator itt = selected.actions.begin(); itt != selected.actions.end(); ++itt) {
			Action *action = *itt;
			if (typeid(*action) == typeid(Mice))
				runMice(selected, (Mice*) action);
		}
		return true;
	}
	return false;
}

void HypnoEngine::loadImage(const Common::String &name, int x, int y, bool transparent) {
	Graphics::Surface *surf = decodeFrame(name, 0);
	drawImage(*surf, x , y, transparent);
}

void HypnoEngine::drawImage(Graphics::Surface &surf, int x, int y, bool transparent) {
	if (transparent)
		_compositeSurface->transBlitFrom(surf, Common::Point(x, y), _transparentColor);
	else 
		_compositeSurface->blitFrom(surf, Common::Point(x, y));
}

Graphics::Surface *HypnoEngine::decodeFrame(const Common::String &name, int n, bool convert) {
	Common::File *file = new Common::File();
	Common::String path = convertPath(name);

	if (!file->open(path))
		error("unable to find video file %s", path.c_str());

	Video::SmackerDecoder vd;
	if (!vd.loadStream(file))
		error("unable to load video %s", path.c_str());

    for(int f = 0; f < n; f++) 
		vd.decodeNextFrame();

	const Graphics::Surface *frame = vd.decodeNextFrame();
	Graphics::Surface *rframe;

	if (convert) {
		rframe = frame->convertTo(_pixelFormat, vd.getPalette());
	} else {
		rframe = frame->convertTo(frame->format, vd.getPalette());
		//rframe->create(frame->w, frame->h, frame->format);
		//rframe->copyRectToSurface(frame->getPixels(), frame->pitch, 0, 0, frame->w, frame->h);
	}

	return rframe;
}

void HypnoEngine::changeScreenMode(Common::String mode) {
	if (mode == "scene") {
		_screenW = 640;
		_screenH = 480;

		initGraphics(_screenW, _screenH, nullptr);

		_compositeSurface->free();
		delete _compositeSurface;

		_compositeSurface = new Graphics::ManagedSurface();
		_compositeSurface->create(_screenW, _screenH, _pixelFormat);

		_transparentColor = _pixelFormat.RGBToColor(0, 0x82, 0);
		_compositeSurface->setTransparentColor(_transparentColor);

	} else if (mode == "arcade") {
		_screenW = 320;
		_screenH = 200;

		initGraphics(_screenW, _screenH, nullptr);

		_compositeSurface->free();
		delete _compositeSurface;

		_compositeSurface = new Graphics::ManagedSurface();
		_compositeSurface->create(_screenW, _screenH, _pixelFormat);

		_transparentColor = _pixelFormat.RGBToColor(0, 0, 0);
		_compositeSurface->setTransparentColor(_transparentColor);
	} else
		error("Unknown screen mode %s", mode.c_str());
}

void HypnoEngine::updateScreen(MVideo &video) {
	const Graphics::Surface *frame = video.videoDecoder->decodeNextFrame();
	video.currentFrame = frame;
	Graphics::Surface *sframe, *cframe;

	if (video.scaled) {
		sframe = frame->scale(_screenW, _screenH);
		cframe = sframe->convertTo(_pixelFormat, video.videoDecoder->getPalette());
	} else
		cframe = frame->convertTo(_pixelFormat, video.videoDecoder->getPalette());
	
	if (video.transparent)
		_compositeSurface->transBlitFrom(*cframe, video.position, _transparentColor);
	else
		_compositeSurface->blitFrom(*cframe, video.position);


	if (video.scaled) { 
		sframe->free();
		delete sframe;
	}

	cframe->free();
	delete cframe;

}


void HypnoEngine::drawScreen() {
	g_system->copyRectToScreen(_compositeSurface->getPixels(), _compositeSurface->pitch, 0, 0, _screenW, _screenH);
	g_system->updateScreen();
}

// Video handling

void HypnoEngine::playVideo(MVideo &video) {
	//debugC(1, kPrivateDebugFunction, "%s(%s)", __FUNCTION__, name.c_str());
	debug("video.path: %s", video.path.c_str());
	Common::File *file = new Common::File();
	Common::String path = convertPath(video.path);

	if (!file->open(path))
		error("unable to find video file %s", path.c_str());

	assert(video.videoDecoder == nullptr);
	video.videoDecoder = new Video::SmackerDecoder();

	if (!video.videoDecoder->loadStream(file))
		error("unable to load video %s", path.c_str());
	video.videoDecoder->start();
}

void HypnoEngine::skipVideo(MVideo &video) {
	video.videoDecoder->close();
	delete video.videoDecoder;
	video.videoDecoder = nullptr;	
	//_currentMovie = "";
}

// Sound handling

void HypnoEngine::stopSound(bool all) {
	// debugC(1, kPrivateDebugFunction, "%s(%d)", __FUNCTION__, all);

	// if (all) {
	// 	_mixer->stopHandle(_fgSoundHandle);
	// 	_mixer->stopHandle(_bgSoundHandle);
	// } else {
	// 	_mixer->stopHandle(_fgSoundHandle);
	// }
}

// Path handling

Common::String HypnoEngine::convertPath(const Common::String &name) {
	Common::String path(name);
	Common::String s1("\\");
	Common::String s2("/");

	while (path.contains(s1))
		Common::replace(path, s1, s2);

	s1 = Common::String("\"");
	s2 = Common::String("");

	Common::replace(path, s1, s2);
	Common::replace(path, s1, s2);

	path.toLowercase();
	return path;
}

} // End of namespace Hypno
