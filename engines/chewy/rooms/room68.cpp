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

#include "chewy/defines.h"
#include "chewy/events.h"
#include "chewy/globals.h"
#include "chewy/room.h"
#include "chewy/rooms/room68.h"
#include "chewy/sound.h"

namespace Chewy {
namespace Rooms {

static const AniBlock ABLOCK38[2] = {
	{ 5, 1, ANI_FRONT, ANI_WAIT, 0 },
	{ 6, 1, ANI_FRONT, ANI_WAIT, 0 },
};

static const AniBlock ABLOCK39[3] = {
	{ 15, 1, ANI_FRONT, ANI_WAIT, 0 },
	{ 17, 1, ANI_FRONT, ANI_WAIT, 0 },
	{ 15, 1, ANI_BACK, ANI_WAIT, 0 },
};


void Room68::entry() {
	_G(spieler).ScrollxStep = 2;
	_G(spieler).DiaAMov = 1;
	_G(SetUpScreenFunc) = setup_func;
	_G(r68HohesC) = -1;
	_G(spieler_mi)[P_HOWARD].Mode = true;
	_G(spieler_mi)[P_NICHELLE].Mode = true;
	
	if (_G(spieler).R68Papagei) {
		_G(det)->showStaticSpr(12);
		_G(det)->start_detail(21, 255, ANI_FRONT);
	}

	if (!_G(spieler).R68DivaWeg) {
		_G(timer_nr)[0] = _G(room)->set_timer(255, 10);
		_G(det)->set_static_ani(18, -1);
	} else
		_G(det)->hideStaticSpr(3);
	
	if (!_G(flags).LoadGame) {
		hideCur();
		if (_G(spieler).PersonRoomNr[P_HOWARD] == 68) {
			set_person_pos(524, 51, P_HOWARD, P_LEFT);
			set_person_pos(550, 54, P_NICHELLE, P_LEFT);
		}
		autoMove(7, P_CHEWY);
		showCur();
	}
}

void Room68::xit() {
	_G(spieler).ScrollxStep = 1;
	if (_G(spieler).PersonRoomNr[P_HOWARD] == 68) {
		_G(spieler).PersonRoomNr[P_HOWARD] = 66;
		_G(spieler).PersonRoomNr[P_NICHELLE] = 66;
	}
}

bool Room68::timer(int16 t_nr, int16 ani_nr) {
	if (t_nr == _G(timer_nr)[0])
		calc_diva();
	else
		return true;

	return false;
}

void Room68::setup_func() {
	switch (_G(r68HohesC)) {
	case 0:
		if (_G(det)->get_ani_status(_G(r68HohesC)) == false) {
			_G(r68HohesC) = 1;
			_G(det)->start_detail(_G(r68HohesC), 1, ANI_FRONT);
		}
		break;

	case 1:
		if (_G(det)->get_ani_status(_G(r68HohesC)) == false) {
			_G(r68HohesC) = 2;
			if (_G(spieler).SpeechSwitch) {
				g_engine->_sound->playSound(2, 0);
				g_engine->_sound->playSound(_G(r68HohesC));
				_G(det)->start_detail(_G(r68HohesC), 255, ANI_FRONT);

				// TODO
				//while (_G(sndPlayer)->getSampleStatus(1) == 4)
				//	set_up_screen(DO_SETUP);
				_G(det)->stop_detail(_G(r68HohesC));
			} else {
				_G(det)->start_detail(_G(r68HohesC), 3, ANI_FRONT);
			}
			
			
			_G(det)->start_detail(_G(r68HohesC), 3, ANI_FRONT);
		}
		break;

	case 2:
		if (_G(det)->get_ani_status(_G(r68HohesC)) == false) {
			_G(r68HohesC) = -1;
			_G(det)->stop_detail(18);
			_G(det)->showStaticSpr(3);
			_G(uhr)->reset_timer(_G(timer_nr)[0], 0);
		}
		break;

	default:
		break;
	}
	calc_person_look();
	int16 ho_y = 75;
	int16 ni_y = 75;
	int16 ch_x = _G(spieler_vector)[P_CHEWY].Xypos[0];
	int16 ho_x, ni_x;

	if (ch_x < 130) {
		ho_x = 223;
		ni_x = 260;
	} else {
		ho_x = 320;
		ni_x = 350;
	}
	goAutoXy(ho_x, ho_y, P_HOWARD, ANI_GO);
	goAutoXy(ni_x, ni_y, P_NICHELLE, ANI_GO);
}

void Room68::look_kaktus() {
	hideCur();
	autoMove(6, P_CHEWY);
	start_aad_wait(383, -1);
	showCur();
}

void Room68::talk_indigo() {
	talk_indigo(-1);
}

void Room68::talk_indigo(int16 aad_nr) {
	hideCur();
	autoMove(3, P_CHEWY);
	_G(room)->set_timer_status(8, TIMER_STOP);
	_G(det)->del_static_ani(8);
	if (aad_nr == -1) {
		_G(det)->set_static_ani(9, -1);
		start_aad_wait(384 + (int16)_G(spieler).R68IndigoDia, -1);
		_G(spieler).R68IndigoDia ^= 1;
		_G(det)->del_static_ani(9);
	} else {
		_G(det)->set_static_ani(12, -1);
		start_aad_wait(aad_nr, -1);
		_G(det)->del_static_ani(12);
	}
	_G(room)->set_timer_status(8, TIMER_START);
	_G(det)->set_static_ani(8, -1);
	showCur();
}

int16 Room68::use_indigo() {
	int16 action_flag = false;
	hideCur();
	if (is_cur_inventar(CLINT_500_INV)) {
		action_flag = true;
		if (_G(spieler).R68Lied) {
			hideCur();
			autoMove(3, P_CHEWY);
			auto_scroll(78, 0);
			del_inventar(_G(spieler).AkInvent);
			talk_indigo(394);
			_G(cur_hide_flag) = false;
			hideCur();
			_G(room)->set_timer_status(8, TIMER_STOP);
			_G(det)->del_static_ani(8);
			_G(det)->stop_detail(8);
			start_detail_wait(13, 3, ANI_FRONT);
			start_detail_wait(25, 1, ANI_FRONT);
			_G(det)->set_static_ani(12, -1);
			talk_indigo(398);
			_G(cur_hide_flag) = false;
			hideCur();
			_G(room)->set_timer_status(8, TIMER_STOP);
			_G(det)->del_static_ani(8);
			_G(det)->stop_detail(8);
			start_detail_wait(26, 1, ANI_FRONT);
			_G(room)->set_timer_status(8, TIMER_START);
			_G(det)->set_static_ani(8, -1);
			new_invent_2_cur(KARTE_INV);
			_G(spieler).R68KarteDa = true;
		} else {
			talk_indigo(397);
		}
	} else if (is_cur_inventar(CLINT_1500_INV) || is_cur_inventar(CLINT_3000_INV)) {
		action_flag = true;
		start_aad_wait(393, -1);
	}
	showCur();
	return action_flag;
}

void Room68::talk_keeper() {
	hideCur();
	autoMove(2, P_CHEWY);
	_G(room)->set_timer_status(20, TIMER_STOP);
	_G(det)->del_static_ani(20);
	start_detail_wait(15, 1, ANI_FRONT);
	_G(det)->set_static_ani(16, -1);
	showCur();
	_G(ssi)[3].X = _G(spieler_vector)[P_CHEWY].Xypos[0] - _G(spieler).scrollx + _G(spieler_mi)[P_CHEWY].HotX;;
	_G(ssi)[3].Y = _G(spieler_vector)[P_CHEWY].Xypos[1] - _G(spieler).scrolly;
	_G(atds)->set_split_win(3, &_G(ssi)[3]);
	start_ads_wait(20);
	_G(cur_hide_flag) = false;
	hideCur();
	_G(det)->del_static_ani(16);
	start_detail_wait(15, 1, ANI_BACK);
	_G(room)->set_timer_status(20, TIMER_START);
	_G(det)->set_static_ani(20, -1);
	showCur();
}

int16 Room68::use_papagei() {
	int16 action_flag = false;
	if (is_cur_inventar(PAPAGEI_INV)) {
		hideCur();
		action_flag = true;
		_G(spieler).R68Papagei = true;
		del_inventar(_G(spieler).AkInvent);
		autoMove(5, P_CHEWY);
		start_spz_wait(CH_LGET_O, 1, false, P_CHEWY);
		_G(det)->showStaticSpr(12);
		_G(det)->start_detail(21, 255, ANI_FRONT);
		_G(atds)->del_steuer_bit(408, ATS_AKTIV_BIT, ATS_DATEI);
		_G(atds)->set_ats_str(407, 1, ATS_DATEI);
		showCur();
	}
	return action_flag;
}

void Room68::calc_diva() {
	if (!_G(spieler).R68DivaWeg) {
		if (!_G(spieler).R68Papagei) {
			if (_G(r68HohesC) == -1) {
				_G(uhr)->reset_timer(_G(timer_nr)[0], 0);
				_G(r68HohesC) = 0;
				_G(det)->hideStaticSpr(3);
				_G(det)->start_detail(_G(r68HohesC), 1, ANI_BACK);
				_G(det)->start_detail(18, 255, ANI_FRONT);
			}
		} else if (!_G(spieler).R68Gutschein && !is_chewy_busy()) {
			hideCur();
			_G(spieler).R68Gutschein = true;
			autoMove(4, P_CHEWY);
			start_aad_wait(386, -1);
			start_spz_wait(CH_LGET_O, 1, false, P_CHEWY);
			new_invent_2_cur(BAR_GUT_INV);
			_G(uhr)->reset_timer(_G(timer_nr)[0], 0);
			showCur();
		}
	}
}

int16 Room68::use_keeper() {
	int16 action_flag = false;
	if (is_cur_inventar(BAR_GUT_INV)) {
		hideCur();
		del_inventar(_G(spieler).AkInvent);
		action_flag = true;
		autoMove(2, P_CHEWY);
		start_spz_wait(CH_LGET_O, 1, false, P_CHEWY);
		_G(room)->set_timer_status(20, TIMER_STOP);
		_G(det)->del_static_ani(20);
		start_ani_block(3, ABLOCK39);
		_G(room)->set_timer_status(20, TIMER_START);
		_G(det)->set_static_ani(20, -1);
		new_invent_2_cur(B_MARY_INV);
		showCur();
	}
	return action_flag;
}

int16 Room68::use_diva() {
	int16 action_flag;
	hideCur();
	if (is_cur_inventar(B_MARY_INV)) {
		del_inventar(_G(spieler).AkInvent);
		action_flag = 1;
		autoMove(4, P_CHEWY);
		_G(uhr)->reset_timer(_G(timer_nr)[0], 0);
		_G(det)->hideStaticSpr(3);
		start_detail_wait(4, 1, ANI_FRONT);
		_G(spieler).R68Gutschein = false;
		_G(det)->showStaticSpr(3);
	} else if (is_cur_inventar(B_MARY2_INV)) {
		del_inventar(_G(spieler).AkInvent);
		action_flag = 1;
		autoMove(4, P_CHEWY);
		_G(det)->hideStaticSpr(3);
		_G(spieler).R68DivaWeg = true;
		start_ani_block(2, ABLOCK38);
		flic_cut(FCUT_083);
		_G(det)->del_static_ani(18);
		register_cutscene(22);
		_G(det)->del_static_ani(18);
		start_detail_wait(7, 1, ANI_FRONT);
		_G(atds)->set_steuer_bit(407, ATS_AKTIV_BIT, ATS_DATEI);
		_G(atds)->set_steuer_bit(412, ATS_AKTIV_BIT, ATS_DATEI);
		set_person_spr(P_RIGHT, P_CHEWY);
		start_aad_wait(402, -1);
	} else
		action_flag = use_papagei();
	showCur();
	return action_flag;
}

void Room68::kostuem_aad(int16 aad_nr) {
	hideCur();
	if (_G(spieler).DiaAMov != -1) {
		autoMove(_G(spieler).DiaAMov, P_CHEWY);
	}

	start_aad_wait(aad_nr, -1);

	if (!_G(spieler).R68DivaWeg)
		start_aad_wait(388, -1);
	else if (!_G(spieler).R67LiedOk)
		start_aad_wait(389, -1);
	else {
		if (_G(spieler).DisplayText == 0)
			_G(sndPlayer)->fadeOut(5);
		
		_G(SetUpScreenFunc) = nullptr;
		del_inventar(_G(spieler).AkInvent);
		goAutoXy(150, -13, P_NICHELLE, ANI_WAIT);
		_G(spieler).PersonHide[P_NICHELLE] = true;
		goAutoXy(161, 59, P_HOWARD, ANI_GO);
		autoMove(4, P_CHEWY);
		start_aad_wait(390, -1);
		start_detail_wait(22, 1, ANI_FRONT);
		_G(spieler).PersonHide[P_HOWARD] = true;
		_G(det)->start_detail(27, 255, ANI_FRONT);

		if (_G(spieler).DisplayText)
			start_detail_wait(23, 3, ANI_FRONT);
		else {
			_G(det)->start_detail(23, 255, ANI_FRONT);
			g_engine->_sound->playSound(109, 1, false);
			wait_show_screen(2);

			g_engine->_sound->waitForSpeechToFinish();

			_G(det)->stop_detail(23);
		}

		if (_G(spieler).DisplayText) {
			g_engine->_sound->playSound(108, 1, false);
		}
		
		_G(det)->start_detail(24, 255, ANI_FRONT);
		set_person_pos(26, 40, P_NICHELLE, P_RIGHT);
		if (_G(spieler).DisplayText) {
			start_aad_wait(391, -1);
		} else {
			wait_show_screen(100);
			start_aad_wait(602, -1);
			wait_show_screen(100);
		}

		_G(room)->set_timer_status(8, TIMER_STOP);
		_G(det)->del_static_ani(8);
		start_detail_wait(10, 1, ANI_FRONT);
		_G(det)->start_detail(11, 255, ANI_FRONT);
		start_aad_wait(396, -1);
		_G(det)->stop_detail(11);
		_G(det)->set_static_ani(12, -1);
		start_aad_wait(611, -1);
		_G(det)->del_static_ani(12);
		_G(det)->start_detail(14, 255, ANI_FRONT);
		start_aad_wait(395, -1);
		_G(det)->stop_detail(14);
		_G(room)->set_timer_status(8, TIMER_START);
		_G(det)->set_static_ani(8, -1);
		start_aad_wait(392, -1);

		g_engine->_sound->waitForSpeechToFinish();
		
		_G(spieler).PersonHide[P_HOWARD] = false;
		_G(det)->stop_detail(27);
		_G(det)->stop_detail(24);
		_G(det)->showStaticSpr(13);
		_G(spieler).R68Lied = true;
		autoMove(1, P_CHEWY);
		auto_scroll(216, 0);
		_G(det)->hideStaticSpr(13);
		_G(spieler).PersonHide[P_NICHELLE] = false;
		set_person_pos(150, -13, P_NICHELLE, P_RIGHT);

		if (_G(spieler).DisplayText) {
			_G(currentSong) = -1;
			load_room_music(_G(spieler).PersonRoomNr[0]);
		}
	}
	showCur();
}

void Room68::talk_papagei() {
	hideCur();
	autoMove(5, P_CHEWY);
	showCur();

	_G(ssi)[2].X = 60;
	_G(ssi)[2].Y = 80;
	_G(atds)->set_split_win(2, &_G(ssi)[2]);
	start_ads_wait(18);
}

} // namespace Rooms
} // namespace Chewy
