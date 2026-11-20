#include "FunctionClassification.hxx"
#include "ProductionType.hxx"
#include "ResolveSubstitution.hxx"
#include "SubstitutionTable.hxx"

namespace swartalf::itanium {

std::expected<FunctionClassification, ClassifyEncodingError> make_classify_error() {
	return std::unexpected(ClassifyEncodingError::error);
}

std::expected<FunctionClassification, ClassifyEncodingError> make_templated(std::expected<FunctionClassification, ClassifyEncodingError> untemplated) {
	if (not untemplated) {
		return std::unexpected(untemplated.error());
	}
	if (untemplated->rs == ReturnSignature::return_type_but_not_mangled) {
		untemplated->rs = ReturnSignature::mangled_return_type;
	}
	untemplated->is_templated = true;
	return untemplated;
}

std::expected<FunctionClassification, ClassifyEncodingError> classify_nested_name(const ParseResult& res, const SubstitutionTableView& subs) {
	for (size_t k = 0; k < res.comps.size(); k++) {
		size_t i = res.comps.size() - 1 - k;

		const auto* p = std::get_if<ParseResult>(&res.comps[i]);
		if (not p) {
			continue;
		}
		if (p->type == ProductionType::unqualified_name) {
			return classify_name(*p, subs);
		}
		if (p->type == ProductionType::prefix and p->comps.size() == 1) {
			const auto* q = std::get_if<ParseResult>(p->comps.data());
			return classify_name(*q, subs);
		}
		if (p->type == ProductionType::template_args) {
			if (k + 1 == res.comps.size()) {
				return make_classify_error();
			}
			size_t j = i - 1;
			const auto* q = std::get_if<ParseResult>(&res.comps[j]);
			if (not q or q->type != ProductionType::template_prefix or q->comps.size() != 1) {
				return make_classify_error();
			}

			const auto* r = std::get_if<ParseResult>(q->comps.data());
			if (not r) {
				return make_classify_error();
			}
			auto untemplated = classify_name(*r, subs);

			return make_templated(untemplated);
		}
	}
	return make_classify_error();
}

std::expected<FunctionClassification, ClassifyEncodingError> classify_name(const ParseResult& res, const SubstitutionTableView& subs) {
	switch (res.type) {
	case ProductionType::name: {
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		if (not p) {
			return make_classify_error();
		}
		if (res.comps.size() == 1) {
			return classify_name(*p, subs);
		}
		const auto* q = std::get_if<ParseResult>(&res.comps[1]);
		if (q and q->type == ProductionType::template_args and p->type == ProductionType::unscoped_template_name) {
			for (const auto& c : p->comps) {
				const auto* r = std::get_if<ParseResult>(&c);
				if (r) {
					auto untemplated = classify_name(*r, subs);
					return make_templated(untemplated);
				}
			}
		}
		return make_classify_error();
	}
	case ProductionType::unscoped_name: {
		if (res.comps.size() == 1) {
			const auto* p = std::get_if<ParseResult>(res.comps.data());
			if (not p) {
				return make_classify_error();
			}
			return classify_name(*p, subs);
		}
		if (res.comps.size() == 2) {
			const auto* p = std::get_if<ParseResult>(&res.comps[1]);
			if (not p) {
				return make_classify_error();
			}
			return classify_name(*p, subs);
		}

		return make_classify_error();
	}
	case ProductionType::nested_name:
	case ProductionType::partial_nested_name: {
		return classify_nested_name(res, subs);
	}
	case ProductionType::unary_operator_name:
	case ProductionType::binary_operator_name:
	case ProductionType::ternary_operator_name:
	case ProductionType::source_name: {
		return FunctionClassification(res.type, false, ReturnSignature::return_type_but_not_mangled);
	}
	case ProductionType::ctor_dtor_name:
	case ProductionType::conversion_operator_name: {
		return FunctionClassification(res.type, false, ReturnSignature::no_return_type);
	}
	case ProductionType::operator_name: {
		const auto* p = std::get_if<ParseResult>(res.comps.data());
		if (not p) {
			return FunctionClassification(res.type, false, ReturnSignature::return_type_but_not_mangled);
		}
		return classify_name(*p, subs);
	}

	case ProductionType::unqualified_name: {
		for (const auto& c : res.comps) {
			const auto* p = std::get_if<ParseResult>(&c);
			if (p) {
				return classify_name(*p, subs);
			}
		}

		return make_classify_error();
	}
	case ProductionType::local_name: {
		for (const auto& c : res.comps) {
			const auto* p = std::get_if<ParseResult>(&c);
			if (p and p->type == ProductionType::name) {
				return classify_name(*p, subs);
			}
		}

		return make_classify_error();
	}
	case ProductionType::substitution: {
		auto [r, s, tp] = resolve_substitution(res, subs);
		if (r) {
			return classify_name(*r, s);
		}
		return make_classify_error();
	}

	default:
		return make_classify_error();
	}
}

std::expected<FunctionClassification, ClassifyEncodingError> classify_encoding(const ParseResult& res, const SubstitutionTableView& subs) {
	if (res.comps.empty()) {
		return make_classify_error();
	}
	const auto* p = std::get_if<ParseResult>(res.comps.data());
	if (not p) {
		return make_classify_error();
	}
	return classify_name(*p, subs);
}

}
