#pragma once
#include "CompositeString.hxx"
#include "../cli/cli_color256.hxx"
#include "../util/Decorator.hxx"

namespace swartalf {

enum class UnderlineStyle : uint8_t {
	none,
	straight,
	twice,
	curvy,
	dotted,
	dashed
};

struct ColorFloat {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;

	constexpr ColorFloat() = default;
	constexpr ColorFloat(float red, float green, float blue) : r(red), g(green), b(blue) {}
	explicit constexpr ColorFloat(const Color256& x) {
		r = float(x.r) * (1.0f / 255.0f);
		g = float(x.g) * (1.0f / 255.0f);
		b = float(x.b) * (1.0f / 255.0f);
	}

	ColorFloat operator*=(float f) {
		r *= f;
		g *= f;
		b *= f;
		return *this;
	}

	friend constexpr ColorFloat operator*(const ColorFloat& x, const ColorFloat& z) {
		ColorFloat w;
		w.r = x.r * z.r;
		w.g = x.g * z.g;
		w.b = x.b * z.b;
		return w;
	}
	explicit constexpr operator Color256() const {
		Color256 c;
		c.r = std::min(255l, lrintf(r * 255.0f));
		c.g = std::min(255l, lrintf(g * 255.0f));
		c.b = std::min(255l, lrintf(b * 255.0f));
		return c;
	}
};

struct ColorFloatCmy {
	float c = 0.0f;
	float m = 0.0f;
	float y = 0.0f;

	constexpr ColorFloatCmy() = default;

	constexpr ColorFloatCmy(float cyan, float magenta, float yellow) : c(cyan), m(magenta), y(yellow) {}

	explicit constexpr ColorFloatCmy(const Color256Cmy& x) {
		c = float(x.c) * (1.0f / 255.0f);
		m = float(x.m) * (1.0f / 255.0f);
		y = float(x.y) * (1.0f / 255.0f);
	}

	friend constexpr ColorFloatCmy operator*(const ColorFloatCmy& a, const ColorFloatCmy& b) {
		ColorFloatCmy x;
		x.c = a.c * b.c;
		x.m = a.m * b.m;
		x.y = a.y * b.y;
		return x;
	}
	constexpr operator ColorFloat() const {
		ColorFloat x;
		x.r = 1.0f - c;
		x.g = 1.0f - m;
		x.b = 1.0f - y;
		return x;
	}

	explicit constexpr ColorFloatCmy(const ColorFloat& x) {
		c = 1.0f - x.r;
		m = 1.0f - x.g;
		y = 1.0f - x.b;
	}

	explicit constexpr operator Color256Cmy() const {
		Color256Cmy x;
		x.c = std::min(255l, lrintf(c * 255.0f));
		x.m = std::min(255l, lrintf(m * 255.0f));
		x.y = std::min(255l, lrintf(y * 255.0f));
		return x;
	}
};

enum class HighlightTags : uint8_t {
	none,
	odr
};

struct Highlight {
	std::string link;
	float alpha = 1.0f;

	Color256Cmy fg;
	Color256Cmy bg;
	Color256Cmy ud;
	UnderlineStyle ul = UnderlineStyle::none;
	bool bold = false;
	bool italic = false;
	uint8_t tags = 0;
};

struct HighlightedTextSegment {
	std::string text;
	Highlight hl;
	HighlightedTextSegment() = default;
	HighlightedTextSegment(std::string_view a) : text(a) {}
	HighlightedTextSegment(std::string_view a, Highlight b) : text(a), hl(std::move(b)) {}

	constexpr size_t size() const {
		return text.size();
	}
	constexpr bool empty() const {
		return text.empty();
	}

	bool ends_with(char c) const {
		return text.ends_with(c);
	}
	bool ends_with(std::string_view s) const {
		return text.ends_with(s);
	}

	bool starts_with(char c) const {
		return text.starts_with(c);
	}
	bool starts_with(std::string_view s) const {
		return text.starts_with(s);
	}
};

struct HighlightedText {
	std::vector<HighlightedTextSegment> sg;

	HighlightedText() = default;
	HighlightedText(const HighlightedTextSegment& t) {
		sg.emplace_back(t);
	}
	HighlightedText(const std::string& t) {
		*this += t;
	}
	HighlightedText(std::string_view t) {
		*this += t;
	}
	HighlightedText(const HighlightedText&) = default;
	HighlightedText(HighlightedText&&) noexcept = default;

	HighlightedText& operator=(const HighlightedText&) = default;
	HighlightedText& operator=(HighlightedText&&) noexcept = default;

	HighlightedText& operator=(std::string_view t) {
		sg.clear();
		*this += t;
		return *this;
	}

	constexpr size_t size() const {
		size_t r = 0;
		for (const auto& t : sg) {
			r += t.size();
		}
		return r;
	}

	bool empty() const {
		return sg.empty();
	}

	std::string collapse() const {
		std::string r;
		for (const auto& t : sg) {
			r += t.text;
		}
		return r;
	}
	HighlightedText& operator+=(const HighlightedTextSegment& ts) {
		if (not ts.empty()) {
			sg.emplace_back(ts);
		}
		return *this;
	}
	HighlightedText& operator+=(std::string_view ts) {
		return *this += HighlightedTextSegment(ts);
	}
	HighlightedText& operator+=(const std::string& ts) {
		return *this += HighlightedTextSegment(ts);
	}
	template <size_t N>
	HighlightedText& operator+=(const char (&ts)[N]) {
		std::string_view sv(ts, N);
		return *this += sv;
	}

