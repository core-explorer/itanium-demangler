#include "CliDecorator.hxx"

namespace swartalf {
std::string CliDecorator::decorate(Decoration deco, std::string_view tn, unsigned level) {
	switch (deco) {
	case Decoration::keyword:
		return fmt::format("\033[1;31m{}\033[0m", tn);
	case Decoration::type_keyword:
		return fmt::format("\033[31m{}\033[0m", tn);
	case Decoration::const_keyword:
		return fmt::format("\033[34m{}\033[0m", tn);
	case Decoration::reference:
		return fmt::format("\033[1;36m{}\033[0m", tn);
	case Decoration::rvalue_reference:
		return fmt::format("\033[1;36m{}\033[0m", tn);
	case Decoration::pointer:
		return fmt::format("\033[1;36m{}\033[0m", tn);
	case Decoration::generic_bold:
		return fmt::format("\033[1m{}\033[0m", tn);
	case Decoration::generic_italic:
		return fmt::format("\033[3m{}\033[0m", tn);
	case Decoration::curly_braces:
		return fmt::format("\033[1;33m{}\033[0m", tn);
	case Decoration::template_brackets:

		switch (level % 4) {
		case 1:
			return fmt::format("\033[35m{}\033[0m", tn);
		case 2:
			return fmt::format("\033[32m{}\033[0m", tn);
		case 3:
			return fmt::format("\033[33m{}\033[0m", tn);
		default:
			return fmt::format("\033[36m{}\033[0m", tn);
		}

	case Decoration::call_parentheses:
		return fmt::format("\033[1m{}\033[0m", tn);
	case Decoration::operator_symbol:
		return fmt::format("\033[1;33m{}\033[0m", tn);
	case Decoration::function_name:
		return fmt::format("\033[1;33m{}\033[0m", tn);
	default:
		return std::string(tn);
	}
}
}