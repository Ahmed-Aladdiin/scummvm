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

#include "twine/script/script_move_v1.h"
#include "common/memstream.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "twine/scene/animations.h"
#include "twine/audio/sound.h"
#include "twine/movies.h"
#include "twine/scene/movements.h"
#include "twine/renderer/redraw.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/scene/scene.h"
#include "twine/twine.h"

namespace TwinE {

/**
 * End of Actor Move Script
 * @note Opcode @c 0x00
 */
static int32 mEND(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::END()");
	ctx.actor->_offsetTrack = -1;
	return 1;
}

/**
 * No Operation
 * @note Opcode @c 0x01
 */
static int32 mNOP(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::NOP()");
	return 0;
}

/**
 * Choose new body for the current actor (Parameter = File3D Body Instance)
 * @note Opcode @c 0x02
 */
static int32 mBODY(TwinEEngine *engine, MoveScriptContext &ctx) {
	BodyType bodyIdx = (BodyType)ctx.stream.readByte();
	engine->_actor->initBody(bodyIdx, ctx.actorIdx);
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::BODY(%i)", (int)bodyIdx);
	return 0;
}

/**
 * Choose new animation for the current actor (Parameter = File3D Animation Instance)
 * @note Opcode @c 0x03
 */
static int32 mANIM(TwinEEngine *engine, MoveScriptContext &ctx) {
	AnimationTypes animIdx = (AnimationTypes)ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::ANIM(%i)", (int)animIdx);
	if (engine->_animations->initAnim(animIdx, AnimType::kAnimationTypeLoop, AnimationTypes::kStanding, ctx.actorIdx)) {
		return 0;
	}
	ctx.undo(1);
	return 1;
}

/**
 * Tell the actor to go to a new position (Parameter = Track Index)
 * @note Opcode @c 0x04
 */
static int32 mGOTO_POINT(TwinEEngine *engine, MoveScriptContext &ctx) {
	engine->_scene->_currentScriptValue = ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::GOTO_POINT(%i)", (int)engine->_scene->_currentScriptValue);

	const IVec3 &sp = engine->_scene->_sceneTracks[engine->_scene->_currentScriptValue];
	const int32 newAngle = engine->_movements->getAngleAndSetTargetActorDistance(ctx.actor->_pos.x, ctx.actor->_pos.z, sp.x, sp.z);

	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		ctx.actor->_beta = newAngle;
	} else {
		engine->_movements->initRealAngleConst(ctx.actor->_beta, newAngle, ctx.actor->_speed, &ctx.actor->_moveAngle);
	}

	if (engine->_movements->_targetActorDistance > 500) {
		ctx.undo(1);
		return 1;
	}

	return 0;
}

/**
 * Wait the end of the current animation
 * @note Opcode @c 0x05
 */
static int32 mWAIT_ANIM(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::WAIT_ANIM()");
	if (!ctx.actor->_dynamicFlags.bAnimEnded) {
		ctx.undo(0);
	} else {
		engine->_movements->clearRealAngle(ctx.actor);
	}
	return 1;
}

/**
 * Loop a certain label (Parameter = Label Number)
 * @note Opcode @c 0x06
 */
static int32 mLOOP(TwinEEngine *engine, MoveScriptContext &ctx) {
	ctx.actor->_offsetTrack = 0;
	ctx.stream.seek(0);
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::LOOP()");
	return 0;
}

/**
 * Make the actor turn around
 * @note Opcode @c 0x07
 */
static int32 mANGLE(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int16 angle = ToAngle(ctx.stream.readSint16LE());
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::ANGLE(%i)", (int)angle);
	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		return 0;
	}
	engine->_scene->_currentScriptValue = angle;
	if (ctx.actor->_moveAngle.numOfStep == 0) {
		engine->_movements->initRealAngleConst(ctx.actor->_beta, angle, ctx.actor->_speed, &ctx.actor->_moveAngle);
	}
	if (ctx.actor->_beta == angle) {
		engine->_movements->clearRealAngle(ctx.actor);
		return 0;
	}
	ctx.undo(2);
	return 1;
}

/**
 * Set new postion for the current actor (Parameter = Track Index)
 * @note Opcode @c 0x08
 */
