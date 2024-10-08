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

#include "backends/imgui/imgui_utils.h"
#include "common/util.h"
#include "twine/debugger/debug_state.h"
#include "twine/debugger/debugtools.h"
#include "twine/debugger/dt-internal.h"
#include "twine/holomap.h"
#include "twine/renderer/redraw.h"
#include "twine/scene/actor.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/grid.h"
#include "twine/scene/scene.h"

#include "twine/shared.h"
#include "twine/twine.h"

namespace ImGuiEx {

bool InputIVec3(const char *label, TwinE::IVec3 &v, ImGuiInputTextFlags flags = 0) {
	int tmp[3] = {v.x, v.y, v.z};
	if (ImGui::InputInt3(label, tmp, flags)) {
		v.x = tmp[0];
		v.y = tmp[1];
		v.z = tmp[2];
		return true;
	}
	return false;
}

bool InputAngle(const char *label, int32 *v, int step = 1, int step_fast = 100, const char *format = "%.2f", ImGuiInputTextFlags flags = 0) {
	double tmp = TwinE::AngleToDegree(*v);
	if (ImGui::InputDouble(label, &tmp, step, step_fast, format, flags)) {
		*v = TwinE::DegreeToAngle(tmp);
		return true;
	}
	ImGui::SetItemTooltip("Angle: %i", (int)*v);
	return false;
}

} // namespace ImGuiEx

