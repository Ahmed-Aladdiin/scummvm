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


#include "backends/graphics/opengl/opengl-graphics.h"
#include "backends/graphics/opengl/texture.h"
#include "backends/graphics/opengl/pipelines/pipeline.h"
#include "backends/graphics/opengl/pipelines/fixed.h"
#include "backends/graphics/opengl/pipelines/shader.h"
#include "backends/graphics/opengl/pipelines/libretro.h"
#include "backends/graphics/opengl/shader.h"
#include "graphics/opengl/debug.h"

#include "common/array.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/algorithm.h"
#include "common/file.h"
#include "gui/debugger.h"
#include "engines/engine.h"
#ifdef USE_OSD
#include "common/tokenizer.h"
#include "common/rect.h"
#if defined(MACOSX)
#include "backends/platform/sdl/macosx/macosx-touchbar.h"
#endif
#endif

#include "graphics/conversion.h"
#ifdef USE_OSD
#include "graphics/fontman.h"
#include "graphics/font.h"
#endif
#ifdef USE_SCALERS
#include "graphics/scalerplugin.h"
#endif

#ifdef USE_PNG
#include "image/png.h"
#else
#include "image/bmp.h"
#endif

#include "common/text-to-speech.h"

#if !USE_FORCED_GLES
#include "backends/graphics/opengl/pipelines/libretro/parser.h"
#endif

namespace OpenGL {

OpenGLGraphicsManager::OpenGLGraphicsManager()
	: _currentState(), _oldState(), _transactionMode(kTransactionNone), _screenChangeID(1 << (sizeof(int) * 8 - 2)),
	  _pipeline(nullptr), _stretchMode(STRETCH_FIT),
	  _defaultFormat(), _defaultFormatAlpha(),
	  _gameScreen(nullptr), _overlay(nullptr),
	  _cursor(nullptr),
	  _cursorHotspotX(0), _cursorHotspotY(0),
	  _cursorHotspotXScaled(0), _cursorHotspotYScaled(0), _cursorWidthScaled(0), _cursorHeightScaled(0),
	  _cursorKeyColor(0), _cursorDontScale(false), _cursorPaletteEnabled(false), _shakeOffsetScaled()
#if !USE_FORCED_GLES
	  , _libretroPipeline(nullptr), _gameScreenTarget(nullptr)
#endif
#ifdef USE_OSD
	  , _osdMessageChangeRequest(false), _osdMessageAlpha(0), _osdMessageFadeStartTime(0), _osdMessageSurface(nullptr),
	  _osdIconSurface(nullptr)
#endif
#ifdef USE_SCALERS
	  , _scalerPlugins(ScalerMan.getPlugins())
#endif
	{
	memset(_gamePalette, 0, sizeof(_gamePalette));
	OpenGLContext.reset();
}

OpenGLGraphicsManager::~OpenGLGraphicsManager() {
	delete _gameScreen;
	delete _overlay;
	delete _cursor;
#ifdef USE_OSD
	delete _osdMessageSurface;
	delete _osdIconSurface;
#endif
#if !USE_FORCED_GLES
	delete _gameScreenTarget;
	ShaderManager::destroy();
#endif
}

bool OpenGLGraphicsManager::hasFeature(OSystem::Feature f) const {
	switch (f) {
	case OSystem::kFeatureAspectRatioCorrection:
	case OSystem::kFeatureCursorPalette:
	case OSystem::kFeatureFilteringMode:
	case OSystem::kFeatureStretchMode:
#ifdef USE_SCALERS
	case OSystem::kFeatureScalers:
#endif
		return true;

#if !USE_FORCED_GLES
	case OSystem::kFeatureShaders:
		return LibRetroPipeline::isSupportedByContext();
#endif

	case OSystem::kFeatureOverlaySupportsAlpha:
		return _defaultFormatAlpha.aBits() > 3;

	default:
		return false;
	}
}

void OpenGLGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	switch (f) {
	case OSystem::kFeatureAspectRatioCorrection:
		assert(_transactionMode != kTransactionNone);
		_currentState.aspectRatioCorrection = enable;
		break;

	case OSystem::kFeatureFilteringMode:
		assert(_transactionMode != kTransactionNone);
		_currentState.filtering = enable;
		break;

	case OSystem::kFeatureCursorPalette:
		_cursorPaletteEnabled = enable;
		updateCursorPalette();
		break;

	default:
		break;
	}
}

bool OpenGLGraphicsManager::getFeatureState(OSystem::Feature f) const {
	switch (f) {
	case OSystem::kFeatureAspectRatioCorrection:
		return _currentState.aspectRatioCorrection;

	case OSystem::kFeatureFilteringMode:
		return _currentState.filtering;

	case OSystem::kFeatureCursorPalette:
		return _cursorPaletteEnabled;

	default:
		return false;
	}
}

namespace {

const OSystem::GraphicsMode glGraphicsModes[] = {
	{ "opengl",  _s("OpenGL"),                GFX_OPENGL  },
	{ nullptr, nullptr, 0 }
};

} // End of anonymous namespace

const OSystem::GraphicsMode *OpenGLGraphicsManager::getSupportedGraphicsModes() const {
	return glGraphicsModes;
}

int OpenGLGraphicsManager::getDefaultGraphicsMode() const {
	return GFX_OPENGL;
}

bool OpenGLGraphicsManager::setGraphicsMode(int mode, uint flags) {
	assert(_transactionMode != kTransactionNone);

	switch (mode) {
	case GFX_OPENGL:
		_currentState.graphicsMode = mode;
		return true;

	default:
		warning("OpenGLGraphicsManager::setGraphicsMode(%d): Unknown graphics mode", mode);
		return false;
	}
}

int OpenGLGraphicsManager::getGraphicsMode() const {
	return _currentState.graphicsMode;
}

#ifdef USE_RGB_COLOR
Graphics::PixelFormat OpenGLGraphicsManager::getScreenFormat() const {
	return _currentState.gameFormat;
}

Common::List<Graphics::PixelFormat> OpenGLGraphicsManager::getSupportedFormats() const {
	Common::List<Graphics::PixelFormat> formats;

	// Our default mode is (memory layout wise) RGBA8888 which is a different
	// logical layout depending on the endianness. We chose this mode because
	// it is the only 32bit color mode we can safely assume to be present in
	// OpenGL and OpenGL ES implementations. Thus, we need to supply different
	// logical formats based on endianness.
#ifdef SCUMM_LITTLE_ENDIAN
	// ABGR8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#else
	// RGBA8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#endif
	// RGB565
	formats.push_back(Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
	// RGBA5551
	formats.push_back(Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0));
	// RGBA4444
	formats.push_back(Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0));

	// These formats are not natively supported by OpenGL ES implementations,
	// we convert the pixel format internally.
