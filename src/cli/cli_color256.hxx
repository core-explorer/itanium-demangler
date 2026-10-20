#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace swartalf {
struct Color256 {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	constexpr Color256() = default;
	constexpr Color256(uint8_t red, uint8_t green, uint8_t blue) :
	    r(red), g(green), b(blue) {}

	constexpr bool operator==(const Color256& other) const = default;
};

struct Color256Cmy {
	uint8_t c = 0;
	uint8_t m = 0;
	uint8_t y = 0;
	constexpr Color256Cmy() = default;
	constexpr Color256Cmy(uint8_t cyan, uint8_t magenta, uint8_t yellow) :
	    c(cyan), m(magenta), y(yellow) {}

	explicit Color256Cmy(Color256 x) {
		c = 255 - x.r;
		m = 255 - x.g;
		y = 255 - x.b;
	}

	explicit operator Color256() const {
		Color256 x;
		x.r = 255 - c;
		x.g = 255 - m;
		x.b = 255 - y;
		return x;
	}

	constexpr bool operator==(const Color256Cmy& other) const = default;
};

constexpr int distl1(const Color256& a, const Color256& b) {
	return std::abs(int(a.r) - int(b.r)) + std::abs(int(a.g) - int(b.g)) + std::abs(int(a.b) - int(b.b));
}

constexpr int distl2sq(const Color256& a, const Color256& b) {
	int dr = int(a.r) - int(b.r);
	int dg = int(a.g) - int(b.g);
	int db = int(a.b) - int(b.b);
	return dr * dr + dg * dg + db * db;
}

constexpr uint8_t hexval(char c) {
	if ('0' <= c && c <= '9') {
		return c - '0';
	} else if ('a' <= c && c <= 'f') {
		return c - 'a' + 10;
	} else if ('A' <= c && c <= 'F') {
		return c - 'A' + 10;
	}
	return 0;
}

constexpr Color256 operator""_rgb(const char* xstr, std::size_t) {
	Color256 value;
	auto str = xstr + 1;
	
	value.r = hexval(str[0]) * 16 + hexval(str[1]);
	value.g = hexval(str[2]) * 16 + hexval(str[3]);
	value.b = hexval(str[4]) * 16 + hexval(str[5]);

	return value;
}

constexpr std::array<Color256, 16> make_color16(uint8_t val) {
	std::array<Color256, 16> c;
	c[0] = {"#000000"_rgb};
	c[1].r = val; 
	c[2].g = val; 
	c[3].r = val;
	c[3].g = val; 
	c[4].b = val; 
	c[5].r = val;
	c[5].b = val;
	c[6].g = val;
	c[6].b = val;
	c[7] = {"#c0c0c0"_rgb};
	
	c[8] = {"#808080"_rgb};
	c[9] = {"#ff0000"_rgb};
	c[10] = {"#00ff00"_rgb};
	c[11] = {"#ffff00"_rgb};
	c[12] = {"#0000ff"_rgb};
	c[13] = {"#ff00ff"_rgb};
	c[14] = {"#00ffff"_rgb};
	c[15] = {"#ffffff"_rgb};
	return c;
}