static int32 mPOS_POINT(TwinEEngine *engine, MoveScriptContext &ctx) {
	engine->_scene->_currentScriptValue = ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::POS_POINT(%i)", (int)engine->_scene->_currentScriptValue);

	const IVec3 &sp = engine->_scene->_sceneTracks[engine->_scene->_currentScriptValue];
	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		ctx.actor->_speed = 0;
	}

	ctx.actor->_pos = sp;

	return 0;
}

/**
 * Specify a new label (Parameter = Label Number)
 * @note Opcode @c 0x09
 */
static int32 mLABEL(TwinEEngine *engine, MoveScriptContext &ctx) {
	ctx.actor->_labelIdx = ctx.stream.readByte();
	ctx.actor->_currentLabelPtr = ctx.stream.pos() - 2;
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::LABEL(%i)", (int)ctx.actor->_labelIdx);
	if (engine->_scene->_currentSceneIdx == LBA1SceneId::Proxima_Island_Museum && ctx.actor->_actorIdx == 2 &&
		(ctx.actor->_labelIdx == 0 || ctx.actor->_labelIdx == 1)) {
		engine->unlockAchievement("LBA_ACH_004");
	}
	return 0;
}

/**
 * Go to a certain label (Parameter = Label Number)
 * @note Opcode @c 0x0A
 */
static int32 mGOTO(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int16 pos = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::GOTO(%i)", (int)pos);
	if (pos == -1) {
		ctx.actor->_offsetTrack = -1;
		return 1;
	}
	ctx.stream.seek(pos);
	return 0;
}

/**
 * Tell the actor to stop the current animation
 * @note Opcode @c 0x0B
 */
static int32 mSTOP(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::STOP()");
	ctx.actor->_offsetTrack = -1;
	return 1;
}

/**
 * Tell the actor to go to a symbolic point
 * @note Opcode @c 0x0C
 */
static int32 mGOTO_SYM_POINT(TwinEEngine *engine, MoveScriptContext &ctx) {
	engine->_scene->_currentScriptValue = ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::GOTO_SYM_POINT(%i)", (int)engine->_scene->_currentScriptValue);

	const IVec3 &sp = engine->_scene->_sceneTracks[engine->_scene->_currentScriptValue];
	const int32 newAngle = LBAAngles::ANGLE_180 + engine->_movements->getAngleAndSetTargetActorDistance(ctx.actor->_pos, sp);

	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		ctx.actor->_beta = newAngle;
	} else {
		engine->_movements->initRealAngleConst(ctx.actor->_beta, newAngle, ctx.actor->_speed, &ctx.actor->_moveAngle);
	}

	if (engine->_movements->_targetActorDistance > 500) {
		ctx.undo(1);
		return 1;
	}

	return 0;
}

/**
 * Wait a certain number of frame update in the current animation
 * @note Opcode @c 0x0D
 */
static int32 mWAIT_NUM_ANIM(TwinEEngine *engine, MoveScriptContext &ctx) {
	bool abortMove = false;
	const int32 animRepeats = ctx.stream.readByte();
	int32 animPos = ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::WAIT_NUM_ANIM(%i, %i)", (int)animRepeats, animPos);
	if (ctx.actor->_dynamicFlags.bAnimEnded) {
		animPos++;

		if (animPos == animRepeats) {
			animPos = 0;
		} else {
			abortMove = true;
		}

		ctx.stream.rewind(1);
		ctx.stream.writeByte(animPos);
	} else {
		abortMove = true;
	}

	if (abortMove) {
		ctx.undo(2);
	}

	return abortMove ? 1 : 0;
}

/**
 * Play a sample (Parameter = Sample index)
 * @note Opcode @c 0x0E
 */
static int32 mSAMPLE(TwinEEngine *engine, MoveScriptContext &ctx) {
	int32 sampleIdx = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::SAMPLE(%i)", (int)sampleIdx);
	engine->_sound->playSample(sampleIdx, 1, ctx.actor->posObj(), ctx.actorIdx);
	return 0;
}

/**
 * Tell the actor to go to a new position (Parameter = Track Index)
 * @note Opcode @c 0x0F
 */
