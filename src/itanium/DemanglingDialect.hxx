#pragma once
#include <cstdint>
namespace swartalf {

enum class ConstOrientation : uint8_t {
	none,
	east_const,
	west_const,
};

enum class UnsignedOrientation : uint8_t {
	none,
	east_unsigned,
	central_unsigned,
	west_unsigned,
};

enum class ModifierSpacing : uint8_t {
	none,
	near,
	far,
};

enum class LiteralDecoration : uint8_t {
	none,
	noncaps,
	caps,
};

enum class FundamentalName : uint8_t {
	none,
	explicit_int,
	unsigned_int,
	implicit_int,
};

enum class CompilerDialect : uint8_t {
	none,
	gcc,
	clang
};

enum class ImitationTarget : uint8_t {
	none,
	demangler,
	debuginfo
};

enum class NullptrType : uint8_t {
	none,
	std_nullptr,
	decltype_nullptr
};

enum class BooleanType : uint8_t {
	none,
	cpp_bool,
	c_bool
};

enum class BFloat16Type : uint8_t {
	none,
	bf16,
	std_bfloat16
};

enum class FloatTemplateConstantStyle : uint8_t {
	none,
	printf_style,
	raw_byte_style,
	hexfloat_style,
	javascript_style
};

enum class InfinityStyle : uint8_t {
	none,
	infinity,
	infty,
	inf,
	unicode
};

enum class NaNStyle : uint8_t {
	none,
	q_all_caps,
	q_small_caps,
	q_mixed_caps,
	all_caps,
	small_caps,
	mixed_caps,
	raw_bytes,
	unicode

};

enum class ComplexOrientation : uint8_t {
	none,
	east_complex,
	west_complex
};

enum class ComplexKeyword : uint8_t {
	no_underscore,
	one_underscore,
	two_underscore
};

enum class RestrictKeyword : uint8_t {
	no_underscore,
	one_underscore,
	two_underscore
};

enum class TargetLongDouble16Byte : uint8_t {
	none,
	extended80,
	binary128
};

enum class ReturnTypeStyle : uint8_t {
	no_return,
	leading_return,
	trailing_return,
	trailing_return_leading_auto
};

struct DemanglingDialect {
	unsigned max_function_parameters = 100;
	unsigned max_template_parameters = 100;
	unsigned nested_function_parameter_reduction = 0;
	unsigned nested_template_parameter_reduction = 0;
	ConstOrientation const_orientation = ConstOrientation::east_const;
	CompilerDialect compiler_dialect = CompilerDialect::gcc;
	ImitationTarget imitation_target = ImitationTarget::demangler;
	ModifierSpacing modifier_spacing = ModifierSpacing::near;
	LiteralDecoration literal_decoration = LiteralDecoration::none;
	FundamentalName fundamental_name = FundamentalName::implicit_int;
	UnsignedOrientation unsigned_orientation = UnsignedOrientation::west_unsigned;
	NullptrType nullptr_type = NullptrType::decltype_nullptr;
	ComplexOrientation complex_orientation = ComplexOrientation::east_complex;
	ComplexKeyword complex_keyword = ComplexKeyword::one_underscore;
	RestrictKeyword restrict_keyword = RestrictKeyword::no_underscore;
	FloatTemplateConstantStyle float_template_constant_style = FloatTemplateConstantStyle::raw_byte_style;
	NaNStyle nan_style = NaNStyle::mixed_caps;
	InfinityStyle infinity_style = InfinityStyle::infinity;
	bool hide_abi_suffix = false;
	bool separating_whitespace = false;
	bool resolve_template_args = true;
	bool resolve_packs = true;
	bool resolve_substitutions = true;
	bool resolve_std_typedefs = false;
	bool display_expressions = true;
	bool parenthesize_expressions = true;
	bool whitespace_operator_expressions = false;
	bool short_constants = false;
	ReturnTypeStyle return_type_style = ReturnTypeStyle::leading_return;
	bool hide_vendor_suffix = false;
	bool resolve_function_signatures = true;
	bool relative_names = false;
	friend bool operator==(const DemanglingDialect& a, const DemanglingDialect& b) = default;
};

}