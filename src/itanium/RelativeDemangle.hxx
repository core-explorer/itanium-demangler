#pragma once
#include "FormatDemangle.hxx"
#include "ParseResult.hxx"
namespace swartalf::itanium {
SkipSpec get_relative_names(const ParseResult& res);
SkipSpec get_relative_names_from_nested_name(const ParseResult& res);
}