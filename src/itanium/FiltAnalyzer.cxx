#include "FiltAnalyzer.hxx"
#include "Base36.hxx"
#include "DemanglingDialect.hxx"
#include "FormatAst.hxx"
#include "FormatDemangle.hxx"
#include "FormatDemangleLimit.hxx"
#include "FormatMangle.hxx"
#include "Grammar.hxx"
#include "ItaniumDemangler.hxx"
#include "ParseName.hxx"
#include "../cli/cli_color.hxx"
#include "../util/print.hxx"
#include "../util/split.hxx"
#include <cxxabi.h>
#include <iostream>

namespace swartalf {
using namespace itanium;
ItaniumDemangler demang;
void printWord(std::string_view word, const FiltAnalyzerConfig& config) {
	auto cat = config.category;
	auto dialect = config.dialect;
	auto remangle = config.remangle;
	auto partial = config.partial;
	auto light_mode = config.light_mode;
	auto short_names = config.short_names;

	DemanglingDialect dv;

	dv.fundamental_name = FundamentalName::unsigned_int;
	dv.literal_decoration = LiteralDecoration::noncaps;

	dv.compiler_dialect = dialect;
	dv.separating_whitespace = dialect == CompilerDialect::gcc;
	if (config.subs) {
		dv.resolve_substitutions = false;
	}
	if (config.templ) {
		dv.resolve_template_args = false;
	}
	if (config.pack) {
		dv.resolve_packs = false;
	}
	if (config.expression) {
		dv.display_expressions = false;
	}

	if (dv.compiler_dialect == CompilerDialect::clang) {
		dv.parenthesize_expressions = false;
		dv.whitespace_operator_expressions = true;
		dv.complex_keyword = ComplexKeyword::no_underscore;
		dv.float_template_constant_style = FloatTemplateConstantStyle::hexfloat_style;
		dv.nan_style = NaNStyle::small_caps;
		dv.infinity_style = InfinityStyle::inf;
		dv.nullptr_type = NullptrType::std_nullptr;
	}

	if (short_names) {
		dv.hide_abi_suffix = true;
		dv.hide_vendor_suffix = true;
		dv.return_type_style = ReturnTypeStyle::no_return;
		dv.float_template_constant_style = FloatTemplateConstantStyle::hexfloat_style;
		dv.literal_decoration = LiteralDecoration::none;
	}
	if (config.trailing) {
		dv.return_type_style = ReturnTypeStyle::trailing_return;
	}
	if (config.hide_return) {
		dv.return_type_style = ReturnTypeStyle::no_return;
	}
	if (config.hide_function) {
		dv.resolve_function_signatures = false;
	}

	if (config.debug) {
		fmt::print("{}\n", std::string(italic(word)));
	}

	dv.max_function_parameters = config.function_params;
	dv.max_template_parameters = config.template_params;

	if ((cat != ProductionType::any_mangled_name) or word.starts_with("_Z") or word.starts_with("_GLOBAL__sub_I__Z") or word.starts_with("__Z") or word.starts_with("__GLOBAL__sub_I__Z")) {
		auto& rules = demang.rules;
		RecursionLimit max_recursion {1000};
		SkipSpec skip;
		size_t indent = 0;
		bool must_consume = not partial;
		using namespace std::string_literals;
		auto parsed = parse_as(word, cat, max_recursion, indent, rules, must_consume);
		if (parsed) {
			if (remangle) {
				auto remangled = format_mangled(*parsed);
				std::string pretty;
				if (remangled == word) {
					pretty = bold(remangled);
				} else {
					pretty = bold_red(remangled);
				}
				fmt::print("{}\n", pretty);
				return;
			}

			SubstitutionTable su = build_substitution_table(*parsed);
			auto subs = view(su);
			NestedTemplateParams nt;

			if (config.debug) {
				for (size_t i = 0; i < subs.entries.size(); ++i) {
					const auto& sub = subs.entries[i];
					std::string subn = "S_";
					if (i > 0) {
						subn = fmt::format("S{}_", encodeNumberBase36(i - 1));
					}
					std::string tpr;
					const auto* ps = std::get_if<ParseResult>(subs.entries[i].comps.data());
					if (subs.entries[i].type == ProductionType::type and ps and ps->type == itanium::ProductionType::template_param) {
						const auto* tr = resolve_template_param(*ps, subs.stp[i]);
						if (tr) {
							tpr = fmt::format(" = {}", format_mangled(*tr));
						}
					} else if (subs.entries[i].type == ProductionType::type and ps) {
						tpr = fmt::format(" {}", to_string_view(ps->type));
					}
					fmt::print(" sub {} {}{}\n", subn, format_mangled(sub), tpr);
				}
				fmt::print("{}\n", to_string(*parsed));
			}

			auto num_params = get_max_num_function_and_template_parameters(*parsed, subs, nt, max_recursion);
			dv.max_function_parameters = std::min(dv.max_function_parameters, num_params.first);
			dv.max_template_parameters = std::min(dv.max_template_parameters, num_params.second);

			std::expected<std::string, ParseError> eraw;
			if (config.length < 100000) {
				auto elraw = format_demangled_ascii_limit(config.length, *parsed, subs, nt, dv, skip, max_recursion);
				if (elraw) {
					eraw = elraw->first;
					dv = elraw->second;
				} else {
					eraw = std::unexpected(elraw.error());
				}
			} else {
				eraw = format_demangled_ascii(*parsed, subs, nt, dv, skip, max_recursion);
			}

			std::expected<std::string, ParseError> epretty;

			if (not eraw) {
				fmt::print("{}", word);
				if (config.check) {
					auto cxxdemangle = demangle(word);
					if (cxxdemangle != word) {
						fmt::print("{}\n", cxxdemangle);
						exit(1);
					}
				}
				return;
			}

			if (colored_output == CliColorMode::uncolored) {
				epretty = eraw;
			} else {
				if (light_mode) {
					epretty = format_demangled_cli_light(*parsed, subs, nt, dv, skip, max_recursion);

				} else {
					epretty = format_demangled_cli_dark(*parsed, subs, nt, dv, skip, max_recursion);
				}
			}
			std::string pretty = epretty ? *epretty : "_ERROR"s;
			std::string raw = eraw ? *eraw : "_ERROR"s;
			std::string cxxdemangle;
			

			if (config.check) {
				cxxdemangle = demangle(word);
			}

			if (config.check and cxxdemangle == word) {
				pretty = bold_magenta(raw);
			} else if ((not config.check) or raw == cxxdemangle or short_names or config.subs or config.templ or config.hide_return or config.pack) {
				
			} else {
				size_t matching = 0;
				while (matching < raw.size() and matching < cxxdemangle.size() and raw[matching] == cxxdemangle[matching]) {
					matching++;
				}

				pretty = std::string(bold_yellow(raw.substr(0, matching))) + std::string(bold_red(raw.substr(matching)));

				fmt::print("{}\n", cxxdemangle);

				if (
				    (not(dialect == CompilerDialect::clang and word.contains("rqX") and not cxxdemangle.contains("requires")))
				    and (not(dialect == CompilerDialect::gcc and raw.contains("lambda(auto") and cxxdemangle.contains("lambda(auto")))
				    and (not(dialect == CompilerDialect::clang and raw.contains("lambda'(auto") and cxxdemangle.contains("lambda'(auto")))) {
					fmt::print("{}\n", pretty);
					exit(1);
				}
			}

			fmt::print("{}", pretty);
			

			return;
		} else {
			fmt::print("failed\n");
			return;
		}
	} else {
		fmt::print("{}", word);
	}
}

void FiltAnalyzer::stream() const {
	std::string line;

	while (std::getline(std::cin, line)) {
		std::vector<std::string_view> lwords;
		split_with_delims(lwords, line, " \t@");
		
		for (size_t i = 0; i < lwords.size(); i++) {
			auto& word = lwords[i];
			printWord(word, config);
			if (i + 1 < word.size()) {
				fmt::print(" ");
			}
		}
		fmt::print("\n");
	}
}

auto FiltAnalyzer::analyze() -> std::expected<AnalysisResult, AnalysisError> {
	AnalysisResult result;

	for (auto& word : words) {
		printWord(word, config);
		fmt::print("\n");
	}
	if (words.empty()) {
		stream();
	}

	return result;
}

std::string demangle(std::string_view mangledName) {
	if (mangledName.empty()) {
		return {};
	}
	int status;
	int shift = 0;
	auto demangleName = mangledName;
	if (mangledName[0] == '*') {
		shift = 1;
		demangleName = mangledName.substr(1);
	}
	auto at_pos = demangleName.find('@');
	std::string_view suffix;
	char* demangledName = nullptr;
	if (at_pos != std::string::npos) {
		suffix = demangleName.substr(at_pos);
		demangleName = demangleName.substr(0, at_pos);
		auto temp = std::string(demangleName);

		if (temp.size() > 9999) {
			return temp;
		}
		demangledName = abi::__cxa_demangle(temp.data(), nullptr, nullptr, &status);
	} else {
		if (demangleName.size() > 9999) {
			return std::string(demangleName);
		}
		auto temp = std::string(demangleName);
		demangledName = abi::__cxa_demangle(temp.data(), nullptr, nullptr, &status);
	}

	if (status == 0) {
		std::string result;
		if (shift) {
			std::string_view dm = demangledName;
			result.reserve(8 + dm.size());
			result = ".hidden ";
			result += dm;
		} else {
			result = demangledName;
		}
		free(demangledName);
		result += suffix;
		return result;
	} else {
		return std::string(mangledName);
	}
}

}
