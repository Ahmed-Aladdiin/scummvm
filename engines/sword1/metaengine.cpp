/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "engines/advancedDetector.h"

#include "sword1/sword1.h"
#include "sword1/control.h"
#include "sword1/logic.h"

#include "common/savefile.h"
#include "common/system.h"
#include "common/translation.h"

#include "graphics/thumbnail.h"
#include "graphics/surface.h"

namespace Sword1 {
		
#define GAMEOPTION_WINDOWS_AUDIO_MODE GUIO_GAMEOPTIONS1

static const ADExtraGuiOptionsMap optionsList[] = {
	{
		GAMEOPTION_WINDOWS_AUDIO_MODE,
		{
			_s("Simulate the audio engine from the Windows executable"),
			_s("Makes the game use softer (logarithmic) audio curves, but removes fade-in and fade-out for "
			   "sound effects, fade-in for music, and automatic music volume attenuation for when speech is playing"),
			"windows_audio_mode",
			false,
			0,
			0
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

} // End of namespace Sword1

class SwordMetaEngine : public AdvancedMetaEngine {
public:
	const char *getName() const override {
		return "sword1";
	}

	bool hasFeature(MetaEngineFeature f) const override;

	SaveStateList listSaves(const char *target) const override;
	int getMaximumSaveSlot() const override;
	void removeSaveState(const char *target, int slot) const override;
	SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const override;
	const ADExtraGuiOptionsMap *getAdvancedExtraGuiOptions() const override {
		return Sword1::optionsList;
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override;

	Common::String getSavegameFile(int saveGameIdx, const char *target) const override {
		if (saveGameIdx == kSavegameFilePattern)
			return Common::String::format("sword1.###");
		else
			return Common::String::format("sword1.%03d", saveGameIdx);
	}
};

bool SwordMetaEngine::hasFeature(MetaEngineFeature f) const {
	return
	    (f == kSupportsListSaves) ||
	    (f == kSupportsLoadingDuringStartup) ||
	    (f == kSupportsDeleteSave) ||
	    (f == kSavesSupportMetaInfo) ||
	    (f == kSavesSupportThumbnail) ||
	    (f == kSavesSupportCreationDate) ||
	    (f == kSavesSupportPlayTime);
}

bool Sword1::SwordEngine::hasFeature(EngineFeature f) const {
	return
	    (f == kSupportsReturnToLauncher) ||
	    (f == kSupportsSavingDuringRuntime) ||
	    (f == kSupportsLoadingDuringRuntime);
}

Common::Error SwordMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	*engine = new Sword1::SwordEngine(syst, desc);
	return Common::kNoError;
}

SaveStateList SwordMetaEngine::listSaves(const char *target) const {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	SaveStateList saveList;
	char saveName[40];

	Common::StringArray filenames = saveFileMan->listSavefiles("sword1.###");

	int slotNum = 0;
	for (Common::StringArray::const_iterator file = filenames.begin(); file != filenames.end(); ++file) {
		// Obtain the last 3 digits of the filename, since they correspond to the save slot
		slotNum = atoi(file->c_str() + file->size() - 3);

		if (slotNum >= 0 && slotNum <= 999) {
			Common::InSaveFile *in = saveFileMan->openForLoading(*file);
			if (in) {
				in->readUint32LE(); // header
				in->read(saveName, 40);
				saveList.push_back(SaveStateDescriptor(this, slotNum, saveName));
				delete in;
			}
		}
	}

	// Sort saves based on slot number.
	Common::sort(saveList.begin(), saveList.end(), SaveStateDescriptorSlotComparator());
	return saveList;
}

int SwordMetaEngine::getMaximumSaveSlot() const { return 999; }

void SwordMetaEngine::removeSaveState(const char *target, int slot) const {
	g_system->getSavefileManager()->removeSavefile(Common::String::format("sword1.%03d", slot));
}

SaveStateDescriptor SwordMetaEngine::querySaveMetaInfos(const char *target, int slot) const {
	Common::String fileName = Common::String::format("sword1.%03d", slot);
	char name[40];
	uint32 playTime = 0;
	byte versionSave;

	Common::InSaveFile *in = g_system->getSavefileManager()->openForLoading(fileName);

	if (in) {
		in->skip(4);        // header
		in->read(name, sizeof(name));
		in->read(&versionSave, 1);      // version

		SaveStateDescriptor desc(this, slot, name);

		if (versionSave < 2) // These older version of the savegames used a flag to signal presence of thumbnail
			in->skip(1);

		if (Graphics::checkThumbnailHeader(*in)) {
			Graphics::Surface *thumbnail;
			if (!Graphics::loadThumbnail(*in, thumbnail)) {
				delete in;
				return SaveStateDescriptor();
			}
			desc.setThumbnail(thumbnail);
		}

		uint32 saveDate = in->readUint32BE();
		uint16 saveTime = in->readUint16BE();
		if (versionSave > 1) // Previous versions did not have playtime data
			playTime = in->readUint32BE();

		int day = (saveDate >> 24) & 0xFF;
		int month = (saveDate >> 16) & 0xFF;
		int year = saveDate & 0xFFFF;

		desc.setSaveDate(year, month, day);

		int hour = (saveTime >> 8) & 0xFF;
		int minutes = saveTime & 0xFF;

		desc.setSaveTime(hour, minutes);

		if (versionSave > 1) {
			desc.setPlayTime(playTime * 1000);
		} else { //We have no playtime data
			desc.setPlayTime(0);
		}

		delete in;

		return desc;
	}

	return SaveStateDescriptor();
}

#if PLUGIN_ENABLED_DYNAMIC(SWORD1)
	REGISTER_PLUGIN_DYNAMIC(SWORD1, PLUGIN_TYPE_ENGINE, SwordMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(SWORD1, PLUGIN_TYPE_ENGINE, SwordMetaEngine);
#endif

namespace Sword1 {

Common::Error SwordEngine::loadGameState(int slot) {
	_systemVars.controlPanelMode = CP_NORMAL;
	_control->restoreGameFromFile(slot);
	reinitialize();
	_control->doRestore();
	reinitRes();
	return Common::kNoError;    // TODO: return success/failure
}

bool SwordEngine::canLoadGameStateCurrently(Common::U32String *msg) {
	return (mouseIsActive() && !_control->isPanelShown()); // Disable GMM loading when game panel is shown
}

Common::Error SwordEngine::saveGameState(int slot, const Common::String &desc, bool isAutosave) {
	_control->setSaveDescription(slot, desc.c_str());
	_control->saveGameToFile(slot);
	return Common::kNoError;    // TODO: return success/failure
}

bool SwordEngine::canSaveGameStateCurrently(Common::U32String *msg) {
	return (mouseIsActive() && !_control->isPanelShown() && Logic::_scriptVars[SCREEN] != 91);
}

} // End of namespace Sword1
