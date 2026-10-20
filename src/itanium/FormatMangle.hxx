#pragma once

#include "FormatAst.hxx"
#include <string>
namespace swartalf::itanium {
ParseResult insert_literals(const ParseResult& res);
ParseResult insert_substitutions(const ParseResult& res);

std::string format_mangled(const ParseResult& res);

}