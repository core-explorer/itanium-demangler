#include "Grammar.hxx"
#include "ParseName.hxx"
#include "ParseResult.hxx"
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace swartalf::itanium {

std::optional<std::pair<ParseResult, ParseResult>> is_templated_name(const ParseResult& res) {
	if (res.type == ProductionType::nested_name) {
		for (size_t k = 0; k < res.comps.size(); ++k) {
			auto i = res.comps.size() - 1 - k;
			const auto& c = res.comps[i];
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (x->type == ProductionType::template_args) {
					ParseResult f = res;
					std::swap(f.comps.back(), f.comps[res.comps.size() - 2]);
					f.comps.pop_back();
					return std::make_pair(f, *x);
				}
				break;
			}
		}
	}

	if (res.type == ProductionType::name) {
		if (res.comps.size() == 1) {
			const auto* x = std::get_if<ParseResult>(res.comps.data());
			if (x) {
				return is_templated_name(*x);
			}
		} else if (res.comps.size() == 2) {
			const auto* x = std::get_if<ParseResult>(res.comps.data());
			const auto* y = std::get_if<ParseResult>(&res.comps[1]);
			if (x and y and x->type == ProductionType::unscoped_template_name and y->type == ProductionType::template_args) {
				const auto* xx = std::get_if<ParseResult>(x->comps.data());
				if (xx) {
					return std::make_pair(*xx, *y);
				}
			}
		}
	}

	if (res.type == ProductionType::class_enum_type) {
		if (res.comps.size() == 1) {
			const auto* x = std::get_if<ParseResult>(res.comps.data());
			if (x) {
				return is_templated_name(*x);
			}
		}
	}
	if (res.type == ProductionType::type) {
		if (res.comps.size() == 1) {
			const auto* x = std::get_if<ParseResult>(res.comps.data());
			if (x) {
				return is_templated_name(*x);
			}
		}
	}
	return std::nullopt;
}

std::optional<ParseResult> is_ctor_or_dtor(const ParseResult& res) {
	if (res.type == ProductionType::ctor_dtor_name) {
		return res;
	}
	if (res.type == ProductionType::nested_name) {
		for (size_t k = 0; k < res.comps.size(); ++k) {
			auto i = res.comps.size() - 1 - k;
			const auto& c = res.comps[i];
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				auto b = is_ctor_or_dtor(*p);
				if (b) {
					return b;
				}
			}
		}
		return std::nullopt;
	}
	if (res.type == ProductionType::name or res.type == ProductionType::unqualified_name) {
		if (res.comps.size() == 1) {
			if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
				return is_ctor_or_dtor(*p);
			}
		}
		return std::nullopt;
	}
	if (res.type == ProductionType::encoding) {
		if (not res.comps.empty()) {
			if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
				return is_ctor_or_dtor(*p);
			}
		}
	}
	if (res.type == ProductionType::mangled_name or res.type == ProductionType::mangled_name_apple) {
		for (const auto& c : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&c); p) {
				return is_ctor_or_dtor(*p);
			}
		}
	}
	return std::nullopt;
}

void modify_ctor_or_dtor(ParseResult& res, std::string_view new_lit) {
	if (res.type == ProductionType::ctor_dtor_name) {
		res.comps[0] = new_lit;
		return;
	}
	if (res.type == ProductionType::nested_name) {
		for (size_t k = 0; k < res.comps.size(); ++k) {
			if (k > 2) {
				break;
			}
			auto i = res.comps.size() - 1 - k;
			auto& c = res.comps[i];
			if (auto* p = std::get_if<ParseResult>(&c); p) {
				modify_ctor_or_dtor(*p, new_lit);
			}
		}
		return;
	}
	if (res.type == ProductionType::name or res.type == ProductionType::unqualified_name) {
		if (res.comps.size() == 1) {
			if (auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
				modify_ctor_or_dtor(*p, new_lit);
			}
		}
		return;
	}
	if (res.type == ProductionType::encoding) {
		if (not res.comps.empty()) {
			if (auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
				modify_ctor_or_dtor(*p, new_lit);
			}
		}
		return;
	}
	if (res.type == ProductionType::mangled_name or res.type == ProductionType::mangled_name_apple) {
		for (auto& c : res.comps) {
			if (auto* p = std::get_if<ParseResult>(&c); p) {
				modify_ctor_or_dtor(*p, new_lit);
			}
		}
	}
}

std::vector<ParseResult> duplicate_ctor_dtor(const ParseResult& res, std::span<const std::string_view> new_lits) {
	std::vector<ParseResult> result(new_lits.size(), res);
	for (size_t i = 0; i < new_lits.size(); ++i) {
		modify_ctor_or_dtor(result[i], new_lits[i]);
	}
	return result;
}

std::optional<ParseResult> is_not_odr_relevant(const ParseResult& res) {
	switch (res.type) {
	case swartalf::itanium::ProductionType::unnamed_type_name:
	case swartalf::itanium::ProductionType::closure_type_name:
		return res;
	case swartalf::itanium::ProductionType::source_name: {
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x and x->type == ProductionType::identifier) {
				const auto* xl = std::get_if<std::string_view>(x->comps.data());
				if (not xl) {
					return std::nullopt;
				}
				if (*xl == "_GLOBAL__N_1" or xl->contains('$') or xl->contains('.')) {
					return res;
				}
			}
		}
		return std::nullopt;
	}
	default:
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				auto inor = is_not_odr_relevant(*x);
				if (inor) {
					return inor;
				}
			}
		}
	}

	return std::nullopt;
}

}