#ifdef SCUMM_LITTLE_ENDIAN
	// RGBA8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#else
	// ABGR8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#endif
	// RGB555, this is used by SCUMM HE 16 bit games.
	formats.push_back(Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0));

	formats.push_back(Graphics::PixelFormat::createFormatCLUT8());

	return formats;
}
#endif

namespace {
const OSystem::GraphicsMode glStretchModes[] = {
	{"center", _s("Center"), STRETCH_CENTER},
	{"pixel-perfect", _s("Pixel-perfect scaling"), STRETCH_INTEGRAL},
	{"even-pixels", _s("Even pixels scaling"), STRETCH_INTEGRAL_AR},
	{"fit", _s("Fit to window"), STRETCH_FIT},
	{"stretch", _s("Stretch to window"), STRETCH_STRETCH},
	{"fit_force_aspect", _s("Fit to window (4:3)"), STRETCH_FIT_FORCE_ASPECT},
	{nullptr, nullptr, 0}
};

} // End of anonymous namespace

const OSystem::GraphicsMode *OpenGLGraphicsManager::getSupportedStretchModes() const {
	return glStretchModes;
}

int OpenGLGraphicsManager::getDefaultStretchMode() const {
	return STRETCH_FIT;
}

bool OpenGLGraphicsManager::setStretchMode(int mode) {
	assert(getTransactionMode() != kTransactionNone);

	if (mode == _stretchMode)
		return true;

	// Check this is a valid mode
	const OSystem::GraphicsMode *sm = getSupportedStretchModes();
	bool found = false;
	while (sm->name) {
		if (sm->id == mode) {
			found = true;
			break;
		}
		sm++;
	}
	if (!found) {
		warning("unknown stretch mode %d", mode);
		return false;
	}

	_stretchMode = mode;
	return true;
}

int OpenGLGraphicsManager::getStretchMode() const {
	return _stretchMode;
}

#ifdef USE_SCALERS
uint OpenGLGraphicsManager::getDefaultScaler() const {
	return ScalerMan.findScalerPluginIndex("normal");
}

uint OpenGLGraphicsManager::getDefaultScaleFactor() const {
	return 1;
}

bool OpenGLGraphicsManager::setScaler(uint mode, int factor) {
	assert(_transactionMode != kTransactionNone);

	int newFactor;
	if (factor == -1)
		newFactor = getDefaultScaleFactor();
	else if (_scalerPlugins[mode]->get<ScalerPluginObject>().hasFactor(factor))
		newFactor = factor;
	else if (_scalerPlugins[mode]->get<ScalerPluginObject>().hasFactor(_oldState.scaleFactor))
		newFactor = _oldState.scaleFactor;
	else
		newFactor = _scalerPlugins[mode]->get<ScalerPluginObject>().getDefaultFactor();

	_currentState.scalerIndex = mode;
	_currentState.scaleFactor = newFactor;

	return true;
}

uint OpenGLGraphicsManager::getScaler() const {
	return _currentState.scalerIndex;
}

uint OpenGLGraphicsManager::getScaleFactor() const {
	return _currentState.scaleFactor;
}
#endif

#if !USE_FORCED_GLES
bool OpenGLGraphicsManager::setShader(const Common::String &fileName) {
	assert(_transactionMode != kTransactionNone);

	// Special case for the 'default' shader
	if (fileName == "default")
		_currentState.shader = "";
	else
		_currentState.shader = fileName;

	return true;
}
#endif

bool OpenGLGraphicsManager::loadShader(const Common::String &fileName) {
#if !USE_FORCED_GLES
	// Load selected shader preset
	if (!fileName.empty()) {
		if (!_libretroPipeline->open(Common::FSNode(fileName))) {
			warning("Failed to load shader %s", fileName.c_str());
			return false;
		}
	}
#endif

	return true;
}

void OpenGLGraphicsManager::beginGFXTransaction() {
	assert(_transactionMode == kTransactionNone);

	// Start a transaction.
	_oldState = _currentState;
	_transactionMode = kTransactionActive;
}

