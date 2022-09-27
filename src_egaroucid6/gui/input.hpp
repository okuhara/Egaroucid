﻿#pragma once
#include <iostream>
#include <future>
#include "./ai.hpp"
#include "function/language.hpp"
#include "function/menu.hpp"
#include "function/graph.hpp"
#include "function/opening.hpp"
#include "function/button.hpp"
#include "function/radio_button.hpp"
#include "gui_common.hpp"

vector<History_elem> import_transcript_processing(vector<History_elem> n_history, History_elem strt_elem, string transcript, bool* failed) {
	Board h_bd = strt_elem.board;
	String transcript_str = Unicode::Widen(transcript).replace(U"\r", U"").replace(U"\n", U"").replace(U" ", U"");
	if (transcript_str.size() % 2 != 0 && transcript_str.size() >= 120) {
		*failed = true;
	}
	else {
		int y, x;
		uint64_t legal;
		Flip flip;
		History_elem history_elem;
		int player = strt_elem.player;
		//history_elem.set(h_bd, player, GRAPH_IGNORE_VALUE, -1, -1, -1, "");
		//n_history.emplace_back(history_elem);
		for (int i = 0; i < (int)transcript_str.size(); i += 2) {
			x = (int)transcript_str[i] - (int)'a';
			if (x < 0 || HW <= x) {
				x = (int)transcript_str[i] - (int)'A';
				if (x < 0 || HW <= x) {
					*failed = true;
					break;
				}
			}
			y = (int)transcript_str[i + 1] - (int)'1';
			if (y < 0 || HW <= y) {
				*failed = true;
				break;
			}
			y = HW_M1 - y;
			x = HW_M1 - x;
			legal = h_bd.get_legal();
			if (1 & (legal >> (y * HW + x))) {
				calc_flip(&flip, &h_bd, y * HW + x);
				h_bd.move_board(&flip);
				player ^= 1;
				if (h_bd.get_legal() == 0ULL) {
					h_bd.pass();
					player ^= 1;
					if (h_bd.get_legal() == 0ULL) {
						h_bd.pass();
						player ^= 1;
						if (i != transcript_str.size() - 2) {
							*failed = true;
							break;
						}
					}
				}
			}
			else {
				*failed = true;
				break;
			}
			n_history.back().next_policy = y * HW + x;
			history_elem.set(h_bd, player, GRAPH_IGNORE_VALUE, -1, y * HW + x, -1, "");
			n_history.emplace_back(history_elem);
		}
	}
	return n_history;
}