constexpr std::array<Color256, 256> make_color256(uint8_t val = 0xCD) {
	std::array<Color256, 16> d = make_color16(val);
	std::array<Color256, 256> c;
	for (std::size_t i = 0; i < 16; i++) {
		c[i] = d[i];
	}
	c[16] = {"#000000"_rgb};
	c[17] = {"#00005f"_rgb};
	c[18] = {"#000087"_rgb};
	c[19] = {"#0000af"_rgb};
	c[20] = "#0000d7"_rgb;
	c[21] = "#0000ff"_rgb;
	c[22] = "#005f00"_rgb;
	c[23] = "#005f5f"_rgb;
	c[24] = "#005f87"_rgb;
	c[25] = "#005faf"_rgb;
	c[26] = "#005fd7"_rgb;
	c[27] = "#005fff"_rgb;
	c[28] = "#008700"_rgb;
	c[29] = "#00875f"_rgb;
	c[30] = "#008787"_rgb;
	c[31] = "#0087af"_rgb;
	c[32] = "#0087d7"_rgb;
	c[33] = "#0087ff"_rgb;
	c[34] = "#00af00"_rgb;
	c[35] = "#00af5f"_rgb;
	c[36] = "#00af87"_rgb;
	c[37] = "#00afaf"_rgb;
	c[38] = "#00afd7"_rgb;
	c[39] = "#00afff"_rgb;
	c[40] = "#00d700"_rgb;
	c[41] = "#00d75f"_rgb;
	c[42] = "#00d787"_rgb;
	c[43] = "#00d7af"_rgb;
	c[44] = "#00d7d7"_rgb;
	c[45] = "#00d7ff"_rgb;
	c[46] = "#00ff00"_rgb;
	c[47] = "#00ff5f"_rgb;
	c[48] = "#00ff87"_rgb;
	c[49] = "#00ffaf"_rgb;
	c[50] = "#00ffd7"_rgb;
	c[51] = "#00ffff"_rgb;
	c[52] = "#5f0000"_rgb;
	c[53] = "#5f005f"_rgb;
	c[54] = "#5f0087"_rgb;
	c[55] = "#5f00af"_rgb;
	c[56] = "#5f00d7"_rgb;
	c[57] = "#5f00ff"_rgb;
	c[58] = "#5f5f00"_rgb;
	c[59] = "#5f5f5f"_rgb;
	c[60] = "#5f5f87"_rgb;
	c[61] = "#5f5faf"_rgb;
	c[62] = "#5f5fd7"_rgb;
	c[63] = "#5f5fff"_rgb;
	c[64] = "#5f8700"_rgb;
	c[65] = "#5f875f"_rgb;
	c[66] = "#5f8787"_rgb;
	c[67] = "#5f87af"_rgb;
	c[68] = "#5f87d7"_rgb;
	c[69] = "#5f87ff"_rgb;
	c[70] = "#5faf00"_rgb;
	c[71] = "#5faf5f"_rgb;
	c[72] = "#5faf87"_rgb;
	c[73] = "#5fafaf"_rgb;
	c[74] = "#5fafd7"_rgb;
	c[75] = "#5fafff"_rgb;
	c[76] = "#5fd700"_rgb;
	c[77] = "#5fd75f"_rgb;
	c[78] = "#5fd787"_rgb;
	c[79] = "#5fd7af"_rgb;
	c[80] = "#5fd7d7"_rgb;
	c[81] = "#5fd7ff"_rgb;
	c[82] = "#5fff00"_rgb;
	c[83] = "#5fff5f"_rgb;
	c[84] = "#5fff87"_rgb;
	c[85] = "#5fffaf"_rgb;
	c[86] = "#5fffd7"_rgb;
	c[87] = "#5fffff"_rgb;
	c[88] = "#870000"_rgb;
	c[89] = "#87005f"_rgb;
	c[90] = "#870087"_rgb;
	c[91] = "#8700af"_rgb;
	c[92] = "#8700d7"_rgb;
	c[93] = "#8700ff"_rgb;
	c[94] = "#875f00"_rgb;
	c[95] = "#875f5f"_rgb;
	c[96] = "#875f87"_rgb;
	c[97] = "#875faf"_rgb;
	c[98] = "#875fd7"_rgb;
	c[99] = "#875fff"_rgb;
	c[100] = "#878700"_rgb;
	c[101] = "#87875f"_rgb;
	c[102] = "#878787"_rgb;
	c[103] = "#8787af"_rgb;
	c[104] = "#8787d7"_rgb;
	c[105] = "#8787ff"_rgb;
	c[106] = "#87af00"_rgb;
	c[107] = "#87af5f"_rgb;
	c[108] = "#87af87"_rgb;
	c[109] = "#87afaf"_rgb;
	c[110] = "#87afd7"_rgb;
	c[111] = "#87afff"_rgb;
	c[112] = "#87d700"_rgb;
	c[113] = "#87d75f"_rgb;
	c[114] = "#87d787"_rgb;
	c[115] = "#87d7af"_rgb;
	c[116] = "#87d7d7"_rgb;
	c[117] = "#87d7ff"_rgb;
	c[118] = "#87ff00"_rgb;
	c[119] = "#87ff5f"_rgb;
	c[120] = "#87ff87"_rgb;
	c[121] = "#87ffaf"_rgb;
	c[122] = "#87ffd7"_rgb;
	c[123] = "#87ffff"_rgb;
	c[124] = "#af0000"_rgb;
	c[125] = "#af005f"_rgb;
	c[126] = "#af0087"_rgb;
	c[127] = "#af00af"_rgb;
	c[128] = "#af00d7"_rgb;
	c[129] = "#af00ff"_rgb;
	c[130] = "#af5f00"_rgb;
	c[131] = "#af5f5f"_rgb;
	c[132] = "#af5f87"_rgb;
	c[133] = "#af5faf"_rgb;
	c[134] = "#af5fd7"_rgb;
	c[135] = "#af5fff"_rgb;
	c[136] = "#af8700"_rgb;
	c[137] = "#af875f"_rgb;
	c[138] = "#af8787"_rgb;
	c[139] = "#af87af"_rgb;
	c[140] = "#af87d7"_rgb;
	c[141] = "#af87ff"_rgb;
	c[142] = "#afaf00"_rgb;
	c[143] = "#afaf5f"_rgb;
	c[144] = "#afaf87"_rgb;
	c[145] = "#afafaf"_rgb;
	c[146] = "#afafd7"_rgb;
	c[147] = "#afafff"_rgb;
	c[148] = "#afd700"_rgb;
	c[149] = "#afd75f"_rgb;
	c[150] = "#afd787"_rgb;
	c[151] = "#afd7af"_rgb;
	c[152] = "#afd7d7"_rgb;
	c[153] = "#afd7ff"_rgb;
	c[154] = "#afff00"_rgb;
	c[155] = "#afff5f"_rgb;
	c[156] = "#afff87"_rgb;
	c[157] = "#afffaf"_rgb;
	c[158] = "#afffd7"_rgb;
	c[159] = "#afffff"_rgb;
	c[160] = "#d70000"_rgb;
	c[161] = "#d7005f"_rgb;
	c[162] = "#d70087"_rgb;
	c[163] = "#d700af"_rgb;
	c[164] = "#d700d7"_rgb;
	c[165] = "#d700ff"_rgb;
	c[166] = "#d75f00"_rgb;
	c[167] = "#d75f5f"_rgb;
	c[168] = "#d75f87"_rgb;
	c[169] = "#d75faf"_rgb;
	c[170] = "#d75fd7"_rgb;
	c[171] = "#d75fff"_rgb;
	c[172] = "#d78700"_rgb;
	c[173] = "#d7875f"_rgb;
	c[174] = "#d78787"_rgb;
	c[175] = "#d787af"_rgb;
	c[176] = "#d787d7"_rgb;
	c[177] = "#d787ff"_rgb;
	c[178] = "#d7af00"_rgb;
	c[179] = "#d7af5f"_rgb;
	c[180] = "#d7af87"_rgb;
	c[181] = "#d7afaf"_rgb;
	c[182] = "#d7afd7"_rgb;
	c[183] = "#d7afff"_rgb;
	c[184] = "#d7d700"_rgb;
	c[185] = "#d7d75f"_rgb;
	c[186] = "#d7d787"_rgb;
	c[187] = "#d7d7af"_rgb;
	c[188] = "#d7d7d7"_rgb;
	c[189] = "#d7d7ff"_rgb;
	c[190] = "#d7ff00"_rgb;
	c[191] = "#d7ff5f"_rgb;
	c[192] = "#d7ff87"_rgb;
	c[193] = "#d7ffaf"_rgb;
	c[194] = "#d7ffd7"_rgb;
	c[195] = "#d7ffff"_rgb;
	c[196] = "#ff0000"_rgb;
	c[197] = "#ff005f"_rgb;
	c[198] = "#ff0087"_rgb;
	c[199] = "#ff00af"_rgb;
	c[200] = "#ff00d7"_rgb;
	c[201] = "#ff00ff"_rgb;
	c[202] = "#ff5f00"_rgb;
	c[203] = "#ff5f5f"_rgb;
	c[204] = "#ff5f87"_rgb;
	c[205] = "#ff5faf"_rgb;
	c[206] = "#ff5fd7"_rgb;
	c[207] = "#ff5fff"_rgb;
	c[208] = "#ff8700"_rgb;
	c[209] = "#ff875f"_rgb;
	c[210] = "#ff8787"_rgb;
	c[211] = "#ff87af"_rgb;
	c[212] = "#ff87d7"_rgb;
	c[213] = "#ff87ff"_rgb;
	c[214] = "#ffaf00"_rgb;
	c[215] = "#ffaf5f"_rgb;
	c[216] = "#ffaf87"_rgb;
	c[217] = "#ffafaf"_rgb;
	c[218] = "#ffafd7"_rgb;
	c[219] = "#ffafff"_rgb;
	c[220] = "#ffd700"_rgb;
	c[221] = "#ffd75f"_rgb;
	c[222] = "#ffd787"_rgb;
	c[223] = "#ffd7af"_rgb;
	c[224] = "#ffd7d7"_rgb;
	c[225] = "#ffd7ff"_rgb;
	c[226] = "#ffff00"_rgb;
	c[227] = "#ffff5f"_rgb;
	c[228] = "#ffff87"_rgb;
	c[229] = "#ffffaf"_rgb;
	c[230] = "#ffffd7"_rgb;
	c[231] = "#ffffff"_rgb;
	c[232] = "#080808"_rgb;
	c[233] = "#121212"_rgb;
	c[234] = "#1c1c1c"_rgb;
	c[235] = "#262626"_rgb;
	c[236] = "#303030"_rgb;
	c[237] = "#3a3a3a"_rgb;
	c[238] = "#444444"_rgb;
	c[239] = "#4e4e4e"_rgb;
	c[240] = "#585858"_rgb;
	c[241] = "#606060"_rgb;
	c[242] = "#666666"_rgb;
	c[243] = "#767676"_rgb;
	c[244] = "#808080"_rgb;
	c[245] = "#8a8a8a"_rgb;
	c[246] = "#949494"_rgb;
	c[247] = "#9e9e9e"_rgb;
	c[248] = "#a8a8a8"_rgb;
	c[249] = "#b2b2b2"_rgb;
	c[250] = "#bcbcbc"_rgb;
	c[251] = "#c6c6c6"_rgb;
	c[252] = "#d0d0d0"_rgb;
	c[253] = "#dadada"_rgb;
	c[254] = "#e4e4e4"_rgb;
	c[255] = "#eeeeee"_rgb;
	return c;
}


