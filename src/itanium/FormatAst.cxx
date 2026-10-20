#include "FormatAst.hxx"
#include "FormatMangle.hxx"
#include "Grammar.hxx"

namespace swartalf::itanium {
using enum ProductionType;

std::string to_string(const TemplateParams& tp) {
	std::string result = "{";
	for (size_t i = 0; i < tp.size(); i++) {
		if (i > 0) {
			result += ", ";
		}
		result += format_mangled(tp.entries[i]);
	}
	result += '}';
	return result;
}

TemplateParams collect_tp(const ParseResult& res) {
	switch (res.type) {
	case name: {
		if (res.comps.size() == 2) {
			if (const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			    q) {
				return collect_tp(*q);
			}
		}
		if (res.comps.size() == 1) {
			if (const auto* q = std::get_if<ParseResult>(res.comps.data());
			    q) {
				return collect_tp(*q);
			}
		}
		return {};
	} break;
	case prefix: {
		if (res.comps.size() == 2) {
			if (const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			    q) {
				return collect_tp(*q);
			}
		}
		if (res.comps.size() == 1) {
			if (const auto* q = std::get_if<ParseResult>(res.comps.data());
			    q) {
				return collect_tp(*q);
			}
		}
		return {};
	} break;
	case nested_name: {
		for (size_t k = 0; k < res.comps.size(); k++) {
			size_t i = res.comps.size() - 1 - k;
			if (const auto* q = std::get_if<ParseResult>(&res.comps[i]);
			    q) {
				auto tp = collect_tp(*q);
				if (not tp.empty()) {
					return tp;
				}
			}
		}
		return {};
	} break;
	case template_args: {
		TemplateParams tpb;
		if (res.type == template_args) {
			for (const auto& c : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&c); p) {
					tpb.entries.emplace_back(*p);
				}
			}
		}
		return tpb;
	}
	default:
		return {};
	}
}

bool SubstitutionTable::check_if_allowed(const ParseResult& res) {
	if ((res.type == builtin_type) or (res.type == substitution) or (res.type == nested_name)) {
		return false;
	}

	if ((res.type == ProductionType::unscoped_template_name or res.type == ProductionType::type or res.type == ProductionType::prefix or res.type == name or res.type == class_enum_type or res.type == template_prefix) and res.comps.size() == 1) {
		if (res.comps.empty()) {
			return false;
		}
		if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p) {
			return check_if_allowed(*p);
		}
	}

	return true;
}

bool SubstitutionTable::add_if_allowed(const ParseResult& res, const NestedTemplateParams& tp) {
	
	if (not check_if_allowed(res)) {
		return false;
	}
	auto mang = format_mangled(res);
	for (auto& m : mangled) {
		if (m == mang) {
			return false;
		}
	}

	
	entries.emplace_back(res);
	mangled.emplace_back(mang);
	stp.emplace_back(tp);
	return true;
}

void build_substitution_types(const ParseResult& res, SubstitutionTable& subs, const NestedTemplateParams& tp);

