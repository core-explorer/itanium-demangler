#include "FormatDemangle.hxx"
#include "Base36.hxx"
#include "DemangleBasicType.hxx"
#include "DemanglingDialect.hxx"
#include "FormatAst.hxx"
#include "FormatMangle.hxx"
#include "FunctionClassification.hxx"
#include "Grammar.hxx"
#include "HighlightedText.hxx"
#include "ParseName.hxx"
#include "ParseResult.hxx"
#include "ProductionType.hxx"
#include "../cli/CliDecorator.hxx"
#include "../util/split.hxx"
#include "../util/string.hxx"
#include <cstring>
#include <expected>
#include <optional>
#include <string_view>
#include <variant>
namespace swartalf::itanium {
using enum ProductionType;
using namespace std::string_literals;
using namespace std::string_view_literals;

const ParseResult* resolve_template_param(const ParseResult& res, const NestedTemplateParams& tp) {
	const auto* p = std::get_if<std::string_view>(res.comps.data());
	if (not p) {
		return nullptr;
	}
	if (not tp.empty()) {
		const auto& tpb = tp.back();
		if (not tpb.empty()) {
			if (*p == "T_" or (*p == "T" and res.comps.size() == 2)) {
				return tpb.entries.data();
			}
			if (*p == "T" and res.comps.size() >= 2) {
				const auto* q = std::get_if<ParseResult>(&res.comps[1]);
				if (q and q->type == nonneg_number) {
					const auto* qq = std::get_if<std::string_view>(q->comps.data());
					if (qq) {
						auto num = parse_uint(*qq);
						if (num) {
							auto [val, len] = *num;
							if (val + 1 < tpb.entries.size()) {
								return &tpb.entries[val + 1];
							}
						}
					}
				}
			}
		}
	}
	return nullptr;
}

std::tuple<const ParseResult*, SubstitutionTableView, const NestedTemplateParams*> resolve_substitution(const ParseResult& res, const SubstitutionTableView& subs) {
	const auto* p = std::get_if<std::string_view>(res.comps.data());
	if ((*p == "S_" or (*p == "S" and res.comps.size() == 2 and std::holds_alternative<std::string_view>(res.comps[1]))) and not subs.entries.empty()) {
		SubstitutionTableView new_subs;
		return std::make_tuple(subs.entries.data(), new_subs, subs.stp.data());
	}
	if (*p == "S" and res.comps.size() >= 2) {
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);
		if (q and q->type == seq_id) {
			const auto* qq = std::get_if<std::string_view>(q->comps.data());
			if (qq) {
				auto num = parseNumberBase36(*qq);
				if (num) {
					auto [val, len] = *num;
					auto idx = val + 1;
					if (idx < subs.entries.size()) {
						SubstitutionTableView new_subs = subs.subspan(0, idx);
						return std::make_tuple(&subs.entries[idx], new_subs, &subs.stp[idx]);
					}
				}
			}
		}
	}

	return {nullptr, {}, nullptr};
}

std::optional<int> get_pack_size(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, RecursionLimit max_recursion) {
	if (max_recursion == 0) {
		return std::nullopt;
	}
	--max_recursion;
	if (res.type == template_param) {
		const auto* tr = resolve_template_param(res, tp);
		if (not tr) {
			return std::nullopt;
		}
		const auto* pk = std::get_if<ParseResult>(tr->comps.data());
		if (not pk or pk->type != template_arg_pack) {
			return std::nullopt;
		}
		int count = 0;
		for (const auto& c : pk->comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				count++;
			}
		}
		return count;
	}

	if (res.type == substitution) {
		auto [sr, new_subs, stp] = resolve_substitution(res, subs);
		if (not sr) {
			return std::nullopt;
		}
		return get_pack_size(*sr, new_subs, *stp, max_recursion);
	}

	for (const auto& c : res.comps) {
		if (const auto* p = std::get_if<ParseResult>(&c); p) {
			auto op = get_pack_size(*p, subs, tp, max_recursion);
			if (op) {
				return op;
			}
		}
	}
	return std::nullopt;
}

namespace {

std::string make_error() {
	return "_ERROR"s;
}

struct PackSelector {
	using value_type = int;
	static constexpr value_type all = -1;
	value_type val = all;
};

template <ProductionType ltype>
struct FormatDemangle {
private:
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult&, const SubstitutionTableView&, const NestedTemplateParams&, const DemanglingDialect&, Formatter&, Stacked<typename Formatter::value_type>&, PackSelector, const SkipSpec&, RecursionLimit) {
		using StringType = Formatter::value_type;
		return StringType(make_error());
	}

	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult&, const SubstitutionTableView&, const NestedTemplateParams&, const DemanglingDialect&, Formatter&, PackSelector, const SkipSpec&, RecursionLimit) {
		using StringType = Formatter::value_type;
		return StringType(make_error());
	}
};

template <ProductionType ltype, typename Formatter>
auto format_demangled_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion);

template <ProductionType ltype, typename Formatter>
auto format_demangled(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion);

template <>
struct FormatDemangle<class_enum_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		(void)stack;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto result = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				return result;
			}
		}

		using StringType = typename Formatter::value_type;
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<extended_qualifier> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (not p) {
					p = x;
				} else {
					q = x;
					break;
				}
			}
		}
		using StringType = typename Formatter::value_type;
		if (not p or not q) {
			return StringType(make_error());
		}
		auto tmp = compose(format_demangled_type(*q, subs, tp, dv, formatter, stack, ps, skip, max_recursion));
		if (not tmp) {
			return tmp;
		}
		*tmp += " ";
		auto tmp2 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
		if (not tmp2) {
			return tmp2;
		}
		*tmp += *tmp2;
		return tmp;
	}
};

template <>
struct FormatDemangle<pointer_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				stack.append(Decoration::pointer, formatter, "*");
				auto result = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not result) {
					return result;
				}
				auto [text, mod, invalid] = stack.pop();
				if (not invalid) {
					result->head += text;
				}

				return result;
			}
		}

		using StringType = typename Formatter::value_type;
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<restrict_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				StringType tmp = " "s;
				switch (dv.restrict_keyword) {
				case swartalf::RestrictKeyword::no_underscore:
					tmp += formatter.decorate(Decoration::const_keyword, "restrict");
					break;
				case swartalf::RestrictKeyword::one_underscore:
					tmp += formatter.decorate(Decoration::const_keyword, "__restrict");
					break;
				case swartalf::RestrictKeyword::two_underscore:
					tmp += formatter.decorate(Decoration::const_keyword, "__restrict__");
					break;
				}

				stack.stack.emplace_back(std::move(tmp), StackModifierType::restrict_);
				auto result = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not result) {
					return result;
				}
				auto [text, mod, invalid] = stack.pop();
				if (not invalid) {
					result->head += text;
				}

				return result;
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<pack_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		if (not dv.resolve_template_args or not dv.resolve_packs) {
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					auto tmp = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					return *tmp + "...";
				}
			}
		}

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto ops = get_pack_size(*p, subs, tp, max_recursion);
				if (not ops) {
					return StringType(make_error());
				}

				if (*ops == 0) {
					return {};
				}

				StringType result;

				for (int i = 0; i < *ops; ++i) {
					Stacked<StringType> pstack;
					PackSelector pi {i};
					auto tmp = format_demangled_type(*p, subs, tp, dv, formatter, pstack, pi, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					if (tmp->empty()) {
						continue;
					}
					if (not result.empty()) {
						result += ", ";
					}
					result += *compose(tmp);
				}
				return result;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<pointer_to_member_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (not p) {
					p = x;
				} else {
					q = x;
					break;
				}
			}
		}
		if (not p or not q) {
			return StringType(make_error());
		}

		Stacked<StringType> pstack;
		auto cn = format_demangled_type(*p, subs, tp, dv, formatter, pstack, ps, skip, max_recursion);
		if (not cn) {
			return cn;
		}

		stack.append(compose(*cn) + formatter.decorate(Decoration::noise, "::") + formatter.decorate(Decoration::pointer, "*"), StackModifierType::pointer);
		auto result = format_demangled_type(*q, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
		if (not result) {
			return result;
		}
		auto [text, mod, invalid] = stack.pop();
		if (not invalid) {
			result->head += " ";
			result->head += text;
		}

		return result;
	}
};

template <>
struct FormatDemangle<reference_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				bool invalidate = false;

				for (auto& [t, m, i] : stack.stack) {
					if (m == StackModifierType::rval_ref) {
						i = true;
					}
					if (m == StackModifierType::ref) {
						invalidate = true;
					}
				}

				if (not invalidate) {
					stack.append(Decoration::reference, formatter, formatter.escape("&"));
				}

				auto result = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not result) {
					return result;
				}

				if (not invalidate) {
					auto [text, mod, invalid] = stack.pop();
					if (not invalid) {
						result->head += text;
					}
				}

				return result;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<rvalue_reference_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				bool invalidate = false;
				for (const auto& [t, m, i] : stack.stack) {
					if (m == StackModifierType::ref or m == StackModifierType::rval_ref) {
						invalidate = true;
					}
				}
				if (not invalidate) {
					stack.append(Decoration::rvalue_reference, formatter, formatter.escape("&&"));
				}

				auto result = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not result) {
					return result;
				}
				if (not invalidate) {
					auto [text, mod, invalid] = stack.pop();
					if (not result->empty() and not invalid) {
						result->head += text;
					}
				}
				return result;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<array_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const ParseResult* xtype = nullptr;
		const ParseResult* ysize = nullptr;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (not xtype) {
					xtype = p;
				} else {
					ysize = xtype;
					xtype = p;
					break;
				}
			}
		}

		if (xtype) {
			auto entry_depth = stack.stack.size();
			auto ttmp = format_demangled_type(*xtype, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
			if (not ttmp) {
				return ttmp;
			}
			auto& tmp = *ttmp;

			if (not stack.empty() and stack.stack.back().mod == StackModifierType::const_ and not stack.stack.back().invalidated) {
				tmp.head += ' ';
				tmp.head += stack.stack.back().text;
				stack.stack.back().invalidated = true;
			}

			auto partial = stack.partial_collapse(entry_depth);

			CompositeStack<StringType> result;
			result.head = tmp.head;
			if (not partial.empty()) {
				result.head += " (" + partial;
				result.tail = ") ";
			} else {
				result.head += ' ';
			}

			if (ysize) {
				result.tail += formatter.decorate(Decoration::array_brackets, "[");
				auto tmp2 = format_demangled(*ysize, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp2) {
					return tmp2;
				}
				result.tail += *tmp2;
				result.tail += formatter.decorate(Decoration::array_brackets, "]");
			} else {
				result.tail += formatter.decorate(Decoration::array_brackets, "[]");
			}

			result.tail += tmp.tail;
			return result;
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<vector_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const ParseResult* xtype = nullptr;
		const ParseResult* ysize = nullptr;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (not xtype) {
					xtype = p;
				} else {
					ysize = xtype;
					xtype = p;
					break;
				}
			}
		}
		if (not xtype or not ysize) {
			return StringType(make_error());
		}

		auto result = compose(format_demangled_type(*xtype, subs, tp, dv, formatter, stack, ps, skip, max_recursion));
		if (not result) {
			return result;
		}

		*result += ' ';

		if (dv.compiler_dialect == CompilerDialect::gcc) {
			*result += formatter.decorate(Decoration::array_brackets, "__vector(");
		} else {
			*result += formatter.decorate(Decoration::array_brackets, "vector[");
		}

		auto tmp = format_demangled(*ysize, subs, tp, dv, formatter, ps, skip, max_recursion);
		if (not tmp) {
			return tmp;
		}
		*result += *tmp;
		if (dv.compiler_dialect == CompilerDialect::gcc) {
			*result += formatter.decorate(Decoration::array_brackets, ")");
		} else {
			*result += formatter.decorate(Decoration::array_brackets, "]");
		}

		return result;
	}
};

template <>
struct FormatDemangle<function_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		(void)stack;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		if (not dv.resolve_function_signatures) {
			return StringType(formatter.decorate(Decoration::generic_italic, "F"));
		}

		const ParseResult* q = nullptr;

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				q = x;
				break;
			}
		}
		CompositeStack<StringType> ret;
		auto entry_depth = stack.stack.size();
		for (const auto& cq : q->comps) {
			const auto* x = std::get_if<ParseResult>(&cq);
			if (x) {
				auto tmp = format_demangled_type(*x, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not tmp) {
					return std::unexpected(tmp.error());
				}
				ret = *tmp;

				break;
			}
		}

		StringType pparams;
		StringType params;
		pparams += formatter.decorate(Decoration::call_parentheses, "(");

		bool had_return = false;
		bool first = true;
		for (const auto& cq : q->comps) {
			const auto* x = std::get_if<ParseResult>(&cq);
			if (x) {
				if (not had_return) {
					had_return = true;
					continue;
				}
				if (first) {
					first = false;
				} else {
					params += ", ";
				}
				Stacked<StringType> pstack;
				auto tmp = format_demangled_type(*x, subs, tp, dv, formatter, pstack, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				params += compose(*tmp);
			}
		}
		if (params != "void") {
			pparams += params;
		}
		pparams += formatter.decorate(Decoration::call_parentheses, ")");

		auto partial = stack.partial_collapse(entry_depth);

		CompositeStack<StringType> result;
		result.head = ret.head;
		if (not partial.empty()) {
			result.head += " (" + partial;
			result.tail = ")";
		} else {
			result.head += ' ';
		}

		result.tail += pparams;
		result.tail += ret.tail;

		return result;
	}
};

template <>
struct FormatDemangle<const_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				stack.append(Decoration::const_keyword, formatter, "const");
				auto result = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not result) {
					return result;
				}

				auto mod = stack.pop();
				if (not mod.invalidated) {
					result->head += " ";
					result->head += mod.text;
				}

				return result;
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<decltype_> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>&, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				std::string s = dv.compiler_dialect == CompilerDialect::gcc ? " "s : ""s;
				auto tmp = format_demangled(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return std::unexpected(tmp.error());
				}
				return formatter.decorate(Decoration::keyword, "decltype") + s + "(" + *tmp + ")";
			}
		}
		using StringType = typename Formatter::value_type;
		return expand(StringType(make_error()));
	}
};