OSystem::TransactionError OpenGLGraphicsManager::endGFXTransaction() {
	assert(_transactionMode == kTransactionActive);

	uint transactionError = OSystem::kTransactionSuccess;

	bool setupNewGameScreen = false;
	if (   _oldState.gameWidth  != _currentState.gameWidth
	    || _oldState.gameHeight != _currentState.gameHeight) {
		setupNewGameScreen = true;
	}

#ifdef USE_RGB_COLOR
	if (_oldState.gameFormat != _currentState.gameFormat) {
		setupNewGameScreen = true;
	}

	// Check whether the requested format can actually be used.
	Common::List<Graphics::PixelFormat> supportedFormats = getSupportedFormats();
	// In case the requested format is not usable we will fall back to CLUT8.
	if (Common::find(supportedFormats.begin(), supportedFormats.end(), _currentState.gameFormat) == supportedFormats.end()) {
		_currentState.gameFormat = Graphics::PixelFormat::createFormatCLUT8();
		transactionError |= OSystem::kTransactionFormatNotSupported;
	}
#endif

#ifdef USE_SCALERS
	if (_oldState.scaleFactor != _currentState.scaleFactor ||
	    _oldState.scalerIndex != _currentState.scalerIndex) {
		setupNewGameScreen = true;
	}
#endif

	do {
		const uint desiredAspect = getDesiredGameAspectRatio();
		const uint requestedWidth  = _currentState.gameWidth;
		const uint requestedHeight = intToFrac(requestedWidth) / desiredAspect;

		// Consider that shader is OK by default
		// If loadVideoMode fails, we won't consider that shader was the error
		bool shaderOK = true;

		if (!loadVideoMode(requestedWidth, requestedHeight,
#ifdef USE_RGB_COLOR
		                   _currentState.gameFormat
#else
		                   Graphics::PixelFormat::createFormatCLUT8()
#endif
		                  )
			|| !(shaderOK = loadShader(_currentState.shader))
		   // HACK: This is really nasty but we don't have any guarantees of
		   // a context existing before, which means we don't know the maximum
		   // supported texture size before this. Thus, we check whether the
		   // requested game resolution is supported over here.
		   || (   _currentState.gameWidth  > (uint)OpenGLContext.maxTextureSize
		       || _currentState.gameHeight > (uint)OpenGLContext.maxTextureSize)) {
			if (_transactionMode == kTransactionActive) {
				// Try to setup the old state in case its valid and is
				// actually different from the new one.
				if (_oldState.valid && _oldState != _currentState) {
					// Give some hints on what failed to set up.
					if (   _oldState.gameWidth  != _currentState.gameWidth
					    || _oldState.gameHeight != _currentState.gameHeight) {
						transactionError |= OSystem::kTransactionSizeChangeFailed;
					}

#ifdef USE_RGB_COLOR
					if (_oldState.gameFormat != _currentState.gameFormat) {
						transactionError |= OSystem::kTransactionFormatNotSupported;
					}
#endif

					if (_oldState.aspectRatioCorrection != _currentState.aspectRatioCorrection) {
						transactionError |= OSystem::kTransactionAspectRatioFailed;
					}

					if (_oldState.graphicsMode != _currentState.graphicsMode) {
						transactionError |= OSystem::kTransactionModeSwitchFailed;
					}

					if (_oldState.filtering != _currentState.filtering) {
						transactionError |= OSystem::kTransactionFilteringFailed;
					}
#ifdef USE_SCALERS
					if (_oldState.scalerIndex != _currentState.scalerIndex) {
						transactionError |= OSystem::kTransactionModeSwitchFailed;
					}
#endif

#if !USE_FORCED_GLES
					if (_oldState.shader != _currentState.shader) {
						transactionError |= OSystem::kTransactionShaderChangeFailed;
					}
#endif
					// Roll back to the old state.
					_currentState = _oldState;
					_transactionMode = kTransactionRollback;

					// Try to set up the old state.
					continue;
				}
				// If the shader failed and we had not a valid old state, try to unset the shader and do it again
				if (!shaderOK && !_currentState.shader.empty()) {
					_currentState.shader = "";
					_transactionMode = kTransactionRollback;
					continue;
				}
			}

			// DON'T use error(), as this tries to bring up the debug
			// console, which WON'T WORK now that we might no have a
			// proper screen.
			warning("OpenGLGraphicsManager::endGFXTransaction: Could not load any graphics mode!");
			g_system->quit();
		}

		// In case we reach this we have a valid state, yay.
		_transactionMode = kTransactionNone;
		_currentState.valid = true;
	} while (_transactionMode == kTransactionRollback);

	if (setupNewGameScreen) {
		delete _gameScreen;
		_gameScreen = nullptr;

#if !USE_FORCED_GLES
		if (_gameScreenTarget != nullptr) {
			_gameScreenTarget->destroy();
			delete _gameScreenTarget;
			_gameScreenTarget = nullptr;
		}
#endif

		bool wantScaler = _currentState.scaleFactor > 1;

#ifdef USE_RGB_COLOR
		_gameScreen = createSurface(_currentState.gameFormat, false, wantScaler);
#else
		_gameScreen = createSurface(Graphics::PixelFormat::createFormatCLUT8(), false, wantScaler);
#endif
		assert(_gameScreen);
		if (_gameScreen->hasPalette()) {
			_gameScreen->setPalette(0, 256, _gamePalette);
		}

#ifdef USE_SCALERS
		if (wantScaler) {
			_gameScreen->setScaler(_currentState.scalerIndex, _currentState.scaleFactor);
		}
#endif

		_gameScreen->allocate(_currentState.gameWidth, _currentState.gameHeight);
		// We fill the screen to all black or index 0 for CLUT8.
#ifdef USE_RGB_COLOR
		if (_currentState.gameFormat.bytesPerPixel == 1) {
			_gameScreen->fill(0);
		} else {
			_gameScreen->fill(_gameScreen->getSurface()->format.RGBToColor(0, 0, 0));
		}
#else
		_gameScreen->fill(0);
#endif

#if !USE_FORCED_GLES
		if (_libretroPipeline) {
			_gameScreenTarget = new TextureTarget();
			_gameScreenTarget->create();
			_gameScreenTarget->setSize(_currentState.gameWidth, _currentState.gameHeight);
		}
#endif
	}

	// Update our display area and cursor scaling. This makes sure we pick up
	// aspect ratio correction and game screen changes correctly.
	recalculateDisplayAreas();
	recalculateCursorScaling();
	updateLinearFiltering();

	// Something changed, so update the screen change ID.
	++_screenChangeID;

	// Since transactionError is a ORd list of TransactionErrors this is
	// clearly wrong. But our API is simply broken.
	return (OSystem::TransactionError)transactionError;
}

int OpenGLGraphicsManager::getScreenChangeID() const {
	return _screenChangeID;
}

void OpenGLGraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	Graphics::PixelFormat requestedFormat;
#ifdef USE_RGB_COLOR
	if (!format) {
		requestedFormat = Graphics::PixelFormat::createFormatCLUT8();
	} else {
		requestedFormat = *format;
	}
	_currentState.gameFormat = requestedFormat;
#endif

	_currentState.gameWidth = width;
	_currentState.gameHeight = height;
	_gameScreenShakeXOffset = 0;
	_gameScreenShakeYOffset = 0;
}

int16 OpenGLGraphicsManager::getWidth() const {
	return _currentState.gameWidth;
}

int16 OpenGLGraphicsManager::getHeight() const {
	return _currentState.gameHeight;
}

void OpenGLGraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	_gameScreen->copyRectToTexture(x, y, w, h, buf, pitch);
}

void OpenGLGraphicsManager::fillScreen(uint32 col) {
	_gameScreen->fill(col);
}

