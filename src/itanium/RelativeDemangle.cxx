#include "RelativeDemangle.hxx"
#include "FormatDemangle.hxx"
#include "ParseResult.hxx"
#include "ProductionType.hxx"
#include <variant>

namespace swartalf::itanium {

std::string_view get_source_name(const ParseResult& res) {
	if (res.type == ProductionType::identifier) {
		const auto* l = std::get_if<std::string_view>(res.comps.data());
		if (l) {
			return *l;
		}
	}
	if (res.type == ProductionType::source_name and res.comps.size() == 2) {
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);
		if (q and q->type == ProductionType::identifier) {
			return get_source_name(*q);
		}
	}
	return {};
}

SkipSpec get_relative_names_from_nested_name(const ParseResult& res) {
	SkipAlternative a;
	a.action = SkipAction::hide_prefix;

	for (const auto& c : res.comps) {
		const auto* x = std::get_if<ParseResult>(&c);
		if (x and x->type == ProductionType::prefix and x->comps.size() == 1) {
			const auto* p = std::get_if<ParseResult>(x->comps.data());
			if (p and p->type == ProductionType::unqualified_name) {
				const auto* q = std::get_if<ParseResult>(p->comps.data());
				if (q) {
					auto l = get_source_name(*q);
					if (not l.empty()) {
						a.demangled_parts.emplace_back(l);
					}
				}
			}
		}
	}

	SkipSpec skip;
	if (not a.demangled_parts.empty()) {
		skip.skip.emplace_back(std::move(a));
	}
	return skip;
}
SkipSpec get_relative_names_from_local_name(const ParseResult& res) {
	for (const auto& c : res.comps) {
		const auto* p = std::get_if<ParseResult>(&c);
		if (p and p->type == ProductionType::encoding) {
			return get_relative_names(*p);
		}
	}

	return {};
}
SkipSpec get_relative_names_from_name(const ParseResult& res) {
	if (res.comps.size() == 1) {
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		if (p and p->type == ProductionType::nested_name) {
			return get_relative_names_from_nested_name(*p);
		}
		if (p and p->type == ProductionType::local_name) {
			return get_relative_names_from_local_name(*p);
		}

		if (p and res.type == ProductionType::name) {
			return get_relative_names_from_name(*p);
		}
	}

	if (res.comps.size() == 2) {
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);

		const auto* l = std::get_if<std::string_view>(res.comps.data());
		if (l and q and *l == "St" and q->type == ProductionType::unqualified_name) {
			SkipAlternative a;
			a.demangled_parts.emplace_back("std");
			a.action = SkipAction::hide_prefix;
			SkipSpec skip;
			skip.skip.emplace_back(std::move(a));
			return skip;
		}

		if (p and q and q->type == ProductionType::template_args) {
			return get_relative_names_from_name(*p);
		}
	}

	return {};
}

SkipSpec get_relative_names(const ParseResult& res) {
	if (res.type == ProductionType::mangled_name or res.type == ProductionType::mangled_name_apple or res.type == ProductionType::any_mangled_name) {
		for (const auto& c : res.comps) {
			const auto* t = std::get_if<ParseResult>(&c);
			if (t) {
				return get_relative_names(*t);
			}
		}

		return {};
	}
	if (res.type == ProductionType::encoding) {
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		if (p and p->type == ProductionType::name) {
			return get_relative_names_from_name(*p);
		}
	}

	return {};
}
}