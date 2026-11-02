#pragma once
#include "CompositeStack.hxx"
#include "../cli/cli_color256.hxx"
#include "../util/BitFlagSet.hxx"
#include "../util/Decorator.hxx"
#include "../util/FlagSet.hxx"
#include "../util/HighlightTags.hxx"

namespace swartalf::itanium {

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

	static ColorFloat cmy(float c, float m, float y) {
		ColorFloat x;
		x.r = 1.0f - c;
		x.g = 1.0f - m;
		x.b = 1.0f - y;
		return x;
	}
};

struct AbstractColor {
	BitFlagSet<Decoration, uint32_t, 21> flags;
	float even_tp = 1.0f;
	float odd_tp = 1.0f;
	float even_fp = 1.0f;
	float odd_fp = 1.0f;
};

struct HighlightDark {
	std::string link;
	float alpha = 1.0f;

	Color256 fg = Color256::white();
	Color256 bg = Color256::white();
	Color256 ud = Color256::white();
	UnderlineStyle ul = UnderlineStyle::none;
	bool bold = false;
	bool italic = false;
	FlagSet<HighlightTags> tags;
};

struct HighlightLight {
	std::string link;
	float alpha = 1.0f;

	AbstractColor ac;

	UnderlineStyle ul = UnderlineStyle::none;
	bool bold = false;
	bool italic = false;
	FlagSet<HighlightTags> tags;
};

template <typename HighlightType>
struct HighlightedTextSegment {
	std::string text;
	HighlightType hl;
	using highlight_type = HighlightType;
	HighlightedTextSegment() = default;
	HighlightedTextSegment(std::string_view a) : text(a) {}
	HighlightedTextSegment(std::string_view a, HighlightType b) : text(a), hl(std::move(b)) {}

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

template <typename HighlightType>
struct HighlightedText {
	std::vector<HighlightedTextSegment<HighlightType>> sg;

	HighlightedText() = default;
	HighlightedText(const HighlightedTextSegment<HighlightType>& t) {
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
	HighlightedText& operator+=(const HighlightedTextSegment<HighlightType>& ts) {
		if (not ts.empty()) {
			sg.emplace_back(ts);
		}
		return *this;
	}
	HighlightedText& operator+=(std::string_view ts) {
		return *this += HighlightedTextSegment<HighlightType>(ts);
	}
	HighlightedText& operator+=(const std::string& ts) {
		return *this += HighlightedTextSegment<HighlightType>(ts);
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
template <typename HighlightType>
inline HighlightedText<HighlightType> operator+(const HighlightedTextSegment<HighlightType>& a, const HighlightedTextSegment<HighlightType>& b) {
	HighlightedText<HighlightType> c;
	c += a;
	c += b;
	return c;
}
template <typename HighlightType>
inline HighlightedText<HighlightType> operator+(const HighlightedTextSegment<HighlightType>& a, std::string_view b) {
	HighlightedText<HighlightType> c;
	c += a;
	c += b;
	return c;
}
template <typename HighlightType>
inline HighlightedText<HighlightType> operator+(std::string_view a, const HighlightedTextSegment<HighlightType>& b) {
	HighlightedText<HighlightType> c;
	c += a;
	c += b;
	return c;
}
template <typename HighlightType, size_t N>
inline HighlightedText<HighlightType> operator+(const HighlightedTextSegment<HighlightType>& a, const char (&b)[N]) {
	HighlightedText<HighlightType> c;
	c += a;
	c += b;
	return c;
}
template <typename HighlightType, size_t N>
inline HighlightedText<HighlightType> operator+(const char (&a)[N], const HighlightedTextSegment<HighlightType>& b) {
	HighlightedText<HighlightType> c;
	c += a;
	c += b;
	return c;
}
template <typename HighlightType>
struct HighlightDecorator {
	using value_type = HighlightedText<HighlightType>;
	HighlightedTextSegment<HighlightType> decorate(Decoration deco, std::string_view tn, unsigned level = 0);
	HighlightedTextSegment<HighlightType> decorate(Decoration deco, const std::string& tn, unsigned level = 0);

	HighlightedTextSegment<HighlightType> decorate(Decoration deco, const HighlightedTextSegment<HighlightType>& ts, unsigned level = 0);
	HighlightedText<HighlightType> decorate(Decoration deco, const HighlightedText<HighlightType>& tn, unsigned level = 0);

	template <size_t N>
	HighlightedTextSegment<HighlightType> decorate(Decoration deco, const char (&tn)[N], unsigned level = 0) {
		return decorate(deco, std::string_view(tn), level);
	}

	void tag(HighlightedTextSegment<HighlightType>& ts, HighlightTags htag) const {
		ts.hl.tags.insert(htag);
	}
	void tag(HighlightedText<HighlightType>& ts, HighlightTags htag) const {
		for (auto& sg : ts.sg) {
			tag(sg, htag);
		}
	}
};

template <typename HighlightType>
std::string render_cli(const HighlightedText<HighlightType>& ht);

template <typename HighlightType>
CompositeStack<std::string> render_cli(const CompositeStack<HighlightedText<HighlightType>>& ht);

template <typename HighlightType>
std::string render_html(const HighlightedText<HighlightType>& ht);

template <typename HighlightType>
std::string render_html(const HighlightedText<HighlightType>& ht, const Navigation& nav);

template <typename HighlightType>
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText<HighlightType>>& ht);
template <typename HighlightType>
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText<HighlightType>>& ht, const Navigation& nav);
template <typename E, typename HighlightType>
std::expected<std::string, E> render_cli(const std::expected<HighlightedText<HighlightType>, E>& ht) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_cli(*ht);
}

template <typename E, typename HighlightType>
std::expected<CompositeStack<std::string>, E> render_cli(const std::expected<CompositeStack<HighlightedText<HighlightType>>, E>& ht) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_cli(*ht);
}

