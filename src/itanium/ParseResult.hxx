#pragma once
#include "Grammar.hxx"
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
namespace swartalf::itanium {

struct ParseResult {
	ProductionType type;
	std::vector<std::variant<std::string_view, ParseResult>> comps;

	ParseResult() = default;
	ParseResult(ParseResult&&) noexcept = default;
	ParseResult& operator=(ParseResult&&) noexcept = default;
	ParseResult& operator=(const ParseResult&) = delete;
	ParseResult(const ParseResult&) = default;
	size_t get_consumed_length() const;

	ParseResult copy() const {
		return ParseResult(*this);
	}

private:
};

inline size_t ParseResult::get_consumed_length() const {
	size_t sum = 0;
	for (const auto& sc : comps) {
		if (std::holds_alternative<std::string_view>(sc)) {
			const auto& sv = std::get<std::string_view>(sc);
			sum += sv.size();
		} else {
			const auto& c = std::get<ParseResult>(sc);
			sum += c.get_consumed_length();
		}
	}

	return sum;
}

std::optional<ProductionType> to_production_type(std::string_view pn);

std::string format_elements_cli(std::span<const ProductionElement> elts, const ProductionGrammar& rules);
std::string to_string(const ParseResult& result);

std::string format_grammar_recursive_cli(const ProductionGrammar& rules, ProductionType k);
std::string format_grammar_recursive_html(const ProductionGrammar& rules, ProductionType k);
}