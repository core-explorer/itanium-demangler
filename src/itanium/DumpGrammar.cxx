#include "FormatAst.hxx"
#include "FormatDemangle.hxx"
#include "Grammar.hxx"
#include "OptimizedGrammar.hxx"
#include "ParseName.hxx"
#include "ParseResult.hxx"
#include "../cli/cli_color.hxx"
#include "../util/format.hxx"
#include "../util/print.hxx"
#include "../web/HtmlFormat.hxx"
#include <array>
#include <optional>
#include <set>
#include <string_view>
namespace swartalf::itanium {
namespace {
std::array<std::string_view, 126> production_names = {
    "none",
    "unary_operator_name",
    "binary_operator_name",
    "ternary_operator_name",
    "operator_name",
    "mangled_name",
    "encoding",
    "data_encoding",
    "vendor_specfic_suffix",
    "bare_function_type",
    "special_object",
    "name",
    "nested_name",
    "unscoped_name",
    "unscoped_template_name",
    "template_args",
    "template_arg",
    "local_name",
    "unqualified_name",
    "substitution",
    "vector_type",
    "cv_qualifiers",
    "ref_qualifier",
    "prefix",
    "template_prefix",
    "closure_prefix",
    "template_param",
    "decltype",
    "abi_tags",
    "ctor_dtor_name",
    "source_name",
    "unnamed_type_name",
    "digit",
    "number",
    "nonneg_number",
    "identifier",
    "abi_tag",
    "seq_id",
    "type",
    "builtin_type",
    "qualified_type",
    "function_type",
    "class_enum_type",
    "array_type",
    "pointer_to_member_type",
    "template_template_param",
    "extended_qualifier",
    "explicit_object_member",
    "expression",
    "exception_spec",
    "function_param",
    "expr_primary",
    "base_unresolved_name",
    "braced_expression",
    "initializer",
    "unresolved_name",
    "unresolved_name_gcc",
    "unresolved_type",
    "unresolved_qualifier_level",
    "simple_id",
    "destructor_name",
    "float",
    "discriminator",
    "closure_type_name",
    "lambda_sig",
    "requires_clause",
    "pointer_type",
    "reference_type",
    "rvalue_reference_type",
    "restrict_type",
    "const_type",
    "volatile_type",
    "bind_expression",
    "function_signature",
    "vtable",
    "construction_vtable",
    "typeinfo",
    "vtt",
    "typeinfo_name_object",
    "fold_expression",
    "fold_left_expression",
    "fold_right_expression",
    "conversion_operator_name",
    "call_offset",
    "nv_offset",
    "v_offset",
    "nonvirtual_thunk",
    "virtual_thunk",
    "virtual_covariant_thunk",
    "guard_variable",
    "lifetime_extended_temporary",
    "template_param_decl",
    "partial_nested_name",
    "requires_expression",
    "requirement_llvm",
    "constrained_type_llvm",
    "type_constraint_llvm",
    "pack_expression",
    "call_expression",
    "mangled_name_apple",
    "prefix_operator_expression",
    "vendor_ext_arg_type_llm",
    "negative_number",
    "template_arg_pack",
    "template_template_type",
    "complex_type_c99",
    "conversion_expression",
    "pack_type",
    "keyword_cast_operator",
    "keyword_cast_expression",
    "keyword_type_expression",
    "keyword_expression",
    "rethrow_expression",
    "type_expression_operator",
    "expression_operator",
    "builtin_real_type",
    "builtin_boolean_type",
    "builtin_integral_type",
    "builtin_other_type",
    "complex_type",
    "pack_indexing_type",
    "pack_indexing_expression",
    "dot_vendor_specific_suffix",
    "global_sub_for_mangled_name",
    "any_mangled_name",
    "overload_set",
};
}

std::string_view to_string_view(ProductionType t) {
	return production_names[static_cast<uint8_t>(t)];
}

std::optional<ProductionType> to_production_type(std::string_view pn) {
	for (size_t i = 0; i < production_names.size(); i++) {
		if (production_names[i] == pn) {
			return static_cast<ProductionType>(i);
		}
	}
	return std::nullopt;
}

enum class RuleClassification : uint8_t {
	single_element_literals_only,
	single_element_productions_only,
	single_alternative_leading_with_literal,
	single_alternative_repeat_production,
	mixed,
	no_alternatives
};

RuleClassification classify(const ProductionRule& rule) {
	bool has_single_literal = false;
	bool has_single_production = false;
	bool has_single_repeat_production = false;
	bool has_other = false;
	bool all_lead_literal = true;

	for (const auto& alt : rule.alternatives) {
		const auto& e = alt.sequence.elements[0].element;
		const auto* el = std::get_if<LiteralSequence>(&e);
		const auto* ep = std::get_if<ProductionType>(&e);
		const auto* er = std::get_if<Repeat<ProductionType>>(&e);
		if (not el) {
			all_lead_literal = false;
		}
		if (alt.sequence.elements.size() != 1) {
			has_other = true;
		} else {
			if (el) {
				has_single_literal = true;
			}
			if (er) {
				has_single_repeat_production = true;
			}
			if (ep) {
				has_single_production = true;
			}
			if (not el and not ep and not er) {
				has_other = true;
			}
		}
	}

	if (has_single_literal and not has_other) {
		return RuleClassification::single_element_literals_only;
	}
	if (has_single_production and not has_other) {
		return RuleClassification::single_element_productions_only;
	}
	if (rule.alternatives.size() == 1) {
		if (has_single_repeat_production) {
			return RuleClassification::single_alternative_repeat_production;
		}
		if (all_lead_literal) {
			return RuleClassification::single_alternative_leading_with_literal;
		}
	}
	if (rule.alternatives.empty()) {
		return RuleClassification::no_alternatives;
	}

	return RuleClassification::mixed;
}

std::string to_string(const ParseResult& result) {
	std::string out;
	out += to_string_view(result.type);
	for (const auto& sc : result.comps) {
		out += " ";
		if (std::holds_alternative<std::string_view>(sc)) {
			const auto& sv = std::get<std::string_view>(sc);
			out += sv;
		} else {
			const auto& c = std::get<ParseResult>(sc);

			out += "{";
			out += to_string(c);
			out += "}";
		}
	}

	return out;
}

std::string to_string(const CharRanges& r) {
	std::string res;
	for (const auto [a, b] : r.ranges) {
		if (a == b) {
			res += fmt::format("{}", a);
		} else {
			res += fmt::format("{}-{}", a, b);
		}
	}
	return res;
}

struct CliRuleHighlighter {
	std::string operator()(std::string_view s, RuleClassification rc) const {
		if (colored_output == CliColorMode::uncolored) {
			return std::string(s);
		} else {
			switch (rc) {
			case RuleClassification::single_element_productions_only:
				return bold_blue(s);
			case RuleClassification::single_element_literals_only:
				return bold_green(s);
			case RuleClassification::single_alternative_leading_with_literal:
				return bold_cyan(s);
			case RuleClassification::single_alternative_repeat_production:
				return bold_magenta(s);
			case RuleClassification::no_alternatives:
				return bold_red(s);
			default:
				if (s == to_string_view(ProductionType::identifier) or s == to_string_view(ProductionType::source_name)
				    or s == to_string_view(ProductionType::nonneg_number) or s == to_string_view(ProductionType::substitution) or s == to_string_view(ProductionType::template_param) or s == to_string_view(ProductionType::seq_id) or s == to_string_view(ProductionType::float_)) {
					return bold_yellow(s);
				}
				return bold(s);
			}
		}
	}
	std::string italic(std::string_view s) const {
		if (colored_output == CliColorMode::uncolored) {
			return std::string(s);
		} else {
			return swartalf::italic(s);
		}
	}
	std::string escape(std::string_view s) const {
		return std::string(s);
	}
};
struct HtmlRuleHighlighter {
	std::string operator()(std::string_view s, RuleClassification rc) const {
		std::string style;
		switch (rc) {
		case RuleClassification::single_element_productions_only:
			style = "color:brown";
			break;
		case RuleClassification::single_element_literals_only:
			style = "color:green;font-weight:bold";
			break;
		case RuleClassification::single_alternative_leading_with_literal:
			style = "color:olive";
			break;
		case RuleClassification::single_alternative_repeat_production:
			style = "color:magenta";
			break;
		case RuleClassification::no_alternatives:
			style = "color:red";
			break;
		default:
			if (s == to_string_view(ProductionType::identifier) or s == to_string_view(ProductionType::source_name)
			    or s == to_string_view(ProductionType::nonneg_number) or s == to_string_view(ProductionType::substitution) or s == to_string_view(ProductionType::template_param) or s == to_string_view(ProductionType::seq_id) or s == to_string_view(ProductionType::float_)) {
				style = "color:yellow;font-weight:bold";
			}
			style = "font-weight:bold";
		}
		return fmt::format("<span style=\"{}\">{}</span>", style, escape_xml(s));
	}
	std::string italic(std::string_view s) const {
		return "<i>" + escape_xml(s) + "</i>";
	}
	std::string escape(std::string_view s) const {
		return escape_xml(s);
	}
};

template <typename Ruler>
std::string format_elements(std::span<const ProductionElement> elts, const ProductionGrammar& rules, const Ruler& highlight_rule) {
	std::string alt_elts;
	bool is_first = true;
	for (const auto& eelt : elts) {
		if (not is_first) {
			alt_elts += " ";
		} else {
			is_first = false;
		}

		const auto& elt = eelt.element;
		if (const auto* p = get_if<LiteralSequence>(&elt); p) {
			alt_elts += fmt::format("\"{}\"", p->literal);
		}

		if (const auto* op = get_if<Optional<LiteralSequence>>(&elt); op) {
			const auto* p = &op->val;
			alt_elts += fmt::format("[\"{}\"]", p->literal);
		}

		if (const auto* p = get_if<ProductionType>(&elt); p) {
			auto it = rules.rules.find(*p);
			auto rc = it == rules.rules.end() ? RuleClassification::no_alternatives : classify(it->second);
			alt_elts += highlight_rule.escape("<") + highlight_rule(to_string_view(*p), rc) + highlight_rule.escape(">");
		}

		if (const auto* op = get_if<Optional<ProductionType>>(&elt); op) {
			const auto* p = &op->val;
			auto it = rules.rules.find(*p);
			auto rc = it == rules.rules.end() ? RuleClassification::no_alternatives : classify(it->second);
			alt_elts += highlight_rule.escape("[<") + highlight_rule(to_string_view(*p), rc) + highlight_rule.escape(">]");
		}

		if (const auto* rp = get_if<Repeat<ProductionType>>(&elt); rp) {
			const auto* p = &rp->val;
			auto it = rules.rules.find(*p);
			auto rc = it == rules.rules.end() ? RuleClassification::no_alternatives : classify(it->second);
			alt_elts += highlight_rule.escape("<") + highlight_rule(to_string_view(*p), rc) + highlight_rule.escape(">+");
		}

		if (const auto* p = get_if<CharRange>(&elt); p) {
			alt_elts += highlight_rule.escape(fmt::format("<{}-{}>", p->start, p->stop));
		}
		if (const auto* rp = get_if<Repeat<CharRanges>>(&elt); rp) {
			const auto* p = &rp->val;
			alt_elts += highlight_rule.escape(fmt::format("<{}>+", to_string(*p)));
		}

		if (const auto* op = get_if<OptionalRepeat<ProductionType>>(&elt); op) {
			const auto* p = &op->val.val;
			auto it = rules.rules.find(*p);
			auto rc = it == rules.rules.end() ? RuleClassification::no_alternatives : classify(it->second);
			alt_elts += highlight_rule.escape("<") + highlight_rule(to_string_view(*p), rc) + highlight_rule.escape(">*");
		}

		if (const auto* op = get_if<HomogeneousAlternative<ProductionType>>(&elt); op) {
			const auto* p0 = &op->val.first;
			const auto* p1 = &op->val.second;
			auto it0 = rules.rules.find(*p0);
			auto rc0 = it0 == rules.rules.end() ? RuleClassification::no_alternatives : classify(it0->second);
			auto it1 = rules.rules.find(*p1);
			auto rc1 = it1 == rules.rules.end() ? RuleClassification::no_alternatives : classify(it1->second);
			alt_elts += highlight_rule.escape("{<") + highlight_rule(to_string_view(*p0), rc0) + highlight_rule.escape("> | <") + highlight_rule(to_string_view(*p1), rc1) + highlight_rule.escape(">}");
		}

		if (const auto* op = get_if<OptionalHomogeneousAlternative<ProductionType>>(&elt); op) {
			const auto* p0 = &op->val.val.first;
			const auto* p1 = &op->val.val.second;
			auto it0 = rules.rules.find(*p0);
			auto rc0 = it0 == rules.rules.end() ? RuleClassification::no_alternatives : classify(it0->second);
			auto it1 = rules.rules.find(*p1);
			auto rc1 = it1 == rules.rules.end() ? RuleClassification::no_alternatives : classify(it1->second);
			alt_elts += highlight_rule.escape("[<") + highlight_rule(to_string_view(*p0), rc0) + highlight_rule.escape("> | <") + highlight_rule(to_string_view(*p1), rc1) + highlight_rule.escape(">]");
		}
	}
	return alt_elts;
}

template <typename Ruler>
std::string format_grammar(const ProductionGrammar& rules, ProductionType k, const Ruler& highlight_rule) {
	auto it = rules.rules.find(k);
	if (it == rules.rules.end()) {
		return {};
	}

	const auto& v = it->second;
	auto rc = classify(v);

	auto tk = to_string_view(k);
	auto sp = std::string(tk.size(), ' ');

	std::string result;
	for (size_t i = 0; i < v.alternatives.size(); i++) {
		const auto& alt = v.alternatives[i].sequence.elements;
		auto salt = format_elements(alt, rules, highlight_rule);

		std::string extra;
		if (alt.size() == 1) {
			const auto* x = std::get_if<LiteralSequence>(&alt[0].element);
			if (x and x->literal != "S_" and x->literal != "T_") {
				RecursionLimit max_recursion {20};
				auto parsed = parse_as(x->literal, k, max_recursion, 0, rules, true);
				if (parsed) {
					SubstitutionTableView subs;
					NestedTemplateParams tp;
					DemanglingDialect dv;
					SkipSpec skip;

					std::string dema = "_ERROR";
					auto edema = format_demangled_ascii(*parsed, subs, tp, dv, skip, max_recursion);
					if (edema) {
						dema = std::move(*edema);
					}
					if (dema == "_ERROR") {
						auto tdema = compose(format_demangled_type_ascii(*parsed, subs, tp, dv, skip, max_recursion));
						if (tdema) {
							dema = std::move(*tdema);
						}
					}
					if (not dema.empty() and dema != "_ERROR") {
						extra = highlight_rule.italic(" # " + dema);
					}
				}
			}
		}

		if (i == 0) {
			result += fmt::format("{} := {}{}\n", highlight_rule(tk, rc), salt, extra);
		} else {
			result += fmt::format("{} := {}{}\n", sp, salt, extra);
		}
	}
	return result;
}

template <typename Ruler>
std::string format_grammar_recursive(const ProductionGrammar& rules, ProductionType k, std::set<ProductionType>& visited, std::vector<ProductionType>& to_visit, const Ruler& highlight_rule) {
	if (visited.contains(k)) {
		return {};
	}
	std::string result;
	result += format_grammar(rules, k, highlight_rule);
	visited.insert(k);

	auto it = rules.rules.find(k);
	if (it == rules.rules.end()) {
		return result;
	}
	const auto& v = it->second;
	for (size_t i = 0; i < v.alternatives.size(); i++) {
		const auto& alt = v.alternatives[i].sequence.elements;
		for (const auto& ee : alt) {
			const auto& e = ee.element;

			if (const auto* pt = std::get_if<ProductionType>(&e); pt) {
				to_visit.emplace_back(*pt);
			}
			if (const auto* pt = std::get_if<Optional<ProductionType>>(&e); pt) {
				to_visit.emplace_back(pt->val);
			}
			if (const auto* pt = std::get_if<Repeat<ProductionType>>(&e); pt) {
				to_visit.emplace_back(pt->val);
			}
			if (const auto* pt = std::get_if<OptionalRepeat<ProductionType>>(&e); pt) {
				to_visit.emplace_back(pt->val.val);
			}
			if (const auto* pt = std::get_if<HomogeneousAlternative<ProductionType>>(&e); pt) {
				to_visit.emplace_back(pt->val.first);
				to_visit.emplace_back(pt->val.second);
			}
			if (const auto* pt = std::get_if<OptionalHomogeneousAlternative<ProductionType>>(&e); pt) {
				to_visit.emplace_back(pt->val.val.first);
				to_visit.emplace_back(pt->val.val.second);
			}
		}
	}
	return result;
}

template <typename Ruler>
std::string format_grammar_recursive(const ProductionGrammar& rules, ProductionType k, const Ruler& highlight_rule) {
	std::set<ProductionType> visited;
	std::vector<ProductionType> to_visit;
	std::string result;
	result += format_grammar_recursive(rules, k, visited, to_visit, highlight_rule);
	for (size_t i = 0; i < to_visit.size(); i++) {
		result += format_grammar_recursive(rules, to_visit[i], visited, to_visit, highlight_rule);
	}
	return result;
}
std::string format_grammar_recursive_cli(const ProductionGrammar& rules, ProductionType k) {
	CliRuleHighlighter highlight_rule;
	return format_grammar_recursive(rules, k, highlight_rule);
}
std::string format_grammar_recursive_html(const ProductionGrammar& rules, ProductionType k) {
	HtmlRuleHighlighter highlight_rule;
	return format_grammar_recursive(rules, k, highlight_rule);
}

template <typename Ruler>
std::string format_reverse_grammar(const ProductionGrammar& rules, const Ruler& highlight_rule) {
	std::map<std::string, std::set<std::string>> reverse_rules;
	for (const auto& [k, v] : rules.rules) {
		auto kn = fmt::format("<{}>", to_string_view(k));
		for (size_t i = 0; i < v.alternatives.size(); i++) {
			const auto& alt = v.alternatives[i].sequence.elements;
			auto salt = format_elements(alt, rules, highlight_rule);
			reverse_rules[salt].insert(kn);
		}
	}

	while (true) {
		bool added_new = false;
		for (auto& [k, v] : reverse_rules) {
			std::set<std::string> to_add;
			for (const auto& q : v) {
				auto it = reverse_rules.find(q);
				if (it != reverse_rules.end()) {
					to_add.insert(it->second.begin(), it->second.end());
				}
			}
			auto before = v.size();
			v.insert(to_add.begin(), to_add.end());
			if (v.size() > before) {
				added_new = true;
			}
		}
		if (not added_new) {
			break;
		}
	}

	std::string result;
	for (const auto& [k, v] : reverse_rules) {
		auto sp = std::string(k.size(), ' ');
		bool first = true;
		for (const auto& q : v) {
			if (first) {
				first = false;
				result += fmt::format("{} -> {}\n", k, q);
			} else {
				result += fmt::format("{} -> {}\n", sp, q);
			}
		}
	}
	return result;
}

std::string format_elements_cli(std::span<const ProductionElement> elts, const ProductionGrammar& rules) {
	CliRuleHighlighter highlight_rule;
	return format_elements(elts, rules, highlight_rule);
}

}