template <>
struct FormatDemangle<complex_type_c99> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				auto tmp = format_demangled_type(*x, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}

				auto kw = "_Complex"sv;
				if (dv.complex_keyword == ComplexKeyword::no_underscore) {
					kw = "complex"sv;
				} else if (dv.complex_keyword == ComplexKeyword::two_underscore) {
					kw = "__complex__"sv;
				}
				if (dv.complex_orientation == ComplexOrientation::east_complex) {
					return compose(*tmp) + " " + formatter.decorate(Decoration::type_keyword, kw);
				}

				return formatter.decorate(Decoration::type_keyword, kw) + " " + compose(*tmp);
			}
		}
		using StringType = typename Formatter::value_type;
		return expand(StringType(make_error()));
	}
};

template <>
struct FormatDemangle<builtin_other_type> {
	template <typename Formatter>
	static typename Formatter::value_type form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>&, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)tp;
		if (const auto* p = std::get_if<std::string_view>(res.comps.data()); p) {
			return formatter.decorate(Decoration::type_keyword, demangleBasicType(*p, dv));
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<builtin_real_type> {
	template <typename Formatter>
	static typename Formatter::value_type form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>&, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)tp;
		if (const auto* p = std::get_if<std::string_view>(res.comps.data()); p) {
			return formatter.decorate(Decoration::type_keyword, demangleBasicType(*p, dv));
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<builtin_integral_type> {
	template <typename Formatter>
	static typename Formatter::value_type form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)tp;
		(void)stack;
		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (not p) {
			return make_error();
		}

		return formatter.decorate(Decoration::type_keyword, demangleBasicType(*p, dv));
	}
};

template <>
struct FormatDemangle<builtin_boolean_type> {
	template <typename Formatter>
	static typename Formatter::value_type form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>&, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)tp;

		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (not p) {
			return make_error();
		}

		return formatter.decorate(Decoration::type_keyword, demangleBasicType(*p, dv));
	}
};

template <typename Formatter>
std::optional<typename Formatter::value_type> form_simple_substitution(const std::string_view* p, const DemanglingDialect& dv, Formatter& formatter, const SkipSpec& skip, RecursionLimit max_recursion) {
	
	(void)max_recursion;
	if (not p) {
		return make_error();
	}

	if (*p == "St") {
		return "std"s;
	}

	if (*p == "Sa") {
		for (const auto& a : skip.skip) {
			if (not a.demangled_parts.empty() and a.demangled_parts[0] == "std") {
				return "allocator"s;
			}
		}

		return "std" + formatter.decorate(Decoration::noise, "::") + "allocator"s;
	}
	if (*p == "Sb") {
		for (const auto& a : skip.skip) {
			if (not a.demangled_parts.empty() and a.demangled_parts[0] == "std") {
				return "basic_string"s;
			}
		}
		return "std" + formatter.decorate(Decoration::noise, "::") + "basic_string"s;
	}
	std::string sw;
	if (dv.separating_whitespace) {
		sw = " ";
	}

	for (const auto& a : skip.skip) {
		if (not a.demangled_parts.empty() and a.demangled_parts[0] == "std") {
			if (*p == "Ss") {
				if (not dv.resolve_std_typedefs) {
					return "string"s;
				}
				return "basic_string<char, char_traits<char>, allocator<char>" + sw + ">"s;
			}
			if (*p == "Si") {
				if (not dv.resolve_std_typedefs) {
					return "istream"s;
				}
				return "basic_istream<char, char_traits<char>" + sw + ">"s;
			}
			if (*p == "So") {
				if (not dv.resolve_std_typedefs) {
					return "ostream"s;
				}
				return "basic_ostream<char, char_traits<char>" + sw + ">"s;
			}
			if (*p == "Sd") {
				return "basic_iostream<char, char_traits<char>" + sw + ">"s;
			}
		}
	}

	if (*p == "Ss") {
		auto sr = formatter.decorate(Decoration::noise, "::");
		if (not dv.resolve_std_typedefs) {
			return "std" + sr + "string"s;
		}
		return "std" + sr + "basic_string<char, std" + sr + "char_traits<char>, std" + sr + "allocator<char>" + sw + ">"s;
	}
	if (*p == "Si") {
		auto sr = formatter.decorate(Decoration::noise, "::");
		if (not dv.resolve_std_typedefs) {
			return "std" + sr + "istream"s;
		}
		return "std" + sr + "basic_istream<char, std" + sr + "char_traits<char>" + sw + ">"s;
	}
	if (*p == "So") {
		auto sr = formatter.decorate(Decoration::noise, "::");
		if (not dv.resolve_std_typedefs) {
			return "std" + sr + "ostream"s;
		}
		return "std" + sr + "basic_ostream<char, std" + sr + "char_traits<char>" + sw + ">"s;
	}
	if (*p == "Sd") {
		auto sr = formatter.decorate(Decoration::noise, "::");
		if (not dv.resolve_std_typedefs) {
			return "std" + sr + "iostream"s;
		}
		return "std" + sr + "basic_iostream<char, std" + sr + "char_traits<char>" + sw + ">"s;
	}

	return std::nullopt;
}

template <>
struct FormatDemangle<vendor_ext_arg_type_llm> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		(void)dv;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		if (res.comps.size() < 2) {
			return StringType(make_error());
		}

		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;
		const ParseResult* r = nullptr;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (not p) {
					p = x;
				} else if (not q) {
					q = x;

				} else {
					r = x;
					break;
				}
			}
		}

		if (not p or not q or not r) {
			return StringType(make_error());
		}

		auto result = format_demangled_type(*r, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
		if (not result) {
			return result;
		}
		auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
		if (not tmp) {
			return std::unexpected(tmp.error());
		}
		result->tail += formatter.decorate(Decoration::noise, "[" + *tmp + "]");

		return result;
	}
};

template <>
struct FormatDemangle<substitution> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		(void)tp;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const auto* p = std::get_if<std::string_view>(res.comps.data());

		using StringType = typename Formatter::value_type;

		auto simple = form_simple_substitution(p, dv, formatter, skip, max_recursion);
		if (simple) {
			return StringType(*simple);
		}

		if ((*p == "S_" or (*p == "S" and res.comps.size() == 2 and std::holds_alternative<std::string_view>(res.comps[1]))) and not subs.entries.empty()) {
			if (not dv.resolve_substitutions) {
				return StringType(formatter.decorate(Decoration::generic_italic, "S"));
			}
			if (subs.entries[0].type == partial_nested_name) {
				return format_demangled<partial_nested_name>(subs.entries[0], {}, subs.stp[0], dv, formatter, ps, skip, max_recursion);
			}
			return format_demangled_type(subs.entries[0], {}, subs.stp[0], dv, formatter, stack, ps, skip, max_recursion);
		}

		if (*p == "S" and res.comps.size() >= 2) {
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (q and q->type == seq_id) {
				const auto* qq = std::get_if<std::string_view>(q->comps.data());
				if (qq) {
					auto num = parseNumberBase36(*qq);
					if (num) {
						auto [val, len] = *num;
						auto idx = val + 1;
						if (idx < subs.entries.size()) {
							if (not dv.resolve_substitutions) {
								return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("S{}", *qq)));
							}
							if (subs.entries[idx].type == partial_nested_name) {
								return format_demangled<partial_nested_name>(subs.entries[idx], subs.subspan(0, idx), subs.stp[idx], dv, formatter, ps, skip, max_recursion);
							}
							return format_demangled_type(subs.entries[idx], subs.subspan(0, idx), subs.stp[idx], dv, formatter, stack, ps, skip, max_recursion);
						}
					}
				}
			}
		}

		return StringType("S?"s);
	}

	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		(void)tp;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const auto* p = std::get_if<std::string_view>(res.comps.data());

		using StringType = typename Formatter::value_type;

		auto simple = form_simple_substitution(p, dv, formatter, skip, max_recursion);
		if (simple) {
			return StringType(*simple);
		}

		if ((*p == "S_" or (*p == "S" and res.comps.size() == 2 and std::holds_alternative<std::string_view>(res.comps[1]))) and not subs.entries.empty()) {
			if (not dv.resolve_substitutions) {
				return StringType(formatter.decorate(Decoration::generic_italic, "S"));
			}
			return format_demangled(subs.entries[0], {}, subs.stp[0], dv, formatter, ps, skip, max_recursion);
		}
		if (*p == "S" and res.comps.size() >= 2) {
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (q and q->type == seq_id) {
				const auto* qq = std::get_if<std::string_view>(q->comps.data());
				if (qq) {
					auto num = parseNumberBase36(*qq);
					if (num) {
						auto [val, len] = *num;
						auto idx = val + 1;
						if (idx < subs.entries.size()) {
							if (not dv.resolve_substitutions) {
								return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("S{}", *qq)));
							}
							return format_demangled(subs.entries[idx], subs.subspan(0, idx), subs.stp[idx], dv, formatter, ps, skip, max_recursion);
						}
					}
				}
			}
		}

		return StringType("S?"s);
	}
};

const ParseResult* resolve_expression(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp) {
	if (res.comps.size() == 1 and (res.type == expression or res.type == template_arg)) {
		const auto* x = std::get_if<ParseResult>(res.comps.data());
		return resolve_expression(*x, subs, tp);
	}

	if (res.type == template_param) {
		const auto* q = resolve_template_param(res, tp);
		return resolve_expression(*q, subs, tp);
	}

	if (res.type == expr_primary) {
		return &res;
	}

	return nullptr;
}

