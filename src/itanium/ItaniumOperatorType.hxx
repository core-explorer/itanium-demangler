#pragma once
#include <cstdint>
#include <string_view>
namespace swartalf {

enum class ItaniumOperatorType : uint8_t {
	none,
	nw, //!<  new
	na, //!< new[]
	dl, //!< delete
	da, //!< delete[]
	ps, //!< + (unary)
	ng, //!< - (unary)
	ad, //!< & (unary)
	de, //!< * (unary)
	co, //!< ~
	pl, //!< +
	mi, //!< -
	ml, //!< *
	dv, //!< /
	rm, //!< %
	an, //!< &
	or_, //!< |
	eo, //!< ^
	aS, //!< =
	pL, //!< +=
	mI, //!< -=
	mL, //!< *=
	dV, //!< /=
	rM, //!< %=
	aN, //!< &=
	oR, //!< |=
	eO, //!< ^=
	ls, //!< <<
	rs, //!< >>
	lS, //!< <<=
	rS, //!< >>=
	eq, //!< ==
	ne, //!< !=
	lt, //!< <
	gt, //!< >
	le, //!< <=
	ge, //!< >=
	ss, //!< <=>
	nt, //!< !
	aa, //!< &&
	oo, //!< ||
	pp, //!< ++ (postfix in <expression> context)
	mm, //!< -- (postfix in <expression> context)
	cm, //!< ,
	pm, //!< ->*
	pt, //!< ->
	cl, //!< ()
	ix, //!< []
	qu, //!< ?
	li, //!< operator ""
	cv, //!< (cast)
	aw, //!< co_await
	v, //!< vendor extended operator
	number_of_options
};

enum class ItaniumOperatorArity : uint8_t {
	no_arguments,
	unary,
	binary,
	ternary,
	variable,
	type_plus_unary,
	literal,
	vendor_extended,
	invalid = 0xff
};

constexpr ItaniumOperatorArity get_operator_arity(ItaniumOperatorType op) {
	switch (op) {
	case ItaniumOperatorType::nw:
	case ItaniumOperatorType::na:
	case ItaniumOperatorType::dl:
	case ItaniumOperatorType::da:
	case ItaniumOperatorType::ps:
	case ItaniumOperatorType::ng:
	case ItaniumOperatorType::ad:
	case ItaniumOperatorType::de:
	case ItaniumOperatorType::co:
		return ItaniumOperatorArity::unary;
	case ItaniumOperatorType::pl:
	case ItaniumOperatorType::mi:
	case ItaniumOperatorType::ml:
	case ItaniumOperatorType::dv:
	case ItaniumOperatorType::rm:
	case ItaniumOperatorType::an:
	case ItaniumOperatorType::or_:
	case ItaniumOperatorType::eo:
	case ItaniumOperatorType::aS:
	case ItaniumOperatorType::pL:
	case ItaniumOperatorType::mI:
	case ItaniumOperatorType::mL:
	case ItaniumOperatorType::dV:
	case ItaniumOperatorType::rM:
	case ItaniumOperatorType::aN:
	case ItaniumOperatorType::oR:
	case ItaniumOperatorType::eO:
	case ItaniumOperatorType::ls:
	case ItaniumOperatorType::rs:
	case ItaniumOperatorType::lS:
	case ItaniumOperatorType::rS:
	case ItaniumOperatorType::eq:
	case ItaniumOperatorType::ne:
	case ItaniumOperatorType::lt:
	case ItaniumOperatorType::gt:
	case ItaniumOperatorType::le:
	case ItaniumOperatorType::ge:
	case ItaniumOperatorType::ss:
	case ItaniumOperatorType::nt:
	case ItaniumOperatorType::aa:
	case ItaniumOperatorType::oo:
		return ItaniumOperatorArity::binary;
	case ItaniumOperatorType::pp:
		return ItaniumOperatorArity::unary;
	case ItaniumOperatorType::mm:
		return ItaniumOperatorArity::unary;
	case ItaniumOperatorType::cm:
		return ItaniumOperatorArity::binary;
	case ItaniumOperatorType::pm:
		return ItaniumOperatorArity::binary;
	case ItaniumOperatorType::pt:
		return ItaniumOperatorArity::binary;
	case ItaniumOperatorType::cl:
		return ItaniumOperatorArity::variable;
	case ItaniumOperatorType::ix:
		return ItaniumOperatorArity::variable;
	case ItaniumOperatorType::qu:
		return ItaniumOperatorArity::ternary;
	case ItaniumOperatorType::li:
		return ItaniumOperatorArity::literal;
	case ItaniumOperatorType::cv:
		return ItaniumOperatorArity::type_plus_unary;
	case ItaniumOperatorType::aw:
		return ItaniumOperatorArity::unary;
	case ItaniumOperatorType::v:
		return ItaniumOperatorArity::vendor_extended;
	default:
		return ItaniumOperatorArity::invalid;
	}
}

constexpr std::string_view to_mangled_name(ItaniumOperatorType op) {
	switch (op) {
	case ItaniumOperatorType::nw:
		return "nw";
	case ItaniumOperatorType::na:
		return "na";
	case ItaniumOperatorType::dl:
		return "dl";
	case ItaniumOperatorType::da:
		return "da";
	case ItaniumOperatorType::ps:
		return "ps";
	case ItaniumOperatorType::ng:
		return "ng";
	case ItaniumOperatorType::ad:
		return "ad";
	case ItaniumOperatorType::de:
		return "de";
	case ItaniumOperatorType::co:
		return "co";
	case ItaniumOperatorType::pl:
		return "pl";
	case ItaniumOperatorType::mi:
		return "mi";
	case ItaniumOperatorType::ml:
		return "ml";
	case ItaniumOperatorType::dv:
		return "dv";
	case ItaniumOperatorType::rm:
		return "rm";
	case ItaniumOperatorType::an:
		return "an";
	case ItaniumOperatorType::or_:
		return "or";
	case ItaniumOperatorType::eo:
		return "eo";
	case ItaniumOperatorType::aS:
		return "aS";
	case ItaniumOperatorType::pL:
		return "pL";
	case ItaniumOperatorType::mI:
		return "mI";
	case ItaniumOperatorType::mL:
		return "mL";
	case ItaniumOperatorType::dV:
		return "dV";
	case ItaniumOperatorType::rM:
		return "rM";
	case ItaniumOperatorType::aN:
		return "aN";
	case ItaniumOperatorType::oR:
		return "oR";
	case ItaniumOperatorType::eO:
		return "eO";
	case ItaniumOperatorType::ls:
		return "ls";
	case ItaniumOperatorType::rs:
		return "rs";
	case ItaniumOperatorType::lS:
		return "lS";
	case ItaniumOperatorType::rS:
		return "rS";
	case ItaniumOperatorType::eq:
		return "eq";
	case ItaniumOperatorType::ne:
		return "ne";
	case ItaniumOperatorType::lt:
		return "lt";
	case ItaniumOperatorType::gt:
		return "gt";
	case ItaniumOperatorType::le:
		return "le";
	case ItaniumOperatorType::ge:
		return "ge";
	case ItaniumOperatorType::ss:
		return "ss";
	case ItaniumOperatorType::nt:
		return "nt";
	case ItaniumOperatorType::aa:
		return "aa";
	case ItaniumOperatorType::oo:
		return "oo";
	case ItaniumOperatorType::pp:
		return "pp";
	case ItaniumOperatorType::mm:
		return "mm";
	case ItaniumOperatorType::cm:
		return "cm";
	case ItaniumOperatorType::pm:
		return "pm";
	case ItaniumOperatorType::pt:
		return "pt";
	case ItaniumOperatorType::cl:
		return "cl";
	case ItaniumOperatorType::ix:
		return "ix";
	case ItaniumOperatorType::qu:
		return "qu";
	case ItaniumOperatorType::li:
		return "li";
	case ItaniumOperatorType::cv:
		return "cv";
	case ItaniumOperatorType::aw:
		return "aw";

	case ItaniumOperatorType::v:
		return "v";
	default:
		return "?";
	}
}

constexpr bool is_named_operator(ItaniumOperatorType op) {
	switch (op) {
	case ItaniumOperatorType::nw:

	case ItaniumOperatorType::na:

	case ItaniumOperatorType::dl:

	case ItaniumOperatorType::da:
		return true;
	default:
		return false;
	}
}

constexpr std::string_view to_demangled_operator_name(ItaniumOperatorType op) {
	switch (op) {
	case ItaniumOperatorType::nw:
		return "new";
	case ItaniumOperatorType::na:
		return "new[]";
	case ItaniumOperatorType::dl:
		return "delete";
	case ItaniumOperatorType::da:
		return "delete[]";
	case ItaniumOperatorType::ps:
		return "+";
	case ItaniumOperatorType::ng:
		return "-";
	case ItaniumOperatorType::ad:
		return "&";
	case ItaniumOperatorType::de:
		return "*";
	case ItaniumOperatorType::co:
		return "~";
	case ItaniumOperatorType::pl:
		return "+";
	case ItaniumOperatorType::mi:
		return "-";
	case ItaniumOperatorType::ml:
		return "*";
	case ItaniumOperatorType::dv:
		return "/";
	case ItaniumOperatorType::rm:
		return "%";
	case ItaniumOperatorType::an:
		return "&";
	case ItaniumOperatorType::or_:
		return "|";
	case ItaniumOperatorType::eo:
		return "^";
	case ItaniumOperatorType::aS:
		return "=";
	case ItaniumOperatorType::pL:
		return "+=";
	case ItaniumOperatorType::mI:
		return "-=";
	case ItaniumOperatorType::mL:
		return "*=";
	case ItaniumOperatorType::dV:
		return "/=";
	case ItaniumOperatorType::rM:
		return "%=";
	case ItaniumOperatorType::aN:
		return "&=";
	case ItaniumOperatorType::oR:
		return "|=";
	case ItaniumOperatorType::eO:
		return "^=";
	case ItaniumOperatorType::ls:
		return "<<";
	case ItaniumOperatorType::rs:
		return ">>";
	case ItaniumOperatorType::lS:
		return "<<=";
	case ItaniumOperatorType::rS:
		return ">>=";
	case ItaniumOperatorType::eq:
		return "==";
	case ItaniumOperatorType::ne:
		return "!=";
	case ItaniumOperatorType::lt:
		return "<";
	case ItaniumOperatorType::gt:
		return ">";
	case ItaniumOperatorType::le:
		return "<=";
	case ItaniumOperatorType::ge:
		return ">=";
	case ItaniumOperatorType::ss:
		return "<=>";
	case ItaniumOperatorType::nt:
		return "!";
	case ItaniumOperatorType::aa:
		return "&&";
	case ItaniumOperatorType::oo:
		return "||";
	case ItaniumOperatorType::pp:
		return "++";
	case ItaniumOperatorType::mm:
		return "--";
	case ItaniumOperatorType::cm:
		return ",";
	case ItaniumOperatorType::pm:
		return "->*";
	case ItaniumOperatorType::pt:
		return "->";
	case ItaniumOperatorType::cl:
		return "()";
	case ItaniumOperatorType::ix:
		return "[]";
	case ItaniumOperatorType::qu:
		return "?";
	case ItaniumOperatorType::li:
		return "\"\"";
	case ItaniumOperatorType::cv:
		return "cv";
	case ItaniumOperatorType::aw:
		return "co_await";
	case ItaniumOperatorType::v:
		return "v";
	default:
		return "?";
	}
}

}