#pragma once
#include "pch.hpp"

static inline std::map<int, int> keyboard2midi = {
	{ 90,  0  }, { 188, 12 }, { 81,  12 + 0  }, { 73,  12 + 12 }, // C
	{ 83,  1  }, { 76,  13 }, { 50,  12 + 1  }, { 57,  12 + 13 }, // C#
	{ 88,  2  }, { 190, 14 }, { 87,  12 + 2  }, { 79,  12 + 14 }, // D
	{ 68,  3  }, { 186, 15 }, { 51,  12 + 3  }, { 48,  12 + 15 }, // D#
	{ 67,  4  }, { 191, 16 }, { 69,  12 + 4  }, { 80,  12 + 16 }, // E
	{ 86,  5  },              { 82,  12 + 5  }, { 219, 12 + 17 }, // F
	{ 71,  6  },              { 53,  12 + 6  }, { 187, 12 + 18 }, // F#
	{ 66,  7  },              { 84,  12 + 7  }, { 221, 12 + 19 }, // G
	{ 72,  8  },              { 54,  12 + 8  },                   // G#
	{ 78,  9  },              { 89,  12 + 9  },                   // A
	{ 74,  10 },              { 55,  12 + 10 },                   // A#
	{ 77,  11 },              { 85,  12 + 11 },                   // B
};

inline float noteToFreq(int note) { return (440. / 32.) * pow(2, ((note - 9) / 12.0)); }

using Sample = float;
using Channel = int;

#define db2lin(db) std::powf(10.0f, 0.05 * (db))
#define lin2db(lin) (20.0f * std::log10f(static_cast<float>(lin)))