template <>
struct FormatDemangle<pack_indexing_type> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;

		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (not p) {
					p = x;
				} else {
					q = x;
				}
			}
		}
		if (p and q) {
			if (p->type == template_param and dv.resolve_template_args) {
				const auto* pp = resolve_template_param(*p, tp);
				if (pp) {
					p = pp;
					if (p->type == template_arg and p->comps.size() == 1) {
						const auto* pq = std::get_if<ParseResult>(p->comps.data());
						if (pq) {
							p = pq;
						}
					}
				}
			}

			if (p->type == template_arg_pack) {
				const auto* pack_index = resolve_expression(*q, subs, tp);
				if (pack_index and dv.resolve_packs) {
					const ParseResult* p1 = nullptr;
					const ParseResult* p2 = nullptr;
					for (const auto& c : pack_index->comps) {
						if (const auto* x = std::get_if<ParseResult>(&c); x) {
							if (not p1) {
								p1 = x;
							} else {
								p2 = x;
							}
						}
						if (p2 and p2->type == number) {
							if (const auto* x = std::get_if<ParseResult>(p2->comps.data()); x) {
								p2 = x;
							}
						}
						if (p2 and p2->type == nonneg_number) {
							const auto* p2l = std::get_if<std::string_view>(p2->comps.data());
							auto np2l = parse_uint(*p2l);
							if (np2l) {
								auto [pi, pi_len] = *np2l;
								if (p->comps.size() > 1 + pi) {
									const auto* indexed_comp = std::get_if<ParseResult>(&p->comps[pi + 1]);
									if (indexed_comp) {
										return format_demangled_type(*indexed_comp, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
									}
								}
							}
						}
					}
				}

				auto tmp0 = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				if (not tmp0) {
					return tmp0;
				}
				auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return std::unexpected(tmp1.error());
				}
				return formatter.decorate(Decoration::generic_italic, "P") + "<" + compose(*tmp0) + ">" + "...[" + *tmp1 + "]";
			}
			auto tmp0 = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return std::unexpected(tmp1.error());
			}
			return compose(*tmp0) + "...[" + *tmp1 + "]";
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<pack_indexing_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;

		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (not p) {
					p = x;
				} else {
					q = x;
				}
			}
		}
		if (p and q) {
			if (p->type == template_param and dv.resolve_template_args) {
				const auto* pp = resolve_template_param(*p, tp);
				if (pp) {
					p = pp;
					if (p->type == template_arg and p->comps.size() == 1) {
						const auto* pq = std::get_if<ParseResult>(p->comps.data());
						if (pq) {
							p = pq;
						}
					}
				}
			}

			if (p->type == template_arg_pack) {
				const auto* pack_index = resolve_expression(*q, subs, tp);
				if (pack_index and dv.resolve_packs) {
					const ParseResult* p1 = nullptr;
					const ParseResult* p2 = nullptr;
					for (const auto& c : pack_index->comps) {
						if (const auto* x = std::get_if<ParseResult>(&c); x) {
							if (not p1) {
								p1 = x;
							} else {
								p2 = x;
							}
						}
						if (p2 and p2->type == number) {
							if (const auto* x = std::get_if<ParseResult>(p2->comps.data()); x) {
								p2 = x;
							}
						}
						if (p2 and p2->type == nonneg_number) {
							const auto* p2l = std::get_if<std::string_view>(p2->comps.data());
							auto np2l = parse_uint(*p2l);
							if (np2l) {
								auto [pi, pi_len] = *np2l;
								if (p->comps.size() > 1 + pi) {
									const auto* indexed_comp = std::get_if<ParseResult>(&p->comps[pi + 1]);
									if (indexed_comp) {
										return format_demangled(*indexed_comp, subs, tp, dv, formatter, ps, skip, max_recursion);
									}
								}
							}
						}
					}
				}

				auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp0) {
					return tmp0;
				}
				auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return tmp1;
				}
				return formatter.decorate(Decoration::generic_italic, "P") + "<" + *tmp0 + ">" + "...[" + *tmp1 + "]";
			}
			auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			return *tmp0 + "...[" + *tmp1 + "]";
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<template_param_decl_llvm> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		const auto* q = std::get_if<std::string_view>(res.comps.data());
		if (*q == "Tn") {
			return {};
		}

		if (*q == "Ty") {
			return StringType(formatter.decorate(Decoration::generic_italic, "typename $T"));
		}
		if (*q == "Tk") {
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					return format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				}
			}
		}

		if (*q == "Tp") {
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					return format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				}
			}
		}

		if (*q == "TL") {
			const std::string_view* l1 = nullptr;
			const std::string_view* l2 = nullptr;
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					if (l1) {
						l2 = std::get_if<std::string_view>(p->comps.data());
					} else {
						l1 = std::get_if<std::string_view>(p->comps.data());
					}
				}
			}

			if (l1 and not l2) {
				auto p1 = parse_uint(*l1);
				if (p1) {
					auto [num1, len1] = *p1;
					if (not dv.resolve_template_args) {
						return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("TL{}_", *l1)));
					}
					auto num2 = 0u;
					if (tp.size() >= num1 + 1) {
						const auto& tl = tp.from_back(num1);
						if (tl.entries.size() > num2) {
							return format_demangled_type(tl.entries[num2], subs, tp, dv, formatter, stack, ps, skip, max_recursion);
						}
					}
				}
			}
			if (l1 and l2) {
				auto p1 = parse_uint(*l1);
				auto p2 = parse_uint(*l2);
				if (p1 and p2) {
					auto [num1, len1] = *p1;
					auto [num2, len2] = *p2;
					if (not dv.resolve_template_args) {
						return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("TL{}_{}", *l1, *l2)));
					}

					if (tp.size() >= num1 + 1) {
						const auto& tl = tp.from_back(num1);
						if (tl.entries.size() > num2) {
							return format_demangled_type(tl.entries[num2], subs, tp, dv, formatter, stack, ps, skip, max_recursion);
						}
					}
				}
			}
			return StringType("TL?"s);
		}

		return StringType(make_error());

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				return format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<template_param> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;

		const auto* pp = std::get_if<ParseResult>(res.comps.data());
		if (pp) {
			if (res.comps.size() == 1) {
				return format_demangled_type(*pp, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
			}
		}
		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (not p) {
			return StringType(make_error());
		}

		if (not tp.empty()) {
			const auto& tpb = tp.back();
			if (not tpb.empty()) {
				if (*p == "T_" or (*p == "T" and res.comps.size() == 2 and std::holds_alternative<std::string_view>(res.comps[1]))) {
					if (not dv.resolve_template_args or (not dv.resolve_packs and std::get_if<ParseResult>(tpb.entries[0].comps.data())->type == template_arg_pack)) {
						return StringType(formatter.decorate(Decoration::generic_italic, "T"));
					}
					return format_demangled_type(tpb.entries[0], subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				}
				if (*p == "T" and res.comps.size() >= 2) {
					const auto* q = std::get_if<ParseResult>(&res.comps[1]);
					if (q and q->type == nonneg_number) {
						const auto* qq = std::get_if<std::string_view>(q->comps.data());
						if (qq) {
							auto num = parse_uint(*qq);
							if (num) {
								auto [val, len] = *num;
								if (val + 1 < tpb.entries.size()) {
									if (not dv.resolve_template_args or (not dv.resolve_packs and std::get_if<ParseResult>(tpb.entries[val + 1].comps.data())->type == template_arg_pack)) {
										return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("T{}", val)));
									}
									return format_demangled_type(tpb.entries[val + 1], subs, tp, dv, formatter, stack, ps, skip, max_recursion);
								}
							}
						}
					}
				}
			}
		}

		// missing template params can happen for lambdas with auto parameters
		if (dv.compiler_dialect == CompilerDialect::clang) {
			return StringType(formatter.decorate(Decoration::type_keyword, "auto"));
		}
		if (*p == "T_") {
			return formatter.decorate(Decoration::type_keyword, "auto") + ":1"s;
		}
		if (*p == "T" and res.comps.size() >= 2) {
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (q and q->type == nonneg_number) {
				const auto* qq = std::get_if<std::string_view>(q->comps.data());
				if (qq) {
					auto num = parse_uint(*qq);
					if (num) {
						auto [val, len] = *num;
						return formatter.decorate(Decoration::type_keyword, "auto") + fmt::format(":{}", val + 1);
					}
				}
			}
		}
		return std::unexpected(ParseError::no_matching_element);
		return StringType("T?"s);
	}

	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (not p) {
			return StringType(make_error());
		}

		if (not tp.empty()) {
			const auto& tpb = tp.back();
			if (not tpb.empty()) {
				if (*p == "T_" or (*p == "T" and res.comps.size() == 2 and std::holds_alternative<std::string_view>(res.comps[1]))) {
					if (not dv.resolve_template_args or (not dv.resolve_packs and std::get_if<ParseResult>(tpb.entries[0].comps.data())->type == template_arg_pack)) {
						return StringType(formatter.decorate(Decoration::generic_italic, "T"));
					}
					return format_demangled(tpb.entries[0], subs, tp, dv, formatter, ps, skip, max_recursion);
				}
				if (*p == "T" and res.comps.size() >= 2) {
					const auto* q = std::get_if<ParseResult>(&res.comps[1]);
					if (q and q->type == nonneg_number) {
						const auto* qq = std::get_if<std::string_view>(q->comps.data());
						if (qq) {
							auto num = parse_uint(*qq);
							if (num) {
								auto [val, len] = *num;
								if (val + 1 < tpb.entries.size()) {
									if (not dv.resolve_template_args or (not dv.resolve_packs and std::get_if<ParseResult>(tpb.entries[val + 1].comps.data())->type == template_arg_pack)) {
										return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("T{}", val)));
									}
									return format_demangled(tpb.entries[val + 1], subs, tp, dv, formatter, ps, skip, max_recursion);
								}
							}
						}
					}
				}
			}
		}

		// missing template params can happen for lambdas with auto parameters
		if (dv.compiler_dialect == CompilerDialect::clang) {
			return StringType(formatter.decorate(Decoration::type_keyword, "auto"));
		}
		if (*p == "T_") {
			return formatter.decorate(Decoration::type_keyword, "auto") + ":1"s;
		}
		if (*p == "T" and res.comps.size() >= 2) {
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (q and q->type == nonneg_number) {
				const auto* qq = std::get_if<std::string_view>(q->comps.data());
				if (qq) {
					auto num = parse_uint(*qq);
					if (num) {
						auto [val, len] = *num;
						return formatter.decorate(Decoration::type_keyword, "auto") + fmt::format(":{}", val + 1);
					}
				}
			}
		}
		return std::unexpected(ParseError::no_matching_element);
		return StringType("T?"s);
	}
};

template <>
struct FormatDemangle<identifier> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)dv;
		(void)subs;
		(void)tp;

		using StringType = typename Formatter::value_type;
		if (const auto* p = std::get_if<std::string_view>(res.comps.data()); p) {
			return StringType(formatter.escape(*p));
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<keyword_cast_operator> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)dv;
		(void)subs;
		(void)tp;

		using StringType = typename Formatter::value_type;
		if (const auto* p = std::get_if<std::string_view>(res.comps.data()); p) {
			const auto& l = *p;
			if (l == "dc") {
				return StringType(formatter.decorate(Decoration::keyword, "dynamic_cast"));
			}
			if (l == "sc") {
				return StringType(formatter.decorate(Decoration::keyword, "static_cast"));
			}
			if (l == "cc") {
				return StringType(formatter.decorate(Decoration::keyword, "const_cast"));
			}
			if (l == "rc") {
				return StringType(formatter.decorate(Decoration::keyword, "reinterpret_cast"));
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<type_expression_operator> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)dv;
		(void)subs;
		(void)tp;

		using StringType = typename Formatter::value_type;
		if (const auto* p = std::get_if<std::string_view>(res.comps.data()); p) {
			const auto& l = *p;
			if (l == "ti") {
				return StringType(formatter.decorate(Decoration::keyword, "typeid"));
			}
			if (l == "st") {
				return StringType(formatter.decorate(Decoration::keyword, "sizeof"));
			}
			if (l == "at") {
				return StringType(formatter.decorate(Decoration::keyword, "alignof"));
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<function_param> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)tp;

		const ParseResult* p = nullptr;
		const std::string_view* q = std::get_if<std::string_view>(res.comps.data());
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				p = x;
			}
		}
		if (dv.compiler_dialect == CompilerDialect::gcc) {
			if (p and p->type == nonneg_number) {
				const auto* pv = std::get_if<std::string_view>(p->comps.data());
				auto num = parse_uint(*pv);
				if (num) {
					auto [val, len] = *num;
					return formatter.decorate(Decoration::generic_italic, "{" + fmt::format("parm#{}", val + 2) + "}");
				}

			} else {
				if (*q == "fpT") {
					return formatter.decorate(Decoration::keyword, "this");
				}
				return formatter.decorate(Decoration::generic_italic, "{parm#1}");
			}
		}

		if (dv.compiler_dialect == CompilerDialect::clang) {
			if (p and p->type == nonneg_number) {
				const auto* pv = std::get_if<std::string_view>(p->comps.data());

				return formatter.decorate(Decoration::generic_italic, fmt::format("fp{}", *pv));

			} else {
				if (*q == "fpT") {
					return formatter.decorate(Decoration::keyword, "this");
				}
				return formatter.decorate(Decoration::generic_italic, "fp");
			}
		}

		using StringType = typename Formatter::value_type;
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<unresolved_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		/*
  <unresolved-name> ::= [gs] <base-unresolved-name>                     # x or (with "gs") ::x
                    ::= sr <unresolved-type> <base-unresolved-name>     # T::x / decltype(p)::x
                    ::= srN <unresolved-type> <unresolved-qualifier-level>+ E <base-unresolved-name>
                                                                        # T::N::x /decltype(p)::N::x
                    ::= [gs] sr <unresolved-qualifier-level>+ E <base-unresolved-name>  
*/

		std::vector<std::string_view> vl;
		std::vector<const ParseResult*> v;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				v.emplace_back(p);
			}
			if (const auto* q = std::get_if<std::string_view>(&c); q) {
				vl.emplace_back(*q);
			}
		}

		if (vl.size() == 0 and v.size() == 1) {
			return format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
		}

		if (vl.size() == 2 and v.size() >= 3 and vl[0] == "srN") {
			StringType result;
			for (const auto* pc : v) {
				const auto& c = *pc;
				if (not result.empty()) {
					result = result += formatter.decorate(Decoration::noise, "::");
				}
				auto tmp = format_demangled(c, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				result += *tmp;
			}
			return result;
		}

		if (vl.size() == 1 and v.size() >= 2 and vl[0] == "sr") {
			StringType result;
			for (const auto* pc : v) {
				const auto& c = *pc;
				if (not result.empty()) {
					result = result += formatter.decorate(Decoration::noise, "::");
				}
				auto tmp = format_demangled(c, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				result += *tmp;
			}
			return result;
		}
		if (vl.size() == 2 and v.size() >= 2 and vl[0] == "sr") {
			StringType result;
			for (const auto* pc : v) {
				const auto& c = *pc;
				if (not result.empty()) {
					result = result += formatter.decorate(Decoration::noise, "::");
				}
				auto tmp = format_demangled(c, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				result += *tmp;
			}
			return result;
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<expression_operator> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)dv;
		(void)subs;
		(void)tp;

		using StringType = typename Formatter::value_type;
		if (const auto* p = std::get_if<std::string_view>(res.comps.data()); p) {
			const auto& l = *p;
			if (l == "te") {
				return StringType(formatter.decorate(Decoration::keyword, "typeid"));
			}
			if (l == "sz") {
				return StringType(formatter.decorate(Decoration::keyword, "sizeof"));
			}
			if (l == "az") {
				return StringType(formatter.decorate(Decoration::keyword, "alignof"));
			}
			if (l == "nx") {
				return StringType(formatter.decorate(Decoration::keyword, "noexcept"));
			}
			if (l == "tw") {
				return StringType(formatter.decorate(Decoration::keyword, "throw"));
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<keyword_type_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() != 2) {
			return StringType(make_error());
		}

		const auto* p = std::get_if<ParseResult>(res.comps.data());
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);
		if (p and q) {
			auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			return *tmp0 + "(" + *tmp1 + ")";
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<keyword_cast_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() != 3) {
			return StringType(make_error());
		}

		const auto* r = std::get_if<ParseResult>(res.comps.data());
		const auto* p = std::get_if<ParseResult>(&res.comps[1]);
		const auto* q = std::get_if<ParseResult>(&res.comps[2]);
		if (p and q) {
			auto tmp0 = format_demangled(*r, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			auto tmp2 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp2) {
				return tmp2;
			}
			return *tmp0 + formatter.escape("<") + *tmp1 + formatter.escape(">") + "(" + *tmp2 + ")";
		}
		return StringType(make_error());
	}
};
template <>
struct FormatDemangle<keyword_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() != 2) {
			return StringType(make_error());
		}

		const auto* p = std::get_if<ParseResult>(res.comps.data());
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);
		if (p and q) {
			const auto* kw = std::get_if<std::string_view>(p->comps.data());
			if (kw and *kw == "tw") {
				auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp0) {
					return tmp0;
				}
				auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return tmp1;
				}
				auto tmp = *tmp0 + " " + *tmp1;
				if (dv.compiler_dialect == CompilerDialect::clang) {
					return tmp;
				}
				return "(" + tmp + ")";
			}
			auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			return *tmp0 + "(" + *tmp1 + ")";
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<rethrow_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)res;
		(void)subs;
		(void)tp;

		using StringType = typename Formatter::value_type;
		if (dv.compiler_dialect == CompilerDialect::clang) {
			return StringType(formatter.decorate(Decoration::keyword, "throw"));
		}

		return "(" + StringType(formatter.decorate(Decoration::keyword, "throw")) + ")";
	}
};