inline unsigned bestMatchl1(Color256 c, std::span<const Color256> lut) {
	unsigned best = 0;

	unsigned best_dist = distl1(c, lut[0]);

	for (unsigned i = 0; i < lut.size(); i++) {
		
		unsigned dist = distl1(c, lut[i]);
		if (dist < best_dist) {
			best = i;
			best_dist = dist;
		}
	}
	
	return best;
}

constexpr unsigned bestMatch(Color256 c, std::span<const Color256> lut) {
	unsigned best = 0;
	unsigned best_dist = distl2sq(c, lut[0]);
	for (unsigned i = 1; i < lut.size(); i++) {
		unsigned dist = distl2sq(c, lut[i]);
		if (dist < best_dist) {
			best = i;
			best_dist = dist;
		}
	}
	return best;
}

inline constexpr std::array<Color256, 256> color256 = make_color256();

inline constexpr std::array<char, 4> make_array(const char (&arr)[4]) {
	return {arr[0], arr[1], arr[2], arr[3]};
}

inline constexpr std::array<std::array<char, 4>, 8> bottom_to_top = {
    make_array("\u2581"), // Lower one eighth block
    make_array("\u2582"), // Lower one quarter block
    make_array("\u2583"), // Lower three eighths block
    make_array("\u2584"), // Lower half block
    make_array("\u2585"), // Lower five eighths block
    make_array("\u2586"), // Lower three quarters block
    make_array("\u2587"), // Lower seven eighths block
    make_array("\u2588") // Full block
};

