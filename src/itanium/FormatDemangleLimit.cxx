#include "FormatDemangleLimit.hxx"
#include "FormatDemangle.hxx"
#include "FunctionClassification.hxx"
#include "ParseResult.hxx"
#include "ProductionType.hxx"

namespace swartalf::itanium {

unsigned count_template_args(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, RecursionLimit max_recursion) {
	unsigned cnt = 0;
	if (max_recursion == 0) {
		return cnt;
	}
	--max_recursion;
	if (res.type == ProductionType::template_args or res.type == ProductionType::template_arg_pack) {
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				if (x->type == ProductionType::template_arg) {
					if (const auto* y = std::get_if<ParseResult>(x->comps.data()); y) {
						x = y;
					}
				}

				if (x->type == ProductionType::template_arg_pack) {
					cnt += count_template_args(*x, subs, tp, max_recursion);
				} else if (x->type == ProductionType::pack_type or x->type == ProductionType::template_param) {
					auto ops = get_pack_size(*x, subs, tp, max_recursion);
					if (ops) {
						cnt += *ops;
					} else {
						++cnt;
					}

				} else {
					++cnt;
				}
			}
		}
	}

	return cnt;
}

std::pair<unsigned, unsigned> get_max_num_function_and_template_parameters(const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, RecursionLimit max_recursion) {
	std::pair<unsigned, unsigned> ftp = {0, 0};
	if (res.type == ProductionType::encoding) {
		auto efc = classify_encoding(res);
		if (not efc) {
			return {0, 0};
		}
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		auto nt = collect_tp(*p);
		auto ntt = tp;
		if (not nt.empty()) {
			ntt.add(std::move(nt));
		}
		auto fc = *efc;
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);
		const auto* r = res.comps.size() >= 3 ? std::get_if<ParseResult>(&res.comps[2]) : nullptr;
		if (not q or q->type != ProductionType::bare_function_type or q->comps.empty()) {
			return {0, 0};
		}
		size_t start_idx = fc.rs == ReturnSignature::mangled_return_type;
		{
			unsigned cnt = 0;
			for (size_t i = start_idx; i < q->comps.size(); ++i) {
				const auto* x = std::get_if<ParseResult>(&q->comps[i]);
				if (x->type == ProductionType::type) {
					if (const auto* y = std::get_if<ParseResult>(x->comps.data()); y) {
						x = y;
					}
				}
				if (x->type == ProductionType::pack_type) {
					auto ops = get_pack_size(*x, subs, ntt, max_recursion);
					if (ops) {
						cnt += *ops;
					} else {
						cnt++;
					}
				} else {
					cnt++;
				}
			}
			ftp.first = std::max(ftp.first, cnt);
		}

		{
			auto tmp = get_max_num_function_and_template_parameters(*p, subs, tp, max_recursion);
			ftp.first = std::max(ftp.first, tmp.first);
			ftp.second = std::max(ftp.second, tmp.second);
		}
		if (r) {
			auto tmp = get_max_num_function_and_template_parameters(*r, subs, ntt, max_recursion);
			ftp.first = std::max(ftp.first, tmp.first);
			ftp.second = std::max(ftp.second, tmp.second);
		}

		for (const auto& c : q->comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				auto tmp = get_max_num_function_and_template_parameters(*x, subs, ntt, max_recursion);
				ftp.first = std::max(ftp.first, tmp.first);
				ftp.second = std::max(ftp.second, tmp.second);
			}
		}
		return ftp;
	}
	if (res.type == ProductionType::template_args) {
		unsigned cnt = count_template_args(res, subs, tp, max_recursion);
		ftp.second = std::max(ftp.second, cnt);
	}

	if (res.type == ProductionType::bare_function_type or res.type == ProductionType::lambda_sig) {
		unsigned cnt = 0;
		for (const auto& c : res.comps) {
			if (const auto* x = std::get_if<ParseResult>(&c); x) {
				cnt++;
			}
		}
		ftp.first = std::max(ftp.first, cnt);
	}

	for (const auto& c : res.comps) {
		if (const auto* x = std::get_if<ParseResult>(&c); x) {
			auto tmp = get_max_num_function_and_template_parameters(*x, subs, tp, max_recursion);
			ftp.first = std::max(ftp.first, tmp.first);
			ftp.second = std::max(ftp.second, tmp.second);
		}
	}
	return ftp;
}

std::expected<std::pair<std::string, DemanglingDialect>, ParseError> format_demangled_ascii_limit(unsigned len, const ParseResult& res, const SubstitutionTableView& subs, const NestedTemplateParams& tp, const DemanglingDialect& dv, const SkipSpec& skip, RecursionLimit max_recursion) {
	auto etmp = format_demangled_ascii(res, subs, tp, dv, skip, max_recursion);
	if (not etmp) {
		return std::unexpected(etmp.error());
	}
	{
		auto& tmp = *etmp;
		if (tmp.size() <= len) {
			return std::make_pair(tmp, dv);
		}
	}
	auto ndv = dv;
	ndv.return_type_style = ReturnTypeStyle::no_return;
	ndv.hide_abi_suffix = true;
	ndv.hide_vendor_suffix = true;
	ndv.whitespace_operator_expressions = false;
	ndv.parenthesize_expressions = false;

	if (ndv != dv) {
		etmp = format_demangled_ascii(res, subs, tp, ndv, skip, max_recursion);
		if (not etmp) {
			return std::unexpected(etmp.error());
		}
	}
	{
		auto& tmp = *etmp;
		if (tmp.size() <= len) {
			return std::make_pair(tmp, ndv);
		}
	}

	bool tp_mode = true;
	while (ndv.max_template_parameters or ndv.max_function_parameters) {
		if (ndv.max_template_parameters >= ndv.max_function_parameters) {
			if (tp_mode) {
				ndv.max_template_parameters--;
				tp_mode = false;
			} else {
				ndv.nested_template_parameter_reduction++;
				tp_mode = true;
			}

		} else {
			ndv.max_function_parameters--;
		}
		etmp = format_demangled_ascii(res, subs, tp, ndv, skip, max_recursion);
		if (not etmp) {
			return std::unexpected(etmp.error());
		}

		{
			auto& tmp = *etmp;
			if (tmp.size() <= len) {
				return std::make_pair(tmp, ndv);
			}
		}
	}

	{
		auto& tmp = *etmp;
		return std::make_pair(tmp, ndv);
	}
}
}