void OpenGLGraphicsManager::updateScreen() {
	if (!_gameScreen) {
		return;
	}

#ifdef USE_OSD
	if (_osdMessageChangeRequest) {
		osdMessageUpdateSurface();
	}

	if (_osdIconSurface) {
		_osdIconSurface->updateGLTexture();
	}
#endif

	// If there's an active debugger, update it
	GUI::Debugger *debugger = g_engine ? g_engine->getDebugger() : nullptr;
	if (debugger)
		debugger->onFrame();

	// We only update the screen when there actually have been any changes.
	if (   !_forceRedraw
		&& !_cursorNeedsRedraw
	    && !_gameScreen->isDirty()
	    && !(_overlayVisible && _overlay->isDirty())
	    && !(_cursorVisible && _cursor && _cursor->isDirty())
#ifdef USE_OSD
	    && !_osdMessageSurface && !_osdIconSurface
#endif
	    ) {
		return;
	}

	// Update changes to textures.
	_gameScreen->updateGLTexture();
	if (_cursorVisible && _cursor) {
		_cursor->updateGLTexture();
	}
	_overlay->updateGLTexture();

	// Clear the screen buffer.
	GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

	if (!_overlayVisible) {
		// The scissor test is enabled to:
		// - Clip the cursor to the game screen
		// - Clip the game screen when the shake offset is non-zero
		_backBuffer.enableScissorTest(true);
	}

	// Alpha blending is disabled when drawing the screen
	_backBuffer.enableBlend(Framebuffer::kBlendModeDisabled);

	bool needsCursor = _cursorVisible && _cursor;

	// First step: Draw the (virtual) game screen.
#if !USE_FORCED_GLES
	if (_libretroPipeline && _libretroPipeline->isInitialized()) {
		Framebuffer *lastFramebuffer = Pipeline::getActivePipeline()->setFramebuffer(_gameScreenTarget);
		_gameScreenTarget->enableBlend(Framebuffer::kBlendModeDisabled);
		Pipeline::getActivePipeline()->drawTexture(_gameScreen->getGLTexture(), 0, 0, _gameScreen->getWidth(), _gameScreen->getHeight());

		// Draw the cursor if necessary.
		if (needsCursor && !_overlayVisible) {
			int gameScreenCursorX = (_cursorX - _gameDrawRect.left) * _gameScreen->getWidth() / _gameDrawRect.width() - _cursorHotspotX;
			int gameScreenCursorY = (_cursorY - _gameDrawRect.top) * _gameScreen->getHeight() / _gameDrawRect.height() - _cursorHotspotY;
			_gameScreenTarget->enableBlend(Framebuffer::kBlendModePremultipliedTransparency);
			Pipeline::getActivePipeline()->drawTexture(_cursor->getGLTexture(), gameScreenCursorX, gameScreenCursorY, _cursor->getWidth(), _cursor->getHeight());
			needsCursor = false;
		}
		Pipeline::getActivePipeline()->setFramebuffer(lastFramebuffer);

		Pipeline *lastPipeline = Pipeline::setPipeline(_libretroPipeline);
		Pipeline::getActivePipeline()->drawTexture(*_gameScreenTarget->getTexture(), _gameDrawRect.left, _gameDrawRect.top, _gameDrawRect.width(), _gameDrawRect.height());
		Pipeline::setPipeline(lastPipeline);
	} else
#endif
	{
		Pipeline::getActivePipeline()->drawTexture(_gameScreen->getGLTexture(), _gameDrawRect.left, _gameDrawRect.top, _gameDrawRect.width(), _gameDrawRect.height());
	}

	// Second step: Draw the overlay if visible.
	if (_overlayVisible) {
		int dstX = (_windowWidth - _overlayDrawRect.width()) / 2;
		int dstY = (_windowHeight - _overlayDrawRect.height()) / 2;
		_backBuffer.enableBlend(Framebuffer::kBlendModeTraditionalTransparency);
		Pipeline::getActivePipeline()->drawTexture(_overlay->getGLTexture(), dstX, dstY, _overlayDrawRect.width(), _overlayDrawRect.height());
	}

	// Third step: Draw the cursor if necessary.
	if (needsCursor) {
		_backBuffer.enableBlend(Framebuffer::kBlendModePremultipliedTransparency);

		Pipeline::getActivePipeline()->drawTexture(_cursor->getGLTexture(),
		                         _cursorX - _cursorHotspotXScaled + _shakeOffsetScaled.x,
		                         _cursorY - _cursorHotspotYScaled + _shakeOffsetScaled.y,
		                         _cursorWidthScaled, _cursorHeightScaled);
	}

	if (!_overlayVisible) {
		_backBuffer.enableScissorTest(false);
	}

#ifdef USE_OSD
	// Fourth step: Draw the OSD.
	if (_osdMessageSurface || _osdIconSurface) {
		_backBuffer.enableBlend(Framebuffer::kBlendModeTraditionalTransparency);
	}

	if (_osdMessageSurface) {
		// Update alpha value.
		const int diff = g_system->getMillis(false) - _osdMessageFadeStartTime;
		if (diff > 0) {
			if (diff >= kOSDMessageFadeOutDuration) {
				// Back to full transparency.
				_osdMessageAlpha = 0;
			} else {
				// Do a fade out.
				_osdMessageAlpha = kOSDMessageInitialAlpha - diff * kOSDMessageInitialAlpha / kOSDMessageFadeOutDuration;
			}
		}

		// Set the OSD transparency.
		Pipeline::getActivePipeline()->setColor(1.0f, 1.0f, 1.0f, _osdMessageAlpha / 100.0f);

		int dstX = (_windowWidth - _osdMessageSurface->getWidth()) / 2;
		int dstY = (_windowHeight - _osdMessageSurface->getHeight()) / 2;

		// Draw the OSD texture.
		Pipeline::getActivePipeline()->drawTexture(_osdMessageSurface->getGLTexture(),
		                                           dstX, dstY, _osdMessageSurface->getWidth(), _osdMessageSurface->getHeight());

		// Reset color.
		Pipeline::getActivePipeline()->setColor(1.0f, 1.0f, 1.0f, 1.0f);

		if (_osdMessageAlpha <= 0) {
			delete _osdMessageSurface;
			_osdMessageSurface = nullptr;

#if defined(MACOSX)
			macOSTouchbarUpdate(nullptr);
#endif
		}
	}

	if (_osdIconSurface) {
		int dstX = _windowWidth - _osdIconSurface->getWidth() - kOSDIconRightMargin;
		int dstY = kOSDIconTopMargin;

		// Draw the OSD icon texture.
		Pipeline::getActivePipeline()->drawTexture(_osdIconSurface->getGLTexture(),
		                                           dstX, dstY, _osdIconSurface->getWidth(), _osdIconSurface->getHeight());
	}
#endif

	_cursorNeedsRedraw = false;
	_forceRedraw = false;
	refreshScreen();
}

Graphics::Surface *OpenGLGraphicsManager::lockScreen() {
	return _gameScreen->getSurface();
}

void OpenGLGraphicsManager::unlockScreen() {
	_gameScreen->flagDirty();
}

void OpenGLGraphicsManager::setFocusRectangle(const Common::Rect& rect) {
}

void OpenGLGraphicsManager::clearFocusRectangle() {
}

int16 OpenGLGraphicsManager::getOverlayWidth() const {
	if (_overlay) {
		return _overlay->getWidth();
	} else {
		return 0;
	}
}

int16 OpenGLGraphicsManager::getOverlayHeight() const {
	if (_overlay) {
		return _overlay->getHeight();
	} else {
		return 0;
	}
}

Graphics::PixelFormat OpenGLGraphicsManager::getOverlayFormat() const {
	return _overlay->getFormat();
}

void OpenGLGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	_overlay->copyRectToTexture(x, y, w, h, buf, pitch);
}

void OpenGLGraphicsManager::clearOverlay() {
	_overlay->fill(0);
}

void OpenGLGraphicsManager::grabOverlay(Graphics::Surface &surface) const {
	const Graphics::Surface *overlayData = _overlay->getSurface();

	assert(surface.w >= overlayData->w);
	assert(surface.h >= overlayData->h);
	assert(surface.format.bytesPerPixel == overlayData->format.bytesPerPixel);

	const byte *src = (const byte *)overlayData->getPixels();
	byte *dst = (byte *)surface.getPixels();
	Graphics::copyBlit(dst, src, surface.pitch, overlayData->pitch, overlayData->w, overlayData->h, overlayData->format.bytesPerPixel);
}