template <typename E, typename HighlightType>
std::expected<std::string, E> render_html(const std::expected<HighlightedText<HighlightType>, E>& ht) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht);
}
template <typename E, typename HighlightType>
std::expected<std::string, E> render_html(const std::expected<HighlightedText<HighlightType>, E>& ht, const Navigation& nav) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht, nav);
}
template <typename E, typename HighlightType>
std::expected<CompositeStack<std::string>, E> render_html(const std::expected<CompositeStack<HighlightedText<HighlightType>>, E>& ht) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht);
}

template <typename E, typename HighlightType>
std::expected<CompositeStack<std::string>, E> render_html(const std::expected<CompositeStack<HighlightedText<HighlightType>>, E>& ht, const Navigation& nav) {
	if (not ht) {
		return std::unexpected(ht.error());
	}
	return render_html(*ht, nav);
}

template <typename HighlightType>
HighlightedText<HighlightType> HighlightDecorator<HighlightType>::decorate(Decoration deco, const HighlightedText<HighlightType>& tn, unsigned level) {
	HighlightedText<HighlightType> result;
	result.sg.reserve(tn.sg.size());
	for (const auto& t : tn.sg) {
		result += decorate(deco, t, level);
	}
	return result;
}

template <typename HighlightType>
HighlightedTextSegment<HighlightType> HighlightDecorator<HighlightType>::decorate(Decoration deco, std::string_view tn, unsigned level) {
	HighlightedTextSegment<HighlightType> t;
	t.text += tn;
	return decorate(deco, t, level);
}

template <typename HighlightType>
HighlightedTextSegment<HighlightType> HighlightDecorator<HighlightType>::decorate(Decoration deco, const std::string& tn, unsigned level) {
	HighlightedTextSegment<HighlightType> t;
	t.text += tn;
	return decorate(deco, t, level);
}

std::string render_cli(const HighlightedTextSegment<HighlightLight>& ht);
std::string render_cli(const HighlightedTextSegment<HighlightDark>& ht);

template <typename HighlightType>
std::string render_cli(const HighlightedText<HighlightType>& ht) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_cli(t);
	}
	return result;
}

template <typename HighlightType>
std::string render_html(const HighlightedText<HighlightType>& ht) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_html(t);
	}
	return result;
}

template <typename HighlightType>
std::string render_html(const HighlightedText<HighlightType>& ht, const Navigation& nav) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_html(t, nav);
	}
	return result;
}

template <typename HighlightType>
CompositeStack<std::string> render_cli(const CompositeStack<HighlightedText<HighlightType>>& ht) {
	CompositeStack<std::string> result;
	result.head = render_cli(ht.head);
	result.tail = render_cli(ht.tail);
	return result;
}

std::string render_html(const HighlightedTextSegment<HighlightLight>& ht);
std::string render_html(const HighlightedTextSegment<HighlightDark>& ht);
template <typename HighlightType>
std::string render_html(const HighlightedTextSegment<HighlightType>& ht, const Navigation& nav);

template <typename HighlightType>
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText<HighlightType>>& ht) {
	CompositeStack<std::string> result;
	result.head = render_html(ht.head);
	result.tail = render_html(ht.tail);
	return result;
}

template <typename HighlightType>
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText<HighlightType>>& ht, const Navigation& nav) {
	CompositeStack<std::string> result;
	result.head = render_html(ht.head, nav);
	result.tail = render_html(ht.tail, nav);
	return result;
}

using HighlightLightFormatter = Formatter<HighlightDecorator<HighlightLight>, AsciiEscaper, NopLinker<HighlightedText<HighlightLight>>>;
using HighlightDarkFormatter = Formatter<HighlightDecorator<HighlightDark>, AsciiEscaper, NopLinker<HighlightedText<HighlightDark>>>;

}