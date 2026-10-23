#pragma once

#include "CompositeString.hxx"
#include "DemanglingDialect.hxx"
#include "FormatAst.hxx"
#include "LengthOnly.hxx"
#include <string_view>
namespace swartalf::itanium {

enum class SkipAction : uint8_t {
	none,
	hide_prefix,
	hide_suffix,
	hide_this,
	replace_this
};

struct SkipAlternative {
	std::vector<std::string> demangled_parts;
	SkipAction action = SkipAction::none;
};

struct DefaultTemplateParamater {
	std::vector<std::string> demangled_parts;
	unsigned param_num = 0;
	ParseResult param_value;
};

struct SkipSpec {
	std::vector<SkipAlternative> skip;
	std::vector<DefaultTemplateParamater> defaults;
};

template <typename Formatter>
std::expected<CompositeStack<typename Formatter::value_type>, ParseError> format_demangled(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, const SkipSpec& skip, RecursionLimit);

std::expected<CompositeStack<std::string>, ParseError> format_demangled_type_ascii(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);

template <typename Formatter>
std::expected<typename Formatter::value_type, ParseError> format_demangled_short(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, const SkipSpec& skip, RecursionLimit);

std::expected<std::string, ParseError> format_demangled_ascii(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);
std::expected<LengthOnly, ParseError> format_demangled_ascii_length(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion);
std::expected<std::string, ParseError> format_demangled_cli_light(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);
std::expected<std::string, ParseError> format_demangled_cli_dark(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);

std::expected<std::string, ParseError> format_demangled_html_light(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);
std::expected<std::string, ParseError> format_demangled_html_dark(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);

std::expected<std::string, ParseError> format_demangled_html(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion, std::string_view prefix);
const ParseResult* resolve_template_param(const ParseResult& res, const NestedTemplateParams& tp);
std::tuple<const ParseResult*, SubstitutionTableView, const NestedTemplateParams*> resolve_substitution(const ParseResult& res, const SubstitutionTableView& subs);

ParseResult restore_substitutions(const ParseResult& r, const SubstitutionTableView& subs);
ParseResult restore_substitutions_and_template_params(const ParseResult& r, const SubstitutionTableView& subs, const NestedTemplateParams& tp);
std::optional<int> get_pack_size(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, RecursionLimit max_recursion);
std::expected<std::string, ParseError> format_demangled_link_html(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion, bool light_mode, std::string_view prefix);
}