inline constexpr std::array<std::array<char, 4>, 8> left_to_right = {
    make_array("\u258F"), // Left seven eighths block
    make_array("\u258E"), // Left three quarters block
    make_array("\u258D"), // Left five eighths block
    make_array("\u258C"), // Left half block
    make_array("\u258B"), // Left three eighths block
    make_array("\u258A"), // Left one quarter block
    make_array("\u2589"), // Left one eighth block
    make_array("\u2588") // Full block
};

inline constexpr std::array<std::array<char, 4>, 8> quadrants = {
    make_array("\u2596"), // Quadrant lower left
    make_array("\u2597"), // Quadrant lower right
    make_array("\u2598"), // Quadrant upper left
    make_array("\u2599"), // Quadrant upper left and lower left and lower right
    make_array("\u259A"), // Quadrant upper left and lower right
    make_array("\u259B"), // Quadrant upper left and upper right and lower left
    make_array("\u259C"), // Quadrant upper left and upper right and lower right
    make_array("\u259D") // Quadrant upper right
};

enum class StaticOverlay {
	none,
	code,
	rwdata,
	rodata,
	writable_code,
	noload,
};

enum class DynamicOverlay {
	none,
	stack,
	thread,
	arena,
	allocation,
	freed,
	stdlib
};

enum class OuterOverlay {
	none,
	code,
	rwdata,
	rodata,
	writable_code,
	noload,

	stack,
	thread,
	arena,
	allocation,
	freed,
	stdlib,
	cie,
	fde
};

enum class InnerOverlay {
	none,
	struct_,
	rtti,
	vtable,
	function,
	note,
	string
};