namespace {
template<typename SrcColor, typename DstColor>
void multiplyColorWithAlpha(const byte *src, byte *dst, const uint w, const uint h,
							const Graphics::PixelFormat &srcFmt, const Graphics::PixelFormat &dstFmt,
							const uint srcPitch, const uint dstPitch, const SrcColor keyColor) {
	for (uint y = 0; y < h; ++y) {
		for (uint x = 0; x < w; ++x) {
			const uint32 color = *(const SrcColor *)src;

			if (color == keyColor) {
				*(DstColor *)dst = 0;
			} else {
				byte a, r, g, b;
				srcFmt.colorToARGB(color, a, r, g, b);

				if (a != 0xFF) {
					r = (int) r * a / 255;
					g = (int) g * a / 255;
					b = (int) b * a / 255;
				}

				*(DstColor *)dst = dstFmt.ARGBToColor(a, r, g, b);
			}

			src += sizeof(SrcColor);
			dst += sizeof(DstColor);
		}

		src += srcPitch - w * srcFmt.bytesPerPixel;
		dst += dstPitch - w * dstFmt.bytesPerPixel;
	}
}
} // End of anonymous namespace

void OpenGLGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format) {

	_cursorKeyColor = keycolor;
	_cursorHotspotX = hotspotX;
	_cursorHotspotY = hotspotY;
	_cursorDontScale = dontScale;

	if (!w || !h) {
		delete _cursor;
		_cursor = nullptr;
		return;
	}

	Graphics::PixelFormat inputFormat;
#ifdef USE_RGB_COLOR
	if (format) {
		inputFormat = *format;
	} else {
		inputFormat = Graphics::PixelFormat::createFormatCLUT8();
	}
#else
	inputFormat = Graphics::PixelFormat::createFormatCLUT8();
#endif

	// In case the color format has changed we will need to create the texture.
	if (!_cursor || _cursor->getFormat() != inputFormat) {
		delete _cursor;
		_cursor = nullptr;

#ifdef USE_SCALERS
		bool wantScaler = (_currentState.scaleFactor > 1) && !dontScale
		                && _scalerPlugins[_currentState.scalerIndex]->get<ScalerPluginObject>().canDrawCursor();
#else
		bool wantScaler = false;
#endif

		GLenum glIntFormat, glFormat, glType;

		Graphics::PixelFormat textureFormat;
		if (inputFormat.bytesPerPixel == 1 || (inputFormat.aBits() && getGLPixelFormat(inputFormat, glIntFormat, glFormat, glType))) {
			// There is two cases when we can use the cursor format directly.
			// The first is when it's CLUT8, here color key handling can
			// always be applied because we use the alpha channel of
			// _defaultFormatAlpha for that.
			// The other is when the input format has alpha bits and
			// furthermore is directly supported.
			textureFormat = inputFormat;
		} else {
			textureFormat = _defaultFormatAlpha;
		}
		_cursor = createSurface(textureFormat, true, wantScaler);
		assert(_cursor);
		updateLinearFiltering();
#ifdef USE_SCALERS
		if (wantScaler) {
			_cursor->setScaler(_currentState.scalerIndex, _currentState.scaleFactor);
		}
#endif
	}

	_cursor->allocate(w, h);
	if (inputFormat.bytesPerPixel == 1) {
		// For CLUT8 cursors we can simply copy the input data into the
		// texture.
		_cursor->copyRectToTexture(0, 0, w, h, buf, w * inputFormat.bytesPerPixel);
	} else {
		// Otherwise it is a bit more ugly because we have to handle a key
		// color properly.

		Graphics::Surface *dst = _cursor->getSurface();
		const uint srcPitch = w * inputFormat.bytesPerPixel;

		// Copy the cursor data to the actual texture surface. This will make
		// sure that the data is also converted to the expected format.

		// Also multiply the color values with the alpha channel.
		// The pre-multiplication allows using a blend mode that prevents
		// color fringes due to filtering.

		if (dst->format.bytesPerPixel == 2) {
			if (inputFormat.bytesPerPixel == 2) {
				multiplyColorWithAlpha<uint16, uint16>((const byte *) buf, (byte *) dst->getPixels(), w, h,
				                                       inputFormat, dst->format, srcPitch, dst->pitch, keycolor);
			} else if (inputFormat.bytesPerPixel == 4) {
				multiplyColorWithAlpha<uint32, uint16>((const byte *) buf, (byte *) dst->getPixels(), w, h,
				                                       inputFormat, dst->format, srcPitch, dst->pitch, keycolor);
			}
		} else {
			if (inputFormat.bytesPerPixel == 2) {
				multiplyColorWithAlpha<uint16, uint32>((const byte *) buf, (byte *) dst->getPixels(), w, h,
				                                       inputFormat, dst->format, srcPitch, dst->pitch, keycolor);
			} else if (inputFormat.bytesPerPixel == 4) {
				multiplyColorWithAlpha<uint32, uint32>((const byte *) buf, (byte *) dst->getPixels(), w, h,
				                                       inputFormat, dst->format, srcPitch, dst->pitch, keycolor);
			}
		}

		// Flag the texture as dirty.
		_cursor->flagDirty();
	}

	// In case we actually use a palette set that up properly.
	if (inputFormat.bytesPerPixel == 1) {
		updateCursorPalette();
	}

	recalculateCursorScaling();
}

void OpenGLGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	// FIXME: For some reason client code assumes that usage of this function
	// automatically enables the cursor palette.
	_cursorPaletteEnabled = true;

	memcpy(_cursorPalette + start * 3, colors, num * 3);
	updateCursorPalette();
}

void OpenGLGraphicsManager::displayMessageOnOSD(const Common::U32String &msg) {
#ifdef USE_OSD
	_osdMessageChangeRequest = true;

	_osdMessageNextData = msg;
#endif // USE_OSD
}