class Import_transcript : public App::Scene {
private:
	Button single_back_button;
	Button back_button;
	Button import_button;
	Button import_from_position_button;
	bool done;
	bool failed;
	string transcript;
	vector<History_elem> n_history;
	bool imported_from_position;

public:
	Import_transcript(const InitData& init) : IScene{ init } {
		single_back_button.init(BACK_BUTTON_SX, BACK_BUTTON_SY, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, BACK_BUTTON_RADIUS, language.get("common", "back"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		back_button.init(BUTTON3_1_SX, BUTTON3_SY, BUTTON3_WIDTH, BUTTON3_HEIGHT, BUTTON3_RADIUS, language.get("common", "back"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		import_button.init(BUTTON3_2_SX, BUTTON3_SY, BUTTON3_WIDTH, BUTTON3_HEIGHT, BUTTON3_RADIUS, language.get("in_out", "import"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		import_from_position_button.init(BUTTON3_3_SX, BUTTON3_SY, BUTTON3_WIDTH, BUTTON3_HEIGHT, BUTTON3_RADIUS, language.get("in_out", "import_from_this_position"), getData().fonts.font15, getData().colors.white, getData().colors.black);
		done = false;
		failed = false;
		imported_from_position = false;
		transcript.clear();
	}

	void update() override {
		Scene::SetBackground(getData().colors.green);
		const int icon_width = (LEFT_RIGHT - LEFT_LEFT) / 2;
		getData().resources.icon.scaled((double)icon_width / getData().resources.icon.width()).draw(X_CENTER - icon_width / 2, 20);
		getData().resources.logo.scaled((double)icon_width / getData().resources.logo.width()).draw(X_CENTER - icon_width / 2, 20 + icon_width);
		int sy = 20 + icon_width + 50;
		if (!done) {
			getData().fonts.font25(language.get("in_out", "input_transcript")).draw(Arg::topCenter(X_CENTER, sy), getData().colors.white);
			Rect text_area{ X_CENTER - 300, sy + 40, 600, 70 };
			text_area.draw(getData().colors.light_cyan).drawFrame(2, getData().colors.black);
			String str = Unicode::Widen(transcript);
			TextInput::UpdateText(str);
			const String editingText = TextInput::GetEditingText();
			bool return_pressed = false;
			if (KeyControl.pressed() && KeyV.down()) {
				String clip_text;
				Clipboard::GetText(clip_text);
				str += clip_text;
			}
			if (str.size()) {
				if (str[str.size() - 1] == '\n') {
					str.replace(U"\n", U"");
					return_pressed = true;
				}
			}
			transcript = str.narrow();
			getData().fonts.font15(str + U'|' + editingText).draw(text_area.stretched(-4), getData().colors.black);
			back_button.draw();
			import_button.draw();
			import_from_position_button.draw();
			if (back_button.clicked() || KeyEscape.pressed()) {
				changeScene(U"Main_scene", SCENE_FADE_TIME);
			}
			if (import_button.clicked() || KeyEnter.pressed()) {
				History_elem history_elem;
				Board h_bd;
				h_bd.reset();
				history_elem.set(h_bd, BLACK, GRAPH_IGNORE_VALUE, -1, -1, -1, "");
				n_history.emplace_back(history_elem);
				n_history = import_transcript_processing(n_history, history_elem, transcript, &failed);
				done = true;
			}
			if (import_from_position_button.clicked()) {
				for (History_elem history_elem : getData().graph_resources.nodes[getData().graph_resources.put_mode]) {
					if (getData().history_elem.board.n_discs() >= history_elem.board.n_discs()) {
						n_history.emplace_back(history_elem);
					}
				}
				n_history = import_transcript_processing(n_history, getData().history_elem, transcript, &failed);
				done = true;
				imported_from_position = true;
			}
		}
		else {
			if (!failed) {
				if (!imported_from_position) {
					getData().graph_resources.init();
					getData().graph_resources.nodes[0] = n_history;
					getData().graph_resources.n_discs = getData().graph_resources.nodes[0].back().board.n_discs();
				}
				else {
					getData().graph_resources.nodes[getData().graph_resources.put_mode] = n_history;
				}
				getData().graph_resources.n_discs = getData().graph_resources.nodes[getData().graph_resources.put_mode].back().board.n_discs();
				getData().graph_resources.need_init = false;
				changeScene(U"Main_scene", SCENE_FADE_TIME);
			}
			else {
				getData().fonts.font25(language.get("in_out", "import_failed")).draw(Arg::topCenter(X_CENTER, sy), getData().colors.white);
				single_back_button.draw();
				if (single_back_button.clicked() || KeyEscape.pressed()) {
					getData().graph_resources.need_init = false;
					changeScene(U"Main_scene", SCENE_FADE_TIME);
				}
			}
		}
	}

	void draw() const override {

	}
};

class Import_board : public App::Scene {
private:
	Button single_back_button;
	Button back_button;
	Button import_button;
	bool done;
	bool failed;
	Board board;
	int player;
	string board_str;

public:
	Import_board(const InitData& init) : IScene{ init } {
		single_back_button.init(BACK_BUTTON_SX, BACK_BUTTON_SY, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, BACK_BUTTON_RADIUS, language.get("common", "back"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		back_button.init(GO_BACK_BUTTON_BACK_SX, GO_BACK_BUTTON_SY, GO_BACK_BUTTON_WIDTH, GO_BACK_BUTTON_HEIGHT, GO_BACK_BUTTON_RADIUS, language.get("common", "back"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		import_button.init(GO_BACK_BUTTON_GO_SX, GO_BACK_BUTTON_SY, GO_BACK_BUTTON_WIDTH, GO_BACK_BUTTON_HEIGHT, GO_BACK_BUTTON_RADIUS, language.get("in_out", "import"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		done = false;
		failed = false;
		board_str.clear();
	}

	void update() override {
		Scene::SetBackground(getData().colors.green);
		const int icon_width = (LEFT_RIGHT - LEFT_LEFT) / 2;
		getData().resources.icon.scaled((double)icon_width / getData().resources.icon.width()).draw(X_CENTER - icon_width / 2, 20);
		getData().resources.logo.scaled((double)icon_width / getData().resources.logo.width()).draw(X_CENTER - icon_width / 2, 20 + icon_width);
		int sy = 20 + icon_width + 50;
		if (!done) {
			getData().fonts.font25(language.get("in_out", "input_board")).draw(Arg::topCenter(X_CENTER, sy), getData().colors.white);
			Rect text_area{ X_CENTER - 300, sy + 40, 600, 70 };
			text_area.draw(getData().colors.light_cyan).drawFrame(2, getData().colors.black);
			String str = Unicode::Widen(board_str);
			TextInput::UpdateText(str);
			const String editingText = TextInput::GetEditingText();
			bool return_pressed = false;
			if (KeyControl.pressed() && KeyV.down()) {
				String clip_text;
				Clipboard::GetText(clip_text);
				str += clip_text;
			}
			if (str.size()) {
				if (str[str.size() - 1] == '\n') {
					str.replace(U"\n", U"");
					return_pressed = true;
				}
			}
			board_str = str.narrow();
			getData().fonts.font15(str + U'|' + editingText).draw(text_area.stretched(-4), getData().colors.black);
			back_button.draw();
			import_button.draw();
			if (back_button.clicked() || KeyEscape.pressed()) {
				changeScene(U"Main_scene", SCENE_FADE_TIME);
			}
			if (import_button.clicked() || KeyEnter.pressed()) {
				failed = import_board_processing();
				done = true;
			}
		}
		else {
			if (!failed) {
				getData().graph_resources.init();
				History_elem history_elem;
				history_elem.reset();
				getData().graph_resources.nodes[0].emplace_back(history_elem);
				history_elem.player = player;
				history_elem.board = board;
				getData().graph_resources.nodes[0].emplace_back(history_elem);
				getData().graph_resources.n_discs = board.n_discs();
				getData().graph_resources.need_init = false;
				changeScene(U"Main_scene", SCENE_FADE_TIME);
			}
			else {
				getData().fonts.font25(language.get("in_out", "import_failed")).draw(Arg::topCenter(X_CENTER, sy), getData().colors.white);
				single_back_button.draw();
				if (single_back_button.clicked() || KeyEscape.pressed()) {
					changeScene(U"Main_scene", SCENE_FADE_TIME);
				}
			}
		}
	}

	void draw() const override {

	}

private:
	bool import_board_processing() {
		String board_str_str = Unicode::Widen(board_str).replace(U"\r", U"").replace(U"\n", U"").replace(U" ", U"");
		bool failed_res = false;
		int bd_arr[HW2];
		Board bd;
		if (board_str_str.size() != HW2 + 1) {
			failed_res = true;
		}
		else {
			for (int i = 0; i < HW2; ++i) {
				if (board_str_str[i] == '0' || board_str_str[i] == 'B' || board_str_str[i] == 'b' || board_str_str[i] == 'X' || board_str_str[i] == 'x' || board_str_str[i] == '*')
					bd_arr[i] = BLACK;
				else if (board_str_str[i] == '1' || board_str_str[i] == 'W' || board_str_str[i] == 'w' || board_str_str[i] == 'O' || board_str_str[i] == 'o')
					bd_arr[i] = WHITE;
				else if (board_str_str[i] == '.' || board_str_str[i] == '-')
					bd_arr[i] = VACANT;
				else {
					failed_res = true;
					break;
				}
			}
			if (board_str_str[HW2] == '0' || board_str_str[HW2] == 'B' || board_str_str[HW2] == 'b' || board_str_str[HW2] == 'X' || board_str_str[HW2] == 'x' || board_str_str[HW2] == '*')
				player = 0;
			else if (board_str_str[HW2] == '1' || board_str_str[HW2] == 'W' || board_str_str[HW2] == 'w' || board_str_str[HW2] == 'O' || board_str_str[HW2] == 'o')
				player = 1;
			else
				failed_res = true;
		}
		if (!failed_res) {
			board.translate_from_arr(bd_arr, player);
		}
		return failed_res;
	}
};

class Edit_board : public App::Scene {
private:
	Button back_button;
	Button set_button;
	Radio_button player_radio;
	Radio_button disc_radio;
	bool done;
	bool failed;
	History_elem history_elem;

public:
	Edit_board(const InitData& init) : IScene{ init } {
		back_button.init(BUTTON2_VERTICAL_SX, BUTTON2_VERTICAL_1_SY, BUTTON2_VERTICAL_WIDTH, BUTTON2_VERTICAL_HEIGHT, BUTTON2_VERTICAL_RADIUS, language.get("common", "back"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		set_button.init(BUTTON2_VERTICAL_SX, BUTTON2_VERTICAL_2_SY, BUTTON2_VERTICAL_WIDTH, BUTTON2_VERTICAL_HEIGHT, BUTTON2_VERTICAL_RADIUS, language.get("in_out", "import"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		done = false;
		failed = false;
		history_elem = getData().history_elem;
		Radio_button_element radio_button_elem;
		player_radio.init();
		radio_button_elem.init(480, 120, getData().fonts.font15, 20, language.get("common", "black"), true);
		player_radio.push(radio_button_elem);
		radio_button_elem.init(480, 140, getData().fonts.font15, 20, language.get("common", "white"), false);
		player_radio.push(radio_button_elem);
		disc_radio.init();
		radio_button_elem.init(480, 210, getData().fonts.font15, 20, language.get("edit_board", "black"), true);
		disc_radio.push(radio_button_elem);
		radio_button_elem.init(480, 230, getData().fonts.font15, 20, language.get("edit_board", "white"), false);
		disc_radio.push(radio_button_elem);
		radio_button_elem.init(480, 250, getData().fonts.font15, 20, language.get("edit_board", "empty"), false);
		disc_radio.push(radio_button_elem);

	}

	void update() override {
		int board_arr[HW2];
		history_elem.board.translate_to_arr(board_arr, BLACK);
		for (int cell = 0; cell < HW2; ++cell) {
			int x = BOARD_SX + (cell % HW) * BOARD_CELL_SIZE + BOARD_CELL_SIZE / 2;
			int y = BOARD_SY + (cell / HW) * BOARD_CELL_SIZE + BOARD_CELL_SIZE / 2;
			if (board_arr[cell] == BLACK) {
				Circle(x, y, DISC_SIZE).draw(Palette::Black);
			}
			else if (board_arr[cell] == WHITE) {
				Circle(x, y, DISC_SIZE).draw(Palette::White);
			}
		}
		for (int cell = 0; cell < HW2; ++cell) {
			int x = BOARD_SX + (cell % HW) * BOARD_CELL_SIZE;
			int y = BOARD_SY + (cell / HW) * BOARD_CELL_SIZE;
			Rect cell_region(x, y, BOARD_CELL_SIZE, BOARD_CELL_SIZE);
			if (cell_region.leftPressed()) {
				board_arr[cell] = disc_radio.checked;
			}
		}
		history_elem.board.translate_from_arr(board_arr, BLACK);
		if (KeyB.pressed()) {
			disc_radio.checked = BLACK;
		}
		else if (KeyW.pressed()) {
			disc_radio.checked = WHITE;
		}
		else if (KeyE.pressed()) {
			disc_radio.checked = VACANT;
		}
		Scene::SetBackground(getData().colors.green);
		getData().fonts.font25(language.get("in_out", "edit_board")).draw(480, 20, getData().colors.white);
		getData().fonts.font20(language.get("in_out", "player")).draw(480, 80, getData().colors.white);
		getData().fonts.font20(language.get("in_out", "color")).draw(480, 170, getData().colors.white);
		draw_board(getData().fonts, getData().colors, history_elem);
		player_radio.draw();
		disc_radio.draw();
		back_button.draw();
		set_button.draw();
		if (back_button.clicked() || KeyEscape.pressed()) {
			changeScene(U"Main_scene", SCENE_FADE_TIME);
		}
		if (set_button.clicked() || KeyEnter.pressed()) {
			if (player_radio.checked != BLACK) {
				history_elem.board.pass();
			}
			history_elem.player = player_radio.checked;
			history_elem.v = GRAPH_IGNORE_VALUE;
			history_elem.level = -1;
			getData().history_elem = history_elem;
			int n_discs = history_elem.board.n_discs();
			int insert_place = (int)getData().graph_resources.nodes[getData().graph_resources.put_mode].size();
			int replace_place = -1;
			for (int i = 0; i < (int)getData().graph_resources.nodes[getData().graph_resources.put_mode].size(); ++i) {
				int node_n_discs = getData().graph_resources.nodes[getData().graph_resources.put_mode][i].board.n_discs();
				if (node_n_discs == n_discs) {
					replace_place = i;
					insert_place = -1;
					break;
				}
				else if (node_n_discs > n_discs) {
					insert_place = i;
					break;
				}
			}
			if (replace_place != -1) {
				cerr << "replace" << endl;
				getData().graph_resources.nodes[getData().graph_resources.put_mode][replace_place] = history_elem;
			}
			else {
				cerr << "insert" << endl;
				getData().graph_resources.nodes[getData().graph_resources.put_mode].insert(getData().graph_resources.nodes[getData().graph_resources.put_mode].begin() + insert_place, history_elem);
			}
			getData().graph_resources.need_init = false;
			getData().graph_resources.n_discs = n_discs;
			changeScene(U"Main_scene", SCENE_FADE_TIME);
		}
	}

	void draw() const override {

	}
};

class Import_game : public App::Scene {
private:
	vector<Game_abstract> game_abstracts;
	vector<Button> buttons;
	Button back_button;
	int strt_idx;
	int n_games;

public:
	Import_game(const InitData& init) : IScene{ init } {
		strt_idx = 0;
		back_button.init(BACK_BUTTON_SX, BACK_BUTTON_SY, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, BACK_BUTTON_RADIUS, language.get("common", "back"), getData().fonts.font25, getData().colors.white, getData().colors.black);
		n_games = 0;
	}

	void update() override {
		getData().fonts.font25(language.get("in_out", "input_game")).draw(Arg::topCenter(X_CENTER, 10), getData().colors.white);

		back_button.draw();
		if (back_button.clicked() || KeyEscape.pressed()) {
			changeScene(U"Main_scene", SCENE_FADE_TIME);
		}
		for (int i = 0; i < n_games; ++i) {
			if (buttons[i].clicked()) {
				string transcript = game_abstracts[i].transcript.narrow();
			}
		}
	}

	void draw() const override {

	}
};