	HighlightedText& operator+=(char c) {
		char a[1] = {c};
		std::string_view sv(a, 1);
		return *this += sv;
	}
	HighlightedText& operator+=(const HighlightedText& t) {
		sg.insert(sg.end(), t.sg.begin(), t.sg.end());
		return *this;
	}
	friend HighlightedText operator+(const HighlightedText& t, const HighlightedText& s) {
		auto a = t;
		a += s;
		return a;
	}
	friend HighlightedText operator+(std::string_view t, const HighlightedText& s) {
		HighlightedText a;
		a += t;
		a += s;
		return a;
	}
	friend HighlightedText operator+(const HighlightedText& t, std::string_view s) {
		auto a = t;
		a += s;
		return a;
	}
	friend HighlightedText operator+(const std::string& t, const HighlightedText& s) {
		HighlightedText a;
		a += t;
		a += s;
		return a;
	}
	friend HighlightedText operator+(const HighlightedText& t, const std::string& s) {
		auto a = t;
		a += s;
		return a;
	}

	template <size_t N>
	friend HighlightedText operator+(const char (&t)[N], const HighlightedText& s) {
		HighlightedText a;
		a += t;
		a += s;
		return a;
	}

	template <size_t N>
	friend HighlightedText operator+(const HighlightedText& t, const char (&s)[N]) {
		auto a = t;
		a += s;
		return a;
	}

	bool ends_with(char c) const {
		if (empty()) {
			return false;
		}
		return sg.back().ends_with(c);
	}
	bool ends_with(std::string_view s) const {
		if (empty()) {
			return false;
		}
		return sg.back().ends_with(s);
	}

	bool starts_with(char c) const {
		if (empty()) {
			return false;
		}
		return sg.front().starts_with(c);
	}
	bool starts_with(std::string_view s) const {
		if (empty()) {
			return false;
		}
		return sg.front().starts_with(s);
	}

	friend bool operator==(const HighlightedText& t, std::string_view sv) {
		if (t.sg.empty()) {
			return sv.empty();
		}
		if (t.sg.size() == 1) {
			return t.sg[0].text == sv;
		}
		return t.collapse() == sv;
	}

	void clear() {
		sg.clear();
	}
};

inline HighlightedText operator+(const HighlightedTextSegment& a, const HighlightedTextSegment& b) {
	HighlightedText c;
	c += a;
	c += b;
	return c;
}

inline HighlightedText operator+(const HighlightedTextSegment& a, std::string_view b) {
	HighlightedText c;
	c += a;
	c += b;
	return c;
}
inline HighlightedText operator+(std::string_view a, const HighlightedTextSegment& b) {
	HighlightedText c;
	c += a;
	c += b;
	return c;
}
template <size_t N>
inline HighlightedText operator+(const HighlightedTextSegment& a, const char (&b)[N]) {
	HighlightedText c;
	c += a;
	c += b;
	return c;
}
template <size_t N>
inline HighlightedText operator+(const char (&a)[N], const HighlightedTextSegment& b) {
	HighlightedText c;
	c += a;
	c += b;
	return c;
}

struct HighlightDecorator {
	using value_type = HighlightedText;
	HighlightedTextSegment decorate(Decoration deco, std::string_view tn, unsigned level = 0);
	HighlightedTextSegment decorate(Decoration deco, const std::string& tn, unsigned level = 0);

	HighlightedTextSegment decorate(Decoration deco, const HighlightedTextSegment& ts, unsigned level = 0);
	HighlightedText decorate(Decoration deco, const HighlightedText& tn, unsigned level = 0);

	template <size_t N>
	HighlightedTextSegment decorate(Decoration deco, const char (&tn)[N], unsigned level = 0) {
		return decorate(deco, std::string_view(tn), level);
	}
};

std::string render_cli(const HighlightedText& ht, bool light_mode);
CompositeStack<std::string> render_cli(const CompositeStack<HighlightedText>& ht, bool light_mode);

std::string render_html(const HighlightedText& ht, bool light_mode);
std::string render_html(const HighlightedText& ht, bool light_mode, const Navigation& nav);
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText>& ht, bool light_mode);
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText>& ht, bool light_mode, const Navigation& nav);
template <typename E>
std::expected<std::string, E> render_cli(const std::expected<HighlightedText, E>& ht, bool light_mode) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_cli(*ht, light_mode);
}

template <typename E>
std::expected<CompositeStack<std::string>, E> render_cli(const std::expected<CompositeStack<HighlightedText>, E>& ht, bool light_mode) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_cli(*ht, light_mode);
}

template <typename E>
std::expected<std::string, E> render_html(const std::expected<HighlightedText, E>& ht, bool light_mode) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht, light_mode);
}
template <typename E>
std::expected<std::string, E> render_html(const std::expected<HighlightedText, E>& ht, bool light_mode, const Navigation& nav) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht, light_mode, nav);
}
template <typename E>
std::expected<CompositeStack<std::string>, E> render_html(const std::expected<CompositeStack<HighlightedText>, E>& ht, bool light_mode) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht, light_mode);
}

template <typename E>
std::expected<CompositeStack<std::string>, E> render_html(const std::expected<CompositeStack<HighlightedText>, E>& ht, bool light_mode, const Navigation& nav) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht, light_mode, nav);
}
using HighlightFormatter = Formatter<HighlightDecorator, AsciiEscaper, NopLinker<HighlightedText>>;

}
