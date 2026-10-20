#pragma once
#include "ItaniumOperatorType.hxx"
#include "ProductionType.hxx"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>
namespace swartalf::itanium {

struct CharRange {
	char start;
	char stop;
	constexpr CharRange(char a, char b) : start(a), stop(b) {}
	constexpr explicit CharRange(char a) : start(a), stop(a) {}
};

struct CharRanges {
	std::vector<CharRange> ranges;

	explicit CharRanges(CharRange ab) {
		ranges.emplace_back(ab);
	}

	template <typename... Args>
	CharRanges(CharRange ab, CharRange cd, Args... args) {
		ranges.emplace_back(ab);
		ranges.emplace_back(cd);
		(ranges.emplace_back(args), ...);
	}
};

struct LiteralSequence {
	std::string_view literal;
	constexpr LiteralSequence() = default;
	explicit constexpr LiteralSequence(std::string_view a) : literal(a) {}
};

template <typename T>
struct Optional {
	using value_type = T;

	Optional() = default;
	explicit Optional(T a) : val(std::move(a)) {}

	T val;
};

template <typename T>
Optional(T v) -> Optional<std::remove_cvref_t<T>>;

template <typename T>
struct HomogeneousAlternative {
	using value_type = T;
	std::pair<T, T> val;
	HomogeneousAlternative() = default;
	HomogeneousAlternative(T a, T b) : val({a, b}) {}
};

template <typename T>
using OptionalHomogeneousAlternative = Optional<HomogeneousAlternative<T>>;

constexpr HomogeneousAlternative<ProductionType> alt(ProductionType pt0, ProductionType pt1) {
	return HomogeneousAlternative<ProductionType> {pt0, pt1};
}

constexpr OptionalHomogeneousAlternative<ProductionType> opt_alt(ProductionType pt0, ProductionType pt1) {
	return Optional {HomogeneousAlternative(pt0, pt1)};
}

constexpr Optional<ProductionType> opt(ProductionType pt) {
	return Optional<ProductionType> {pt};
}

constexpr Optional<LiteralSequence> opt(std::string_view lit) {
	return Optional<LiteralSequence> {LiteralSequence {lit}};
}

template <typename T>
struct Repeat {
	using value_type = T;
	T val;
	Repeat() = default;
	explicit Repeat(T a) : val(std::move(a)) {}
};

constexpr Repeat<ProductionType> rep(ProductionType pt) {
	return Repeat<ProductionType> {pt};
}

constexpr Repeat<CharRanges> rep(CharRanges pt) {
	return Repeat<CharRanges> {pt};
}

template <typename T>
using OptionalRepeat = Optional<Repeat<T>>;

constexpr OptionalRepeat<ProductionType> opt_rep(ProductionType pt) {
	return Optional {Repeat<ProductionType>(pt)};
}

struct ProductionElement {
	std::variant<LiteralSequence,
	    ProductionType,
	    Optional<ProductionType>,
	    Repeat<ProductionType>,
	    Optional<LiteralSequence>,
	    OptionalRepeat<ProductionType>,
	    CharRange,
	    Repeat<CharRanges>,
	    HomogeneousAlternative<ProductionType>,
	    OptionalHomogeneousAlternative<ProductionType>>
	    element;

	ProductionElement() = default;
	ProductionElement(ProductionType pt) :
	    element(pt) {}
	ProductionElement(Optional<ProductionType> pt) :
	    element(pt) {}
	ProductionElement(Optional<LiteralSequence> lit) :
	    element(lit) {}
	ProductionElement(Repeat<ProductionType> pt) :
	    element(pt) {}
	ProductionElement(CharRange pt) :
	    element(pt) {}
	ProductionElement(Repeat<CharRanges> pt) :
	    element(pt) {}
	ProductionElement(OptionalRepeat<ProductionType> pt) :
	    element(pt) {}
	ProductionElement(HomogeneousAlternative<ProductionType> pt) :
	    element(pt) {}
	ProductionElement(OptionalHomogeneousAlternative<ProductionType> pt) :
	    element(pt) {}
	ProductionElement(std::string_view lit) :
	    element(LiteralSequence {lit}) {}
	ProductionElement(ItaniumOperatorType op) :
	    element(LiteralSequence {to_mangled_name(op)}) {}
};

struct ProductionSequence {
	std::vector<ProductionElement> elements;
};

struct ProductionAlternative {
	ProductionSequence sequence;

	template <typename T>
	void add_element(T&& e) {
		sequence.elements.emplace_back(std::forward<T>(e));
	}
};

struct ProductionRule {
	std::vector<ProductionAlternative> alternatives;
	ProductionRule() = default;
	ProductionRule(std::vector<ProductionAlternative>&& alts) :
	    alternatives(std::move(alts)) {}
};

struct ProductionGrammar {
	std::map<ProductionType, ProductionRule> rules;
	std::unordered_map<char, std::unordered_set<ProductionType>> lookup;

	template <ProductionType pt, typename... Args>
	void add_rule(Args&&... args) {
		auto it = rules.find(pt);
		if (it == rules.end()) {
			auto [new_it, inserted] = rules.emplace(pt, ProductionRule {});
			it = new_it;
		}

		auto& rule = it->second;
		rule.alternatives.emplace_back();
		auto& alt = rule.alternatives.back();
		alt.sequence.elements.reserve(sizeof...(args));
		(alt.sequence.elements.emplace_back(args), ...);
	}
};

ProductionGrammar build_grammar();

}