#include "OptimizedGrammar.hxx"
#include "Grammar.hxx"
#include <map>
#include <set>
#include <string_view>
#include <variant>

namespace swartalf::itanium {

std::set<char> get_basic_alphabet() {
	std::set<char> alphabet;
	for (char c = '0'; c <= '9'; ++c) {
		alphabet.insert(c);
	}
	for (char c = 'A'; c <= 'Z'; ++c) {
		alphabet.insert(c);
	}
	for (char c = 'a'; c <= 'z'; ++c) {
		alphabet.insert(c);
	}
	alphabet.insert('_');
	alphabet.insert('.');
	alphabet.insert('$');
	return alphabet;
}

std::set<char> get_possible_starts(const ProductionElement& elt) {
	std::set<char> result;
	const auto* x = std::get_if<LiteralSequence>(&elt.element);
	if (x) {
		result.insert(x->literal[0]);
	}
	const auto* ox = std::get_if<Optional<LiteralSequence>>(&elt.element);
	if (ox) {
		result.insert(ox->val.literal[0]);
	}
	const auto* y = std::get_if<CharRange>(&elt.element);
	if (y) {
		for (auto c = y->start; c <= y->stop; ++c) {
			result.insert(c);
		}
	}
	const auto* z = std::get_if<Repeat<CharRanges>>(&elt.element);
	if (z) {
		for (const auto& r : z->val.ranges) {
			for (auto c = r.start; c <= r.stop; ++c) {
				result.insert(c);
			}
		}
	}
	return result;
}

bool is_optional(const ProductionElement& elt) {
	if (std::holds_alternative<Optional<LiteralSequence>>(elt.element)) {
		return true;
	}
	if (std::holds_alternative<Optional<ProductionType>>(elt.element)) {
		return true;
	}
	if (std::holds_alternative<OptionalRepeat<ProductionType>>(elt.element)) {
		return true;
	}
	return false;
}

std::set<char> get_possible_starts(const ProductionSequence& seq) {
	std::set<char> result;
	for (size_t i = 0; i < seq.elements.size(); i++) {
		auto tmp = get_possible_starts(seq.elements[i]);
		for (auto c : tmp) {
			result.insert(c);
		}

		if (not is_optional(seq.elements[i])) {
			break;
		}
	}
	return result;
}

std::set<char> get_possible_starts(const ProductionAlternative& alt) {
	return get_possible_starts(alt.sequence);
}

std::set<char> get_possible_starts(const ProductionRule& rule) {
	std::set<char> result;
	for (const auto& alt : rule.alternatives) {
		auto tmp = get_possible_starts(alt);
		for (auto c : tmp) {
			result.insert(c);
		}
	}
	return result;
}

std::set<char> propagate_possible_starts(const ProductionRule& rule, const std::map<ProductionType, std::set<char>>& basic_possible_start, const ProductionGrammar& rules);

std::set<char> propagate_possible_starts(const ProductionElement& elt, const std::map<ProductionType, std::set<char>>& basic_possible_start, const ProductionGrammar& rules) {
	(void)rules;
	std::set<char> result;
	const auto* x = std::get_if<LiteralSequence>(&elt.element);
	if (x) {
		result.insert(x->literal[0]);
	}
	const auto* ox = std::get_if<Optional<LiteralSequence>>(&elt.element);
	if (ox) {
		result.insert(ox->val.literal[0]);
	}
	const auto* y = std::get_if<CharRange>(&elt.element);
	if (y) {
		for (auto c = y->start; c <= y->stop; ++c) {
			result.insert(c);
		}
	}
	const auto* z = std::get_if<Repeat<CharRanges>>(&elt.element);
	if (z) {
		for (const auto& r : z->val.ranges) {
			for (auto c = r.start; c <= r.stop; ++c) {
				result.insert(c);
			}
		}
	}

	const auto* u = std::get_if<ProductionType>(&elt.element);
	if (u) {
		auto tmp = basic_possible_start.find(*u)->second;
		for (auto c : tmp) {
			result.insert(c);
		}
	}
	const auto* ru = std::get_if<Repeat<ProductionType>>(&elt.element);
	if (ru) {
		auto tmp = basic_possible_start.find(ru->val)->second;
		for (auto c : tmp) {
			result.insert(c);
		}
	}
	const auto* ou = std::get_if<Repeat<ProductionType>>(&elt.element);
	if (ou) {
		auto tmp = basic_possible_start.find(ou->val)->second;
		for (auto c : tmp) {
			result.insert(c);
		}
	}
	const auto* oru = std::get_if<OptionalRepeat<ProductionType>>(&elt.element);
	if (oru) {
		auto tmp = basic_possible_start.find(oru->val.val)->second;
		for (auto c : tmp) {
			result.insert(c);
		}
	}

	return result;
}
std::set<char> propagate_possible_starts(const ProductionSequence& seq, const std::map<ProductionType, std::set<char>>& basic_possible_start, const ProductionGrammar& rules) {
	std::set<char> result;
	for (size_t i = 0; i < seq.elements.size(); i++) {
		auto tmp = propagate_possible_starts(seq.elements[i], basic_possible_start, rules);
		for (auto c : tmp) {
			result.insert(c);
		}

		if (not is_optional(seq.elements[i])) {
			break;
		}
	}
	return result;
}

std::set<char> propagate_possible_starts(const ProductionAlternative& alt, const std::map<ProductionType, std::set<char>>& basic_possible_start, const ProductionGrammar& rules) {
	return propagate_possible_starts(alt.sequence, basic_possible_start, rules);
}

std::set<char> propagate_possible_starts(const ProductionRule& rule, const std::map<ProductionType, std::set<char>>& basic_possible_start, const ProductionGrammar& rules) {
	std::set<char> result;
	for (const auto& alt : rule.alternatives) {
		auto tmp = propagate_possible_starts(alt, basic_possible_start, rules);
		for (auto c : tmp) {
			result.insert(c);
		}
	}
	return result;
}

std::unordered_map<char, std::unordered_set<ProductionType>> optimize(const ProductionGrammar& rules) {
	std::map<ProductionType, std::set<char>> possible_start;
	std::map<ProductionType, std::set<char>> possible_start1;

	std::unordered_map<char, std::unordered_set<ProductionType>> possible_rules;

	for (const auto& [k, v] : rules.rules) {
		possible_start[k] = get_possible_starts(v);
	}

	for (size_t i = 0; i < 10; i++) {
		for (const auto& [k, v] : rules.rules) {
			possible_start1[k] = propagate_possible_starts(v, possible_start, rules);
		}
		std::swap(possible_start, possible_start1);
	}

	for (auto& [k, v] : possible_start) {
		for (auto c : v) {
			possible_rules[c].insert(k);
		}
	}

	return possible_rules;
}
}