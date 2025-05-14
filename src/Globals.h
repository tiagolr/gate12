#pragma once

namespace globals {
	inline unsigned int COLOR_BG = 0xff181818;
	inline unsigned int COLOR_ACTIVE = 0xffff8050;
	inline unsigned int COLOR_ACTIVE_LIGHT = 0xffffb193;
	inline unsigned int COLOR_ACTIVE_DARK = 0xff88442b;
	inline unsigned int COLOR_NEUTRAL = 0xff666666;
	inline unsigned int COLOR_NEUTRAL_LIGHT = 0x99999999;
	inline unsigned int COLOR_SEEK = 0xff80ffff;
	inline unsigned int COLOR_KNOB = 0xff272727;
	inline unsigned int COLOR_AUDIO = 0xffffd42a;
	inline unsigned int COLOR_MIDI = 0xff50a9ff;
	inline unsigned int COLOR_SELECTION = 0xff50a9ff;
	inline unsigned int COLOR_SEQ_MAX = 0xffffffff;
	inline unsigned int COLOR_SEQ_MIN = 0xffffffff;
	inline unsigned int COLOR_SEQ_INVX = 0xff00ffff;
	inline unsigned int COLOR_SEQ_TEN = 0xff50ff60;
	inline unsigned int COLOR_SEQ_TENA = 0xffffee50;
	inline unsigned int COLOR_SEQ_TENR = 0xffffB950;

	inline const int LATENCY_MILLIS = 5;
	inline const int AUDIO_COOLDOWN_MILLIS = 50;
	inline const int AUDIO_DRUMSBUF_MILLIS = 20;
	inline const int AUDIO_NOTE_LENGTH_MILLIS = 100; 
	inline const int MAX_UNDO = 100;

	// view consts
	inline const int PLUG_WIDTH = 640;
	inline const int PLUG_HEIGHT = 650;
	inline const int MAX_PLUG_WIDTH = 640 * 3;
	inline const int MAX_PLUG_HEIGHT = 650 * 2;
	inline const int PLUG_PADDING = 15;
	inline const int HOVER_RADIUS = 8;
	inline const int POINT_RADIUS = 4;
	inline const int MPOINT_RADIUS = 3;
	inline const int MSEL_PADDING = 8;

	// paint mode
	inline const int PAINT_PATS_IDX = 100; // starting index of paint patterns, audio patterns always range 0..11
	inline const int PAINT_PATS = 32;

	// sequencer
	inline const int SEQ_PAT_IDX = 1000;
	inline const int SEQ_MAX_CELLS = 32;
};