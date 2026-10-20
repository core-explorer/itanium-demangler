#include "HighlightedText.hxx"

namespace swartalf {

HighlightedText HighlightDecorator::decorate(Decoration deco, const HighlightedText& tn, unsigned level) {
	HighlightedText result;
	for (const auto& t : tn.sg) {
		result += decorate(deco, t, level);
	}
	return result;
}

HighlightedTextSegment HighlightDecorator::decorate(Decoration deco, std::string_view tn, unsigned level) {
	HighlightedTextSegment t;
	t.text += tn;
	return decorate(deco, t, level);
}

HighlightedTextSegment HighlightDecorator::decorate(Decoration deco, const std::string& tn, unsigned level) {
	HighlightedTextSegment t;
	t.text += tn;
	return decorate(deco, t, level);
}

Color256Cmy blend(const Color256Cmy& x, const Color256Cmy& z) {
	auto xp = ColorFloat(Color256(x));
	auto zp = ColorFloat(Color256(z));
	auto wp = xp * zp;
	auto w = Color256Cmy(Color256(wp));
	return w;
}

Highlight blend(const Highlight& hi, const Highlight& ho) {
	Highlight res;

	res.fg = blend(hi.fg, ho.fg);

	res.bg = blend(hi.bg, ho.bg);

	res.ud = blend(hi.ud, ho.ud);

	res.ul = UnderlineStyle(std::max(unsigned(hi.ul), unsigned(ho.ul)));

	res.bold = hi.bold or ho.bold;
	res.italic = hi.italic or ho.italic;
	res.alpha = hi.alpha * ho.alpha;
	return res;
}

HighlightedTextSegment HighlightDecorator::decorate(Decoration deco, const HighlightedTextSegment& ts, unsigned level) {
	(void)deco;
	(void)level;

	const auto& [tn, ihl] = ts;
	Highlight hl;

	if (deco == Decoration::function_name) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.1f, 0.1f, 0.5f));
		
	}
	if (deco == Decoration::pointer) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.2f, 0.1f, 0.2f));
		hl.bold = true;
	}
	if (deco == Decoration::reference) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.2f, 0.1f, 0.2f));
	}
	if (deco == Decoration::noise) {
		hl.alpha = 1.0f - 0.25f;
	}
	if (deco == Decoration::rvalue_reference) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.1f, 0.3f, 0.1f));
	}
	if (deco == Decoration::template_brackets) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.5f, 0.1f, 0.1f));
	}
	if (deco == Decoration::array_brackets) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.25f, 0.1f, 0.25f));
	}
	if (deco == Decoration::keyword) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.1f, 0.1f, 0.1f));
	}
	if (deco == Decoration::type_keyword) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.1f, 0.2f, 0.2f));
	}
	if (deco == Decoration::operator_symbol) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.1f, 0.2f, 0.1f));
	}
	if (deco == Decoration::const_keyword) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.2f, 0.2f, 0.2f));
	}
	if (deco == Decoration::call_parameter_even) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.2f, 0.0f, 0.0f));
	}
	if (deco == Decoration::call_parameter_odd) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.0f, 0.2f, 0.0f));
	}
	if (deco == Decoration::template_parameter_even) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.1f, 0.0f, 0.1f));
	}
	if (deco == Decoration::template_parameter_odd) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.0f, 0.1f, 0.1f));
	}
	if (deco == Decoration::call_parentheses) {
		hl.fg = Color256Cmy(ColorFloatCmy(0.5f, 0.1f, 0.1f));
		hl.bold = true;
	}
	if (deco == Decoration::generic_bold) {
		hl.bold = true;
	}
	if (deco == Decoration::generic_italic) {
		hl.italic = true;
	}

	auto ohl = blend(ihl, hl);
	return HighlightedTextSegment(tn, ohl);
}

float gammodrify(float f) {
	if (f < 0.5f) {
		float x = sqrtf(2.0f * f);
		return 0.5f * x;
	}
	float x = 2.0f * f - 1.0f;
	return 0.5f * x * x + 0.5f;
}

ColorFloat gammodrify(ColorFloat f) {
	ColorFloat z;
	z.r = gammodrify(f.r);
	z.g = gammodrify(f.g);
	z.b = gammodrify(f.b);
	return z;
}

enum class ColorTransform:uint8_t {
	rgb,
	rbg,
	brg,
	bgr,
	grb,
	gbr,
	cmy,
	cym,
	mcy,
	myc,
	ycm,
	ymc,

};

