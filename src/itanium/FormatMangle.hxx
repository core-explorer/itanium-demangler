#pragma once

#include "ParseResult.hxx"
#include <string>
namespace swartalf::itanium {

std::string format_mangled(const ParseResult& res);

}
