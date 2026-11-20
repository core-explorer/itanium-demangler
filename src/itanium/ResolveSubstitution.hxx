#pragma once
#include "ParseResult.hxx"
#include "SubstitutionTable.hxx"
namespace swartalf::itanium {
const ParseResult* resolve_template_param(const ParseResult& res, const NestedTemplateParams& tp);
std::tuple<const ParseResult*, SubstitutionTableView, const NestedTemplateParams*> resolve_substitution(const ParseResult& res, const SubstitutionTableView& subs);
}