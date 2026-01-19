#pragma once

namespace globals {
	constexpr unsigned int COLOR_BG = 0xff181818;
	constexpr unsigned int COLOR_ACTIVE = 0xffff8050;
	constexpr unsigned int COLOR_ACTIVE_LIGHT = 0xffffb193;
	constexpr unsigned int COLOR_ACTIVE_DARK = 0xff88442b;
	constexpr unsigned int COLOR_NEUTRAL = 0xff666666;
	constexpr unsigned int COLOR_NEUTRAL_LIGHT = 0x99999999;
	constexpr unsigned int COLOR_SEEK = 0xff80ffff;
	constexpr unsigned int COLOR_KNOB = 0xff272727;
	constexpr unsigned int COLOR_AUDIO = 0xffffd42a;
	constexpr unsigned int COLOR_MIDI = 0xff50a9ff;
	constexpr unsigned int COLOR_SELECTION = 0xff50a9ff;
	constexpr unsigned int COLOR_SEQ_MAX = 0xffffffff;
	constexpr unsigned int COLOR_SEQ_MIN = 0xffffffff;
	constexpr unsigned int COLOR_SEQ_INVX = 0xff00ffff;
	constexpr unsigned int COLOR_SEQ_TEN = 0xff50ff60;
	constexpr unsigned int COLOR_SEQ_TENA = 0xffffee50;
	constexpr unsigned int COLOR_SEQ_TENR = 0xffffB950;
	constexpr unsigned int COLOR_SEQ_SKEW = 0xffffB950;
	constexpr unsigned int COLOR_BEVEL = 0xff101010;

	constexpr int ANTICLICK_LOW_MILLIS = 5;
	constexpr int ANTICLICK_HIGH_MILLIS = 10;
	constexpr int AUDIO_LATENCY_MILLIS = 5;
	constexpr int AUDIO_COOLDOWN_MILLIS = 50;
	constexpr int AUDIO_DRUMSBUF_MILLIS = 20;
	constexpr int AUDIO_NOTE_LENGTH_MILLIS = 100;
	constexpr int MAX_UNDO = 100;

	// view consts
	constexpr int PLUG_WIDTH = 640;
	constexpr int PLUG_HEIGHT = 650;
	constexpr int MAX_PLUG_WIDTH = 640 * 3;
	constexpr int MAX_PLUG_HEIGHT = 650 * 2;
	constexpr int PLUG_PADDING = 15;
	constexpr int HOVER_RADIUS = 7;
	constexpr int POINT_RADIUS = 4;
	constexpr int MPOINT_RADIUS = 3;
	constexpr int MPOINT_HOVER_RADIUS = 5;
	constexpr int MSEL_PADDING = 8;

	// paint mode
	constexpr int PAINT_PATS_IDX = 100; // starting index of paint patterns, audio patterns always range 0..11
	constexpr int PAINT_PATS = 32;

	// sequencer
	constexpr int SEQ_PAT_IDX = 1000;
	constexpr int SEQ_MAX_CELLS = 32;
};