#ifdef USE_OSD
void OpenGLGraphicsManager::osdMessageUpdateSurface() {
	// Split up the lines.
	Common::Array<Common::U32String> osdLines;
	Common::U32StringTokenizer tokenizer(_osdMessageNextData, "\n");
	while (!tokenizer.empty()) {
		osdLines.push_back(tokenizer.nextToken());
	}

	// Do the actual drawing like the SDL backend.
	const Graphics::Font *font = getFontOSD();

	// Determine a rect which would contain the message string (clipped to the
	// screen dimensions).
	const int vOffset = 6;
	const int lineSpacing = 1;
	const int lineHeight = font->getFontHeight() + 2 * lineSpacing;
	uint width = 0;
	uint height = lineHeight * osdLines.size() + 2 * vOffset;
	for (uint i = 0; i < osdLines.size(); i++) {
		width = MAX<uint>(width, font->getStringWidth(osdLines[i]) + 14);
	}

	// Clip the rect
	width  = MIN<uint>(width,  _gameDrawRect.width());
	height = MIN<uint>(height, _gameDrawRect.height());

	delete _osdMessageSurface;
	_osdMessageSurface = nullptr;

	_osdMessageSurface = createSurface(_defaultFormatAlpha);
	assert(_osdMessageSurface);
	// We always filter the osd with GL_LINEAR. This assures it's
	// readable in case it needs to be scaled and does not affect it
	// otherwise.
	_osdMessageSurface->enableLinearFiltering(true);

	_osdMessageSurface->allocate(width, height);

	Graphics::Surface *dst = _osdMessageSurface->getSurface();

	// Draw a dark gray rect.
	const uint32 color = dst->format.RGBToColor(40, 40, 40);
	dst->fillRect(Common::Rect(0, 0, width, height), color);

	// Render the message in white
	const uint32 white = dst->format.RGBToColor(255, 255, 255);
	for (uint i = 0; i < osdLines.size(); ++i) {
		font->drawString(dst, osdLines[i],
		                 0, i * lineHeight + vOffset + lineSpacing, width,
		                 white, Graphics::kTextAlignCenter, 0, true);
	}

	_osdMessageSurface->updateGLTexture();

#if defined(MACOSX)
	macOSTouchbarUpdate(_osdMessageNextData.encode().c_str());
#endif

	// Init the OSD display parameters.
	_osdMessageAlpha = kOSDMessageInitialAlpha;
	_osdMessageFadeStartTime = g_system->getMillis() + kOSDMessageFadeOutDelay;

	if (ConfMan.hasKey("tts_enabled", "scummvm") &&
			ConfMan.getBool("tts_enabled", "scummvm")) {
		Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();
		if (ttsMan)
			ttsMan->say(_osdMessageNextData);
	}
	// Clear the text update request
	_osdMessageNextData.clear();
	_osdMessageChangeRequest = false;
}
#endif

void OpenGLGraphicsManager::displayActivityIconOnOSD(const Graphics::Surface *icon) {
#ifdef USE_OSD
	if (_osdIconSurface) {
		delete _osdIconSurface;
		_osdIconSurface = nullptr;

		// Make sure the icon is cleared on the next update
		_forceRedraw = true;
	}

	if (icon) {
		Graphics::Surface *converted = icon->convertTo(_defaultFormatAlpha);

		_osdIconSurface = createSurface(_defaultFormatAlpha);
		assert(_osdIconSurface);
		// We always filter the osd with GL_LINEAR. This assures it's
		// readable in case it needs to be scaled and does not affect it
		// otherwise.
		_osdIconSurface->enableLinearFiltering(true);

		_osdIconSurface->allocate(converted->w, converted->h);

		Graphics::Surface *dst = _osdIconSurface->getSurface();

		// Copy the icon to the texture
		dst->copyRectToSurface(*converted, 0, 0, Common::Rect(0, 0, converted->w, converted->h));

		converted->free();
		delete converted;
	}
#endif
}

void OpenGLGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	assert(_gameScreen->hasPalette());

	memcpy(_gamePalette + start * 3, colors, num * 3);
	_gameScreen->setPalette(start, num, colors);

	// We might need to update the cursor palette here.
	updateCursorPalette();
}

void OpenGLGraphicsManager::grabPalette(byte *colors, uint start, uint num) const {
	assert(_gameScreen->hasPalette());

	memcpy(colors, _gamePalette + start * 3, num * 3);
}

void OpenGLGraphicsManager::handleResizeImpl(const int width, const int height) {
	// Setup backbuffer size.
	_backBuffer.setDimensions(width, height);

	uint overlayWidth = width;
	uint overlayHeight = height;

	// WORKAROUND: We can only support surfaces up to the maximum supported
	// texture size. Thus, in case we encounter a physical size bigger than
	// this maximum texture size we will simply use an overlay as big as
	// possible and then scale it to the physical display size. This sounds
	// bad but actually all recent chips should support full HD resolution
	// anyway. Thus, it should not be a real issue for modern hardware.
	if (   overlayWidth  > (uint)OpenGLContext.maxTextureSize
	    || overlayHeight > (uint)OpenGLContext.maxTextureSize) {
		const frac_t outputAspect = intToFrac(_windowWidth) / _windowHeight;

		if (outputAspect > (frac_t)FRAC_ONE) {
			overlayWidth  = OpenGLContext.maxTextureSize;
			overlayHeight = intToFrac(overlayWidth) / outputAspect;
		} else {
			overlayHeight = OpenGLContext.maxTextureSize;
			overlayWidth  = fracToInt(overlayHeight * outputAspect);
		}
	}

	// HACK: We limit the minimal overlay size to 256x200, which is the
	// minimum of the dimensions of the two resolutions 256x240 (NES) and
	// 320x200 (many DOS games use this). This hopefully assure that our
	// GUI has working layouts.
	overlayWidth = MAX<uint>(overlayWidth, 256);
	overlayHeight = MAX<uint>(overlayHeight, 200);

	if (!_overlay || _overlay->getFormat() != _defaultFormatAlpha) {
		delete _overlay;
		_overlay = nullptr;

		_overlay = createSurface(_defaultFormatAlpha);
		assert(_overlay);
	}
	_overlay->allocate(overlayWidth, overlayHeight);
	_overlay->fill(0);

	// Re-setup the scaling and filtering for the screen and cursor
	recalculateDisplayAreas();
	recalculateCursorScaling();
	updateLinearFiltering();

	// Something changed, so update the screen change ID.
	++_screenChangeID;
}

void OpenGLGraphicsManager::notifyContextCreate(ContextType type,
	const Graphics::PixelFormat &defaultFormat,
	const Graphics::PixelFormat &defaultFormatAlpha) {
	// Initialize pipeline.
	Pipeline::setPipeline(nullptr);
	delete _pipeline;
	_pipeline = nullptr;

#if !USE_FORCED_GLES
	if (_libretroPipeline) {
		delete _libretroPipeline;
		_libretroPipeline = nullptr;
	}
#endif

	OpenGLContext.initialize(type);

#if !USE_FORCED_GLES
	if (OpenGLContext.shadersSupported) {
		ShaderMan.notifyCreate();
		_pipeline = new ShaderPipeline(ShaderMan.query(ShaderManager::kDefault));
	}
#endif

#if !USE_FORCED_GLES2
	if (_pipeline == nullptr) {
		_pipeline = new FixedPipeline();
	}
#endif

	// Disable 3D properties.
	GL_CALL(glDisable(GL_CULL_FACE));
	GL_CALL(glDisable(GL_DEPTH_TEST));
	GL_CALL(glDisable(GL_DITHER));

	_pipeline->setColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Setup backbuffer state.

	// Default to black as clear color.
	_backBuffer.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	_pipeline->setFramebuffer(&_backBuffer);

	// Setup LibRetro pipeline.

#if !USE_FORCED_GLES
	if (LibRetroPipeline::isSupportedByContext()) {
		_libretroPipeline = new LibRetroPipeline();
		_libretroPipeline->setColor(1.0f, 1.0f, 1.0f, 1.0f);
		_libretroPipeline->setFramebuffer(&_backBuffer);
	}
#endif

	// We use a "pack" alignment (when reading from textures) to 4 here,
	// since the only place where we really use it is the BMP screenshot
	// code and that requires the same alignment too.
	GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 4));

	// Refresh the output screen dimensions if some are set up.
	if (_windowWidth != 0 && _windowHeight != 0) {
		handleResize(_windowWidth, _windowHeight);
	}

	// TODO: Should we try to convert textures into one of those formats if
	// possible? For example, when _gameScreen is CLUT8 we might want to use
	// defaultFormat now.
	_defaultFormat = defaultFormat;
	_defaultFormatAlpha = defaultFormatAlpha;

	if (_gameScreen) {
		_gameScreen->recreate();
	}