void build_substitution_table(const ParseResult& res, SubstitutionTable& subs, const NestedTemplateParams& tp, bool primary) {
	
	switch (res.type) {
	case ProductionType::type: {
		if (not res.comps.empty()) {
			for (const auto& comp : res.comps) {
				if (const auto* p = std::get_if<ParseResult>(&comp); p) {
					build_substitution_table(*p, subs, tp, false);
				}
			}

			if (const auto* p = std::get_if<ParseResult>(res.comps.data()); not p or p->type != builtin_type) {
				subs.add_if_allowed(res, tp);
			}
		}
	} break;

	case swartalf::itanium::ProductionType::nested_name: {
		std::vector<ParseResult> comps;
		for (size_t i = 0; i < res.comps.size(); i++) {
			if (const auto* p = std::get_if<ParseResult>(&res.comps[i]); p) {
				if (p->type != cv_qualifiers and p->type != ref_qualifier and p->type != explicit_object_member) {
					comps.emplace_back(*p);
				}
			}
		}
		for (size_t i = 0; i < comps.size(); i++) {
			if (i == 0) {
				if (comps[i].type == prefix and comps[i].comps.size() == 1) {
					build_substitution_types(comps[i], subs, tp);
				}
				if (comps[i].type == prefix and comps[i].comps.size() == 2) {
					
					subs.add_if_allowed(*std::get_if<ParseResult>(comps[i].comps.data()), tp);
					build_substitution_types(*std::get_if<ParseResult>(&comps[i].comps[1]), subs, tp);
				}
				subs.add_if_allowed(comps[i], tp);

			} else {
				ParseResult p;
				p.type = partial_nested_name;
				for (size_t j = 0; j <= i; j++) {
					if (j == i) {
						if (comps[j].type != template_prefix) {
							if (comps[j].type == prefix and comps[j].comps.size() == 2) {
								if (const auto* q = std::get_if<ParseResult>(comps[j].comps.data()); q) {
									auto pp = p;

									pp.comps.emplace_back(*q);

									subs.add_if_allowed(pp, tp);
									if (const auto* r = std::get_if<ParseResult>(&comps[j].comps[1]); r) {
										build_substitution_table(*r, subs, tp, false);
									}
								}
							}
							if (comps[j].type == prefix and comps[j].comps.size() == 1) {
								build_substitution_types(comps[j], subs, tp);
							}
							if (comps[j].type == prefix and comps[j].comps.size() == 2) {
								build_substitution_types(*std::get_if<ParseResult>(comps[j].comps.data()), subs, tp);
								build_substitution_types(*std::get_if<ParseResult>(&comps[j].comps[1]), subs, tp);
							}
							if (comps[j].type == template_args) {
								build_substitution_table(comps[j], subs, tp, false);
							}
						}
					}
					

					p.comps.emplace_back(comps[j]);
				}
				if ((not primary) or i + 1 < comps.size()) {
					subs.add_if_allowed(p, tp);
				}
			}
		}

	} break;

	case name: {
		if (res.comps.size() == 2) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			const auto* q = std::get_if<ParseResult>(&res.comps[1]);
			if (p and q and p->type == unscoped_template_name and q->type == template_args) {
				subs.add_if_allowed(*p, tp);
				build_substitution_table(*q, subs, tp, false);
				if (not primary) {
					subs.add_if_allowed(res, tp);
				}
			}
		}
	}
		for (const auto& comp : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&comp); p) {
				build_substitution_table(*p, subs, tp, primary);
			}
		}
		break;
	case data_encoding:
	case encoding: {
		primary = true;

		auto ntt = tp;
		bool first = true;
		for (const auto& comp : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&comp); p) {
				if (first) {
					build_substitution_table(*p, subs, tp, primary && p->type != template_args);
					auto nt = collect_tp(*p);
					if (not nt.empty()) {
						ntt.add(std::move(nt));
					}
					first = false;
				} else {
					build_substitution_table(*p, subs, ntt, primary && p->type != template_args);
				}
			}
		}
	} break;
	default:

		for (const auto& comp : res.comps) {
			if (const auto* p = std::get_if<ParseResult>(&comp); p) {
				build_substitution_table(*p, subs, tp, primary && p->type != template_args);
			}
		}

		break;
	}
}

void build_substitution_types(const ParseResult& res, SubstitutionTable& subs, const NestedTemplateParams& tp) {
	if (res.type == ProductionType::type) {
		build_substitution_table(res, subs, tp, false);
		return;
	}

	for (const auto& comp : res.comps) {
		if (const auto* p = std::get_if<ParseResult>(&comp); p) {
			build_substitution_types(*p, subs, tp);
		}
	}
}

SubstitutionTable build_substitution_table(const ParseResult& res) {
	SubstitutionTable subs;
	NestedTemplateParams tp;
	build_substitution_table(res, subs, tp, true);
	return subs;
}
}