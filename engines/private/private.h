#ifndef PRIVATE_H
#define PRIVATE_H

#include "common/random.h"
#include "common/serializer.h"
#include "engines/advancedDetector.h"
#include "engines/engine.h"
#include "gui/debugger.h"

#include "common/installer_archive.h"

#include "audio/mixer.h"
#include "video/smk_decoder.h"
#include "graphics/palette.h"
#include "graphics/managed_surface.h"

#include "private/grammar.h"

namespace Image {
class ImageDecoder;
}

namespace Graphics {
struct ManagedSurface;
}

struct ADGameDescription;

namespace Private {

class Console;

const uint kPrivateDebug = 1;

// sounds

const int kPaperShuffleSound[7] = {32, 33, 34, 35, 36, 37, 39};

// police

const int kPoliceBustVideos[6] = {1, 2, 4, 5, 7, 8};

// points

const Common::Point kPrivateOriginZero(0, 0);
const Common::Point kPrivateOriginOne(64, 48);

// settings

static Common::String kPauseMovie("kPauseMovie");
static Common::String kGoIntro("kGoIntro");
static Common::String kPoliceBustFromMO("kPoliceBustFromMO");
static Common::String kMainDesktop("kMainDesktop");
static Common::String kPoliceIndex("kPoliceIndex");
static Common::String kPOGoBustMovie("kPOGoBustMovie");
static Common::String kStartGame("kStartGame");

// structs

typedef struct ExitInfo {
    Common::String *nextSetting;
    Common::Rect   *rect;
    Common::String *cursor;
} ExitInfo;

typedef struct MaskInfo {
    Graphics::ManagedSurface *surf;
    Common::String *nextSetting;
    Common::Point *point;
    Symbol *flag1;
    Symbol *flag2;
    Common::String *cursor;
} MaskInfo;

typedef struct PhoneInfo {
    Common::String *sound;
    Symbol *flag;
    int val;
} PhoneInfo;

typedef struct DossierInfo {
    Common::String *page1;
    Common::String *page2;
} DossierInfo;

// lists

typedef Common::List<ExitInfo> ExitList;
typedef Common::List<MaskInfo> MaskList;
typedef Common::List<Common::String> SoundList;
typedef Common::List<PhoneInfo> PhoneList;
typedef Common::List<Common::String> InvList;

// arrays

typedef Common::Array<DossierInfo> DossierArray;

// hash tables

typedef Common::HashMap<Common::String, bool> PlayedMediaTable;

class PrivateEngine : public Engine {
private:
    Common::RandomSource *_rnd;

    Graphics::PixelFormat _pixelFormat;
    Image::ImageDecoder *_image;
    Graphics::ManagedSurface *_compositeSurface;

    int _screenW, _screenH;

public:
    PrivateEngine(OSystem *syst, const ADGameDescription *gd);
    ~PrivateEngine();

    const ADGameDescription *_gameDescription;

    bool isDemo() const;

    Audio::SoundHandle _fgSoundHandle;
    Audio::SoundHandle _bgSoundHandle;
    Video::SmackerDecoder *_videoDecoder;
    Common::InstallerArchive _installerArchive;

    Common::Error run() override;
    void restartGame();
    void clearAreas();
    void initializePath(const Common::FSNode &gamePath) override;

    // User input

    void selectPauseMovie(Common::Point);
    void selectMask(Common::Point);
    void selectExit(Common::Point);
    void selectLoadGame(Common::Point);
    void selectSaveGame(Common::Point);

    // Cursors

    bool cursorPauseMovie(Common::Point);
    bool cursorExit(Common::Point);
    bool cursorMask(Common::Point);

    bool hasFeature(EngineFeature f) const override;
    bool canLoadGameStateCurrently() override {
        return true;
    }
    bool canSaveAutosaveCurrently() override  {
        return false;
    }
    bool canSaveGameStateCurrently() override {
        return true;
    }

    Common::Error loadGameStream(Common::SeekableReadStream *stream) override;
    Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override;
    void syncGameStream(Common::Serializer &s);

    Common::String convertPath(Common::String);
    void playVideo(const Common::String &);
    void skipVideo();

    void loadImage(const Common::String &file, int x, int y);
    void drawScreenFrame(Graphics::Surface *);

    void changeCursor(Common::String);
    void initCursors();

    Graphics::ManagedSurface *loadMask(const Common::String &, int, int, bool);
    void drawMask(Graphics::ManagedSurface *);
    bool inMask(Graphics::ManagedSurface*, Common::Point);
    uint32 _transparentColor;
    void drawScreen();

    // global state
    const Common::Point *_origin;
    Common::String *_nextSetting;
    Common::String *_currentSetting;
    Common::String *_nextVS;
    Common::String *_frame;
    bool            _toTake;

    // Dossiers

    DossierArray _dossiers;
    unsigned int _dossierSuspect;
    unsigned int _dossierPage;
    MaskInfo *_dossierNextSuspectMask;
    MaskInfo *_dossierPrevSuspectMask;
    bool selectDossierNextSuspect(Common::Point);
    bool selectDossierPrevSuspect(Common::Point);
    void loadDossier();

    // Police Bust

    int policeVideoIndex;
    void policeBust();
    bool _policeBustEnabled;
    void startPoliceBust();
    void checkPoliceBust();
    int _numberClicks;
    int _maxNumberClicks;
    int _sirenWarning;
    Common::String *_policeBustSetting;

    // Diary

    InvList inventory;
    Common::String *_diaryLocPrefix;
    void loadLocations(Common::Rect *);
    void loadInventory(uint32, Common::Rect *, Common::Rect *);

    // Save/Load games
    MaskInfo *_saveGameMask;
    MaskInfo *_loadGameMask;

    int _mode;
    bool _modified;
    Common::String *_nextMovie;
    PlayedMediaTable _playedMovies;
    PlayedMediaTable _playedPhoneClips;
    Common::String *_repeatedMovieExit;
    Common::String *_pausedSetting;

    // Masks/Exits

    ExitList _exits;
    MaskList _masks;

    // Sounds

    void playSound(const Common::String &, uint, bool, bool);
    void stopSound(bool);
    bool _noStopSounds;

    Common::String *getPaperShuffleSound();
    Common::String *_globalAudioPath;

    Common::String *getTakeSound();
    Common::String *getTakeLeaveSound();
    Common::String *getLeaveSound();
    Common::String *_sirenSound;

    // Radios

    //Common::String *_radioSound;
    Common::String *_infaceRadioPath;

    MaskInfo *_AMRadioArea;
    //Common::String *_AMRadioPrefix;

    MaskInfo *_policeRadioArea;
    //Common::String *_policeRadioPrefix;

    MaskInfo *_phoneArea;
    Common::String *_phonePrefix;
    Common::String *_phoneCallSound;

    SoundList _AMRadio;
    SoundList _policeRadio;
    PhoneList _phone;

    char *getRandomPhoneClip(char *, int, int);

    void selectAMRadioArea(Common::Point);
    void selectPoliceRadioArea(Common::Point);
    void selectPhoneArea(Common::Point);
    void checkPhoneCall();

    // Random values

    bool getRandomBool(uint);

    // Timers

    bool installTimer(uint32, Common::String *);
    void removeTimer();

};

extern PrivateEngine *g_private;

class Console : public GUI::Debugger {
public:
    Console(PrivateEngine *vm) {
    }
    virtual ~Console(void) {
    }
};

} // End of namespace Private

#endif