template <>
struct FormatDemangle<number> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
			if (p->type == nonneg_number) {
				return format_demangled<nonneg_number>(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			}
			if (p->type == negative_number) {
				return format_demangled<negative_number>(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<source_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (p->type == identifier) {
					return format_demangled<identifier>(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				}
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<unscoped_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() == 2) {
			const auto* p = std::get_if<std::string_view>(res.comps.data());
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (p and q and *p == "St") {
				for (const auto& a : skip.skip) {
					if (not a.demangled_parts.empty() and a.demangled_parts[0] == "std") {
						return format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
					}
				}

				auto tmp = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "std" + formatter.decorate(Decoration::noise, "::") + *tmp;
			}
		}
		if (res.comps.size() == 1) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			if (p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return formatter.link_to(res, *tmp, subs, tp);
			}
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<nested_name> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		CompositeStack<StringType> result;
		std::optional<ParseResult> cvq;
		std::optional<ParseResult> refq;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		std::vector<const ParseResult*> v;

		std::vector<std::string> skipped;

		for (size_t i = 0; i < res.comps.size(); ++i) {
			const auto& c = res.comps[i];
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				v.emplace_back(p);
			}
		}

		for (size_t i = 0; i < v.size(); ++i) {
			const auto* p = v[i];
			if (p->type == explicit_object_member) {
				continue;
			}
			if (p->type == cv_qualifiers) {
				cvq = *p;
				continue;
			}
			if (p->type == ref_qualifier) {
				refq = *p;
				continue;
			}

			if (p->comps.size() == 1 and p->type == template_prefix) {
				if (const auto* pp = std::get_if<ParseResult>(p->comps.data()); pp) {
					p = pp;
				}
			}
			bool was_ctor_dtor = false;
			if (p->type == unqualified_name and (p->comps.size() == 1 or (p->comps.size() == 2 and std::get_if<ParseResult>(&p->comps[1])->type == abi_tags))) {
				if (const auto* pp = std::get_if<ParseResult>(p->comps.data()); pp and pp->type == ctor_dtor_name) {
					const auto* o = v[i - 1];

					if (const auto* oo = std::get_if<ParseResult>(o->comps.data()); oo and oo->type == template_prefix) {
						if (const auto* ooo = std::get_if<ParseResult>(oo->comps.data()); ooo) {
							o = ooo;
						}
					}
					if (const auto* l = std::get_if<std::string_view>(pp->comps.data()); l) {
						if ((*l)[0] == 'C') {
							if (not result.empty() and p->type != template_args) {
								result += formatter.decorate(Decoration::noise, "::");
							}

							if (l->starts_with("CI") and dv.compiler_dialect == CompilerDialect::gcc) {
								const auto* lt = std::get_if<ParseResult>(&pp->comps[1]);
								auto tmp = format_demangled_short(*lt, subs, tp, dv, formatter, ps, skip, max_recursion);
								if (not tmp) {
									return std::unexpected(tmp.error());
								}
								result += formatter.decorate(Decoration::generic_bold, *tmp);
							} else {
								auto tmp1 = format_demangled_short(*o, subs, tp, dv, formatter, ps, skip, max_recursion);
								if (not tmp1) {
									return std::unexpected(tmp1.error());
								}
								result += formatter.decorate(Decoration::generic_bold, *tmp1);
							}

							was_ctor_dtor = true;
						} else if ((*l)[0] == 'D') {
							if (not result.empty() and p->type != template_args) {
								result += formatter.decorate(Decoration::noise, "::");
							}
							auto tmp1 = format_demangled_short(*o, subs, tp, dv, formatter, ps, skip, max_recursion);
							if (not tmp1) {
								return std::unexpected(tmp1.error());
							}
							result += formatter.decorate(Decoration::generic_bold, "~" + *tmp1);
							was_ctor_dtor = true;
						}
					}
					if (p->comps.size() == 2) {
						const auto* tags = std::get_if<ParseResult>(&p->comps[1]);
						auto tmp1 = format_demangled(*tags, subs, tp, dv, formatter, ps, skip, max_recursion);
						if (not tmp1) {
							return std::unexpected(tmp1.error());
						}
						result += *tmp1;
					}
				}
			}
			if (not was_ctor_dtor) {
				auto ctmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;

				if (tmp.empty()) {
					continue;
				}

				bool skip_this = false;
				for (const auto& alt : skip.skip) {
					bool this_alt = true;
					if (alt.demangled_parts.size() > skipped.size()) {
						for (size_t si = 0; si < skipped.size(); si++) {
							if (alt.demangled_parts[si] != skipped[si]) {
								this_alt = false;
								break;
							}
						}
						if (this_alt and alt.demangled_parts[skipped.size()] == tmp) {
							skipped.emplace_back(alt.demangled_parts[skipped.size()]);
							skip_this = true;
							break;
						}
					}
				}
				if (skip_this) {
					continue;
				}

				if (dv.separating_whitespace and result.head.ends_with('<') and tmp.starts_with('<')) {
					result += ' ';
				}
				if (tmp == "_GLOBAL__N_1") {
					if (not result.empty() and p->type != template_args) {
						result += formatter.decorate(Decoration::noise, "::");
					}
					result += formatter.decorate(Decoration::generic_italic, "(anonymous namespace)"s);
				} else {
					if ((p->type != prefix and p->type != template_args and i + 1 == v.size())
					    or (p->type != prefix and p->type != template_args and i + 2 == v.size() and v[i + 1]->type == template_args)) {
						if (not result.empty() and p->type != template_args) {
							result += formatter.decorate(Decoration::noise, "::");
						}
						if ((i + 1 == v.size() and p->type != template_args)
						    or ((i + 2 == v.size()) and v.back()->type == template_args)) {
							tmp = formatter.link_to(res, tmp, subs, tp);
						}

						result += formatter.decorate(Decoration::generic_bold, tmp);
					} else {
						if (not result.empty() and p->type != template_args) {
							result += formatter.decorate(Decoration::noise, "::");
						}
						if ((i + 1 == v.size() and p->type != template_args)
						    or ((i + 2 == v.size()) and v.back()->type == template_args)) {
							tmp = formatter.link_to(res, tmp, subs, tp);
						}
						result += tmp;
					}
				}
			}
		}
		if (cvq) {
			const auto* c = std::get_if<std::string_view>(cvq->comps.data());
			if (c and *c == "K") {
				
				result.tail += formatter.decorate(Decoration::const_keyword, "const");
			}
			if (c and *c == "V") {
				
				result.tail += formatter.decorate(Decoration::const_keyword, "volatile");
			}
			if (c and *c == "KV") {
				
				result.tail += formatter.decorate(Decoration::const_keyword, "const");
				result.tail += ' ';
				result.tail += formatter.decorate(Decoration::const_keyword, "volatile");
			}
		}
		if (refq) {
			const auto* c = std::get_if<std::string_view>(refq->comps.data());
			if (c and *c == "R") {
				if (not result.tail.empty()) {
					result.tail += ' ';
				}
				result.tail += formatter.decorate(Decoration::reference, formatter.escape("&"));
			}
			if (c and *c == "O") {
				if (not result.tail.empty()) {
					result.tail += ' ';
				}
				result.tail += formatter.decorate(Decoration::rvalue_reference, formatter.escape("&&"));
			}
		}
		return result;
	}
};

template <>
struct FormatDemangle<partial_nested_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		typename Formatter::value_type result;
		std::vector<std::string> skipped;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				if (tmp->empty()) {
					continue;
				}

				bool skip_this = false;
				for (const auto& alt : skip.skip) {
					bool this_alt = true;
					if (alt.demangled_parts.size() > skipped.size()) {
						for (size_t si = 0; si < skipped.size(); si++) {
							if (alt.demangled_parts[si] != skipped[si]) {
								this_alt = false;
								break;
							}
						}
						if (this_alt and alt.demangled_parts[skipped.size()] == *tmp) {
							skipped.emplace_back(alt.demangled_parts[skipped.size()]);
							skip_this = true;
							break;
						}
					}
				}
				if (skip_this) {
					continue;
				}

				if (not result.empty() and p->type != template_args) {
					result += formatter.decorate(Decoration::noise, "::");
				}

				if (*tmp == "_GLOBAL__N_1") {
					result += formatter.decorate(Decoration::generic_italic, "(anonymous namespace)");
				} else {
					result += *tmp;
				}
			}
		}

		return result;
	}
};

template <typename Formatter>
std::expected<typename Formatter::value_type, ParseError> print_template_args(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, unsigned remaining_template_args, RecursionLimit max_recursion) {
	if (max_recursion == 0) {
		return std::unexpected(ParseError::recursion_reached);
	}
	--max_recursion;
	using StringType = typename Formatter::value_type;

	StringType params;
	size_t pcount = 0;

	for (const auto& c : res.comps) {
		if (const auto* p = std::get_if<ParseResult>(&c); p) {
			if (p->type == requires_clause) {
				params += ' ';
				auto ntt = tp;
				auto nt = collect_tp(res);
				ntt.add(std::move(nt));
				auto ctmp = format_demangled(*p, subs, ntt, dv, formatter, ps, skip, max_recursion);
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;
				params += tmp;
				continue;
			}

			std::expected<typename Formatter::value_type, ParseError> ctmp;
			bool was_pack = false;
			unsigned pack_size = 0;
			if (p->type == template_arg) {
				if (const auto* x = std::get_if<ParseResult>(p->comps.data()); x) {
					if (x->type == template_arg_pack) {
						ctmp = print_template_args(*x, subs, tp, dv, formatter, ps, skip, remaining_template_args - pcount, max_recursion);
						auto ops = get_pack_size(*x, subs, tp, max_recursion);
						if (ops) {
							pack_size = *ops;
						}
						was_pack = true;
					}
				}
			}
			if (not was_pack) {
				ctmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			}
			if (not ctmp) {
				return ctmp;
			}
			auto& tmp = *ctmp;
			if (tmp.empty()) {
				continue;
			}
			if (not params.empty()) {
				params += ", "sv;
			}

			if (pcount >= dv.max_template_parameters) {
				params += ".";
				break;
			}

			if (pcount & 1) {
				params += formatter.decorate(Decoration::template_parameter_odd, tmp);
			} else {
				params += formatter.decorate(Decoration::template_parameter_even, tmp);
			}

			if (was_pack) {
				pcount += pack_size;
			} else {
				pcount++;
			}
		}
	}

	return params;
}

template <>
struct FormatDemangle<template_args> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		unsigned remaining_template_args = dv.max_template_parameters;

		using StringType = typename Formatter::value_type;
		StringType result;
		result += formatter.decorate(Decoration::template_brackets, formatter.escape("<"));
		if (dv.max_template_parameters > 0 and dv.nested_template_parameter_reduction > 0) {
			auto adv = dv;
			if (dv.nested_template_parameter_reduction >= dv.max_template_parameters) {
				adv.max_template_parameters = 0;
			} else {
				adv.max_template_parameters = dv.max_template_parameters - dv.nested_template_parameter_reduction;
			}
			auto tmp = print_template_args(res, subs, tp, adv, formatter, ps, skip, remaining_template_args, max_recursion);
			if (not tmp) {
				return tmp;
			}
			result += *tmp;

		} else {
			auto tmp = print_template_args(res, subs, tp, dv, formatter, ps, skip, remaining_template_args, max_recursion);
			if (not tmp) {
				return tmp;
			}
			result += *tmp;
		}

		if (result.ends_with('>') and dv.separating_whitespace) {
			result += ' ';
		}
		result += formatter.decorate(Decoration::template_brackets, formatter.escape(">"));
		return result;
	}
};

template <>
struct FormatDemangle<template_arg_pack> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		typename Formatter::value_type result;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		int pcount = 0;
		PackSelector pa {PackSelector::all};
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (ps.val == PackSelector::all or pcount == ps.val) {
					auto ctmp = format_demangled(*p, subs, tp, dv, formatter, pa, skip, max_recursion);
					if (not ctmp) {
						return ctmp;
					}
					auto& tmp = *ctmp;
					if (tmp.empty()) {
						continue;
					}
					if (not result.empty()) {
						result += ", ";
					}

					if (pcount & 1) {
						result += formatter.decorate(Decoration::template_parameter_odd, tmp);
					} else {
						result += formatter.decorate(Decoration::template_parameter_even, tmp);
					}
				}
				pcount++;
			}
		}

		return result;
	}

	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		typename Formatter::value_type result;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		int pcount = 0;
		PackSelector pa {PackSelector::all};
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (ps.val == PackSelector::all or pcount == ps.val) {
					auto ctmp = format_demangled_type(*p, subs, tp, dv, formatter, stack, pa, skip, max_recursion);
					if (not ctmp) {
						return ctmp;
					}
					auto tmp = compose(*ctmp);
					if (tmp.empty()) {
						continue;
					}
					if (not result.empty()) {
						result += ", ";
					}

					if (pcount & 1) {
						result += formatter.decorate(Decoration::template_parameter_odd, tmp);
					} else {
						result += formatter.decorate(Decoration::template_parameter_even, tmp);
					}
				}
				pcount++;
			}
		}

		return result;
	}
};

template <>
struct FormatDemangle<abi_tags> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		StringType result;
		if (dv.hide_abi_suffix) {
			return {};
		}
		result += "[abi:";
		StringType tags;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto ctmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;
				if (tmp.empty()) {
					continue;
				}
				if (not tags.empty()) {
					tags += ", ";
				}

				tags += tmp;
			}
		}
		result += tags;
		result += ']';

		return formatter.decorate(Decoration::noise, result);
	}
};

template <>
struct FormatDemangle<vtable> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "vtable " + formatter.decorate(Decoration::noise, "for") + " " + *tmp;
			}
		}
		return make_error();
	}
};
template <>
struct FormatDemangle<global_sub_for_mangled_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "global variable construction " + formatter.decorate(Decoration::noise, "for") + " " + *tmp;
			}
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<construction_vtable> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		const ParseResult* p = nullptr;
		const ParseResult* pn = nullptr;
		const ParseResult* q = nullptr;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (not p) {
					p = x;
				} else if (not pn) {
					pn = x;
				} else {
					q = x;
					break;
				}
			}
		}

		if (p and q) {
			auto tmp0 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			return "construction vtable " + formatter.decorate(Decoration::noise, "for") + " " + *tmp0 + formatter.decorate(Decoration::generic_italic, "-in-") + *tmp1;
		}

		return make_error();
	}
};

