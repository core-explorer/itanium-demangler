#pragma once
#include "FormatDemangle.hxx"
#include "ParseResult.hxx"
#include "ProductionType.hxx"

namespace swartalf::itanium {

std::pair<unsigned, unsigned> get_max_num_function_and_template_parameters(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, RecursionLimit max_recursion);

std::expected<std::pair<std::string, DemanglingDialect>, ParseError> format_demangled_ascii_limit(unsigned len, const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit);
}