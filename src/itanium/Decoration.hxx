#pragma once
#include <cstdint>
namespace swartalf {

enum class Decoration : uint8_t {
	keyword,
	type_keyword,
	const_keyword,
	reference,
	rvalue_reference,
	pointer,
	generic_bold,
	generic_italic,
	curly_braces,
	template_brackets,
	call_parentheses,
	array_brackets,
	operator_symbol,
	function_name,
	call_parameter_even,
	call_parameter_odd,
	template_parameter_even,
	template_parameter_odd,
	number,
	string,
	noise
};
}