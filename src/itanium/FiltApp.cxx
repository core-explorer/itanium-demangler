#include "FiltApp.hxx"
#include "DemanglingDialect.hxx"
#include "FiltAnalyzer.hxx"
#include "FiltArgs.hxx"
#include "ParseName.hxx"
#include "ParseResult.hxx"
#include "../cli/ArgParser.hxx"
#include "../cli/ColorMode.hxx"
#include "../cli/cli_color.hxx"
#include "../cli/terminal_size.hxx"
#include "../util/print.hxx"
#include "../util/string.hxx"
#include "../util/version.hxx"
namespace swartalf {

using namespace swartalf::filt;
using namespace swartalf::itanium;

int FiltApp::run(const ParsingResult& parseResult) {
	using enum OptionName;
	using enum OptionValueType;

	FiltAnalyzer analyzer;
	auto& config = analyzer.config;
	bool grammar_dump = false;

	for (const auto& opt : parseResult.options) {
		switch (to_option_name(opt.long_name)) {
		case OptionName::color:
			if (auto color_mode = to_ColorMode(opt.value); color_mode.has_value()) {
				set_color_mode(color_mode.value());
			} else {
				colored_println(stderr, "Invalid argument for --color", opt.value, "error");
				return 1;
			}
			break;

		case OptionName::help: {
			parser.printUsage(stdout);
			return 0;
		}
		case OptionName::grammar:
			grammar_dump = true;
			break;
		case OptionName::subs:
			config.subs = true;
			break;
		case OptionName::debug:
			config.debug = true;
			break;
		case OptionName::check:
			config.check = true;
			break;
		case OptionName::trailing:
			config.trailing = true;
			break;
		case OptionName::templ:
			config.templ = true;
			break;
		case OptionName::pack:
			config.pack = true;
			break;
		case OptionName::return_:
			config.hide_return = true;
			break;
		case OptionName::function_signature:
			config.hide_function = true;
			break;
		case OptionName::remangle:
			config.remangle = true;
			break;
		case OptionName::expression:
			config.expression = true;
			break;
		case OptionName::short_:
			config.short_names = true;
			break;
		case OptionName::light_mode:
			config.light_mode = true;
			break;
		case OptionName::length: {
			auto val = parse_uint(opt.value);
			if (not val) {
				colored_println(stderr, "Invalid argument", opt.long_name, "error");
				return 1;
			}
			auto [value, string_length] = *val;
			config.length = value;

		} break;
		case OptionName::function_params: {
			auto val = parse_uint(opt.value);
			if (not val) {
				colored_println(stderr, "Invalid argument", opt.long_name, "error");
				return 1;
			}
			auto [value, string_length] = *val;
			config.function_params = value;

		} break;
		case OptionName::terminal_width: {
			auto val = parse_uint(opt.value);
			if (not val) {
				colored_println(stderr, "Invalid argument", opt.long_name, "error");
				return 1;
			}

			auto cval = get_output_columns();
			if (not cval) {
				colored_println(stderr, "failed to determine numnber of terminal columns", opt.long_name, "error");
				return 1;
			}

			config.length = val->first * *cval;

		} break;
		case OptionName::template_params: {
			auto val = parse_uint(opt.value);
			if (not val) {
				colored_println(stderr, "Invalid argument", opt.long_name, "error");
				return 1;
			}
			auto [value, string_length] = *val;
			config.template_params = value;

		} break;
		case OptionName::category: {
			auto cat = to_production_type(opt.value);
			if (not cat) {
				colored_println(stderr, "Invalid argument", opt.long_name, "error");
				return 1;
			}
			config.category = *cat;
		} break;
		case OptionName::dialect: {
			if (opt.value == "gcc" or opt.value == "GCC" or opt.value == "gnu" or opt.value == "GNU") {
				config.dialect = CompilerDialect::gcc;
			} else if (opt.value == "clang" or opt.value == "Clang" or opt.value == "llvm" or opt.value == "LLVM") {
				config.dialect = CompilerDialect::clang;
			} else {
				colored_println(stderr, "Invalid argument", opt.long_name, "error");
				return 1;
			}

		} break;
		case OptionName::partial:
			config.partial = true;
			break;
		case OptionName::version:
			println(stdout, "{} {}", parser.programName, get_version_string());
			return 0;
		default:
			colored_println(stderr, "Invalid option", opt.long_name, "error");
			return 1;
		}
	}

	if (grammar_dump) {
		auto rules = build_grammar();
		auto g = format_grammar_recursive_cli(rules, config.category);
		fmt::print("{}", g);
		return 0;
	}

	const auto& parsedArgs = parseResult;
	for (const auto& arg : parsedArgs.args) {
		analyzer.words.emplace_back(arg.value);
	}

	auto ar = analyzer.analyze();
	if (not ar) {
		const auto& error = ar.error();
		colored_println(stderr, error.message, error.path, error.severity);
		return 1;
	}

	return 0;
}
}
