#include "ParseName.hxx"
#include "Grammar.hxx"
#include "../cli/cli_color.hxx"
#include "../util/format.hxx"
#include "../util/string.hxx"
#include <cstdlib>
#include <expected>
#include <optional>
#include <span>
#include <string_view>
#include <variant>

namespace swartalf::itanium {
using enum ProductionType;

std::expected<ParseResult, ParseError> try_alternative(std::string_view input, ProductionType type, std::span<const ProductionElement> elts, RecursionLimit max_recursion, size_t indent, ParseResult&& partial, const ProductionGrammar& rules, bool must_consume) {
	if (max_recursion == 0) {
		return std::unexpected(ParseError::recursion_reached);
	}
	--max_recursion;
	indent++;
	must_consume = false;

	if (input.empty()) {
		return std::unexpected(ParseError::input_too_short);
	}

	if (type == prefix and input.starts_with('E')) {
		return std::unexpected(ParseError::mismatched_literal);
	}

	for (size_t i = 0; i < elts.size(); ++i) {
		const auto& elt = elts[i];
		const auto* p0 = get_if<ProductionType>(&elt.element);

		if (p0 and *p0 == closure_prefix and type == prefix) {
			if (i == 0 and p0 and *p0 != template_args and input.starts_with("I")) {
				return std::unexpected(ParseError::mismatched_literal);
			}
		}
	}
	if (type == template_prefix and elts.size() == 2 and input.starts_with("I")) {
		return std::unexpected(ParseError::mismatched_literal);
	}

	if (elts.empty()) {
		return std::unexpected(ParseError::no_matching_element);
	}

	const auto& elt = elts.front().element;
	if (const auto* p = get_if<LiteralSequence>(&elt); p) {
		if (input.starts_with(p->literal)) {
			if (elts.size() == 1) {
				ParseResult result {std::move(partial)};
				result.comps.emplace_back(input.substr(0, p->literal.size()));
				if (must_consume and result.get_consumed_length() != input.size()) {
					return std::unexpected(ParseError::input_too_long);
				}

				return result;
			} else {
				ParseResult result {std::move(partial)};
				result.type = type;
				result.comps.emplace_back(input.substr(0, p->literal.size()));
				auto remaining = input.substr(p->literal.size());
				auto next_elts = elts.subspan(1);
				return try_alternative(remaining, type, next_elts, max_recursion, indent, std::move(result), rules, must_consume);
			}
		} else {
			return std::unexpected(ParseError::mismatched_literal);
		}
	}

	if (const auto* op = get_if<Optional<LiteralSequence>>(&elt); op) {
		const auto* p = &op->val;
		if (input.starts_with(p->literal)) {
			ParseResult result {std::move(partial)};
			result.comps.emplace_back(input.substr(0, p->literal.size()));
			if (elts.size() == 1) {
				return result;
			} else {
				auto remaining = input.substr(p->literal.size());
				return try_alternative(remaining, type, elts.subspan(1), max_recursion, indent, std::move(result), rules, must_consume);
			}
		} else if (elts.size() == 1) {
			if (must_consume and partial.get_consumed_length() != input.size()) {
				return std::unexpected(ParseError::input_too_long);
			}
			return partial;
		} else {
			auto next_elts = elts.subspan(1);
			return try_alternative(input, type, next_elts, max_recursion, indent, std::move(partial), rules, must_consume);
		}
	}

	if (const auto* p = get_if<ProductionType>(&elt); p) {
		auto it = rules.rules.find(*p);
		if (it == rules.rules.end()) {
			return std::unexpected(ParseError::no_matching_rule);
		}
		const auto& new_rule = it->second;
		auto ruleres = try_rule(input, *p, new_rule, max_recursion, indent, rules, must_consume and elts.size() == 1);
		if (ruleres) {
			auto pcl = partial.get_consumed_length();
			ParseResult result {std::move(partial)};
			result.comps.emplace_back(std::move(*ruleres));
			if (elts.size() == 1) {
				if (must_consume and result.get_consumed_length() - pcl != input.size()) {
					return std::unexpected(ParseError::input_too_long);
				}
				return result;
			} else {
				auto remaining = input.substr(result.get_consumed_length() - pcl);
				auto next_elts = elts.subspan(1);
				bool all_optional = true;
				for (const auto& ee : next_elts) {
					const auto& e = ee.element;
					if (std::holds_alternative<LiteralSequence>(e) or std::holds_alternative<ProductionType>(e) or std::holds_alternative<Repeat<ProductionType>>(e)) {
						all_optional = false;
						break;
					}
				}
				if (remaining.empty()) {
					if (not all_optional) {
						return std::unexpected(ParseError::input_too_short);
					}
					return result;
				}

				return try_alternative(remaining, type, next_elts, max_recursion, indent, std::move(result), rules, must_consume);
			}

		} else {
			return std::unexpected(ruleres.error());
		}
	}

	if (const auto* op = get_if<Optional<ProductionType>>(&elt); op) {
		const auto* p = &op->val;
		auto it = rules.rules.find(*p);
		if (it == rules.rules.end()) {
			return std::unexpected(ParseError::no_matching_rule);
		}
		const auto& new_rule = it->second;
		auto ruleres = try_rule(input, *p, new_rule, max_recursion, indent, rules, must_consume and elts.size() == 1);
		if (ruleres) {
			auto pcl = partial.get_consumed_length();
			ParseResult result {std::move(partial)};
			result.comps.emplace_back(std::move(*ruleres));
			if (elts.size() == 1) {
				if (must_consume and result.get_consumed_length() - pcl != input.size()) {
					return std::unexpected(ParseError::input_too_long);
				}
				return result;
			} else {
				auto remaining = input.substr(result.get_consumed_length() - pcl);
				return try_alternative(remaining, type, elts.subspan(1), max_recursion, indent, std::move(result), rules, must_consume);
			}

		} else if (elts.size() == 1) {
			if (must_consume and partial.get_consumed_length() != input.size()) {
				return std::unexpected(ParseError::input_too_long);
			}
			return partial;
		} else {
			return try_alternative(input, type, elts.subspan(1), max_recursion, indent, std::move(partial), rules, must_consume);
		}
	}

	if (const auto* rp = get_if<Repeat<ProductionType>>(&elt); rp) {
		const auto* p = &rp->val;
		auto it = rules.rules.find(*p);
		if (it == rules.rules.end()) {
			return std::unexpected(ParseError::no_matching_rule);
		}

		const auto& new_rule = it->second;
		auto ruleres = try_rule(input, *p, new_rule, max_recursion, indent, rules, false);
		if (ruleres) {
			std::optional<std::string_view> next_literal;
			if (elts.size() >= 2) {
				const auto* plit = std::get_if<LiteralSequence>(&elts[1].element);
				if (plit) {
					next_literal = plit->literal;
				}
			}
			std::vector<ParseResult> vresult;
			std::vector<std::string_view> vremaining;
			auto pcl = partial.get_consumed_length();
			vresult.emplace_back(std::move(partial));
			vresult.back().comps.emplace_back(std::move(*ruleres));
			vremaining.emplace_back(input.substr(vresult.back().get_consumed_length() - pcl));
			while (not vremaining.back().empty()) {
				if (next_literal and vremaining.back().starts_with(*next_literal)) {
					break;
				}
				if (*p == prefix and vremaining.back().starts_with('E')) {
					break;
				}
				auto extrares = try_rule(vremaining.back(), *p, new_rule, max_recursion, indent, rules, false);
				if (extrares) {
					auto last_result = vresult.back().copy();
					vresult.emplace_back(std::move(last_result));

					auto ecl = extrares->get_consumed_length();
					vresult.back().comps.emplace_back(std::move(*extrares));
					auto last_remaining = vremaining.back();
					vremaining.emplace_back(last_remaining.substr(ecl));
				} else {
					break;
				}
			}

			if (elts.size() == 1) {
				auto& result = vresult.back();
				if (must_consume and result.get_consumed_length() - pcl != input.size()) {
					return std::unexpected(ParseError::input_too_long);
				}
				return std::move(vresult.back());
			} else {
				for (size_t k = 0; k < vresult.size(); k++) {
					auto i = vresult.size() - 1 - k;
					auto& remaining = vremaining[i];
					if (remaining.empty()) {
						continue;
					}
					auto next_elts = elts.subspan(1);
					auto& result = vresult[i];
					auto next = try_alternative(remaining, type, next_elts, max_recursion, indent, std::move(result), rules, must_consume);
					if (next) {
						return next;
					}
				}
			}

		} else {
			return std::unexpected(ruleres.error());
		}
	}

	if (const auto* op = get_if<OptionalRepeat<ProductionType>>(&elt); op) {
		const auto* p = &op->val.val;
		auto it = rules.rules.find(*p);
		if (it == rules.rules.end()) {
			return std::unexpected(ParseError::no_matching_rule);
		}
		const auto& new_rule = it->second;
		std::optional<std::string_view> next_literal;
		if (elts.size() >= 2) {
			const auto* plit = std::get_if<LiteralSequence>(&elts[1].element);
			if (plit) {
				next_literal = plit->literal;
			}
		}

		auto ruleres = (next_literal and input.starts_with(*next_literal)) ? std::unexpected(ParseError::mismatched_literal) : try_rule(input, *p, new_rule, max_recursion, indent, rules, false);
		if (ruleres) {
			std::vector<ParseResult> vresult;
			std::vector<std::string_view> vremaining;
			auto pcl = partial.get_consumed_length();
			vresult.emplace_back(std::move(partial));
			vresult.back().comps.emplace_back(std::move(*ruleres));
			vremaining.emplace_back(input.substr(vresult.back().get_consumed_length() - pcl));

			while (not vremaining.back().empty()) {
				if (next_literal and vremaining.back().starts_with(*next_literal)) {
					break;
				}
				auto extrares = try_rule(vremaining.back(), *p, new_rule, max_recursion, indent, rules, false);
				if (extrares) {
					auto& last_result = vresult.back();
					vresult.emplace_back(last_result.copy());

					auto ecl = extrares->get_consumed_length();
					vresult.back().comps.emplace_back(std::move(*extrares));
					auto last_remaining = vremaining.back();
					vremaining.emplace_back(last_remaining.substr(ecl));
				} else {
					break;
				}
			}

			if (elts.size() == 1) {
				auto& result = vresult.back();
				if (must_consume and result.get_consumed_length() - partial.get_consumed_length() != input.size()) {
					return std::unexpected(ParseError::input_too_long);
				}
				return std::move(vresult.back());
			} else {
				for (size_t k = 0; k < vresult.size(); k++) {
					auto i = vresult.size() - 1 - k;
					auto& remaining = vremaining[i];

					if (remaining.empty()) {
						continue;
					}
					auto& result = vresult[i];
					auto next_elts = elts.subspan(1);
					auto next = try_alternative(remaining, type, next_elts, max_recursion, indent, std::move(result), rules, must_consume);
					if (next) {
						return next;
					}
				}
			}

		} else if (elts.size() == 1) {
			if (must_consume and partial.get_consumed_length() != input.size()) {
				return std::unexpected(ParseError::input_too_long);
			}
			return partial;
		} else {
			auto next_elts = elts.subspan(1);
			return try_alternative(input, type, next_elts, max_recursion, indent, std::move(partial), rules, must_consume);
		}
	}

	return std::unexpected(ParseError::no_matching_element);
}

std::string to_string_cli(const ProductionAlternative& alt, ProductionGrammar& rules) {
	return format_elements_cli(alt.sequence.elements, rules);
}

std::expected<ParseResult, ParseError> try_alternative(std::string_view input, ProductionType type, const ProductionAlternative& alt, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume) {
	ParseResult partial;
	partial.type = type;

	auto elts = std::span(alt.sequence.elements);
	return try_alternative(input, type, elts, max_recursion, indent, std::move(partial), rules, must_consume);
}

std::expected<std::vector<ParseResult>, ParseError> convert_names(std::vector<ParseResult>&& names) {
	std::vector<ParseResult> result;
	for (size_t i = 0; i < names.size(); i++) {
		auto& n = names[i];

		if (i + 1 == names.size()) {
			result.emplace_back(std::move(n));
		} else if ((names[i].type == prefix or names[i].type == unqualified_name) and i + 1 < names.size() and names[i + 1].type == template_args) {
			ParseResult tpfx;

			tpfx.type = template_prefix;
			tpfx.comps.emplace_back(std::move(n));
			ParseResult pfx;
			pfx.type = prefix;
			pfx.comps.emplace_back(std::move(tpfx));
			const auto& targs = names[i + 1];
			pfx.comps.emplace_back(targs.copy());

			result.emplace_back(std::move(pfx));
			i++;
			continue;
		} else if (n.type == prefix) {
			result.emplace_back(std::move(n));
		} else if (n.type == unqualified_name) {
			ParseResult pfx;
			pfx.type = prefix;
			pfx.comps.emplace_back(std::move(n));
			result.emplace_back(std::move(pfx));
		} else if (n.type == template_args) {
			return std::unexpected(ParseError::mismatched_literal);
		}
	}

	return result;
}

std::expected<ParseResult, ParseError> try_rule_nested_name(std::string_view input, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume) {
	constexpr ProductionType type = nested_name;
	if (max_recursion == 0) {
		return std::unexpected(ParseError::recursion_reached);
	}
	--max_recursion;
	indent++;

	auto remaining = input.substr(1);

	auto xobjparse = parse_as(remaining, explicit_object_member, max_recursion, indent, rules, false);
	if (xobjparse) {
		remaining = remaining.substr(xobjparse->get_consumed_length());
	}

	auto cvparse = parse_as(remaining, cv_qualifiers, max_recursion, indent, rules, false);
	if (cvparse) {
		remaining = remaining.substr(cvparse->get_consumed_length());
	}

	auto refparse = parse_as(remaining, ref_qualifier, max_recursion, indent, rules, false);
	if (refparse) {
		remaining = remaining.substr(1);
	}

	std::vector<ParseResult> names;
	auto old_remaining = remaining;
	while (not remaining.empty() and remaining[0] != 'E') {
		auto uq = parse_as(remaining, unqualified_name, max_recursion, indent, rules, false);
		if (uq) {
			remaining = remaining.substr(uq->get_consumed_length());
			names.emplace_back(std::move(*uq));
		} else if (auto pfx = parse_as(remaining, prefix, max_recursion, indent, rules, false); pfx) {
			remaining = remaining.substr(pfx->get_consumed_length());
			names.emplace_back(std::move(*pfx));

		} else if (auto targs = parse_as(remaining, template_args, max_recursion, indent, rules, false); targs) {
			remaining = remaining.substr(targs->get_consumed_length());
			names.emplace_back(std::move(*targs));
		} else {
			break;
		}
	}

	while (not names.empty() and names.back().type == prefix) {
		names.pop_back();
	}
	remaining = old_remaining;
	for (const auto& n : names) {
		remaining = remaining.substr(n.get_consumed_length());
	}

	if (names.size() < 2) {
		return std::unexpected(ParseError::no_matching_element);
	}

	if (remaining.empty()) {
		return std::unexpected(ParseError::input_too_short);
	}

	if (remaining[0] == 'E' and names.back().type == unqualified_name) {
		auto cnames = convert_names(std::move(names));
		if (not cnames or cnames->size() < 2) {
			return std::unexpected(cnames.error());
		}

		ParseResult result;
		result.type = type;
		result.comps.emplace_back("N");
		if (xobjparse) {
			result.comps.emplace_back(std::move(*xobjparse));
		}
		if (cvparse) {
			result.comps.emplace_back(std::move(*cvparse));
		}
		if (refparse) {
			result.comps.emplace_back(std::move(*refparse));
		}

		for (auto& cn : *cnames) {
			result.comps.emplace_back(std::move(cn));
		}

		result.comps.emplace_back("E");
		if (must_consume and result.get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return result;
	}

	if (remaining[0] == 'E' and names.back().type == template_args and names.size() >= 2 and names[names.size() - 2].type == prefix) {
		auto targs = std::move(names.back());
		names.pop_back();

		auto cnames = convert_names(std::move(names));
		if (not cnames or cnames->empty()) {
			return std::unexpected(cnames.error());
		}
		auto* p = std::get_if<ParseResult>(cnames->back().comps.data());
		if (not p or (p->type != substitution and p->type != template_param)) {
			return std::unexpected(ParseError::no_matching_alternative);
		}

		ParseResult result;
		result.type = type;
		result.comps.emplace_back("N");
		if (xobjparse) {
			result.comps.emplace_back(std::move(*xobjparse));
		}
		if (cvparse) {
			result.comps.emplace_back(std::move(*cvparse));
		}
		if (refparse) {
			result.comps.emplace_back(std::move(*refparse));
		}

		ParseResult tpfx;
		tpfx.type = template_prefix;
		tpfx.comps.emplace_back(std::move(*p));

		for (size_t i = 0; i + 1 < cnames->size(); i++) {
			auto& n = (*cnames)[i];
			result.comps.emplace_back(std::move(n));
		}
		result.comps.emplace_back(std::move(tpfx));
		result.comps.emplace_back(std::move(targs));
		result.comps.emplace_back("E");
		if (must_consume and result.get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return result;
	}

	if (remaining[0] == 'E' and names.back().type == template_args and names.size() >= 3 and names[names.size() - 2].type == unqualified_name) {
		auto targs = std::move(names.back());
		names.pop_back();

		auto cnames = convert_names(std::move(names));
		if (not cnames or cnames->size() < 2) {
			return std::unexpected(cnames.error());
		}

		ParseResult result;
		result.type = type;
		result.comps.emplace_back("N");
		if (xobjparse) {
			result.comps.emplace_back(std::move(*xobjparse));
		}
		if (cvparse) {
			result.comps.emplace_back(std::move(*cvparse));
		}
		if (refparse) {
			result.comps.emplace_back(std::move(*refparse));
		}

		ParseResult tpfx;
		tpfx.type = template_prefix;
		tpfx.comps.emplace_back(std::move(cnames->back()));

		for (size_t i = 0; i + 1 < cnames->size(); i++) {
			auto& n = (*cnames)[i];
			result.comps.emplace_back(std::move(n));
		}
		result.comps.emplace_back(std::move(tpfx));
		result.comps.emplace_back(std::move(targs));
		result.comps.emplace_back("E");
		if (must_consume and result.get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return result;
	}

	return std::unexpected(ParseError::no_matching_alternative);
}

std::expected<ParseResult, ParseError> try_rule_float(std::string_view input, RecursionLimit, size_t, const ProductionGrammar&, bool must_consume) {
	if (input.empty()) {
		return std::unexpected(ParseError::input_too_short);
	}
	auto remaining = input;
	while (not remaining.empty() and (('0' <= remaining[0] and remaining[0] <= '9') or ('a' <= remaining[0] and remaining[0] <= 'f'))) {
		remaining = remaining.substr(1);
	}

	if (remaining.empty()) {
		return std::unexpected(ParseError::input_too_short);
	}
	auto hexfloat = input.substr(0, input.size() - remaining.size());
	if (hexfloat.empty() or (hexfloat.size() & 1)) {
		return std::unexpected(ParseError::mismatched_literal);
	}
	ParseResult result;
	result.type = float_;
	result.comps.emplace_back(hexfloat);
	if (must_consume and result.get_consumed_length() != input.size()) {
		return std::unexpected(ParseError::input_too_long);
	}
	return result;
}

std::expected<ParseResult, ParseError> try_rule_expr_primary(std::string_view input, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume) {
	if (max_recursion == 0) {
		return std::unexpected(ParseError::recursion_reached);
	}
	constexpr auto ltype = ProductionType::expr_primary;
	if (not input.starts_with('L')) {
		return std::unexpected(ParseError::mismatched_literal);
	}
	if (input.starts_with("L_Z")) {
		auto remaining = input.substr(3);
		auto enc = parse_as(remaining, encoding, max_recursion, indent, rules, false);
		if (not enc) {
			enc = parse_as(remaining, data_encoding, max_recursion, indent, rules, false);
		}
		if (enc) {
			remaining = remaining.substr(enc->get_consumed_length());
			if (remaining.starts_with('E')) {
				ParseResult result;
				result.type = ltype;
				result.comps.emplace_back("L_Z");
				result.comps.emplace_back(std::move(*enc));
				result.comps.emplace_back("E");
				if (must_consume and result.get_consumed_length() != input.size()) {
					return std::unexpected(ParseError::input_too_long);
				}
				return result;
			}
		}
	}

	auto remaining = input.substr(1);
	if (remaining.empty()) {
		return std::unexpected(ParseError::input_too_short);
	}
	std::string_view float_type_str;
	switch (remaining[0]) {
	case 'd':
	case 'e':
	case 'f':
	case 'g':
		float_type_str = remaining.substr(0, 1);
		remaining = remaining.substr(1);
		break;

	case 'D':
		if (remaining.starts_with("DF16b") or remaining.starts_with("DF16_") or remaining.starts_with("DF32_") or remaining.starts_with("DF64_")) {
			float_type_str = remaining.substr(0, 5);
			remaining = remaining.substr(5);
		} else if (remaining.starts_with("DF128_")) {
			float_type_str = remaining.substr(0, 6);
			remaining = remaining.substr(6);
		}

	default:
		break;
	}

	std::expected<ParseResult, ParseError> ptype;
	std::expected<ParseResult, ParseError> val;
	if (not float_type_str.empty()) {
		ptype = parse_as(float_type_str, type, max_recursion, indent, rules, false);
		if (not ptype) {
			return std::unexpected(ptype.error());
		}
		val = try_rule_float(remaining, max_recursion, indent, rules, false);

	} else {
		ptype = parse_as(remaining, type, max_recursion, indent, rules, false);
		if (not ptype) {
			return std::unexpected(ptype.error());
		}
		remaining = remaining.substr(ptype->get_consumed_length());
		val = parse_as(remaining, number, max_recursion, indent, rules, false);
	}
	if (not val) {
		return std::unexpected(val.error());
	}
	remaining = remaining.substr(val->get_consumed_length());
	if (remaining.empty()) {
		return std::unexpected(ParseError::input_too_short);
	}
	if (remaining[0] != 'E') {
		return std::unexpected(ParseError::mismatched_literal);
	}
	ParseResult result;
	result.type = ltype;
	result.comps.emplace_back("L");
	result.comps.emplace_back(std::move(*ptype));
	result.comps.emplace_back(std::move(*val));
	result.comps.emplace_back("E");
	if (must_consume and result.get_consumed_length() != input.size()) {
		return std::unexpected(ParseError::input_too_long);
	}
	return result;
}

std::expected<ParseResult, ParseError> try_rule(std::string_view input, ProductionType type, const ProductionRule& rule, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume) {
	if (max_recursion == 0) {
		return std::unexpected(ParseError::recursion_reached);
	}

	if (input.starts_with('E')) {
		return std::unexpected(ParseError::mismatched_literal);
	}
	if (type == vendor_specfic_suffix) {
		ParseResult result;
		result.type = type;
		result.comps.emplace_back(input);
		return result;
	}

	if (type == nested_name) {
		if (not input.starts_with('N')) {
			return std::unexpected(ParseError::mismatched_literal);
		}
		return try_rule_nested_name(input, max_recursion, indent, rules, must_consume);
	}

	if (type == expr_primary) {
		if (not input.starts_with('L')) {
			return std::unexpected(ParseError::mismatched_literal);
		}
		return try_rule_expr_primary(input, max_recursion, indent, rules, must_consume);
	}

	--max_recursion;
	indent++;

	if (type == ProductionType::nonneg_number) {
		if (input.empty()) {
			return std::unexpected(ParseError::input_too_short);
		}
		if (input.front() < '0' or input.front() > '9') {
			return std::unexpected(ParseError::mismatched_literal);
		}
		auto remaining = input;
		while (not remaining.empty() and ('0' <= remaining.front() and remaining.front() <= '9')) {
			remaining = remaining.substr(1);
		}
		ParseResult result;
		result.type = type;
		result.comps.emplace_back(input.substr(0, input.size() - remaining.size()));
		if (must_consume and result.get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return result;
	}

	if (type == ProductionType::substitution) {
		if (not input.starts_with('S') or input.size() < 2) {
			return std::unexpected(ParseError::input_too_short);
		}

		switch (input[1]) {
		case '_': {
			ParseResult result;
			result.type = substitution;
			result.comps.emplace_back(input.substr(0, 2));
			return result;
		}
		case 't':
		case 'a':
		case 'b':
		case 's':
		case 'i':
		case 'o':
		case 'd': {
			ParseResult result;
			result.type = substitution;
			result.comps.emplace_back(input.substr(0, 2));
			return result;
		}

		default:
			break;
		}

		auto full = input;
		input = input.substr(1);

		if ((input.front() < '0' or input.front() > '9') and (input.front() < 'A' or input.front() > 'Z')) {
			return std::unexpected(ParseError::mismatched_literal);
		}
		auto remaining = input;
		while (not remaining.empty() and (('0' <= remaining.front() and remaining.front() <= '9') or ('A' <= remaining.front() and remaining.front() <= 'Z'))) {
			remaining = remaining.substr(1);
		}
		if (not remaining.starts_with('_')) {
			return std::unexpected(ParseError::mismatched_literal);
		}
		ParseResult seqid;
		seqid.type = seq_id;
		seqid.comps.emplace_back(input.substr(0, input.size() - remaining.size()));

		ParseResult result;
		result.type = type;
		result.comps.emplace_back("S");
		result.comps.emplace_back(std::move(seqid));
		result.comps.emplace_back("_");

		if (must_consume and result.get_consumed_length() != full.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return result;
	}

	if (type == ProductionType::digit) {
		if (input.empty()) {
			return std::unexpected(ParseError::input_too_short);
		}
		if (input.front() < '0' or input.front() > '9') {
			return std::unexpected(ParseError::mismatched_literal);
		}

		ParseResult result;
		result.type = type;
		result.comps.emplace_back(input.substr(0, 1));
		if (must_consume and result.get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return result;
	}

	if (type == ProductionType::source_name) {
		if (input.empty()) {
			return std::unexpected(ParseError::input_too_short);
		}
		if (input.front() < '0' or input.front() > '9') {
			return std::unexpected(ParseError::mismatched_literal);
		}
		auto remaining = input;
		while (not remaining.empty() and ('0' <= remaining.front() and remaining.front() <= '9')) {
			remaining = remaining.substr(1);
		}
		auto num = input.substr(0, input.size() - remaining.size());
		auto numvl = parse_uint(num);
		if (not numvl) {
			return std::unexpected(ParseError::mismatched_literal);
		}
		auto [numval, numlen] = *numvl;
		if (remaining.size() < numval or numval == 0) {
			return std::unexpected(ParseError::input_too_short);
		}
		auto ident = remaining.substr(0, numval);

		ParseResult subres;
		subres.type = identifier;
		subres.comps.emplace_back(ident);

		ParseResult result;
		result.type = type;
		result.comps.emplace_back(num);
		result.comps.emplace_back(std::move(subres));
		if (must_consume and result.get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}

		return result;
	}

	for (const auto& alt : rule.alternatives) {
		auto res = try_alternative(input, type, alt, max_recursion, indent, rules, must_consume);
		if (res) {
			auto& result = *res;
			if (must_consume and result.get_consumed_length() != input.size()) {
				return std::unexpected(ParseError::input_too_long);
			}

			return res;
		}
	}

	return std::unexpected(ParseError::no_matching_alternative);
}

std::expected<ParseResult, ParseError> parse_as(std::string_view input, ProductionType ltype, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules, bool must_consume) {
	auto it = rules.rules.find(ltype);
	if (it == rules.rules.end()) {
		return std::unexpected(ParseError::no_matching_rule);
	}
	const auto& [type, rule] = *it;

	auto opt = try_rule(input, type, rule, max_recursion, indent, rules, must_consume);
	if (opt) {
		if (must_consume and opt->get_consumed_length() != input.size()) {
			return std::unexpected(ParseError::input_too_long);
		}
		return opt;
	}

	return std::unexpected(ParseError::no_matching_rule);
}

std::expected<ParseResult, ParseError> parse(std::string_view input, RecursionLimit max_recursion, size_t indent, const ProductionGrammar& rules) {
	return parse_as(input, any_mangled_name, max_recursion, indent, rules, true);
}

std::expected<ParseResult, ParseError> parse(std::string_view input, const ProductionGrammar& rules) {
	RecursionLimit max_recursion {2000};
	size_t indent = 0;
	return parse(input, max_recursion, indent, rules);
}

}