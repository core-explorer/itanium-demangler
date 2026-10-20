#pragma once
#include "../cli/ArgParser.hxx"
#include <cstdint>
namespace swartalf::filt {

enum class OptionName : uint8_t {
	invalid_option,
	help,
	version,
	color,
	remangle,
	grammar,
	category,
	partial,
	dialect,
	light_mode,
	short_,
	subs,
	templ,
	pack,
	return_,
	debug,
	check,
	trailing,
	expression,
	length,
	function_params,
	template_params,
	function_signature,
	terminal_width,
	number_of_options

};

constexpr std::string_view to_string_view(OptionName name) {
	switch (name) {
	case OptionName::invalid_option:
		return "invalid-option";
	case OptionName::help:
		return "help";
	case OptionName::version:
		return "version";
	case OptionName::remangle:
		return "remangle";
	case OptionName::color:
		return "color";
	case OptionName::grammar:
		return "grammar";
	case OptionName::category:
		return "category";
	case OptionName::partial:
		return "partial";
	case OptionName::dialect:
		return "dialect";
	case OptionName::light_mode:
		return "light-mode";
	case OptionName::short_:
		return "short";
	case OptionName::subs:
		return "subs";
	case OptionName::templ:
		return "templ";
	case OptionName::pack:
		return "pack";
	case OptionName::return_:
		return "return";
	case OptionName::function_signature:
		return "function-signature";
	case OptionName::debug:
		return "debug";
	case OptionName::check:
		return "check";
	case OptionName::trailing:
		return "trailing";
	case OptionName::expression:
		return "expression";
	case OptionName::length:
		return "length";
	case OptionName::function_params:
		return "function-params";
	case OptionName::template_params:
		return "template-params";
	case OptionName::terminal_width:
		return "terminal-width";
	default:
		return "invalid-option";
	}
}

constexpr OptionName to_option_name(std::string_view name) {
	for (size_t i = 0; i < static_cast<size_t>(OptionName::number_of_options); ++i) {
		auto opt = static_cast<OptionName>(i);
		if (to_string_view(opt) == name) {
			return opt;
		}
	}
	return OptionName::invalid_option;
}

}

namespace swartalf {
ArgParser makeFiltParser();
}