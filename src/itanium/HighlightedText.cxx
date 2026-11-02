#include "HighlightedText.hxx"

namespace swartalf::itanium {

Color256 mblend(const Color256& x, const Color256& z) {
	auto xp = ColorFloat(x);
	auto zp = ColorFloat(z);
	auto wp = xp * zp;
	auto w = Color256(wp);
	return w;
}

float gblend(float hi, float ho) {
	return sqrtf(hi * ho);
}

AbstractColor blend(const AbstractColor& hi, const AbstractColor& ho) {
	AbstractColor res;
	res.flags = hi.flags | ho.flags;
	res.even_fp = gblend(hi.even_fp, ho.even_fp);
	res.odd_fp = gblend(hi.odd_fp, ho.odd_fp);
	res.even_tp = gblend(hi.even_tp, ho.even_tp);
	res.odd_tp = gblend(hi.odd_tp, ho.odd_tp);
	return res;
}

HighlightLight blend(const HighlightLight& hi, const HighlightLight& ho) {
	HighlightLight res;

	res.ul = UnderlineStyle(std::max(unsigned(hi.ul), unsigned(ho.ul)));

	res.bold = hi.bold or ho.bold;
	res.italic = hi.italic or ho.italic;
	res.alpha = hi.alpha * ho.alpha;
	res.link = hi.link;
	res.tags = hi.tags | ho.tags;

	res.ac = blend(hi.ac, ho.ac);
	return res;
}

HighlightDark blend(const HighlightDark& hi, const HighlightDark& ho) {
	HighlightDark res;

	res.fg = mblend(hi.fg, ho.fg);
	res.bg = mblend(hi.bg, ho.bg);
	res.ud = mblend(hi.ud, ho.ud);
	res.ul = UnderlineStyle(std::max(unsigned(hi.ul), unsigned(ho.ul)));

	res.bold = hi.bold or ho.bold;
	res.italic = hi.italic or ho.italic;
	res.alpha = hi.alpha * ho.alpha;
	res.link = hi.link;
	res.tags = hi.tags | ho.tags;

	return res;
}

template <>
HighlightedTextSegment<HighlightDark> HighlightDecorator<HighlightDark>::decorate(Decoration deco, const HighlightedTextSegment<HighlightDark>& ts, unsigned level) {
	(void)level;

	const auto& [tn, ihl] = ts;
	HighlightDark hl;

	if (deco == Decoration::function_name) {
		hl.fg = Color256(ColorFloat::cmy(0.1f, 0.1f, 0.5f));
	}
	if (deco == Decoration::pointer) {
		hl.fg = Color256(ColorFloat::cmy(0.2f, 0.1f, 0.2f));
		hl.bold = true;
	}
	if (deco == Decoration::reference) {
		hl.fg = Color256(ColorFloat::cmy(0.2f, 0.1f, 0.2f));
	}
	if (deco == Decoration::noise) {
		hl.alpha = 1.0f - 0.25f;
	}
	if (deco == Decoration::rvalue_reference) {
		hl.fg = Color256(ColorFloat::cmy(0.1f, 0.3f, 0.1f));
	}
	if (deco == Decoration::template_brackets) {
		hl.fg = Color256(ColorFloat::cmy(0.5f, 0.1f, 0.1f));
	}
	if (deco == Decoration::array_brackets) {
		hl.fg = Color256(ColorFloat::cmy(0.25f, 0.1f, 0.25f));
	}
	if (deco == Decoration::keyword) {
		hl.fg = Color256(ColorFloat::cmy(0.1f, 0.1f, 0.1f));
	}
	if (deco == Decoration::type_keyword) {
		hl.fg = Color256(ColorFloat::cmy(0.1f, 0.2f, 0.2f));
	}
	if (deco == Decoration::operator_symbol) {
		hl.fg = Color256(ColorFloat::cmy(0.1f, 0.2f, 0.1f));
	}
	if (deco == Decoration::const_keyword) {
		hl.fg = Color256(ColorFloat::cmy(0.2f, 0.2f, 0.2f));
	}
	if (deco == Decoration::call_parameter_even) {
		hl.fg = Color256(ColorFloat::cmy(0.2f, 0.0f, 0.0f));
	}
	if (deco == Decoration::call_parameter_odd) {
		hl.fg = Color256(ColorFloat::cmy(0.0f, 0.2f, 0.0f));
	}
	if (deco == Decoration::template_parameter_even) {
		hl.fg = Color256(ColorFloat::cmy(0.1f, 0.0f, 0.1f));
	}
	if (deco == Decoration::template_parameter_odd) {
		hl.fg = Color256(ColorFloat::cmy(0.0f, 0.1f, 0.1f));
	}
	if (deco == Decoration::call_parentheses) {
		hl.fg = Color256(ColorFloat::cmy(0.5f, 0.1f, 0.1f));
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

template <>
HighlightedTextSegment<HighlightLight> HighlightDecorator<HighlightLight>::decorate(Decoration deco, const HighlightedTextSegment<HighlightLight>& ts, unsigned level) {
	(void)level;

	const auto& [tn, ihl] = ts;
	HighlightLight hl;

	hl.ac.flags.insert(deco);

	if (deco == Decoration::pointer) {
		hl.bold = true;
	}
	if (deco == Decoration::noise) {
		hl.alpha = 1.0f - 0.25f;
	}

	if (deco == Decoration::call_parameter_even) {
		hl.ac.even_fp *= 0.8f;
	}
	if (deco == Decoration::call_parameter_odd) {
		hl.ac.odd_fp *= 0.8f;
	}
	if (deco == Decoration::template_parameter_even) {
		hl.ac.even_tp *= 0.8f;
	}
	if (deco == Decoration::template_parameter_odd) {
		hl.ac.odd_tp *= 0.8f;
	}
	if (deco == Decoration::call_parentheses) {
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

Color256 ablend(Color256 a, Color256 b) {
	Color256 c;
	c.r = (a.r + b.r) / 2;
	c.g = (a.g + b.g) / 2;
	c.b = (a.b + b.b) / 2;
	return c;
}

Color256 wblend(float f, Color256 a, Color256 b) {
	Color256 c;
	float g = 1.0f - f;
	c.r = f * a.r + g * b.r;
	c.g = f * a.g + g * b.g;
	c.b = f * a.b + g * b.b;
	return c;
}

std::string render_cli(const HighlightedTextSegment<HighlightDark>& ht) {
	//assume white fg, black bg
	const auto& [text, hl] = ht;
	std::string result;

	if (hl.bold) {
		result += "\033[1m";
	}
	if (hl.italic) {
		result += "\033[3m";
	}

	auto f = ColorFloat(Color256(hl.fg));
	f *= hl.alpha;
	auto fg = Color256(f);
	result += fmt::format("\033[38;2;{};{};{}m", fg.r, fg.g, fg.b);

	result += text;

	result += "\033[39m";

	if (hl.italic) {
		result += "\033[23m";
	}
	if (hl.bold) {
		result += "\033[22m";
	}

	return result;
}

std::string render_cli(const HighlightedTextSegment<HighlightLight>& ht) {
	const auto& [text, hl] = ht;
	std::string result;

	if (hl.bold) {
		result += "\033[1m";
	}
	if (hl.italic) {
		result += "\033[3m";
	}

	auto fg = "#0A2540"_rgb;

	if (ht.hl.ac.flags.contains(Decoration::template_brackets)) {
		fg = "#0A4580"_rgb;
	}

	if (ht.hl.ac.flags.contains(Decoration::call_parentheses)) {
		fg = "#800A45"_rgb;
	}

	if (ht.hl.ac.flags.contains(Decoration::noise)) {
		fg = "#606060"_rgb;
	}
	if (ht.hl.ac.even_fp < 1.0f) {
		auto even_fc = "#806020"_rgb;
		float w = ht.hl.ac.even_fp * 0.5f;
		w = 0.125f;
		(void)w;
		fg = wblend(w, fg, even_fc);
	}

	if (ht.hl.ac.odd_fp < 1.0f) {
		auto odd_fc = "#408090"_rgb;
		float w = ht.hl.ac.odd_fp * 0.5f;
		(void)w;
		w = 0.125f;
		fg = wblend(w, fg, odd_fc);
	}

	if (ht.hl.ac.even_tp < 1.0f) {
		auto even_tc = "#206020"_rgb;
		float w = ht.hl.ac.odd_tp * 0.5f;
		w = 0.125f;
		(void)w;
		fg = wblend(w, fg, even_tc);
	}

	if (ht.hl.ac.odd_tp < 1.0f) {
		auto odd_tc = "#206090"_rgb;
		float w = ht.hl.ac.odd_tp * 0.5f;
		(void)w;
		w = 0.125f;
		fg = wblend(w, fg, odd_tc);
	}

	if (ht.hl.ac.flags.contains(Decoration::function_name)) {
		auto bg = "#FFFFDF"_rgb;

		result += fmt::format("\033[48;2;{};{};{}m", bg.r, bg.g, bg.b);
	}

	result += fmt::format("\033[38;2;{};{};{}m", fg.r, fg.g, fg.b);

	result += text;

	result += "\033[39;49m";

	if (hl.italic) {
		result += "\033[23m";
	}
	if (hl.bold) {
		result += "\033[22m";
	}

	return result;
}

std::string render_html(const HighlightedTextSegment<HighlightDark>& ht) {
	constexpr bool light_mode = false;
	const auto& [text, hl] = ht;
	std::string style = "style=\"";

	if (hl.bold) {
		style += "font-weight: bold;";
	}
	if (hl.italic) {
		style += "font-style: italic;";
	}

	if constexpr (light_mode) {
		float a = 1.0f - hl.alpha;
		auto fg = Color256(ColorFloat(a, a, a));
		style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);
		auto bg = "#808080"_rgb;

		style += fmt::format("background-color: rgb({},{},{});", bg.r, bg.g, bg.b);
	} else {
		auto f = ColorFloat(Color256(hl.fg));
		f *= hl.alpha;
		auto fg = Color256(f);
		style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);
	}

	style += "\"";

	if (hl.tags.contains(HighlightTags::odr)) {
		return fmt::format("<span {}>{}</span>", style, escape_xml(text) + "[ODR]");
	}

	return fmt::format("<span {}>{}</span>", style, escape_xml(text));
}

std::string render_html(const HighlightedTextSegment<HighlightLight>& ht) {
	const auto& [text, hl] = ht;
	std::string style = "style=\"";

	if (hl.bold) {
		style += "font-weight: bold;";
	}
	if (hl.italic) {
		style += "font-style: italic;";
	}

	auto fg = "#0A2540"_rgb;

	if (ht.hl.ac.flags.contains(Decoration::template_brackets)) {
		fg = "#0A4580"_rgb;
	}

	if (ht.hl.ac.flags.contains(Decoration::call_parentheses)) {
		fg = "#800A45"_rgb;
	}

	if (ht.hl.ac.flags.contains(Decoration::noise)) {
		fg = "#606060"_rgb;
	}
	if (ht.hl.ac.even_fp < 1.0f) {
		auto even_fc = "#806020"_rgb;
		float w = ht.hl.ac.even_fp * 0.5f;
		w = 0.125f;
		(void)w;
		fg = wblend(w, fg, even_fc);
	}

	if (ht.hl.ac.odd_fp < 1.0f) {
		auto odd_fc = "#408090"_rgb;
		float w = ht.hl.ac.odd_fp * 0.5f;
		(void)w;
		w = 0.125f;
		fg = wblend(w, fg, odd_fc);
	}

	if (ht.hl.ac.even_tp < 1.0f) {
		auto even_tc = "#206020"_rgb;
		float w = ht.hl.ac.odd_tp * 0.5f;
		w = 0.125f;
		(void)w;
		fg = wblend(w, fg, even_tc);
	}

	if (ht.hl.ac.odd_tp < 1.0f) {
		auto odd_tc = "#206090"_rgb;
		float w = ht.hl.ac.odd_tp * 0.5f;
		(void)w;
		w = 0.125f;
		fg = wblend(w, fg, odd_tc);
	}

	if (ht.hl.ac.flags.contains(Decoration::function_name)) {
		auto bg = "#FFFFDF"_rgb;

		style += fmt::format("background-color: rgb({},{},{});", bg.r, bg.g, bg.b);
	}

	style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);

	style += "\"";

	if (hl.tags.contains(HighlightTags::odr)) {
		return fmt::format("<span {}>{}</span>", style, escape_xml(text) + "[ODR]");
	}

	return fmt::format("<span {}>{}</span>", style, escape_xml(text));
}

template <typename HighlightType>
std::string render_html(const HighlightedTextSegment<HighlightType>& ht, const Navigation& nav) {
	constexpr bool light_mode = std::is_same_v<HighlightType, HighlightLight>;
	const auto& [text, hl] = ht;
	std::string style = "style=\"";

	if (hl.bold) {
		style += "font-weight: bold;";
	}
	if (hl.italic) {
		style += "font-style: italic;";
	}

	if constexpr (light_mode) {
		float a = 1.0f - hl.alpha;
		auto fg = Color256(ColorFloat(a, a, a));
		auto bg = "#808080"_rgb;
		style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);
		style += fmt::format("background-color: rgb({},{},{});", bg.r, bg.g, bg.b);
	} else {
		auto f = ColorFloat(Color256(hl.fg));
		f *= hl.alpha;
		auto fg = Color256(f);
		style += fmt::format("color: rgb({},{},{});", fg.r, fg.g, fg.b);
	}

	style += "\"";

	auto disp = fmt::format("<span {}>{}</span>", style, escape_xml(text));
	if (hl.tags.contains(HighlightTags::odr)) {
		disp += "<span class=\"tag odr\">ODR</span>";
	}
	if (not hl.link.empty()) {
		return hyperlink(hl.link, disp, nav);
	}
	return disp;
}

template std::string render_html(const HighlightedTextSegment<HighlightLight>& ht, const Navigation& nav);
template std::string render_html(const HighlightedTextSegment<HighlightDark>& ht, const Navigation& nav);

}