template <>
struct FormatDemangle<typeinfo> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "typeinfo " + formatter.decorate(Decoration::noise, "for") + " " + *tmp;
			}
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<vtt> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "VTT " + formatter.decorate(Decoration::noise, "for") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<nonvirtual_thunk> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		bool is_first = true;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (is_first) {
					is_first = false;
					continue;
				}
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "non-virtual thunk " + formatter.decorate(Decoration::noise, "to") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<virtual_thunk> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		bool is_first = true;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (is_first) {
					is_first = false;
					continue;
				}
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "virtual thunk " + formatter.decorate(Decoration::noise, "to") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<virtual_covariant_thunk> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		bool is_first = true;
		bool is_second = true;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				if (is_first) {
					is_first = false;
					continue;
				}
				if (is_second) {
					is_second = false;
					continue;
				}
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "covariant return thunk " + formatter.decorate(Decoration::noise, "to") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<guard_variable> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "guard variable " + formatter.decorate(Decoration::noise, "for") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<typeinfo_name_object> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "typeinfo name " + formatter.decorate(Decoration::noise, "for") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<unary_operator_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)dv;
		(void)tp;

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<std::string_view>(&c); p) {
				const auto& l = *p;
				for (size_t i = 0; i < size_t(ItaniumOperatorType::number_of_options); ++i) {
					auto op = static_cast<ItaniumOperatorType>(i);
					if (l == to_mangled_name(op)) {
						return formatter.decorate(Decoration::keyword, "operator") + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op));
					}
				}
			}
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<binary_operator_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)subs;
		(void)dv;
		(void)tp;

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<std::string_view>(&c); p) {
				const auto& l = *p;
				for (size_t i = 0; i < size_t(ItaniumOperatorType::number_of_options); ++i) {
					auto op = static_cast<ItaniumOperatorType>(i);
					if (l == to_mangled_name(op)) {
						return formatter.decorate(Decoration::keyword, "operator") + formatter.decorate(Decoration::operator_symbol, formatter.escape(to_demangled_operator_name(op)));
					}
				}
			}
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<bare_function_type> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		StringType result;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		result += formatter.decorate(Decoration::call_parentheses, "(");

		StringType params;
		size_t pcount = 0;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				Stacked<StringType> pstack;
				auto ctmp = compose(format_demangled_type(*p, subs, tp, dv, formatter, pstack, ps, skip, max_recursion));
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;
				if (tmp.empty()) {
					continue;
				}
				if (not params.empty()) {
					params += ", "sv;
				}

				if (pcount & 1) {
					params += formatter.decorate(Decoration::call_parameter_odd, tmp);
				} else {
					params += formatter.decorate(Decoration::call_parameter_even, tmp);
				}
				pcount++;
			}
		}

		if (params != "void") {
			result += params;
		}
		result += formatter.decorate(Decoration::call_parentheses, ")");

		return result;
	}
};

template <>
struct FormatDemangle<lambda_sig> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		StringType result;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		result += formatter.decorate(Decoration::call_parentheses, "(");

		StringType params;
		size_t pcount = 0;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto ctmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;
				if (tmp.empty()) {
					continue;
				}
				if (not params.empty()) {
					params += ", "sv;
				}

				if (pcount & 1) {
					params += formatter.decorate(Decoration::call_parameter_odd, tmp);
				} else {
					params += formatter.decorate(Decoration::call_parameter_even, tmp);
				}
				pcount++;
			}
		}

		if (params != "void") {
			result += params;
		}
		result += formatter.decorate(Decoration::call_parentheses, ")");

		return result;
	}
};

template <>
struct FormatDemangle<operator_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (not p) {
			const auto* q = std::get_if<ParseResult>(res.comps.data());
			return format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
		}
		using StringType = typename Formatter::value_type;

		if (*p == "nw") {
			return formatter.decorate(Decoration::keyword, "operator") + " " + formatter.decorate(Decoration::keyword, "new");
		}
		if (*p == "na") {
			return formatter.decorate(Decoration::keyword, "operator") + " " + formatter.decorate(Decoration::keyword, "new") + formatter.decorate(Decoration::array_brackets, "[]");
		}
		if (*p == "dl") {
			return formatter.decorate(Decoration::keyword, "operator") + " " + formatter.decorate(Decoration::keyword, "delete");
		}
		if (*p == "da") {
			return formatter.decorate(Decoration::keyword, "operator") + " " + formatter.decorate(Decoration::keyword, "delete") + formatter.decorate(Decoration::array_brackets, "[]");
		}
		if (*p == "cl") {
			return formatter.decorate(Decoration::keyword, "operator") + formatter.decorate(Decoration::call_parentheses, "()");
		}
		if (*p == "ix") {
			return formatter.decorate(Decoration::keyword, "operator") + formatter.decorate(Decoration::array_brackets, "[]");
		}
		if (*p == "aw") {
			return formatter.decorate(Decoration::keyword, "operator") + " " + formatter.decorate(Decoration::keyword, "co_await");
		}
		if (*p == "li") {
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			auto tmp = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp) {
				return tmp;
			}
			return formatter.decorate(Decoration::keyword, "operator") + formatter.decorate(Decoration::operator_symbol, "\"\"") + " " + *tmp;
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<vendor_specfic_suffix> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		const auto* l = std::get_if<std::string_view>(res.comps.data());
		(void)subs;
		(void)tp;

		if (dv.hide_vendor_suffix) {
			return {};
		}

		using StringType = typename Formatter::value_type;
		if (dv.compiler_dialect == CompilerDialect::gcc) {
			std::vector<std::string_view> parts;
			split(parts, *l, '.');
			auto is_digit = [](char c) { return '0' <= c and c <= '9'; };

			StringType result;
			for (size_t i = 0; i < parts.size(); i++) {
				if (not result.empty()) {
					result += ' ';
				}

				if (i + 1 < parts.size() and is_digit(parts[i + 1][0])) {
					result += fmt::format("[clone .{}.{}]", parts[i], parts[i + 1]);
					i++;
				} else {
					result += fmt::format("[clone .{}]", parts[i]);
				}
			}

			return formatter.decorate(Decoration::noise, result);
		} else {
			return formatter.decorate(Decoration::noise, fmt::format("(.{})", *l));
		}
	}
};

template <>
struct FormatDemangle<unnamed_type_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() >= 2) {
			const auto* x = std::get_if<std::string_view>(res.comps.data());
			const auto* y = std::get_if<ParseResult>(&res.comps[1]);
			if (x and y and *x == "Ut" and y->type == nonneg_number) {
				const auto* z = std::get_if<std::string_view>(y->comps.data());
				if (z) {
					if (dv.compiler_dialect == CompilerDialect::clang) {
						return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("'unnamed{}'", *z)));
					}
					auto num = parse_uint(*z);
					if (num) {
						auto [val, len] = *num;

						return StringType(formatter.decorate(Decoration::generic_italic, fmt::format("{}unnamed type#{}{}", '{', val + 2, '}')));
					}
				}
			}
		}

		if (res.comps.size() == 1) {
			const auto* x = std::get_if<std::string_view>(res.comps.data());
			const auto* y = std::get_if<ParseResult>(res.comps.data());
			if (x and *x == "Ut_") {
				if (dv.compiler_dialect == CompilerDialect::clang) {
					return StringType(formatter.decorate(Decoration::generic_italic, "'unnamed'"));
				}
				return StringType(formatter.decorate(Decoration::generic_italic, "{unnamed type#1}"));
			}
			if (y) {
				return format_demangled(*y, subs, tp, dv, formatter, ps, skip, max_recursion);
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<float_> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)dv;
		(void)subs;
		(void)tp;

		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (not p) {
			return make_error();
		}
		return formatter.decorate(Decoration::number, *p);
	}
};

template <>
struct FormatDemangle<nonneg_number> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector, const SkipSpec&, RecursionLimit) {
		(void)dv;
		(void)subs;
		(void)tp;

		const auto* p = std::get_if<std::string_view>(res.comps.data());
		if (not p) {
			return make_error();
		}
		return formatter.decorate(Decoration::number, *p);
	}
};

template <>
struct FormatDemangle<name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() == 2) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (p and q) {
				auto na = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not na) {
					return na;
				}
				auto ta = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not ta) {
					return ta;
				}
				return *na + *ta;
			}
		}

		if (res.comps.size() == 1) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			if (p) {
				return format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
			}
		}

		return make_error();
	}

	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		if (res.comps.size() == 2) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (p and q) {
				auto na = format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
				auto ta = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not na) {
					return na;
				}
				if (not ta) {
					return ta;
				}
				*na += *ta;
				return na;
			}
		}

		if (res.comps.size() == 1) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			if (p) {
				return format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<negative_number> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				auto tmp = format_demangled(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return "-" + *tmp;
			}
		}
		return make_error();
	}
};

template <typename F, typename Formatter>
typename Formatter::value_type format_hexfloat(F v, const DemanglingDialect& dv, Formatter& formatter) {
	if (std::isinf(v)) {
		typename Formatter::value_type tmp;
		if (dv.nan_style == NaNStyle::mixed_caps and dv.infinity_style == InfinityStyle::infinity) {
			tmp = formatter.decorate(Decoration::generic_italic, "Infinity");
		}
		if (dv.nan_style == NaNStyle::small_caps and dv.infinity_style == InfinityStyle::infinity) {
			tmp = formatter.decorate(Decoration::generic_italic, "infinity");
		}
		if (dv.nan_style == NaNStyle::all_caps and dv.infinity_style == InfinityStyle::infinity) {
			tmp = formatter.decorate(Decoration::generic_italic, "INFINITY");
		}
		if (dv.nan_style == NaNStyle::mixed_caps and dv.infinity_style == InfinityStyle::inf) {
			tmp = formatter.decorate(Decoration::generic_italic, "Inf");
		}
		if (dv.nan_style == NaNStyle::small_caps and dv.infinity_style == InfinityStyle::inf) {
			tmp = formatter.decorate(Decoration::generic_italic, "inf");
		}
		if (dv.nan_style == NaNStyle::all_caps and dv.infinity_style == InfinityStyle::inf) {
			tmp = formatter.decorate(Decoration::generic_italic, "INF");
		}
		if (dv.infinity_style == InfinityStyle::unicode) {
			tmp = formatter.decorate(Decoration::generic_italic, "∞");
		}
		if (v < F(0)) {
			return "-" + tmp;
		}
		return tmp;
	}
	if (std::isnan(v)) {
		if (dv.nan_style == NaNStyle::mixed_caps) {
			return formatter.decorate(Decoration::generic_italic, "NaN");
		}
		if (dv.nan_style == NaNStyle::all_caps) {
			return formatter.decorate(Decoration::generic_italic, "NAN");
		}
		if (dv.nan_style == NaNStyle::small_caps) {
			return formatter.decorate(Decoration::generic_italic, "nan");
		}
	}
	return fmt::format("{:a}", v);
}

template <>
struct FormatDemangle<expr_primary> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (not p) {
					p = x;
				} else if (not q) {
					q = x;
					break;
				}
			}
		}

		if (not p or not q) {
			if (p) {
				const auto* l = std::get_if<std::string_view>(res.comps.data());
				if (l and *l == "L_Z") {
					return format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				}
			}
			return make_error();
		}

		const auto* pp = std::get_if<ParseResult>(p->comps.data());

		if (pp and pp->type == builtin_type and q->type == number) {
			const auto* ppp = std::get_if<ParseResult>(pp->comps.data());

			const std::string_view* pl = nullptr;
			const ParseResult* qr = nullptr;
			const std::string_view* qrl = nullptr;
			for (const auto& cc : ppp->comps) {
				const auto* x = std::get_if<std::string_view>(&cc);
				if (x) {
					pl = x;
					break;
				}
			}
			for (const auto& cc : q->comps) {
				const auto* x = std::get_if<ParseResult>(&cc);
				if (x) {
					qr = x;
					break;
				}
			}
			if (not pl or not qr) {
				return make_error();
			}

			if (*pl == "b" and qr->type == nonneg_number) {
				for (const auto& cc : qr->comps) {
					const auto* x = std::get_if<std::string_view>(&cc);
					if (x) {
						qrl = x;
						break;
					}
				}
				if (not qrl) {
					return make_error();
				}
				if (*qrl == "0") {
					return "false"s;
				}
				if (*qrl == "1") {
					return "true"s;
				}
				return make_error();
			}

			if (*pl == "i") {
				return format_demangled<number>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
			}
			if (*pl == "j") {
				auto tmp = format_demangled<number>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}

				if (dv.literal_decoration == LiteralDecoration::caps) {
					*tmp += "U";
				} else if (dv.literal_decoration == LiteralDecoration::noncaps) {
					*tmp += "u";
				}

				return tmp;
			}
			if (*pl == "l") {
				auto tmp = format_demangled<number>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}

				if (dv.literal_decoration == LiteralDecoration::caps) {
					*tmp += "L";
				} else if (dv.literal_decoration == LiteralDecoration::noncaps) {
					*tmp += "l";
				}

				return tmp;
			}
			if (*pl == "m") {
				auto tmp = format_demangled<number>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}

				if (dv.literal_decoration == LiteralDecoration::caps) {
					*tmp += "UL";
				} else if (dv.literal_decoration == LiteralDecoration::noncaps) {
					*tmp += "ul";
				}

				return tmp;
			}
			if (*pl == "x") {
				auto tmp = format_demangled<number>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}

				if (dv.literal_decoration == LiteralDecoration::caps) {
					*tmp += "LL";
				} else if (dv.literal_decoration == LiteralDecoration::noncaps) {
					*tmp += "ll";
				}

				return tmp;
			}
			if (*pl == "y") {
				auto tmp = format_demangled<number>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}

				if (dv.literal_decoration == LiteralDecoration::caps) {
					*tmp += "ULL";
				} else if (dv.literal_decoration == LiteralDecoration::noncaps) {
					*tmp += "ull";
				}

				return tmp;
			}
		}

		if (pp and pp->type == builtin_type and q->type == float_) {
			const auto* ppp = std::get_if<ParseResult>(pp->comps.data());

			const std::string_view* pl = nullptr;

			for (const auto& cc : ppp->comps) {
				const auto* x = std::get_if<std::string_view>(&cc);
				if (x) {
					pl = x;
					break;
				}
			}

			if (dv.float_template_constant_style == FloatTemplateConstantStyle::raw_byte_style) {
				if (*pl == "f" or *pl == "d" or *pl == "e" or *pl == "g" or *pl == "DF16b") {
					auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp0) {
						return tmp0;
					}
					auto tmp1 = format_demangled<float_>(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp1) {
						return tmp1;
					}

					return "(" + *tmp0 + ")" + formatter.decorate(Decoration::generic_italic, "[" + *tmp1 + "]");
				}
			}

			if (dv.float_template_constant_style == FloatTemplateConstantStyle::hexfloat_style) {
#if !defined(__EMSCRIPTEN__)
				if (*pl == "DF16_") {
					const auto* qv = std::get_if<std::string_view>(q->comps.data());
					_Float16 v = 0;
					if (qv and qv->size() == 2 * sizeof(v)) {
						auto iv = parse_uint_base16(*qv);
						if (iv) {
							auto [val, len] = *iv;
							uint16_t uv = val;
							memcpy(&v, &uv, sizeof(v));
							auto fv = float(v);
							return format_hexfloat(fv, dv, formatter) + "f16";
						}
					}
				}
#endif

#if !defined(__EMSCRIPTEN__)
				if (*pl == "DF16b") {
					const auto* qv = std::get_if<std::string_view>(q->comps.data());
					__bf16 v = 0;
					if (qv and qv->size() == 2 * sizeof(v)) {
						auto iv = parse_uint_base16(*qv);
						if (iv) {
							auto [val, len] = *iv;
							uint16_t uv = val;
							memcpy(&v, &uv, sizeof(v));
							auto fv = float(v);
							return format_hexfloat(fv, dv, formatter) + "f";
						}
					}
				}
#endif

				if (*pl == "f" or *pl == "DF32_") {
					const auto* qv = std::get_if<std::string_view>(q->comps.data());
					float v = 0;
					if (qv and qv->size() == 2 * sizeof(v)) {
						auto iv = parse_uint_base16(*qv);
						if (iv) {
							auto [val, len] = *iv;
							uint32_t uv = val;
							memcpy(&v, &uv, sizeof(v));
							return format_hexfloat(v, dv, formatter) + "f";
						}
					}
				}
				if (*pl == "d" or *pl == "DF64_") {
					const auto* qv = std::get_if<std::string_view>(q->comps.data());
					double v = 0;
					if (qv and qv->size() == 2 * sizeof(v)) {
						auto iv = parse_uint_base16(*qv);
						if (iv) {
							auto [val, len] = *iv;
							uint64_t uv = val;
							memcpy(&v, &uv, sizeof(v));
							return format_hexfloat(v, dv, formatter);
						}
					}
				}
			}
		}

		if (const auto* lz = std::get_if<std::string_view>(res.comps.data()); lz and *lz == "L_Z") {
		}

		using StringType = typename Formatter::value_type;
		Stacked<StringType> pstack;
		StringType result;
		result += "(";
		auto tmp0 = compose(format_demangled_type(*p, subs, tp, dv, formatter, pstack, ps, skip, max_recursion));
		if (not tmp0) {
			return tmp0;
		}
		result += *tmp0;
		result += ")";
		auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
		if (not tmp1) {
			return tmp1;
		}
		result += *tmp1;
		return result;
	}

	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		(void)stack;
		return expand(form(res, subs, tp, dv, formatter, ps, skip, max_recursion));
	}
};

