#include "Decorator.hxx"
#include "../web/HtmlGenerator.hxx"
namespace swartalf {

std::string XmlDecorator::decorate(Decoration deco, std::string_view tn, unsigned level) {
	(void)level;
	using namespace std::string_view_literals;
	switch (deco) {
	case Decoration::noise:
		return span_noise(tn);
	case Decoration::generic_italic:
		return span_idiosyncratic(tn);
	case Decoration::generic_bold:
		return "<strong>" + std::string(tn) + "</strong>";

	case Decoration::keyword:
		return keyword(tn);

	case Decoration::type_keyword:
		return type_keyword(tn);
	case Decoration::const_keyword:
		return const_keyword(tn);
	case Decoration::template_brackets:
	case Decoration::array_brackets:
	case Decoration::call_parentheses:
	case Decoration::curly_braces: {
		auto i = level % span_braces.size();
		if (tn == "["sv or tn == "("sv or tn == "{"sv or tn == "&lt;"sv) {
			return span_braces[i].open() + brace(tn);
		}
		if (tn == "]"sv or tn == ")"sv or tn == "}"sv or tn == "&gt;"sv) {
			return brace(tn) + span_braces[i].close();
		}

		return span_braces[i](brace(tn));
	}
	default:
		return std::string(tn);
	}
}

}
