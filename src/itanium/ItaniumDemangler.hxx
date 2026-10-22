#pragma once
#include "Grammar.hxx"
#include "ParseName.hxx"

namespace swartalf::itanium {

struct ItaniumDemangler {
	ItaniumDemangler() {
		rules = itanium::build_grammar();
		extend_grammar(rules);
	}
	itanium::ProductionGrammar rules;

	std::expected<ParseResult, ParseError> parse(std::string_view input) const {
		return itanium::parse(input, rules);
	}
	std::expected<ParseResult, ParseError> parse_as(std::string_view input, ProductionType ltype, RecursionLimit max_recursion, size_t indent, bool must_consume) const {
		return itanium::parse_as(input, ltype, max_recursion, indent, rules, must_consume);
	}
	std::expected<ParseResult, ParseError> try_rule(std::string_view input, ProductionType type, const ProductionRule& rule, RecursionLimit max_recursion, size_t indent, bool must_consume) const {
		return itanium::try_rule(input, type, rule, max_recursion, indent, rules, must_consume);
	}
};

}
