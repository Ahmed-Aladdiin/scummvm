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

#include "ags/lib/std/utility.h"
#include "ags/shared/script/cc_common.h"
#include "ags/shared/util/string.h"
#include "ags/globals.h"

namespace AGS3 {

using namespace AGS::Shared;

void ccSetOption(int optbit, int onoroff) {
	if (onoroff)
		_G(ccCompOptions) |= optbit;
	else
		_G(ccCompOptions) &= ~optbit;
}

int ccGetOption(int optbit) {
	if (_G(ccCompOptions) & optbit)
		return 1;

	return 0;
}

// Returns full script error message and callstack (if possible)
extern std::pair<String, String> cc_error_at_line(const char *error_msg);
// Returns script error message without location or callstack
extern String cc_error_without_line(const char *error_msg);

void cc_clear_error() {
	_GP(ccError) = ScriptError();
}

bool cc_has_error() {
	return _GP(ccError).HasError;
}

const ScriptError &cc_get_error() {
	return _GP(ccError);
}

void cc_error(const char *descr, ...) {
	_GP(ccError).IsUserError = false;
	if (descr[0] == '!') {
		_GP(ccError).IsUserError = true;
		descr++;
	}

	va_list ap;
	va_start(ap, descr);
	String displbuf = String::FromFormatV(descr, ap);
	va_end(ap);

	if (_G(currentline) > 0) {
		// [IKM] Implementation is project-specific
		std::pair<String, String> errinfo = cc_error_at_line(displbuf.GetCStr());
		_GP(ccError).ErrorString = errinfo.first;
		_GP(ccError).CallStack = errinfo.second;
	} else {
		_GP(ccError).ErrorString = cc_error_without_line(displbuf.GetCStr());
		_GP(ccError).CallStack = "";
	}

	_GP(ccError).HasError = 1;
	_GP(ccError).Line = _G(currentline);
}

} // namespace AGS3