ColorFloat transform(ColorFloat x, ColorTransform mode) {
	float r = x.r;
	float g = x.g;
	float b = x.b;
	float c = 0.5f * (b + g);
	float m = 0.5f * (r + b);
	float y = 0.5f * (r + g);
	/*
	012
	021
	102
	120
	201
	210
	*/
	using enum ColorTransform;
	switch (mode) {
	case rgb:
		return {r, g, b};
	case rbg:
		return {r, b, g};
	case brg:
		return {b, r, g};
	case bgr:
		return {b, g, r};
	case grb:
		return {g, r, b};
	case gbr:
		return {g, b, r};

	case cmy:
		return {c, m, y};
	case cym:
		return {c, y, m};
	case mcy:
		return {m, c, y};
	case myc:
		return {m, y, c};
	case ycm:
		return {y, c, m};
	case ymc:
		return {y, m, c};
	default: {
		float gray = 0.299f * r + 0.587f * g + 0.114f * b;
		return {gray, gray, gray};
	}
	}
}

std::string render_cli(const HighlightedTextSegment& ht, bool light_mode) {
	//assume white fg, black bg
	const auto& [text, hl] = ht;
	std::string result;

	if (hl.bold) {
		result += "\033[1m";
	}
	if (hl.italic) {
		result += "\033[3m";
	}
	if (light_mode) {
		float a = 1.0f - hl.alpha;
		auto fg = Color256(ColorFloat(a, a, a));
		auto bg = Color256(hl.fg);
		result += fmt::format("\033[38;2;{};{};{}m", fg.r, fg.g, fg.b);

		result += fmt::format("\033[48;2;{};{};{}m", bg.r, bg.g, bg.b);
	} else {
		auto f = ColorFloat(Color256(hl.fg));
		f *= hl.alpha;
		auto fg = Color256(f);
		result += fmt::format("\033[38;2;{};{};{}m", fg.r, fg.g, fg.b);
	}
	result += text;
	if (light_mode) {
		result += "\033[39;49m";

	} else {
		result += "\033[39m";
	}

	if (hl.italic) {
		result += "\033[23m";
	}
	if (hl.bold) {
		result += "\033[22m";
	}

	return result;
}

std::string render_html(const HighlightedTextSegment& ht, bool light_mode) {
	const auto& [text, hl] = ht;
	std::string style = "style=\"";

	if (hl.bold) {
		style += "font-weight: bold;";
	}
	if (hl.italic) {
		style += "font-style: italic;";
	}

	if (light_mode) {
		float a = 1.0f - hl.alpha;
		auto fg = Color256(ColorFloat(a, a, a));
		auto bg = Color256(hl.fg);
		style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);
		style += fmt::format("background-color: rgb({},{},{});", bg.r, bg.g, bg.b);
	} else {
		auto f = ColorFloat(Color256(hl.fg));
		f *= hl.alpha;
		auto fg = Color256(f);
		style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);
	}

	style += "\"";

	return fmt::format("<span {}>{}</span>", style, escape_xml(text));
}

std::string render_cli_light(const HighlightedTextSegment& ht) {
	return render_cli(ht, true);
}
std::string render_cli_dark(const HighlightedTextSegment& ht) {
	return render_cli(ht, false);
}

std::string render_cli(const HighlightedText& ht, bool light_mode) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_cli(t, light_mode);
	}
	return result;
}

std::string render_html_light(const HighlightedTextSegment& ht) {
	return render_html(ht, true);
}
std::string render_html_dark(const HighlightedTextSegment& ht) {
	return render_html(ht, false);
}

std::string render_html(const HighlightedText& ht, bool light_mode) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_html(t, light_mode);
	}
	return result;
}

CompositeStack<std::string> render_cli(const CompositeStack<HighlightedText>& ht, bool light_mode) {
	CompositeStack<std::string> result;
	result.head = render_cli(ht.head, light_mode);
	result.tail = render_cli(ht.tail, light_mode);
	return result;
}

std::string render_cli_dark(const HighlightedText& ht) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_cli_dark(t);
	}
	return result;
}

std::string render_cli_light(const HighlightedText& ht) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_cli_light(t);
	}
	return result;
}
CompositeStack<std::string> render_html(const CompositeStack<HighlightedText>& ht, bool light_mode) {
	CompositeStack<std::string> result;
	result.head = render_html(ht.head, light_mode);
	result.tail = render_html(ht.tail, light_mode);
	return result;
}

std::string render_html_dark(const HighlightedText& ht) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_html_dark(t);
	}
	return result;
}

std::string render_html_light(const HighlightedText& ht) {
	std::string result;
	for (const auto& t : ht.sg) {
		result += render_html_light(t);
	}
	return result;
}
}