namespace TwinE {

#define MAIN_WINDOW_TITLE "Debug window"
#define HOLOMAP_FLAGS_TITLE "Holomap flags"
#define GAME_FLAGS_TITLE "Game flags"
#define ACTOR_DETAILS_TITLE "Actor"
#define MENU_TEXT_TITLE "Menu texts"

static const char *toString(ShapeType type) {
	switch (type) {
	case ShapeType::kNone:
		return "None";
	case ShapeType::kSolid:
		return "Solid";
	case ShapeType::kStairsTopLeft:
		return "StairsTopLeft";
	case ShapeType::kStairsTopRight:
		return "StairsTopRight";
	case ShapeType::kStairsBottomLeft:
		return "StairsBottomLeft";
	case ShapeType::kStairsBottomRight:
		return "StairsBottomRight";
	case ShapeType::kDoubleSideStairsTop1:
		return "DoubleSideStairsTop1";
	case ShapeType::kDoubleSideStairsBottom1:
		return "DoubleSideStairsBottom1";
	case ShapeType::kDoubleSideStairsLeft1:
		return "DoubleSideStairsLeft1";
	case ShapeType::kDoubleSideStairsRight1:
		return "DoubleSideStairsRight1";
	case ShapeType::kDoubleSideStairsTop2:
		return "DoubleSideStairsTop2";
	case ShapeType::kDoubleSideStairsBottom2:
		return "DoubleSideStairsBottom2";
	case ShapeType::kDoubleSideStairsLeft2:
		return "DoubleSideStairsLeft2";
	case ShapeType::kDoubleSideStairsRight2:
		return "DoubleSideStairsRight2";
	case ShapeType::kFlatBottom1:
		return "FlatBottom1";
	case ShapeType::kFlatBottom2:
		return "FlatBottom2";
	default:
		return "Unknown";
	}
}

void onImGuiInit() {
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = false;
	icons_config.OversampleH = 3;
	icons_config.OversampleV = 3;
	icons_config.GlyphOffset = {0, 4};

	static const ImWchar icons_ranges[] = {ICON_MIN_MS, ICON_MAX_MS, 0};
	ImGui::addTTFFontFromArchive("MaterialSymbolsSharp.ttf", 16.f, &icons_config, icons_ranges);

	_tinyFont = ImGui::addTTFFontFromArchive("FreeSans.ttf", 10.0f, nullptr, nullptr);
}

static void holomapFlagsPopup(TwinEEngine *engine) {
	if (ImGui::BeginPopup(HOLOMAP_FLAGS_TITLE)) {
		if (ImGui::BeginTable("###holomapflags", 8)) {
			for (int i = 0; i < engine->numHoloPos(); ++i) {
				ImGui::TableNextColumn();
				Common::String id = Common::String::format("[%03d]", i);
				ImGuiEx::InputInt(id.c_str(), &engine->_gameState->_holomapFlags[i]);
			}
			ImGui::EndTable();
		}
		ImGui::EndPopup();
	}
}

static void gameFlagsPopup(TwinEEngine *engine) {
	if (ImGui::BeginPopup(GAME_FLAGS_TITLE)) {
		ImGui::Text("Chapter %i", engine->_gameState->getChapter());
		if (ImGui::BeginTable("###gameflags", 8)) {
			for (int i = 0; i < NUM_GAME_FLAGS; ++i) {
				ImGui::TableNextColumn();
				Common::String id = Common::String::format("[%03d]", i);
				int16 val = engine->_gameState->hasGameFlag(i);
				if (ImGuiEx::InputInt(id.c_str(), &val)) {
					engine->_gameState->setGameFlag(i, val);
				}
			}
			ImGui::EndTable();
		}
		ImGui::EndPopup();
	}
}

static void menuTextsPopup(TwinEEngine *engine) {
	if (ImGui::BeginPopup(MENU_TEXT_TITLE)) {
		int id = (int)engine->_debugState->_textBankId;
		if (ImGui::InputInt("Text bank", &id)) {
			engine->_debugState->_textBankId = (TextBankId)id;
		}
		const TextBankId oldTextBankId = engine->_text->textBank();
		engine->_text->initDial(engine->_debugState->_textBankId);
		for (int32 i = 0; i < 1000; ++i) {
			char buf[256];
			if (engine->_text->getMenuText((TextId)i, buf, sizeof(buf))) {
				ImGui::Text("%4i: %s\n", i, buf);
			}
		}
		engine->_text->initDial(oldTextBankId);
		ImGui::EndPopup();
	}
}

static void actorDetailsWindow(int &actorIdx, TwinEEngine *engine) {
	ActorStruct *actor = engine->_scene->getActor(actorIdx);
	if (actor == nullptr) {
		return;
	}
	if (ImGui::Begin(ACTOR_DETAILS_TITLE)) {
		if (actorIdx < 0 || actorIdx > engine->_scene->_nbObjets) {
			actorIdx = 0;
		}
		Common::String currentActorLabel = Common::String::format("Actor %i", actorIdx);
		if (ImGui::BeginCombo("Actor", currentActorLabel.c_str())) {
			for (int i = 0; i < engine->_scene->_nbObjets; ++i) {
				Common::String label = Common::String::format("Actor %i", i);
				const bool selected = i == actorIdx;
				if (ImGui::Selectable(label.c_str(), selected)) {
					actorIdx = i;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();

		ImGuiEx::InputIVec3("Pos", actor->_posObj);
		ImGuiEx::InputAngle("Rotation", &actor->_beta);
		ImGuiEx::InputInt("Speed", &actor->_speed);
		ImGuiEx::InputInt("Life", &actor->_lifePoint);
		ImGuiEx::InputInt("Armor", &actor->_armor);
		ImGuiEx::InputIVec3("Bounding box mins", actor->_boundingBox.mins);
		ImGuiEx::InputIVec3("Bounding box maxs", actor->_boundingBox.maxs);

		if (ImGui::BeginTable("Properties", 2)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			ImGui::TableNextColumn();
			ImGui::Text("Followed");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_followedActor);
			ImGui::TableNextColumn();
			ImGui::Text("Control mode");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_controlMode);
			ImGui::TableNextColumn();
			ImGui::Text("Delay");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_delayInMillis);
			ImGui::TableNextColumn();
			ImGui::Text("Strength");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_strengthOfHit);
			ImGui::TableNextColumn();
			ImGui::Text("Hit by");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_hitBy);
			ImGui::TableNextColumn();
			ImGui::Text("Bonus");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_bonusParameter);
			ImGui::TableNextColumn();
			ImGui::Text("Brick shape");
			ImGui::TableNextColumn();
			ImGui::Text("%s", toString(actor->brickShape()));
			ImGui::TableNextColumn();
			ImGui::Text("Brick causes damage");
			ImGui::TableNextColumn();
			ImGuiEx::Boolean(actor->brickCausesDamage());
			ImGui::TableNextColumn();
			ImGui::Text("Collision");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_objCol);
			ImGui::TableNextColumn();
			ImGui::Text("Body");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_body); // TODO: link to resources
			ImGui::TableNextColumn();
			ImGui::Text("Gen body");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_genBody);
			ImGui::TableNextColumn();
			ImGui::Text("Save gen body");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_saveGenBody);
			ImGui::TableNextColumn();
			ImGui::Text("Gen anim");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_genAnim);
			ImGui::TableNextColumn();
			ImGui::Text("Next gen anim");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_nextGenAnim);
			ImGui::TableNextColumn();
			ImGui::Text("Ptr anim action");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_ptrAnimAction);
			ImGui::TableNextColumn();
			ImGui::Text("Sprite");
			ImGui::TableNextColumn();
			ImGui::Text("%i", actor->_sprite);
			ImGui::TableNextColumn();
			ImGui::Text("A3DS");
			ImGui::TableNextColumn();
			ImGui::Text("%i %i %i", actor->A3DS.Num, actor->A3DS.Deb, actor->A3DS.Fin);

			ImGui::EndTable();
		}
	}
	ImGui::End();
}