#if !USE_FORCED_GLES
	if (_gameScreenTarget) {
		_gameScreenTarget->create();
	}
#endif

	if (_overlay) {
		_overlay->recreate();
	}

	if (_cursor) {
		_cursor->recreate();
	}

#ifdef USE_OSD
	if (_osdMessageSurface) {
		_osdMessageSurface->recreate();
	}

	if (_osdIconSurface) {
		_osdIconSurface->recreate();
	}
#endif

	// Everything is ready: activate the pipeline
	Pipeline::setPipeline(_pipeline);

}

void OpenGLGraphicsManager::notifyContextDestroy() {
	if (_gameScreen) {
		_gameScreen->destroy();
	}

#if !USE_FORCED_GLES
	if (_gameScreenTarget) {
		_gameScreenTarget->destroy();
	}
#endif

	if (_overlay) {
		_overlay->destroy();
	}

	if (_cursor) {
		_cursor->destroy();
	}

#ifdef USE_OSD
	if (_osdMessageSurface) {
		_osdMessageSurface->destroy();
	}

	if (_osdIconSurface) {
		_osdIconSurface->destroy();
	}
#endif

#if !USE_FORCED_GLES
	if (OpenGLContext.shadersSupported) {
		ShaderMan.notifyDestroy();
	}
#endif

	// Destroy rendering pipeline.
	Pipeline::setPipeline(nullptr);
	delete _pipeline;
	_pipeline = nullptr;

#if !USE_FORCED_GLES
	if (_libretroPipeline) {
		delete _libretroPipeline;
		_libretroPipeline = nullptr;
	}
#endif

	// Rest our context description since the context is gone soon.
	OpenGLContext.reset();
}

Surface *OpenGLGraphicsManager::createSurface(const Graphics::PixelFormat &format, bool wantAlpha, bool wantScaler) {
	GLenum glIntFormat, glFormat, glType;

#ifdef USE_SCALERS
	if (wantScaler) {
		// TODO: Ensure that the requested pixel format is supported by the scaler
		if (getGLPixelFormat(format, glIntFormat, glFormat, glType)) {
			return new ScaledTexture(glIntFormat, glFormat, glType, format, format);
		} else {
#ifdef SCUMM_LITTLE_ENDIAN
			return new ScaledTexture(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24), format);
#else
			return new ScaledTexture(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0), format);
#endif
		}
	}
#endif

	if (format.bytesPerPixel == 1) {
#if !USE_FORCED_GLES
		if (TextureCLUT8GPU::isSupportedByContext()) {
			return new TextureCLUT8GPU();
		}
#endif

		const Graphics::PixelFormat &virtFormat = wantAlpha ? _defaultFormatAlpha : _defaultFormat;
		const bool supported = getGLPixelFormat(virtFormat, glIntFormat, glFormat, glType);
		if (!supported) {
			return nullptr;
		} else {
			return new FakeTexture(glIntFormat, glFormat, glType, virtFormat, format);
		}
	} else if (getGLPixelFormat(format, glIntFormat, glFormat, glType)) {
		return new Texture(glIntFormat, glFormat, glType, format);
	} else if (OpenGLContext.packedPixelsSupported && format == Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0)) {
		// OpenGL ES does not support a texture format usable for RGB555.
		// Since SCUMM uses this pixel format for some games (and there is no
		// hope for this to change anytime soon) we use pixel format
		// conversion to a supported texture format.
		return new TextureRGB555();
#ifdef SCUMM_LITTLE_ENDIAN
	} else if (format == Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0)) { // RGBA8888
#else
	} else if (format == Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)) { // ABGR8888
#endif
		return new TextureRGBA8888Swap();
	} else {
#ifdef SCUMM_LITTLE_ENDIAN
		return new FakeTexture(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24), format);
#else
		return new FakeTexture(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0), format);
#endif
	}
}

bool OpenGLGraphicsManager::getGLPixelFormat(const Graphics::PixelFormat &pixelFormat, GLenum &glIntFormat, GLenum &glFormat, GLenum &glType) const {
#ifdef SCUMM_LITTLE_ENDIAN
	if (pixelFormat == Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)) { // ABGR8888
#else
	if (pixelFormat == Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0)) { // RGBA8888
#endif
		glIntFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_BYTE;
		return true;
	} else if (!OpenGLContext.packedPixelsSupported) {
		return false;
	} else if (pixelFormat == Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0)) { // RGB565
		glIntFormat = GL_RGB;
		glFormat = GL_RGB;
		glType = GL_UNSIGNED_SHORT_5_6_5;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0)) { // RGBA5551
		glIntFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_SHORT_5_5_5_1;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0)) { // RGBA4444
		glIntFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_SHORT_4_4_4_4;
		return true;
#if !USE_FORCED_GLES && !USE_FORCED_GLES2
	// The formats below are not supported by every GLES implementation.
	// Thus, we do not mark them as supported when a GLES context is setup.
	} else if (isGLESContext()) {
		return false;
#ifdef SCUMM_LITTLE_ENDIAN
	} else if (pixelFormat == Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0)) { // RGBA8888
		glIntFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_INT_8_8_8_8;
		return true;
#endif
	} else if (pixelFormat == Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0)) { // RGB555
		glIntFormat = GL_RGB;
		glFormat = GL_BGRA;
		glType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 4, 4, 4, 4, 8, 4, 0, 12)) { // ARGB4444
		glIntFormat = GL_RGBA;
		glFormat = GL_BGRA;
		glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		return true;
#ifdef SCUMM_BIG_ENDIAN
	} else if (pixelFormat == Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)) { // ABGR8888
		glIntFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_INT_8_8_8_8_REV;
		return true;
#endif
	} else if (pixelFormat == Graphics::PixelFormat(4, 8, 8, 8, 8, 8, 16, 24, 0)) { // BGRA8888
		glIntFormat = GL_RGBA;
		glFormat = GL_BGRA;
		glType = GL_UNSIGNED_INT_8_8_8_8;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 5, 6, 5, 0, 0, 5, 11, 0)) { // BGR565
		glIntFormat = GL_RGB;
		glFormat = GL_RGB;
		glType = GL_UNSIGNED_SHORT_5_6_5_REV;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 5, 5, 5, 1, 1, 6, 11, 0)) { // BGRA5551
		glIntFormat = GL_RGBA;
		glFormat = GL_BGRA;
		glType = GL_UNSIGNED_SHORT_5_5_5_1;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 4, 4, 4, 4, 0, 4, 8, 12)) { // ABGR4444
		glIntFormat = GL_RGBA;
		glFormat = GL_RGBA;
		glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		return true;
	} else if (pixelFormat == Graphics::PixelFormat(2, 4, 4, 4, 4, 4, 8, 12, 0)) { // BGRA4444
		glIntFormat = GL_RGBA;
		glFormat = GL_BGRA;
		glType = GL_UNSIGNED_SHORT_4_4_4_4;
		return true;
#endif // !USE_FORCED_GLES && !USE_FORCED_GLES2
	} else {
		return false;
	}
}

