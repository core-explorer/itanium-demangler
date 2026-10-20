#pragma once
#include "Grammar.hxx"

namespace swartalf::itanium {
std::flat_map<char, std::flat_set<ProductionType>> optimize(const ProductionGrammar& rules);
}