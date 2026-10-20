#pragma once
#include "Grammar.hxx"

namespace swartalf::itanium {
std::unordered_map<char, std::unordered_set<ProductionType>> optimize(const ProductionGrammar& rules);
}