template <>
struct FormatDemangle<encoding> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		auto fc = classify_encoding(res);
		using StringType = typename Formatter::value_type;
		if (not fc) {
			return StringType(make_error());
		}

		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;
		const ParseResult* r = nullptr;

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (not p) {
					p = x;
				} else if (not q) {
					q = x;

				} else {
					r = x;
					break;
				}
			}
		}

		auto nt = collect_tp(*p);
		auto ntt = tp;
		if (not nt.empty()) {
			ntt.add(std::move(nt));
		}

		if (fc->rs == ReturnSignature::mangled_return_type) {
			CompositeStack<StringType> result;
			StringType ret;
			for (const auto& cq : q->comps) {
				const auto* x = std::get_if<ParseResult>(&cq);
				if (x) {
					Stacked<StringType> pstack;
					auto tmp = compose(format_demangled_type(*x, subs, ntt, dv, formatter, pstack, ps, skip, max_recursion));
					if (not tmp) {
						return tmp;
					}
					ret += *tmp;

					break;
				}
			}

			bool explicit_object_param = false;
			if (p->type == name) {
				auto full_name = format_mangled(res);
				auto nominal_name = format_mangled(*p);
				formatter.add_replacement(nominal_name, full_name);

				const auto* pq = std::get_if<ParseResult>(p->comps.data());
				if (not pq) {
					return make_error();
				}
				if (pq->type == nested_name) {
					const auto* e = std::get_if<ParseResult>(&pq->comps[1]);
					if (e and e->type == explicit_object_member) {
						explicit_object_param = true;
					}
					auto tmp = format_demangled<nested_name>(*pq, subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return std::unexpected(tmp.error());
					}
					result = *tmp;
				} else if (pq->type == local_name) {
					auto tmp = format_demangled<local_name>(*pq, subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return std::unexpected(tmp.error());
					}
					result = *tmp;
				} else {
					auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					result = std::move(*tmp);
				}
				formatter.drop_replacement(nominal_name);
			} else {
				return make_error();
			}
			result.head = formatter.decorate(Decoration::function_name, result.head);

			if (dv.return_type_style == ReturnTypeStyle::leading_return) {
				result.head = ret + " " + result.head;
			} else if (dv.return_type_style == ReturnTypeStyle::trailing_return_leading_auto) {
				result.head = formatter.decorate(Decoration::type_keyword, "auto") + " " + result.head;
			}

			StringType params;
			result += formatter.decorate(Decoration::call_parentheses, "(");

			bool had_return = false;
			bool first = true;
			size_t pcount = 0;
			for (const auto& cq : q->comps) {
				const auto* x = std::get_if<ParseResult>(&cq);
				if (x) {
					if (not had_return) {
						had_return = true;
						continue;
					}
					Stacked<StringType> pstack;
					auto ctmp = compose(format_demangled_type(*x, subs, ntt, dv, formatter, pstack, ps, skip, max_recursion));
					if (not ctmp) {
						return ctmp;
					}
					auto& tmp = *ctmp;
					if (tmp.empty()) {
						continue;
					}
					

					if (first) {
						first = false;
						if (explicit_object_param) {
							params += formatter.decorate(Decoration::keyword, "this");
							params += ' ';
						}
					} else {
						params += ", ";
					}

					if (pcount >= dv.max_function_parameters) {
						params += ".";
						break;
					}

					if (pcount & 1) {
						params += formatter.decorate(Decoration::call_parameter_odd, tmp);
					} else {
						params += formatter.decorate(Decoration::call_parameter_even, tmp);
					}
					pcount++;
				}
			}

			if (params != "void") {
				result += params;
			}
			result += formatter.decorate(Decoration::call_parentheses, ")");
			if (not result.tail.empty()) {
				result.head += ' ';
				result.head += std::move(result.tail);
			}

			if (dv.return_type_style == ReturnTypeStyle::trailing_return or dv.return_type_style == ReturnTypeStyle::trailing_return_leading_auto) {
				result.head += " -> ";
				result.head += ret;
			}

			auto& fin = result.head;

			if (r) {
				fin += ' ';
				auto tdv = dv;
				if (dv.compiler_dialect == CompilerDialect::clang) {
					tdv.resolve_template_args = false;
					
				}
				auto tmp = format_demangled(*r, subs, ntt, tdv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				fin += *tmp;
			}
			return fin;
		}
		CompositeStack<StringType> result;
		bool explicit_object_param = false;
		if (p->type == name) {
			auto full_name = format_mangled(res);
			auto nominal_name = format_mangled(*p);
			formatter.add_replacement(nominal_name, full_name);

			const auto* pq = std::get_if<ParseResult>(p->comps.data());
			if (not pq) {
				return make_error();
			}
			if (pq->type == nested_name) {
				const auto* e = std::get_if<ParseResult>(&pq->comps[1]);
				if (e and e->type == explicit_object_member) {
					explicit_object_param = true;
				}
				auto tmp = format_demangled<nested_name>(*pq, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return std::unexpected(tmp.error());
				}
				result = std::move(*tmp);
			} else if (pq->type == local_name) {
				auto tmp = format_demangled<local_name>(*pq, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return std::unexpected(tmp.error());
				}
				result = std::move(*tmp);
			} else {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				result = std::move(*tmp);
			}
			formatter.drop_replacement(nominal_name);
		} else {
			return make_error();
		}
		result.head = formatter.decorate(Decoration::function_name, result.head);

		StringType params;
		result += formatter.decorate(Decoration::call_parentheses, "(");

		bool first = true;
		size_t pcount = 0;
		for (const auto& cq : q->comps) {
			const auto* x = std::get_if<ParseResult>(&cq);
			if (x) {
				Stacked<StringType> pstack;
				auto ctmp = compose(format_demangled_type(*x, subs, ntt, dv, formatter, pstack, ps, skip, max_recursion));
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;
				if (tmp.empty()) {
					continue;
				}
				if (first) {
					first = false;
					if (explicit_object_param) {
						params += formatter.decorate(Decoration::keyword, "this");
						params += ' ';
					}
				} else {
					params += ", ";
				}

				if (pcount >= dv.max_function_parameters) {
					params += ".";
					break;
				}

				if (pcount & 1) {
					params += formatter.decorate(Decoration::call_parameter_odd, tmp);
				} else {
					params += formatter.decorate(Decoration::call_parameter_even, tmp);
				}
				pcount++;
			}
		}

		if (params != "void") {
			result += params;
		}
		result += formatter.decorate(Decoration::call_parentheses, ")");

		if (not result.tail.empty()) {
			result.head += ' ';
			result.head += std::move(result.tail);
		}
		auto& fin = result.head;

		if (r) {
			fin += ' ';
			auto tdv = dv;
			if (dv.compiler_dialect == CompilerDialect::clang) {
				tdv.resolve_template_args = false;
				
			}
			auto tmp = format_demangled(*r, subs, ntt, tdv, formatter, ps, skip, max_recursion);
			if (not tmp) {
				return tmp;
			}
			fin += *tmp;
		}
		return fin;
	}
};

template <>
struct FormatDemangle<local_name> {
	template <typename Formatter>
	static std::expected<CompositeStack<typename Formatter::value_type>, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;
		using StringType = typename Formatter::value_type;
		(void)tp;
		NestedTemplateParams fresh_tp;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (not p) {
					p = x;
				} else {
					q = x;
					break;
				}
			}
		}
		if (p and q and p->type == encoding and q->type == name) {
			auto ddv = dv;
			if (dv.compiler_dialect == CompilerDialect::gcc) {
				ddv.return_type_style = ReturnTypeStyle::no_return;
			}

			auto ctmp = format_demangled(*p, subs, fresh_tp, ddv, formatter, ps, skip, max_recursion);
			if (not ctmp) {
				return std::unexpected(ctmp.error());
			}
			auto tmp = *ctmp + formatter.decorate(Decoration::noise, "::");
			CompositeStack<StringType> result;

			const auto* qq = std::get_if<ParseResult>(q->comps.data());
			if (qq and qq->type == nested_name) {
				auto nn = format_demangled<nested_name>(*qq, subs, fresh_tp, ddv, formatter, ps, skip, max_recursion);
				if (not nn) {
					return nn;
				}
				result = *nn;
			} else {
				auto tmp1 = format_demangled(*q, subs, fresh_tp, ddv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return std::unexpected(tmp1.error());
				}
				result = std::move(*tmp1);
			}

			result.head = tmp + result.head;
			return result;
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<closure_type_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		const ParseResult* q = nullptr;
		const ParseResult* r = nullptr;
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (not q) {
					q = x;
				} else {
					r = x;
					break;
				}
			}
		}

		if (not q) {
			return std::unexpected(ParseError::input_too_short);
		}

		StringType ret;
		if (dv.compiler_dialect == CompilerDialect::clang) {
			ret += '\'';
		} else {
			ret += '{';
		}
		ret += "lambda"s;
		if (dv.compiler_dialect == CompilerDialect::clang) {
			if (r) {
				const auto* p = std::get_if<std::string_view>(r->comps.data());
				if (p) {
					ret += *p;
				}
			}

			ret += '\'';
		}

		StringType params;
		params += '(';

		bool first = true;
		for (const auto& cq : q->comps) {
			const auto* x = std::get_if<ParseResult>(&cq);
			if (x) {
				auto ctmp = format_demangled(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not ctmp) {
					return ctmp;
				}
				auto& tmp = *ctmp;
				if (tmp.empty()) {
					continue;
				}
				if (first) {
					first = false;
				} else {
					params += ", "s;
				}
				params += tmp;
			}
		}

		params += ')';
		if (params == "(void)") {
			params = "()";
		}
		ret += params;
		if (dv.compiler_dialect == CompilerDialect::gcc) {
			ret += '#';
			if (not r) {
				ret += '1';
			} else {
				const auto* p = std::get_if<std::string_view>(r->comps.data());
				if (not p) {
					return StringType(make_error());
				}
				auto v = parse_uint(*p);
				if (v) {
					ret += fmt::format("{}", v->first + 2);
				}
			}
		}
		if (dv.compiler_dialect == CompilerDialect::gcc) {
			ret += '}';
		}
		ret = formatter.decorate(Decoration::generic_italic, ret);
		return ret;
	}
};

template <>
struct FormatDemangle<requires_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> v;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		const auto* l = std::get_if<std::string_view>(res.comps.data());
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (x->type == requirement_llvm) {
					v.emplace_back(x);
				} else {
					return make_error();
				}
			}
		}
		if (l and v.size() == 1) {
			auto tmp = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp) {
				return tmp;
			}
			return "{" + *tmp + ";}";
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<conversion_operator_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		for (const auto& c : res.comps) {
			const auto* p = std::get_if<ParseResult>(&c);
			if (p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return formatter.decorate(Decoration::keyword, "operator") + " " + *tmp;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<requires_clause> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			const auto* p = std::get_if<ParseResult>(&c);
			if (p) {
				auto tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return formatter.decorate(Decoration::keyword, "requires") + " " + *tmp;
			}
		}
		return make_error();
	}
};

