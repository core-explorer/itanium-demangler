#pragma once

#include "ParseName.hxx"

namespace swartalf::itanium {
ParseResult drop_literals(const ParseResult& res);

struct TemplateParams {
	std::vector<ParseResult> entries;
	constexpr bool empty() const { return entries.empty(); }
	constexpr size_t size() const { return entries.size(); }
};
std::string to_string(const TemplateParams& tp);
struct NestedTemplateParams {
	constexpr bool empty() const { return entries.empty(); }
	constexpr size_t size() const { return entries.size(); }
	const TemplateParams& back() const { return entries.back(); }
	const TemplateParams& from_back(size_t idx) const { return entries[entries.size() - 1 - idx]; }

	void add(TemplateParams&& tp) {
		entries.emplace_back(std::move(tp));
	}

private:
	std::vector<TemplateParams> entries;
};

struct SubstitutionTable {
	std::vector<ParseResult> entries;
	std::vector<std::string> mangled;
	std::vector<NestedTemplateParams> stp;

	static bool check_if_allowed(const ParseResult& res);
	bool add_if_allowed(const ParseResult& res, const NestedTemplateParams& tp);
};

struct SubstitutionTableView {
	std::span<const ParseResult> entries;
	std::span<const std::string> mangled;
	std::span<const NestedTemplateParams> stp;

	template <typename... Args>
	SubstitutionTableView subspan(Args&&... args) const {
		SubstitutionTableView res;
		res.entries = entries.subspan(std::forward<Args>(args)...);
		res.mangled = mangled.subspan(std::forward<Args>(args)...);
		res.stp = stp.subspan(std::forward<Args>(args)...);
		return res;
	}
};

inline SubstitutionTableView view(const SubstitutionTable& su) {
	SubstitutionTableView res;
	res.entries = su.entries;
	res.mangled = su.mangled;
	res.stp = su.stp;
	return res;
}

TemplateParams collect_tp(const ParseResult& res);
SubstitutionTable build_substitution_table(const ParseResult& res);

}