static int32 mGOTO_POINT_3D(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int32 trackId = ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::GOTO_POINT_3D(%i)", (int)trackId);
	if (!ctx.actor->_staticFlags.bIsSpriteActor) {
		return 0;
	}

	engine->_scene->_currentScriptValue = trackId;

	const IVec3 &sp = engine->_scene->_sceneTracks[engine->_scene->_currentScriptValue];
	ctx.actor->_beta = engine->_movements->getAngleAndSetTargetActorDistance(ctx.actor->_pos.x, ctx.actor->_pos.z, sp.x, sp.z);
	ctx.actor->_spriteActorRotation = engine->_movements->getAngleAndSetTargetActorDistance(ctx.actor->_pos.y, 0, sp.y, engine->_movements->_targetActorDistance);

	if (engine->_movements->_targetActorDistance > 100) {
		ctx.undo(1);
		return 1;
	}
	ctx.actor->_pos = sp;

	return 0;
}

/**
 * Specify a new rotation speed for the current actor (Parameter = Rotation speed) [ 0 means fast, 32767 means slow ]
 * @note Opcode @c 0x10
 */
static int32 mSPEED(TwinEEngine *engine, MoveScriptContext &ctx) {
	ctx.actor->_speed = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::SPEED(%i)", (int)ctx.actor->_speed);

	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		engine->_movements->setActorAngle(LBAAngles::ANGLE_0, ctx.actor->_speed, LBAAngles::ANGLE_17, &ctx.actor->_moveAngle);
	}

	return 0;
}

/**
 * Set actor as background (Parameter = 1 (true); = 0 (false))
 * @note Opcode @c 0x11
 */
static int32 mBACKGROUND(TwinEEngine *engine, MoveScriptContext &ctx) {
	const uint8 val = ctx.stream.readByte();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::BACKGROUND(%i)", (int)val);
	if (val != 0) {
		if (!ctx.actor->_staticFlags.bIsBackgrounded) {
			ctx.actor->_staticFlags.bIsBackgrounded = 1;
			if (ctx.actor->_dynamicFlags.bIsDrawn) {
				engine->_redraw->_firstTime = true;
			}
		}
	} else {
		if (ctx.actor->_staticFlags.bIsBackgrounded) {
			ctx.actor->_staticFlags.bIsBackgrounded = 0;
			if (ctx.actor->_dynamicFlags.bIsDrawn) {
				engine->_redraw->_firstTime = true;
			}
		}
	}

	return 0;
}

/**
 * Number os seconds to wait.
 * @note Opcode @c 0x12
 */
static int32 mWAIT_NUM_SECOND(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int32 numSeconds = ctx.stream.readByte();
	int32 currentTime = ctx.stream.readSint32LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::WAIT_NUM_SECOND(%i, %i)", (int)numSeconds, currentTime);

	if (currentTime == 0) {
		currentTime = engine->_lbaTime + TO_SECONDS(numSeconds);
		ctx.stream.rewind(4);
		ctx.stream.writeSint32LE(currentTime);
	}

	if (engine->_lbaTime < currentTime) {
		ctx.undo(5);
		return 1;
	}

	ctx.stream.rewind(4);
	ctx.stream.writeSint32LE(0);

	return 0;
}

/**
 * To not use Bodies.
 * @note Opcode @c 0x13
 */
static int32 mNO_BODY(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::NO_BODY()");
	engine->_actor->initBody(BodyType::btNone, ctx.actorIdx);
	return 0;
}

/**
 * Change actor orientation. (Parameter = New Angle)
 * @note Opcode @c 0x14
 */
static int32 mBETA(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int16 beta = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::BETA(%i)", (int)beta);

	ctx.actor->_beta = beta;

	if (!ctx.actor->_staticFlags.bIsSpriteActor) {
		engine->_movements->clearRealAngle(ctx.actor);
	}

	return 0;
}

