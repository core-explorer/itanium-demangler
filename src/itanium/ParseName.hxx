#pragma once
#include "Grammar.hxx"
#include "ParseResult.hxx"
#include "../util/RecursionLimit.hxx"
#include "../util/RefPointer.hxx"
#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>
#include <vector>

namespace swartalf::itanium {

enum class ParseError : uint8_t {
	recursion_reached,
	no_matching_rule,
	no_matching_alternative,
	no_matching_element,
	mismatched_literal,
	input_too_short,
	input_too_long
};

std::expected<ParseResult, ParseError> parse(std::string_view input, const ProductionGrammar& rules);
std::expected<ParseResult, ParseError> parse_as(std::string_view input, ProductionType ltype, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume);
std::expected<ParseResult, ParseError> try_rule(std::string_view input, ProductionType type, const ProductionRule& rule, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume);

std::optional<ParseResult> is_ctor_or_dtor(const ParseResult& res);
std::vector<ParseResult> duplicate_ctor_dtor(const ParseResult& res, std::span<const std::string_view> new_lits);
std::optional<std::pair<ParseResult, ParseResult>> is_templated_name(const ParseResult& res);

std::optional<ParseResult> is_not_odr_relevant(const ParseResult& res);

}