static void gameStateMenu(TwinEEngine *engine) {
	if (ImGui::BeginMenu("Game State")) {
		int keys = engine->_gameState->_inventoryNumKeys;
		if (ImGui::InputInt("Keys", &keys)) {
			engine->_gameState->setKeys(keys);
		}
		int kashes = engine->_gameState->_goldPieces;
		if (ImGui::InputInt("Cash", &kashes)) {
			engine->_gameState->setKashes(kashes);
		}
		int zlitos = engine->_gameState->_zlitosPieces;
		if (ImGui::InputInt("Zlitos", &zlitos)) {
			engine->_gameState->setZlitos(zlitos);
		}
		int magicPoints = engine->_gameState->_magicPoint;
		if (ImGui::InputInt("Magic points", &magicPoints)) {
			engine->_gameState->setMagicPoints(magicPoints);
		}
		int magicLevel = engine->_gameState->_magicLevelIdx;
		if (ImGui::InputInt("Magic level", &magicLevel)) {
			engine->_gameState->_magicLevelIdx = CLIP<int16>(magicLevel, 0, 4);
		}
		int leafs = engine->_gameState->_inventoryNumLeafs;
		if (ImGui::InputInt("Leafs", &leafs)) {
			engine->_gameState->setLeafs(leafs);
		}
		int leafBoxes = engine->_gameState->_inventoryNumLeafsBox;
		if (ImGui::InputInt("Leaf boxes", &leafBoxes)) {
			engine->_gameState->setLeafBoxes(leafBoxes);
		}
		int gas = engine->_gameState->_inventoryNumGas;
		if (ImGui::InputInt("Gas", &gas)) {
			engine->_gameState->setGas(gas);
		}
		ImGui::EndMenu();
	}
}

static void gridMenu(TwinEEngine *engine) {
	if (ImGui::BeginMenu("Grid")) {
		ImGui::Text("World cube %i %i %i", engine->_grid->_worldCube.x, engine->_grid->_worldCube.y, engine->_grid->_worldCube.z);
#if 0
		Grid *grid = engine->_grid;

		if (ImGui::Button(ICON_MS_ADD)) {
			grid->_cellingGridIdx++;
			if (grid->_cellingGridIdx > 133) {
				grid->_cellingGridIdx = 133;
			}
		}
		if (ImGui::Button(ICON_MS_REMOVE)) {
			grid->_cellingGridIdx--;
			if (grid->_cellingGridIdx < 0) {
				grid->_cellingGridIdx = 0;
			}
		}
		// Enable/disable celling grid
		if (ImGui::Button("Apply ceiling grid")) {
			if (grid->_useCellingGrid == -1) {
				grid->_useCellingGrid = 1;
				// grid->createGridMap();
				grid->initCellingGrid(grid->_cellingGridIdx);
				debug("Enable Celling Grid index: %d", grid->_cellingGridIdx);
				engine->_scene->_needChangeScene = SCENE_CEILING_GRID_FADE_2; // tricky to make the fade
			} else if (grid->_useCellingGrid == 1) {
				grid->_useCellingGrid = -1;
				grid->copyMapToCube();
				engine->_redraw->_firstTime = true;
				debug("Disable Celling Grid index: %d", grid->_cellingGridIdx);
				engine->_scene->_needChangeScene = SCENE_CEILING_GRID_FADE_2; // tricky to make the fade
			}
		}
#endif
		ImGui::EndMenu();
	}
}