static int32 mOPEN_GENERIC(TwinEEngine *engine, MoveScriptContext &ctx, int32 angle) {
	const int16 doorStatus = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::OPEN(%i, %i)", (int)doorStatus, angle);
	if (ctx.actor->_staticFlags.bIsSpriteActor && ctx.actor->_staticFlags.bUsesClipping) {
		ctx.actor->_beta = angle;
		ctx.actor->_doorWidth = doorStatus;
		ctx.actor->_dynamicFlags.bIsSpriteMoving = 1;
		ctx.actor->_speed = 1000;
		engine->_movements->setActorAngle(LBAAngles::ANGLE_0, LBAAngles::ANGLE_351, LBAAngles::ANGLE_17, &ctx.actor->_moveAngle);
	}
	if (engine->_scene->_currentSceneIdx == LBA1SceneId::Proxima_Island_Museum && ctx.actor->_actorIdx == 16) {
		engine->unlockAchievement("LBA_ACH_009");
	}
	return 0;
}

/**
 * Open the door (left way) (Parameter = distance to open).
 * @note Opcode @c 0x15
 */
static int32 mOPEN_LEFT(TwinEEngine *engine, MoveScriptContext &ctx) {
	return mOPEN_GENERIC(engine, ctx, LBAAngles::ANGLE_270);
}

/**
 * Open the door (right way) (Parameter = distance to open).
 * @note Opcode @c 0x16
 */
static int32 mOPEN_RIGHT(TwinEEngine *engine, MoveScriptContext &ctx) {
	return mOPEN_GENERIC(engine, ctx, LBAAngles::ANGLE_90);

}

/**
 * Open the door (up way) (Parameter = distance to open).
 * @note Opcode @c 0x17
 */
static int32 mOPEN_UP(TwinEEngine *engine, MoveScriptContext &ctx) {
	return mOPEN_GENERIC(engine, ctx, LBAAngles::ANGLE_180);

}

/**
 * Open the door (down way) (Parameter = distance to open).
 * @note Opcode @c 0x18
 */
static int32 mOPEN_DOWN(TwinEEngine *engine, MoveScriptContext &ctx) {
	return mOPEN_GENERIC(engine, ctx, LBAAngles::ANGLE_0);
}

/**
 * Close the door.
 * @note Opcode @c 0x19
 */
static int32 mCLOSE(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::CLOSE()");
	if (ctx.actor->_staticFlags.bIsSpriteActor && ctx.actor->_staticFlags.bUsesClipping) {
		ctx.actor->_doorWidth = 0;
		ctx.actor->_dynamicFlags.bIsSpriteMoving = 1;
		ctx.actor->_speed = -1000;
		engine->_movements->setActorAngle(LBAAngles::ANGLE_0, -LBAAngles::ANGLE_351, LBAAngles::ANGLE_17, &ctx.actor->_moveAngle);
	}
	return 0;
}

/**
 * Wait till door close.
 * @note Opcode @c 0x1A
 */
static int32 mWAIT_DOOR(TwinEEngine *engine, MoveScriptContext &ctx) {
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::WAIT_DOOR()");
	if (ctx.actor->_staticFlags.bIsSpriteActor && ctx.actor->_staticFlags.bUsesClipping) {
		if (ctx.actor->_speed) {
			ctx.undo(0);
			return 1;
		}
	}
	return 0;
}

/**
 * Generate a random sample.
 * @note Opcode @c 0x1B
 */
static int32 mSAMPLE_RND(TwinEEngine *engine, MoveScriptContext &ctx) {
	int32 sampleIdx = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::SAMPLE_RND(%i)", (int)sampleIdx);
	engine->_sound->playSample(sampleIdx, 1, ctx.actor->posObj(), ctx.actorIdx);
	return 0;
}

/**
 * Play always the sample (Parameter = Sample index)
 * @note Opcode @c 0x1C
 */
static int32 mSAMPLE_ALWAYS(TwinEEngine *engine, MoveScriptContext &ctx) {
	int32 sampleIdx = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::SAMPLE_ALWAYS(%i)", (int)sampleIdx);
	if (!engine->_sound->isSamplePlaying(sampleIdx)) { // if its not playing
		engine->_sound->playSample(sampleIdx, -1, ctx.actor->posObj(), ctx.actorIdx);
	}
	return 0;
}

/**
 * Stop playing the sample
 * @note Opcode @c 0x1D
 */
static int32 mSAMPLE_STOP(TwinEEngine *engine, MoveScriptContext &ctx) {
	int32 sampleIdx = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::SAMPLE_STOP(%i)", (int)sampleIdx);
	engine->_sound->stopSample(sampleIdx);
	return 0;
}