template <>
struct FormatDemangle<expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> v;
		std::vector<std::string_view> vl;
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		if (res.comps.size() == 1) {
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					if (p->type == expr_primary or dv.display_expressions) {
						return format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
					}
				}
			}
		}

		if (not dv.display_expressions) {
			return formatter.decorate(Decoration::generic_italic, "X");
		}

		bool has_pi = false;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
				if (x->type == initializer) {
					has_pi = true;
				}
			}
		}

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<std::string_view>(&c);
			if (x) {
				vl.emplace_back(*x);
			} else {
				break;
			}
		}

		if (v.size() == 2 and v[0]->type == unary_operator_name and v[0]->comps.size() == 1) {
			const auto* x = std::get_if<std::string_view>(v[0]->comps.data());
			if (not x) {
				return make_error();
			}
			for (size_t i = 0; i < unsigned(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				const auto& l = *x;
				if (to_mangled_name(op) == l) {
					StringType result;
					if (op != ItaniumOperatorType::pp and op != ItaniumOperatorType::mm) {
						result += formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op));
					}

					const auto* x1 = std::get_if<ParseResult>(v[1]->comps.data());

					auto tmp = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					if ((x1 and x1->type == binary_operator_name and dv.parenthesize_expressions) or (dv.parenthesize_expressions and dv.compiler_dialect == CompilerDialect::gcc)) {
						result += "(" + *tmp + ")";
					} else {
						result += *tmp;
					}
					if (op == ItaniumOperatorType::pp or op == ItaniumOperatorType::mm) {
						result += formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op));
					}

					return result;
				}
			}
		}

		if (v.size() == 4 and v[0]->type == ternary_operator_name and v[0]->comps.size() == 1) {
			const auto* x = std::get_if<std::string_view>(v[0]->comps.data());
			if (not x) {
				return make_error();
			}

			StringType result;

			auto tmp0 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			result += *tmp0;

			if (dv.compiler_dialect == CompilerDialect::clang) {
				result += ' ';
			}
			result += formatter.decorate(Decoration::operator_symbol, "?");
			if (dv.compiler_dialect == CompilerDialect::clang) {
				result += ' ';
			}
			auto tmp1 = format_demangled(*v[2], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			result += *tmp1;
			result += ' ';
			result += formatter.decorate(Decoration::operator_symbol, ":");
			result += ' ';
			auto tmp2 = format_demangled(*v[3], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp2) {
				return tmp2;
			}
			result += *tmp2;

			return result;
		}

		if (v.size() == 3 and v[0]->type == binary_operator_name and v[0]->comps.size() == 1) {
			const auto* x = std::get_if<std::string_view>(v[0]->comps.data());
			if (not x) {
				return make_error();
			}
			for (size_t i = 0; i < unsigned(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				const auto& l = *x;
				if (to_mangled_name(op) == l) {
					StringType result;

					const auto* x1 = std::get_if<ParseResult>(v[1]->comps.data());
					auto tmp0 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp0) {
						return tmp0;
					}
					if ((x1 and x1->type == binary_operator_name and dv.parenthesize_expressions) or (dv.parenthesize_expressions and dv.compiler_dialect == CompilerDialect::gcc)) {
						result += '(';
						result += *tmp0;
						result += ')';
					} else {
						result += *tmp0;
					}

					if (dv.whitespace_operator_expressions) {
						result += ' ';
					}
					result += formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op));
					if (dv.whitespace_operator_expressions) {
						result += ' ';
					}
					const auto* x2 = std::get_if<ParseResult>(v[2]->comps.data());
					auto tmp1 = format_demangled(*v[2], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp1) {
						return tmp1;
					}
					if ((x2 and x2->type == binary_operator_name and dv.parenthesize_expressions) or (dv.parenthesize_expressions and dv.compiler_dialect == CompilerDialect::gcc)) {
						result += '(';
						result += *tmp1;
						result += ')';
					} else {
						result += *tmp1;
					}
					return result;
				}
			}
		}

		if (vl.size() == 2) {
			if (v.size() == 2 and vl[0] == "gs" and vl[1] == "nw") {
				StringType result = formatter.decorate(Decoration::noise, "::") + formatter.decorate(Decoration::keyword, "new") + " (";

				auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp0) {
					return tmp0;
				}
				auto tmp1 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return tmp1;
				}
				result += *tmp0;
				result += ") ";
				result += *tmp1;
				return result;
			}

			if (has_pi and v.size() == 3 and vl[0] == "gs" and vl[1] == "nw") {
				StringType result = formatter.decorate(Decoration::noise, "::") + formatter.decorate(Decoration::keyword, "new") + " (";

				auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp0) {
					return tmp0;
				}
				auto tmp1 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return tmp1;
				}
				auto tmp2 = format_demangled(*v[2], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp2) {
					return tmp2;
				}
				result += *tmp0;
				result += ") ";
				result += *tmp1;
				result += '(';
				result += *tmp2;
				result += ')';
				return result;
			}
		}

		return make_error();
	}
};

template <>
struct FormatDemangle<prefix_operator_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> v;
		std::vector<std::string_view> vl;
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
			}
		}

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<std::string_view>(&c);
			if (x) {
				vl.emplace_back(*x);
			} else {
				break;
			}
		}

		if (v.size() == 1) {
			const auto* x = std::get_if<std::string_view>(res.comps.data());
			if (not x) {
				return make_error();
			}
			for (size_t i = 0; i < unsigned(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				const auto& l = x->substr(0, 2);
				if (to_mangled_name(op) == l) {
					StringType result;
					result += formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op));
					const auto* x1 = std::get_if<ParseResult>(v[1]->comps.data());
					auto tmp = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					if ((x1 and x1->type == binary_operator_name and dv.parenthesize_expressions) or (dv.parenthesize_expressions and dv.compiler_dialect == CompilerDialect::gcc)) {
						result += "(" + *tmp + ")";
					} else {
						result += *tmp;
					}

					return result;
				}
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<fold_left_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> w;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				w.emplace_back(x);
			}
		}
		using namespace std::string_literals;
		auto v = std::span(w).subspan(1);
		const auto* l = std::get_if<std::string_view>(w[0]->comps.data());

		if (v.size() == 1) {
			for (size_t i = 0; i < size_t(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				if (*l == to_mangled_name(op)) {
					auto tmp = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					return "(... " + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op)) + " (" + *tmp + "...))";
				}
			}
		}

		if (v.size() == 2) {
			for (size_t i = 0; i < size_t(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				if (*l == to_mangled_name(op)) {
					auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp0) {
						return tmp0;
					}
					auto tmp1 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp1) {
						return tmp1;
					}
					return "(" + *tmp0 + " " + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op)) + " ... " + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op)) + " (" + *tmp1 + "...))";
				}
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<fold_right_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> w;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				w.emplace_back(x);
			}
		}
		using namespace std::string_literals;
		auto v = std::span(w).subspan(1);
		const auto* l = std::get_if<std::string_view>(w[0]->comps.data());

		if (v.size() == 1) {
			for (size_t i = 0; i < size_t(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				if (*l == to_mangled_name(op)) {
					auto tmp = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp) {
						return tmp;
					}
					return "((" + *tmp + "...) " + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op)) + " ...)";
				}
			}
		}

		if (v.size() == 2) {
			for (size_t i = 0; i < size_t(ItaniumOperatorType::number_of_options); ++i) {
				auto op = static_cast<ItaniumOperatorType>(i);
				if (*l == to_mangled_name(op)) {
					auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp0) {
						return tmp0;
					}
					auto tmp1 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
					if (not tmp1) {
						return tmp1;
					}
					return "((" + *tmp0 + "...) " + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op)) + " ... " + formatter.decorate(Decoration::operator_symbol, to_demangled_operator_name(op)) + " " + *tmp1 + ")";
				}
			}
		}

		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<call_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> v;
		std::vector<std::string_view> vl;
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
			}
		}

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<std::string_view>(&c);
			if (x) {
				vl.emplace_back(*x);
			} else {
				break;
			}
		}
		if (vl.size() == 1 and not v.empty() and vl[0] == "cl") {
			StringType result;
			auto wrapped = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not wrapped) {
				return wrapped;
			}
			if (dv.compiler_dialect == CompilerDialect::gcc and v[0]->type == base_unresolved_name and v[0]->comps.size() == 1 and std::get_if<ParseResult>(v[0]->comps.data())->type == simple_id and std::get_if<ParseResult>(v[0]->comps.data())->comps.size() == 2) {
				result += '(';
			}
			result += *wrapped;
			if (dv.compiler_dialect == CompilerDialect::gcc and v[0]->type == base_unresolved_name and v[0]->comps.size() == 1 and std::get_if<ParseResult>(v[0]->comps.data())->type == simple_id and std::get_if<ParseResult>(v[0]->comps.data())->comps.size() == 2) {
				result += ')';
			}
			result += '(';
			StringType params;
			for (size_t i = 1; i < v.size(); i++) {
				if (not params.empty()) {
					params += ", ";
				}

				auto tmp = format_demangled(*v[i], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				params += *tmp;
			}

			result += params;
			result += ')';
			return result;
		}

		return make_error();
	}
};

template <>
struct FormatDemangle<conversion_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		std::vector<const ParseResult*> v;
		std::vector<std::string_view> vl;
		using StringType = typename Formatter::value_type;
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
			}
		}

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<std::string_view>(&c);
			if (x) {
				vl.emplace_back(*x);
			} else {
				break;
			}
		}

		if (vl.size() == 1 and v.size() == 2 and vl[0] == "cv" and v[0]->type == ProductionType::type and v[1]->type == expression) {
			auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			StringType tmp = "(" + *tmp0 + ")";

			tmp += formatter.decorate(Decoration::call_parentheses, "(");
			auto tmp1 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			tmp += *tmp1;
			tmp += formatter.decorate(Decoration::call_parentheses, ")");
			return tmp;
		}
		if (not vl.empty() and not v.empty() and vl[0] == "cv" and v[0]->type == ProductionType::type) {
			auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			StringType tmp = "(" + *tmp0 + ")";

			tmp += formatter.decorate(Decoration::call_parentheses, "(");
			StringType params;
			for (size_t i = 1; i < v.size(); i++) {
				if (not params.empty()) {
					params += ", ";
				}
				auto tmp1 = format_demangled(*v[i], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp1) {
					return tmp1;
				}
				params += *tmp1;
			}
			tmp += params;
			tmp += formatter.decorate(Decoration::call_parentheses, ")");
			return tmp;
		}

		return make_error();
	}
};

template <>
struct FormatDemangle<pack_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		std::vector<const ParseResult*> v;
		std::vector<std::string_view> vl;
		using StringType = typename Formatter::value_type;

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
			}
		}

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<std::string_view>(&c);
			if (x) {
				vl.emplace_back(*x);
			} else {
				break;
			}
		}

		if (v.size() == 1 and vl.size() == 1 and vl[0] == "sp") {
			if (not dv.resolve_packs or not dv.resolve_template_args) {
				auto tmp = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
				if (not tmp) {
					return tmp;
				}
				return *tmp + "...";
			}
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					auto ops = get_pack_size(*p, subs, tp, max_recursion);
					if (not ops) {
						return StringType(make_error());
					}

					if (*ops == 0) {
						return {};
					}

					StringType result;

					for (int i = 0; i < *ops; ++i) {
						PackSelector pi {i};
						auto ctmp = format_demangled(*p, subs, tp, dv, formatter, pi, skip, max_recursion);
						if (not ctmp) {
							return ctmp;
						}
						auto& tmp = *ctmp;
						if (tmp.empty()) {
							continue;
						}
						if (not result.empty()) {
							result += ", ";
						}
						result += tmp;
					}
					return result;
				}
			}
			return StringType(make_error());
		}

		return make_error();
	}
};

template <>
struct FormatDemangle<bind_expression> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		std::vector<const ParseResult*> v;
		std::vector<std::string_view> vl;
		using StringType = typename Formatter::value_type;

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
			}
		}

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<std::string_view>(&c);
			if (x) {
				vl.emplace_back(*x);
			} else {
				break;
			}
		}

		if (vl.size() == 1 and v.size() == 2) {
			auto tmp0 = format_demangled(*v[0], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp0) {
				return tmp0;
			}
			auto tmp1 = format_demangled(*v[1], subs, tp, dv, formatter, ps, skip, max_recursion);
			if (not tmp1) {
				return tmp1;
			}
			if (vl[0] == "dt") {
				return *tmp0 + "." + *tmp1;
			}
			if (vl[0] == "pt") {
				return *tmp0 + "->" + *tmp1;
			}
			if (vl[0] == "ds") {
				return *tmp0 + ".*" + *tmp1;
			}
		}
		return StringType(make_error());
	}
};

template <>
struct FormatDemangle<unresolved_name_gcc> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;

		if (res.comps.size() < 2) {
			return make_error();
		}

		const ParseResult* p = nullptr;
		const ParseResult* q = nullptr;

		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				if (not p) {
					p = x;
				} else if (not q) {
					q = x;

					break;
				}
			}
		}

		if (not p or not q) {
			return make_error();
		}
		auto tmp0 = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
		if (not tmp0) {
			return tmp0;
		}
		auto tmp1 = format_demangled(*q, subs, tp, dv, formatter, ps, skip, max_recursion);
		if (not tmp1) {
			return tmp1;
		}
		return *tmp0 + formatter.decorate(Decoration::noise, "::") + *tmp1;
	}
};

template <>
struct FormatDemangle<mangled_name> {
	template <typename Formatter>
	static std::expected<typename Formatter::value_type, ParseError> form(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
		if (max_recursion == 0) {
			return std::unexpected(ParseError::recursion_reached);
		}
		--max_recursion;
		using StringType = typename Formatter::value_type;
		StringType result;
		bool first = true;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				std::expected<StringType, ParseError> tmp;
				if (first) {
					auto fullname = format_mangled(res);
					auto nominal_name = format_mangled(*x);
					formatter.add_replacement(nominal_name, fullname);
					tmp = format_demangled(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
					formatter.drop_replacement(nominal_name);
					first = false;
				} else {
					tmp = format_demangled(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
				}
				if (not tmp) {
					return tmp;
				}

				if (tmp->empty()) {
					continue;
				}
				if (not result.empty()) {
					result += ' ';
				}
				result += *tmp;
			}
		}
		return result;
	}
};

template <ProductionType ltype, typename Formatter>
auto format_demangled(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
	return FormatDemangle<ltype>::form(res, subs, tp, dv, formatter, ps, skip, max_recursion);
}

template <ProductionType ltype, typename Formatter>
auto format_demangled_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
	return FormatDemangle<ltype>::form_type(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
}

}