bool OpenGLGraphicsManager::gameNeedsAspectRatioCorrection() const {
	if (_currentState.aspectRatioCorrection) {
		const uint width = getWidth();
		const uint height = getHeight();

		// In case we enable aspect ratio correction we force a 4/3 ratio.
		// But just for 320x200 and 640x400 games, since other games do not need
		// this.
		return (width == 320 && height == 200) || (width == 640 && height == 400);
	}

	return false;
}

int OpenGLGraphicsManager::getGameRenderScale() const {
	return _currentState.scaleFactor;
}

void OpenGLGraphicsManager::recalculateDisplayAreas() {
	if (!_gameScreen) {
		return;
	}

	WindowedGraphicsManager::recalculateDisplayAreas();

	// Setup drawing limitation for game graphics.
	// This involves some trickery because OpenGL's viewport coordinate system
	// is upside down compared to ours.
	_backBuffer.setScissorBox(_gameDrawRect.left,
	                          _windowHeight - _gameDrawRect.height() - _gameDrawRect.top,
	                          _gameDrawRect.width(),
	                          _gameDrawRect.height());

	_shakeOffsetScaled = Common::Point(_gameScreenShakeXOffset * _activeArea.drawRect.width() / _activeArea.width,
		_gameScreenShakeYOffset * _activeArea.drawRect.height() / _activeArea.height);

	// Update the cursor position to adjust for new display area.
	setMousePosition(_cursorX, _cursorY);

	// Force a redraw to assure screen is properly redrawn.
	_forceRedraw = true;
}

void OpenGLGraphicsManager::updateCursorPalette() {
	if (!_cursor || !_cursor->hasPalette()) {
		return;
	}

	if (_cursorPaletteEnabled) {
		_cursor->setPalette(0, 256, _cursorPalette);
	} else {
		_cursor->setPalette(0, 256, _gamePalette);
	}

	_cursor->setColorKey(_cursorKeyColor);
}

void OpenGLGraphicsManager::recalculateCursorScaling() {
	if (!_cursor || !_gameScreen) {
		return;
	}

	// By default we use the unscaled versions.
	_cursorHotspotXScaled = _cursorHotspotX;
	_cursorHotspotYScaled = _cursorHotspotY;
	_cursorWidthScaled = _cursor->getWidth();
	_cursorHeightScaled = _cursor->getHeight();

	// In case scaling is actually enabled we will scale the cursor according
	// to the game screen.
	if (!_cursorDontScale) {
		const frac_t screenScaleFactorX = intToFrac(_gameDrawRect.width()) / _gameScreen->getWidth();
		const frac_t screenScaleFactorY = intToFrac(_gameDrawRect.height()) / _gameScreen->getHeight();

		_cursorHotspotXScaled = fracToInt(_cursorHotspotXScaled * screenScaleFactorX);
		_cursorWidthScaled    = fracToInt(_cursorWidthScaled    * screenScaleFactorX);

		_cursorHotspotYScaled = fracToInt(_cursorHotspotYScaled * screenScaleFactorY);
		_cursorHeightScaled   = fracToInt(_cursorHeightScaled   * screenScaleFactorY);
	}
}

void OpenGLGraphicsManager::updateLinearFiltering() {
#if !USE_FORCED_GLES
	if (_libretroPipeline && _libretroPipeline->isInitialized()) {
		// Apply filtering in LibRetro first input texture which is _gameScreenTarget
		if (_gameScreen) {
			_gameScreen->enableLinearFiltering(false);
			_gameScreenTarget->getTexture()->enableLinearFiltering(_currentState.filtering);
		}

		if (_cursor) {
			_cursor->enableLinearFiltering(_currentState.filtering && _overlayVisible);
		}
	} else
#endif
	{
		if (_gameScreen) {
			_gameScreen->enableLinearFiltering(_currentState.filtering);
		}

		if (_cursor) {
			_cursor->enableLinearFiltering(_currentState.filtering);
		}
	}

	// The overlay UI should also obey the filtering choice (managed via the Filter Graphics checkbox in Graphics Tab).
	// Thus, when overlay filtering is disabled, scaling in OPENGL is done with GL_NEAREST (nearest neighbor scaling).
	// It may look crude, but it should be crispier and it's left to user choice to enable filtering.
	if (_overlay) {
		_overlay->enableLinearFiltering(_currentState.filtering);
	}

}

#ifdef USE_OSD
const Graphics::Font *OpenGLGraphicsManager::getFontOSD() const {
	return FontMan.getFontByUsage(Graphics::FontManager::kLocalizedFont);
}
#endif

bool OpenGLGraphicsManager::saveScreenshot(const Common::String &filename) const {
	const uint width  = _windowWidth;
	const uint height = _windowHeight;

	// A line of a BMP image must have a size divisible by 4.
	// We calculate the padding bytes needed here.
	// Since we use a 4 byte per pixel mode, we can use 0 here, since it is
	// equal to (4 - (width * 4)) % 4. (4 - (width * Bpp)) % 4, is the usual
	// way of computing the padding bytes required).
	// GL_PACK_ALIGNMENT is 4, so this line padding is required for PNG too
	const uint linePaddingSize = 0;
	const uint lineSize        = width * 4 + linePaddingSize;

	Common::DumpFile out;
	if (!out.open(filename)) {
		return false;
	}

	Common::Array<uint8> pixels;
	pixels.resize(lineSize * height);
	GL_CALL(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels.front()));

#ifdef SCUMM_LITTLE_ENDIAN
	const Graphics::PixelFormat format(4, 8, 8, 8, 8, 0, 8, 16, 24);
#else
	const Graphics::PixelFormat format(4, 8, 8, 8, 8, 24, 16, 8, 0);
#endif
	Graphics::Surface data;
	data.init(width, height, lineSize, &pixels.front(), format);
	data.flipVertical(Common::Rect(width, height));

#ifdef USE_PNG
	return Image::writePNG(out, data);
#else
	return Image::writeBMP(out, data);
#endif
}

} // End of namespace OpenGL