/**
 * Play FLA cutscenes (Parameter = Cutscene Name)
 * @note Opcode @c 0x1E
 */
static int32 mPLAY_FLA(TwinEEngine *engine, MoveScriptContext &ctx) {
	int strIdx = 0;
	char movie[64];
	do {
		const byte c = ctx.stream.readByte();
		movie[strIdx++] = c;
		if (c == '\0') {
			break;
		}
		if (strIdx >= ARRAYSIZE(movie)) {
			error("Max string size exceeded for fla name");
		}
	} while (true);

	debugC(3, kDebugLevels::kDebugScripts, "MOVE::PLAY_FLA(%s)", movie);
	engine->_movie->playMovie(movie);
	engine->setPalette(engine->_screens->_paletteRGBA);
	engine->_screens->clearScreen();
	return 0;
}

/**
 * Repeat sample (Parameter = Sample index).
 * @note Opcode @c 0x1F
 */
static int32 mREPEAT_SAMPLE(TwinEEngine *engine, MoveScriptContext &ctx) {
	ctx.numRepeatSample = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::REPEAT_SAMPLE(%i)", (int)ctx.numRepeatSample);
	return 0;
}

/**
 * Play a sample (Parameter = Sample index)
 * @note Opcode @c 0x20
 */
static int32 mSIMPLE_SAMPLE(TwinEEngine *engine, MoveScriptContext &ctx) {
	int32 sampleIdx = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::SIMPLE_SAMPLE(%i)", (int)sampleIdx);
	engine->_sound->playSample(sampleIdx, ctx.numRepeatSample, ctx.actor->posObj(), ctx.actorIdx);
	ctx.numRepeatSample = 1;
	return 0;
}

/**
 * The actor rotate to Twinsen direction (Parameter = -1 (near); = 0 (far))
 * @note Opcode @c 0x21
 */
static int32 mFACE_HERO(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int16 angle = ToAngle(ctx.stream.readSint16LE());
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::FACE_HERO(%i)", (int)angle);
	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		return 0;
	}
	engine->_scene->_currentScriptValue = angle;
	if (engine->_scene->_currentScriptValue == -1 && ctx.actor->_moveAngle.numOfStep == 0) {
		engine->_scene->_currentScriptValue = engine->_movements->getAngleAndSetTargetActorDistance(ctx.actor->posObj(), engine->_scene->_sceneHero->posObj());
		engine->_movements->initRealAngleConst(ctx.actor->_beta, engine->_scene->_currentScriptValue, ctx.actor->_speed, &ctx.actor->_moveAngle);
		ctx.stream.rewind(2);
		ctx.stream.writeSint16LE(engine->_scene->_currentScriptValue);
	}

	if (ctx.actor->_beta != engine->_scene->_currentScriptValue) {
		ctx.undo(2);
		return 1;
	}
	engine->_movements->clearRealAngle(ctx.actor);
	ctx.stream.rewind(2);
	ctx.stream.writeSint16LE(-1);
	return 0;
}

/**
 * Generate an random angle for the current actor
 * @note Opcode @c 0x22
 */
static int32 mANGLE_RND(TwinEEngine *engine, MoveScriptContext &ctx) {
	const int16 val1 = ctx.stream.readSint16LE();
	const int16 val2 = ctx.stream.readSint16LE();
	debugC(3, kDebugLevels::kDebugScripts, "MOVE::LBAAngles::ANGLE_RND(%i, %i)", (int)val1, (int)val2);
	if (ctx.actor->_staticFlags.bIsSpriteActor) {
		return 0;
	}

	engine->_scene->_currentScriptValue = val2;

	if (engine->_scene->_currentScriptValue == -1 && ctx.actor->_moveAngle.numOfStep == 0) {
		if (engine->getRandomNumber() & 1) {
			const int32 newAngle = ctx.actor->_beta + LBAAngles::ANGLE_90 + (ABS(val1) >> 1);
			engine->_scene->_currentScriptValue = ClampAngle(newAngle - engine->getRandomNumber(val1));
		} else {
			const int32 newAngle = ctx.actor->_beta - LBAAngles::ANGLE_90 + (ABS(val1) >> 1);
			engine->_scene->_currentScriptValue = ClampAngle(newAngle - engine->getRandomNumber(val1));
		}

		engine->_movements->initRealAngleConst(ctx.actor->_beta, engine->_scene->_currentScriptValue, ctx.actor->_speed, &ctx.actor->_moveAngle);
		ctx.stream.rewind(2);
		ctx.stream.writeSint16LE(engine->_scene->_currentScriptValue);
	}

	if (ctx.actor->_beta != engine->_scene->_currentScriptValue) {
		ctx.undo(4);
		return 1;
	}
	engine->_movements->clearRealAngle(ctx.actor);
	ctx.stream.rewind(2);
	ctx.stream.writeSint16LE(-1);
	return 0;
}