static void debuggerMenu(TwinEEngine *engine) {
	if (ImGui::BeginMenu("Debugger")) {
		if (ImGui::MenuItem("Texts")) {
			engine->_debugState->_openPopup = MENU_TEXT_TITLE;
		}
		if (ImGui::MenuItem("Holomap flags")) {
			engine->_debugState->_openPopup = HOLOMAP_FLAGS_TITLE;
		}
		if (ImGui::MenuItem("Game flags")) {
			engine->_debugState->_openPopup = GAME_FLAGS_TITLE;
		}

		ImGui::SeparatorText("Actions");

		if (ImGui::MenuItem("Center actor")) {
			ActorStruct *actor = engine->_scene->getActor(OWN_ACTOR_SCENE_INDEX);
			actor->_posObj = engine->_grid->_worldCube;
			actor->_posObj.y += 1000;
		}

		ImGui::SeparatorText("Scene");

		Scene *scene = engine->_scene;
		GameState *gameState = engine->_gameState;
		ImGui::Text("Scene: %i", scene->_currentSceneIdx);
		ImGui::Text("Scene name: %s", gameState->_sceneName);

		ImGui::SeparatorText("Options");

		ImGui::Checkbox("Free camera", &engine->_debugState->_useFreeCamera);
		ImGui::Checkbox("God mode", &engine->_debugState->_godMode);

		if (ImGui::Checkbox("Bounding boxes", &engine->_debugState->_showingActors)) {
			engine->_redraw->_firstTime = true;
		}
		if (ImGui::Checkbox("Clipping", &engine->_debugState->_showingClips)) {
			engine->_redraw->_firstTime = true;
		}
		if (ImGui::Checkbox("Zones", &engine->_debugState->_showingZones)) {
			engine->_redraw->_firstTime = true;
		}

		if (engine->_debugState->_showingZones) {
			if (ImGui::CollapsingHeader("Zones")) {
				static const struct ZonesDesc {
					const char *name;
					ZoneType type;
					const char *desc;
				} d[] = {
					{"Cube", ZoneType::kCube, "Change to another scene"},
					{"Camera", ZoneType::kCamera, "Binds camera view"},
					{"Sceneric", ZoneType::kSceneric, "For use in Life Script"},
					{"Grid", ZoneType::kGrid, "Set disappearing Grid fragment"},
					{"Object", ZoneType::kObject, "Give bonus"},
					{"Text", ZoneType::kText, "Displays text message"},
					{"Ladder", ZoneType::kLadder, "Hero can climb on it"},
					{"Escalator", ZoneType::kEscalator, nullptr},
					{"Hit", ZoneType::kHit, nullptr},
					{"Rail", ZoneType::kRail, nullptr}};

				for (int i = 0; i < ARRAYSIZE(d); ++i) {
					ImGui::CheckboxFlags(d[i].name, &engine->_debugState->_typeZones, (1u << (uint32)d[i].type));
					if (d[i].desc) {
						ImGui::SetTooltip(d[i].desc);
					}
				}
			}
		}

		if (ImGui::BeginCombo("Scene", gameState->_sceneName)) {
			for (int i = 0; i < engine->numHoloPos(); ++i) {
				Common::String name = Common::String::format("[%03d] %s", i, engine->_holomap->getLocationName(i));
				if (ImGui::Selectable(name.c_str(), i == engine->_scene->_currentSceneIdx)) {
					scene->_currentSceneIdx = i;
					scene->_needChangeScene = scene->_currentSceneIdx;
					engine->_redraw->_firstTime = true;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::EndMenu();
	}
}

void onImGuiRender() {
	if (!debugChannelSet(-1, kDebugImGui)) {
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoMouse;
		return;
	}
	static int currentActor = 0;

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoMouse);

	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
	TwinEEngine *engine = (TwinEEngine *)g_engine;

	if (ImGui::BeginMainMenuBar()) {
		debuggerMenu(engine);
		gameStateMenu(engine);
		gridMenu(engine);
		ImGui::EndMainMenuBar();
	}

	actorDetailsWindow(currentActor, engine);

	menuTextsPopup(engine);
	holomapFlagsPopup(engine);
	gameFlagsPopup(engine);

	if (engine->_debugState->_openPopup) {
		ImGui::OpenPopup(engine->_debugState->_openPopup);
		engine->_debugState->_openPopup = nullptr;
	}
}

void onImGuiCleanup() {
}

} // namespace TwinE