enum class OuterOrder {
	even,
	odd
};

enum class InnerOrder {
	even,
	odd
};

enum class Element {
	none,
	offset,
	address,
	label,
	symbol,
	type,
	file,
	string,
	number,
	size,
	bytes,
	type_name,
	enum_,
	expression,
	padding,
	mnemonic,
	operands,
};

struct ColorConfig {
	OuterOverlay outer_overlay = OuterOverlay::none;
	InnerOverlay inner_overlay = InnerOverlay::none;
	OuterOrder outer_order = OuterOrder::even;
	InnerOrder inner_order = InnerOrder::even;
};
struct TextElement {
	std::string text;
	Element element;
	unsigned pad_width = 0;
	bool stretch = false;
	TextElement() = default;
	TextElement(const std::string& t, Element e = Element::none, unsigned w = 0) :
	    text(t), element(e), pad_width(w) {}
	TextElement(std::string&& t, Element e = Element::none, unsigned w = 0) :
	    text(std::move(t)), element(e), pad_width(w) {}
};

constexpr Color256 blend(const Color256& original, const Color256& overlay, uint8_t alpha) {
	auto blend_channel = [alpha](uint8_t orig, uint8_t over) {
		return static_cast<uint8_t>((alpha * over + (128 - alpha) * orig) >> 7);
	};
	return Color256 {
	    blend_channel(original.r, overlay.r),
	    blend_channel(original.g, overlay.g),
	    blend_channel(original.b, overlay.b)};
}

struct TextLine {
	std::vector<TextElement> elements;
	template <typename... Args>
	void emplace_back(Args&&... args) {
		elements.emplace_back(std::forward<Args>(args)...);
	}
	auto begin() {
		return elements.begin();
	}
	auto end() {
		return elements.end();
	}
	auto begin() const {
		return elements.begin();
	}
	auto end() const {
		return elements.end();
	}

	TextLine() = default;
	TextLine(const TextLine&) = default;
	TextLine(TextLine&&) = default;
	TextLine(TextElement&& e) {
		elements.emplace_back(std::move(e));
	}
	TextLine(const std::string& s) {
		elements.emplace_back(s);
	}
	TextLine(std::string&& s) {
		elements.emplace_back(std::move(s));
	}
};

struct TextBlock {
	std::vector<TextLine> lines;
	template <typename... Args>
	void emplace_back(Args&&... args) {
		lines.emplace_back(std::forward<Args>(args)...);
	}
	auto begin() {
		return lines.begin();
	}
	auto end() {
		return lines.end();
	}
	auto begin() const {
		return lines.begin();
	}
	auto end() const {
		return lines.end();
	}

	void clear() {
		lines.clear();
	}
};

std::string format_element(const TextElement& te0, const ColorConfig& config);
inline std::string format_element(std::string_view text, Element element, const ColorConfig& config = ColorConfig()) {
	return format_element(TextElement(std::string(text), element), config);
}

inline std::string format_cli(const TextElement& te0, TextElement& te1, const ColorConfig& config) {
	return format_element(te0, config) + format_element(te1, config);
}

inline std::string format_cli(const TextElement& te_offset, const TextElement& te0, TextElement& te1, const ColorConfig& config) {
	return format_element(te_offset, config) + format_element(te0, config) + format_element(te1, config);
}

inline std::string format_cli(const TextElement& te_offset, const TextElement& te_bytes, const TextElement& te0, TextElement& te1, const ColorConfig& config) {
	return format_element(te_offset, config) + format_element(te_bytes, config) + format_element(te0, config) + format_element(te1, config);
}

void print(const TextBlock& block, ColorConfig config);

void print(const TextLine& line, ColorConfig config);
void align(TextBlock& block);

inline Color256 fblend(const Color256& original, const Color256& overlay, float alpha) {
	Color256 result;
	float opp = 1.0f - alpha;
	result.r = std::rintf(alpha * float(int(original.r)) + opp * float(int(overlay.r)));
	result.g = std::rintf(alpha * float(int(original.g)) + opp * float(int(overlay.g)));
	result.b = std::rintf(alpha * float(int(original.b)) + opp * float(int(overlay.b)));
	return result;
}
std::string block_dither256(Color256 c);

std::string foreground_color(Color256 current);
std::string foreground_color16(Color256 current);
std::string foreground_color256(Color256 current);
std::string foreground_color_rgb(Color256 current);
std::string background_color_rgb(Color256 current);
std::string fgbgcolor_rgb(Color256 fg, Color256 bg);

}