#include "ResolveSubstitution.hxx"
#include "Base36.hxx"
#include "ProductionType.hxx"
#include "../util/string.hxx"
namespace swartalf::itanium {
using enum ProductionType;

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

}