static const ScriptMoveFunction function_map[] = {
	{"END", mEND},
	{"NOP", mNOP},
	{"BODY", mBODY},
	{"ANIM", mANIM},
	{"GOTO_POINT", mGOTO_POINT},
	{"WAIT_ANIM", mWAIT_ANIM},
	{"LOOP", mLOOP},
	{"ANGLE", mANGLE},
	{"POS_POINT", mPOS_POINT},
	{"LABEL", mLABEL},
	{"GOTO", mGOTO},
	{"STOP", mSTOP},
	{"GOTO_SYM_POINT", mGOTO_SYM_POINT},
	{"WAIT_NUM_ANIM", mWAIT_NUM_ANIM},
	{"SAMPLE", mSAMPLE},
	{"GOTO_POINT_3D", mGOTO_POINT_3D},
	{"SPEED", mSPEED},
	{"BACKGROUND", mBACKGROUND},
	{"WAIT_NUM_SECOND", mWAIT_NUM_SECOND},
	{"NO_BODY", mNO_BODY},
	{"BETA", mBETA},
	{"OPEN_LEFT", mOPEN_LEFT},
	{"OPEN_RIGHT", mOPEN_RIGHT},
	{"OPEN_UP", mOPEN_UP},
	{"OPEN_DOWN", mOPEN_DOWN},
	{"CLOSE", mCLOSE},
	{"WAIT_DOOR", mWAIT_DOOR},
	{"SAMPLE_RND", mSAMPLE_RND},
	{"SAMPLE_ALWAYS", mSAMPLE_ALWAYS},
	{"SAMPLE_STOP", mSAMPLE_STOP},
	{"PLAY_FLA", mPLAY_FLA},
	{"REPEAT_SAMPLE", mREPEAT_SAMPLE},
	{"SIMPLE_SAMPLE", mSIMPLE_SAMPLE},
	{"FACE_HERO", mFACE_HERO},
	{"LBAAngles::ANGLE_RND", mANGLE_RND}
#if 0 // lba2
	,
	{"REM", mREM},
	{"WAIT_NB_DIZIEME", mWAIT_NB_DIZIEME},
	{"DO", mDO},
	{"SPRITE", mSPRITE},
	{"WAIT_NB_SECOND_RND", mWAIT_NB_SECOND_RND},
	{"AFF_TIMER", mAFF_TIMER},
	{"SET_FRAME", mSET_FRAME},
	{"SET_FRAME_3DS", mSET_FRAME_3DS},
	{"SET_START_3DS", mSET_START_3DS},
	{"SET_END_3DS", mSET_END_3DS},
	{"START_ANIM_3DS", mSTART_ANIM_3DS},
	{"STOP_ANIM_3DS", mSTOP_ANIM_3DS},
	{"WAIT_ANIM_3DS", mWAIT_ANIM_3DS},
	{"WAIT_FRAME_3DS", mWAIT_FRAME_3DS},
	{"WAIT_NB_DIZIEME_RND", mWAIT_NB_DIZIEME_RND},
	{"DECALAGE", mDECALAGE},
	{"FREQUENCE", mFREQUENCE},
	{"VOLUME", mVOLUME}
#endif
	};

ScriptMoveV1::ScriptMoveV1(TwinEEngine *engine) : ScriptMove(engine, function_map, ARRAYSIZE(function_map)) {
}

} // namespace TwinE