template <typename Formatter>
std::expected<typename Formatter::value_type, ParseError> format_demangled_short(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
	if (max_recursion == 0) {
		return std::unexpected(ParseError::recursion_reached);
	}
	--max_recursion;
	if (res.type == substitution) {
		const auto* p = std::get_if<std::string_view>(res.comps.data());
		using StringType = typename Formatter::value_type;
		if (not p) {
			return StringType(make_error());
		}

		if (*p == "Sa") {
			return StringType("allocator"s);
		}
		if (*p == "Sb") {
			return StringType("basic_string"s);
		}
		if (*p == "Ss") {
			return StringType("basic_string"s);
		}
		if (*p == "Si") {
			return StringType("basic_istream"s);
		}
		if (*p == "So") {
			return StringType("basic_ostream"s);
		}
		if (*p == "Sd") {
			return StringType("basic_iostream"s);
		}
	}
	if (res.type == prefix and res.comps.size() == 1) {
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		if (p) {
			return format_demangled_short(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
		}
	}

	if (res.type == nested_name) {
		std::vector<const ParseResult*> v;
		for (const auto& c : res.comps) {
			const auto* x = std::get_if<ParseResult>(&c);
			if (x) {
				v.emplace_back(x);
			}
		}
		while (not v.empty() and v.back()->type == template_args) {
			v.pop_back();
		}
		return format_demangled(*v.back(), subs, tp, dv, formatter, ps, skip, max_recursion);
	}
	if ((res.type == type or res.type == name or res.type == class_enum_type or res.type == unscoped_template_name) and res.comps.size() == 1) {
		const auto* x = std::get_if<ParseResult>(res.comps.data());
		return format_demangled_short(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
	}

	if ((res.type == name) and res.comps.size() == 2) {
		const auto* x = std::get_if<ParseResult>(res.comps.data());
		return format_demangled_short(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
	}

	if ((res.type == unscoped_name) and res.comps.size() == 2) {
		const auto* x = std::get_if<ParseResult>(&res.comps[1]);
		return format_demangled_short(*x, subs, tp, dv, formatter, ps, skip, max_recursion);
	}

	return format_demangled(res, subs, tp, dv, formatter, ps, skip, max_recursion);
}

template <typename Formatter>
std::expected<typename Formatter::value_type, ParseError> format_demangled(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
	using StringType = typename Formatter::value_type;
	switch (res.type) {
	case source_name:
		return format_demangled<source_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case identifier:
		return format_demangled<identifier>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case nested_name:
		return compose(format_demangled<nested_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion));
	case local_name:
		return compose(format_demangled<local_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion));
	case template_args:
		return format_demangled<template_args>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case substitution:
		return format_demangled<substitution>(res, subs, tp, dv, formatter, ps, skip, max_recursion);

	case template_param:
		return format_demangled<template_param>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case operator_name:
		return format_demangled<operator_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case abi_tags:
		return format_demangled<abi_tags>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case negative_number:
		return format_demangled<negative_number>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case nonneg_number:
		return format_demangled<nonneg_number>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case number:
		return format_demangled<number>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case expr_primary:
		return format_demangled<expr_primary>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case encoding:
		return format_demangled<encoding>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case partial_nested_name:
		return format_demangled<partial_nested_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case unscoped_name:
		return format_demangled<unscoped_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case unary_operator_name:
		return format_demangled<unary_operator_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case binary_operator_name:
		return format_demangled<binary_operator_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case vtable:
		return format_demangled<vtable>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case typeinfo:
		return format_demangled<typeinfo>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case vtt:
		return format_demangled<vtt>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case typeinfo_name_object:
		return format_demangled<typeinfo_name_object>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case guard_variable:
		return format_demangled<guard_variable>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case nonvirtual_thunk:
		return format_demangled<nonvirtual_thunk>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case virtual_covariant_thunk:
		return format_demangled<virtual_covariant_thunk>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case virtual_thunk:
		return format_demangled<virtual_thunk>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case construction_vtable:
		return format_demangled<construction_vtable>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case global_sub_for_mangled_name:
		return format_demangled<global_sub_for_mangled_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case closure_type_name:
		return format_demangled<closure_type_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);

	case unresolved_name_gcc:
		return format_demangled<unresolved_name_gcc>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case template_arg_pack:
		return format_demangled<template_arg_pack>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case expression:
		return format_demangled<expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case requires_clause:
		return format_demangled<requires_clause>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case requires_expression:
		return format_demangled<requires_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case conversion_operator_name:
		return format_demangled<conversion_operator_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case lambda_sig:
		return format_demangled<lambda_sig>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case vendor_specfic_suffix:
		return format_demangled<vendor_specfic_suffix>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case mangled_name:
		return format_demangled<mangled_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case unnamed_type_name:
		return format_demangled<unnamed_type_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case keyword_type_expression:
		return format_demangled<keyword_type_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case keyword_cast_expression:
		return format_demangled<keyword_cast_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case keyword_expression:
		return format_demangled<keyword_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case keyword_cast_operator:
		return format_demangled<keyword_cast_operator>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case type_expression_operator:
		return format_demangled<type_expression_operator>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case expression_operator:
		return format_demangled<expression_operator>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case rethrow_expression:
		return format_demangled<rethrow_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case float_:
		return format_demangled<float_>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case bare_function_type:
		return format_demangled<bare_function_type>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case name:
		return format_demangled<name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case function_param:
		return format_demangled<function_param>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case unresolved_name:
		return format_demangled<unresolved_name>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case bind_expression:
		return format_demangled<bind_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case pack_expression:
		return format_demangled<pack_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case call_expression:
		return format_demangled<call_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case conversion_expression:
		return format_demangled<conversion_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case prefix_operator_expression:
		return format_demangled<prefix_operator_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case fold_left_expression:
		return format_demangled<fold_left_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case fold_right_expression:
		return format_demangled<fold_right_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);
	case pack_indexing_expression:
		return format_demangled<pack_indexing_expression>(res, subs, tp, dv, formatter, ps, skip, max_recursion);

	case type:
	case builtin_type:
	case qualified_type: {
		Stacked<StringType> pstack;
		if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
			return compose(format_demangled_type(*p, subs, tp, dv, formatter, pstack, ps, skip, max_recursion));
		}
		return StringType(make_error());
	}

	default:
		break;
	}

	if ((res.comps.size() == 1 and std::get_if<ParseResult>(res.comps.data())) or res.type == simple_id or res.type == prefix or res.type == template_arg or res.type == unqualified_name or res.type == abi_tag or res.type == closure_prefix or res.type == fold_expression or res.type == dot_vendor_specific_suffix or res.type == mangled_name_apple) {
		StringType result;
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				std::expected<StringType, ParseError> tmp;
				if (p->type == ProductionType::type) {
					Stacked<StringType> pstack;
					tmp = compose(format_demangled_type(*p, subs, tp, dv, formatter, pstack, ps, skip, max_recursion));
				} else {
					tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				}
				if (not tmp) {
					return tmp;
				}

				if (result.empty()) {
					result = std::move(*tmp);
				} else {
					if (result.ends_with('<') and tmp->starts_with('<') and dv.separating_whitespace) {
						result += ' ';
					}
					result += *tmp;
				}
			}
		}

		return result;
	}

	return StringType(make_error());
}

template <typename Formatter>
std::expected<CompositeStack<typename Formatter::value_type>, ParseError> format_demangled_type(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, Formatter& formatter, Stacked<typename Formatter::value_type>& stack, PackSelector ps, const SkipSpec& skip, RecursionLimit max_recursion) {
	using StringType = typename Formatter::value_type;
	switch (res.type) {
	case template_param:
		return format_demangled_type<template_param>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case substitution:
		return format_demangled_type<substitution>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case template_arg_pack:
		return format_demangled_type<template_arg_pack>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);

	case reference_type:
		return format_demangled_type<reference_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case rvalue_reference_type:
		return format_demangled_type<rvalue_reference_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case const_type:
		return format_demangled_type<const_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case pointer_type:
		return format_demangled_type<pointer_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case restrict_type:
		return format_demangled_type<restrict_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case pointer_to_member_type:
		return format_demangled_type<pointer_to_member_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);

	case vendor_ext_arg_type_llm:
		return format_demangled_type<vendor_ext_arg_type_llm>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case function_type:
		return format_demangled_type<function_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);

	case decltype_:
		return format_demangled_type<decltype_>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case array_type:
		return format_demangled_type<array_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case vector_type:
		return format_demangled_type<vector_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case builtin_boolean_type:
		return format_demangled_type<builtin_boolean_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case builtin_real_type:
		return format_demangled_type<builtin_real_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case builtin_integral_type:
		return format_demangled_type<builtin_integral_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case builtin_other_type:
		return format_demangled_type<builtin_other_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case complex_type_c99:
		return format_demangled_type<complex_type_c99>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case class_enum_type:
		return format_demangled_type<class_enum_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case pack_type:
		return format_demangled_type<pack_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case extended_qualifier:
		return format_demangled_type<extended_qualifier>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case expr_primary:
		return format_demangled_type<expr_primary>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case name:
		return format_demangled_type<name>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case template_param_decl_llvm:
		return format_demangled_type<template_param_decl_llvm>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case pack_indexing_type:
		return format_demangled_type<pack_indexing_type>(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
	case type:
	case builtin_type:
	case qualified_type:
	case template_arg: {
		if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
			return format_demangled_type(*p, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
		}
		return StringType(make_error());
	}
	default:
		break;
	}

	if ((res.comps.size() == 1 and std::get_if<ParseResult>(res.comps.data())) or res.type == simple_id or res.type == prefix) {
		StringType result;

		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				std::expected<StringType, ParseError> tmp;
				if (p->type == ProductionType::type) {
					Stacked<StringType> pstack;
					tmp = compose(format_demangled_type(*p, subs, tp, dv, formatter, pstack, ps, skip, max_recursion));
				} else {
					tmp = format_demangled(*p, subs, tp, dv, formatter, ps, skip, max_recursion);
				}
				if (not tmp) {
					return std::unexpected(tmp.error());
				}

				if (result.empty()) {
					result = std::move(*tmp);
				} else {
					if (result.ends_with('<') and tmp->starts_with('<') and dv.separating_whitespace) {
						result += ' ';
					}
					result += *tmp;
				}
			}
		}

		return result;
	}
	return StringType(make_error());
}

std::expected<std::string, ParseError> format_demangled_ascii(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	AsciiFormatter formatter;
	PackSelector ps;
	return format_demangled(res, subs, tp, dv, formatter, ps, skip, max_recursion);
}

std::expected<CompositeStack<std::string>, ParseError> format_demangled_type_ascii(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	AsciiFormatter formatter;
	Stacked<std::string> stack;
	PackSelector ps;
	return format_demangled_type(res, subs, tp, dv, formatter, stack, ps, skip, max_recursion);
}

std::expected<HighlightedText, ParseError> format_demangled_high(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	HighlightFormatter formatter;
	PackSelector ps;
	return format_demangled(res, subs, tp, dv, formatter, ps, skip, max_recursion);
}

std::expected<std::string, ParseError> format_demangled_cli_light(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	auto tmp = format_demangled_high(res, subs, tp, dv, skip, max_recursion);
	auto result = render_cli(tmp, true);
	return result;
}

std::expected<std::string, ParseError> format_demangled_cli_dark(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	auto tmp = format_demangled_high(res, subs, tp, dv, skip, max_recursion);
	auto result = render_cli(tmp, false);
	return result;
}

std::expected<std::string, ParseError> format_demangled_html_light(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	auto tmp = format_demangled_high(res, subs, tp, dv, skip, max_recursion);
	auto result = render_html(tmp, true);
	return result;
}

std::expected<std::string, ParseError> format_demangled_html_dark(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	auto tmp = format_demangled_high(res, subs, tp, dv, skip, max_recursion);
	auto result = render_html(tmp, false);
	return result;
}

ParseResult restore_substitutions(const ParseResult& r, const SubstitutionTableView& subs) {
	if (r.type == ProductionType::substitution) {
		auto [subbed, new_subs, tp] = resolve_substitution(r, subs);
		if (subbed) {
			return *subbed;
		} else {
			return r;
		}
	}
	ParseResult res;
	res.type = r.type;
	for (const auto& c : r.comps) {
		const auto* p = std::get_if<ParseResult>(&c);
		if (p) {
			res.comps.emplace_back(restore_substitutions(*p, subs));
		} else {
			res.comps.emplace_back(c);
		}
	}

	return res;
}

ParseResult restore_substitutions_and_template_params(const ParseResult& r, const SubstitutionTableView& subs, const NestedTemplateParams& tp) {
	if (r.type == ProductionType::substitution) {
		if (r.comps.size() > 1 or *std::get_if<std::string_view>(r.comps.data()) == "S_") {
			auto [subbed, new_subs, stp] = resolve_substitution(r, subs);
			if (subbed) {
				return *subbed;
				return restore_substitutions_and_template_params(*subbed, subs, *stp);
			} else {
				return r;
			}
		} else {
			return r;
		}
	} else if (r.type == ProductionType::template_param) {
		const auto* subbed = resolve_template_param(r, tp);
		if (subbed) {
			return *subbed;
			return restore_substitutions_and_template_params(*subbed, subs, tp);
		} else {
			return r;
		}
	}
	ParseResult res;
	res.type = r.type;
	for (const auto& c : r.comps) {
		const auto* p = std::get_if<ParseResult>(&c);
		if (p) {
			res.comps.emplace_back(restore_substitutions_and_template_params(*p, subs, tp));
		} else {
			res.comps.emplace_back(c);
		}
	}

	return res;
}

struct ItaniumXmlLinker {
	using value_type = std::string;
	std::string link_target(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp) const {
		auto tmp = restore_substitutions_and_template_params(res, subs, tp);
		auto true_mang = format_mangled(res);
		auto tit = replacement.find(true_mang);
		if (tit != replacement.end()) {
			return fmt::format("/{}/{}", prefix, tit->second);
		}

		auto mang = format_mangled(tmp);

		auto it = replacement.find(mang);
		if (it != replacement.end()) {
			return fmt::format("/{}/{}", prefix, it->second);
		}
		return fmt::format("/{}/{}", prefix, mang);
	}
	std::string link_to(const ParseResult& res, std::string_view tn, const SubstitutionTableView& subs, const NestedTemplateParams& tp) const {
		return hyperlink(link_target(res, subs, tp), tn, cfg.navigation);
	}
	std::map<std::string, std::string> replacement;
	void add_replacement(std::string a, const std::string& b) {
		auto it = replacement.find(b);
		if (it != replacement.end()) {
			replacement.try_emplace(std::move(a), it->second);
		} else {
			replacement.try_emplace(std::move(a), b);
		}
	}
	void drop_replacement(const std::string& a) {
		replacement.erase(a);
	}
	std::string prefix;
	PageConfig cfg;
	bool no_linking = false;
};

using ItaniumXmlFormatter = Formatter<XmlDecorator, XmlEscaper, ItaniumXmlLinker>;

std::expected<std::string, ParseError> format_demangled_html(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion, std::string_view prefix) {
	ItaniumXmlFormatter formatter;
	formatter.linker.prefix = prefix;
	PackSelector ps;
	return format_demangled(res, subs, tp, dv, formatter, ps, skip, max_recursion);
}

}
