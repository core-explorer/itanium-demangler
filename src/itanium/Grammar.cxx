#include "Grammar.hxx"
#include "OptimizedGrammar.hxx"
#include "ProductionType.hxx"
#include "../util/string.hxx"
namespace swartalf::itanium {

namespace {
using enum ProductionType;

void add_rules(ProductionGrammar& rules) {
	rules.add_rule<dot_vendor_specific_suffix>(".", vendor_specfic_suffix);
	rules.add_rule<mangled_name>("_Z", encoding, opt(dot_vendor_specific_suffix));
	rules.add_rule<mangled_name>("_Z", data_encoding);
	rules.add_rule<mangled_name_apple>("_", mangled_name);
	rules.add_rule<mangled_name_apple>("_", global_sub_for_mangled_name);
	rules.add_rule<global_sub_for_mangled_name>("_GLOBAL__sub_I_", mangled_name);
	rules.add_rule<any_mangled_name>(mangled_name);
	rules.add_rule<any_mangled_name>(mangled_name_apple);
	rules.add_rule<any_mangled_name>(global_sub_for_mangled_name);

	rules.add_rule<encoding>(name, bare_function_type, opt(requires_clause));

	rules.add_rule<data_encoding>(name);
	rules.add_rule<data_encoding>(special_object);
	rules.add_rule<requires_clause>("Q", expression);
}

void add_name_rules(ProductionGrammar& rules) {
	rules.add_rule<number>(nonneg_number);
	rules.add_rule<number>(negative_number);
	rules.add_rule<negative_number>("n", nonneg_number);
	rules.add_rule<name>(nested_name);

	rules.add_rule<name>(unscoped_template_name, template_args);
	rules.add_rule<name>(unscoped_name);

	rules.add_rule<name>(local_name);
	rules.add_rule<unscoped_name>(unqualified_name);
	rules.add_rule<unscoped_name>("St", unqualified_name);
	rules.add_rule<unscoped_template_name>(unscoped_name);
	rules.add_rule<unscoped_template_name>(substitution);

	rules.add_rule<nested_name>("N", opt(explicit_object_member), opt(cv_qualifiers), opt(ref_qualifier), rep(prefix), unqualified_name, "E");
	rules.add_rule<nested_name>("N", opt(explicit_object_member), opt(cv_qualifiers), opt(ref_qualifier), template_prefix, template_args, "E");

	rules.add_rule<explicit_object_member>("H");

	rules.add_rule<prefix>(closure_prefix);

	rules.add_rule<prefix>(unqualified_name);
	rules.add_rule<prefix>(template_param);
	rules.add_rule<prefix>(decltype_);
	rules.add_rule<prefix>(substitution);

	rules.add_rule<template_prefix>(unqualified_name);
	rules.add_rule<template_prefix>(rep(prefix), unqualified_name);
	rules.add_rule<template_prefix>(template_param);
	rules.add_rule<template_prefix>(substitution);

	rules.add_rule<unqualified_name>(source_name, opt(abi_tags));

	rules.add_rule<unqualified_name>("L", mangled_name, "E");
	rules.add_rule<unqualified_name>("L", source_name);

	rules.add_rule<unqualified_name>(operator_name, opt(abi_tags));
	rules.add_rule<unqualified_name>(ctor_dtor_name, opt(abi_tags));
	rules.add_rule<unqualified_name>(unnamed_type_name, opt(abi_tags));
	rules.add_rule<unqualified_name>("DC", rep(source_name), "E", opt(abi_tags));
	rules.add_rule<source_name>(nonneg_number, identifier);

	rules.add_rule<abi_tags>(rep(abi_tag));
	rules.add_rule<abi_tag>("B", source_name);
}

void add_operator_rules(ProductionGrammar& rules) {
	using enum ItaniumOperatorType;
	rules.add_rule<operator_name>(unary_operator_name);
	rules.add_rule<operator_name>(binary_operator_name);
	rules.add_rule<operator_name>(ternary_operator_name);

	rules.add_rule<operator_name>(nw);
	rules.add_rule<operator_name>(na);
	rules.add_rule<operator_name>(dl);
	rules.add_rule<operator_name>(da);
	rules.add_rule<operator_name>(aw);
	rules.add_rule<unary_operator_name>(ps);
	rules.add_rule<unary_operator_name>(ng);
	rules.add_rule<unary_operator_name>(ad);
	rules.add_rule<unary_operator_name>(de);
	rules.add_rule<unary_operator_name>(co);
	rules.add_rule<binary_operator_name>(pl);
	rules.add_rule<binary_operator_name>(mi);
	rules.add_rule<binary_operator_name>(ml);
	rules.add_rule<binary_operator_name>(dv);
	rules.add_rule<binary_operator_name>(rm);
	rules.add_rule<binary_operator_name>(an);
	rules.add_rule<binary_operator_name>(or_);
	rules.add_rule<binary_operator_name>(eo);
	rules.add_rule<binary_operator_name>(aS);
	rules.add_rule<binary_operator_name>(pL);
	rules.add_rule<binary_operator_name>(mI);
	rules.add_rule<binary_operator_name>(mL);
	rules.add_rule<binary_operator_name>(dV);
	rules.add_rule<binary_operator_name>(rM);
	rules.add_rule<binary_operator_name>(aN);
	rules.add_rule<binary_operator_name>(oR);
	rules.add_rule<binary_operator_name>(eO);
	rules.add_rule<binary_operator_name>(ls);
	rules.add_rule<binary_operator_name>(rs);
	rules.add_rule<binary_operator_name>(lS);
	rules.add_rule<binary_operator_name>(rS);
	rules.add_rule<binary_operator_name>(eq);
	rules.add_rule<binary_operator_name>(ne);
	rules.add_rule<binary_operator_name>(lt);
	rules.add_rule<binary_operator_name>(gt);
	rules.add_rule<binary_operator_name>(le);
	rules.add_rule<binary_operator_name>(ge);
	rules.add_rule<binary_operator_name>(ss);
	rules.add_rule<unary_operator_name>(nt);
	rules.add_rule<binary_operator_name>(aa);
	rules.add_rule<binary_operator_name>(oo);
	rules.add_rule<unary_operator_name>(pp);
	rules.add_rule<unary_operator_name>(mm);
	rules.add_rule<binary_operator_name>(cm);
	rules.add_rule<binary_operator_name>(pm);
	rules.add_rule<binary_operator_name>(pt);
	rules.add_rule<operator_name>(cl);
	rules.add_rule<operator_name>(ix);
	rules.add_rule<ternary_operator_name>(qu);
	rules.add_rule<operator_name>(conversion_operator_name);
	rules.add_rule<conversion_operator_name>(cv, type);
	rules.add_rule<operator_name>(li, source_name);
	rules.add_rule<operator_name>(v, digit, source_name);
}

void add_special_rules(ProductionGrammar& rules) {
	rules.add_rule<vtable>("TV", type);
	rules.add_rule<vtt>("TT", type);
	rules.add_rule<typeinfo>("TI", type);
	rules.add_rule<typeinfo_name_object>("TS", type);
	rules.add_rule<construction_vtable>("TC", type, nonneg_number, "_", type);

	rules.add_rule<special_object>(vtable);
	rules.add_rule<special_object>(vtt);
	rules.add_rule<special_object>(typeinfo);
	rules.add_rule<special_object>(typeinfo_name_object);
	rules.add_rule<special_object>(construction_vtable);
	rules.add_rule<special_object>(nonvirtual_thunk);
	rules.add_rule<special_object>(virtual_thunk);
	rules.add_rule<special_object>(virtual_covariant_thunk);

	rules.add_rule<nonvirtual_thunk>("Th", nv_offset, "_", encoding);
	rules.add_rule<virtual_thunk>("Tv", v_offset, "_", encoding);
	rules.add_rule<call_offset>("h", nv_offset, "_");
	rules.add_rule<call_offset>("v", v_offset, "_");
	rules.add_rule<nv_offset>(number);
	rules.add_rule<v_offset>(number, "_", number);
	rules.add_rule<virtual_covariant_thunk>("Tc", call_offset, call_offset, encoding);
}

void add_ctor_rules(ProductionGrammar& rules) {
	rules.add_rule<ctor_dtor_name>("C1");
	rules.add_rule<ctor_dtor_name>("C2");
	rules.add_rule<ctor_dtor_name>("C3");
	rules.add_rule<ctor_dtor_name>("C4");
	rules.add_rule<ctor_dtor_name>("CI1", type);
	rules.add_rule<ctor_dtor_name>("CI2", type);
	rules.add_rule<ctor_dtor_name>("D0");
	rules.add_rule<ctor_dtor_name>("D1");
	rules.add_rule<ctor_dtor_name>("D2");
	rules.add_rule<ctor_dtor_name>("D4");
}

void add_guard_rules(ProductionGrammar& rules) {
	rules.add_rule<special_object>(guard_variable);
	rules.add_rule<special_object>(lifetime_extended_temporary);
	rules.add_rule<guard_variable>("GV", name);
	rules.add_rule<lifetime_extended_temporary>("GR", name, opt(seq_id), "_");
}

void add_type_rules(ProductionGrammar& rules) {
	rules.add_rule<type>(builtin_type);
	rules.add_rule<type>(qualified_type);
	rules.add_rule<type>(function_type);
	rules.add_rule<type>(class_enum_type);
	rules.add_rule<type>(array_type);
	rules.add_rule<type>(vector_type);
	rules.add_rule<type>(pointer_to_member_type);
	rules.add_rule<type>(template_param);
	rules.add_rule<type>(template_template_type);
	rules.add_rule<type>(decltype_);
	rules.add_rule<type>(pointer_type);
	rules.add_rule<type>(reference_type);
	rules.add_rule<type>(rvalue_reference_type);
	rules.add_rule<type>(complex_type_c99);
	rules.add_rule<type>(pack_indexing_type);
	rules.add_rule<pack_indexing_type>("Dy", type, expression);
	rules.add_rule<pack_indexing_type>("Dy", template_arg_pack, expression);

	rules.add_rule<type>(substitution);
	rules.add_rule<type>(constrained_type_llvm);
	rules.add_rule<pointer_type>("P", type);
	rules.add_rule<reference_type>("R", type);
	rules.add_rule<rvalue_reference_type>("O", type);
	rules.add_rule<complex_type_c99>("C", type);

	rules.add_rule<template_template_type>(template_template_param, template_args);
	rules.add_rule<type_constraint_llvm>(name);
	rules.add_rule<constrained_type_llvm>("Dk", type_constraint_llvm);
	rules.add_rule<constrained_type_llvm>("DK", type_constraint_llvm);

	rules.add_rule<qualified_type>(const_type);
	rules.add_rule<qualified_type>(volatile_type);
	rules.add_rule<qualified_type>(restrict_type);
	rules.add_rule<qualified_type>(vendor_ext_arg_type_llm);
	rules.add_rule<qualified_type>(extended_qualifier);
	rules.add_rule<extended_qualifier>("U", source_name, type);
	rules.add_rule<vendor_ext_arg_type_llm>("Ua", source_name, template_args, type);
	rules.add_rule<const_type>("K", type);
	rules.add_rule<volatile_type>("V", type);
	rules.add_rule<restrict_type>("r", type);

	rules.add_rule<cv_qualifiers>("KV");
	rules.add_rule<cv_qualifiers>("K");
	rules.add_rule<cv_qualifiers>("V");

	rules.add_rule<ref_qualifier>("R");
	rules.add_rule<ref_qualifier>("O");
}

void add_builtin_type_rules(ProductionGrammar& rules) {
	rules.add_rule<builtin_type>(builtin_real_type);
	rules.add_rule<builtin_type>(builtin_integral_type);
	rules.add_rule<builtin_type>(builtin_boolean_type);
	rules.add_rule<builtin_type>(builtin_other_type);

	rules.add_rule<builtin_other_type>("v");
	rules.add_rule<builtin_integral_type>("w");
	rules.add_rule<builtin_boolean_type>("b");
	rules.add_rule<builtin_integral_type>("c");
	rules.add_rule<builtin_integral_type>("a");
	rules.add_rule<builtin_integral_type>("h");
	rules.add_rule<builtin_integral_type>("s");
	rules.add_rule<builtin_integral_type>("t");
	rules.add_rule<builtin_integral_type>("i");
	rules.add_rule<builtin_integral_type>("j");
	rules.add_rule<builtin_integral_type>("l");
	rules.add_rule<builtin_integral_type>("m");
	rules.add_rule<builtin_integral_type>("x");
	rules.add_rule<builtin_integral_type>("y");
	rules.add_rule<builtin_integral_type>("n");
	rules.add_rule<builtin_integral_type>("o");
	rules.add_rule<builtin_real_type>("f");
	rules.add_rule<builtin_real_type>("d");
	rules.add_rule<builtin_real_type>("e");
	rules.add_rule<builtin_real_type>("g");
	rules.add_rule<builtin_other_type>("z");
	rules.add_rule<builtin_real_type>("DF16b");

	rules.add_rule<builtin_real_type>("DF16_");
	rules.add_rule<builtin_real_type>("DF32_");
	rules.add_rule<builtin_real_type>("DF64_");
	rules.add_rule<builtin_real_type>("DF128_");

	rules.add_rule<builtin_integral_type>("Di");
	rules.add_rule<builtin_integral_type>("Ds");
	rules.add_rule<builtin_integral_type>("Du");
	rules.add_rule<builtin_other_type>("Da");
	rules.add_rule<builtin_other_type>("Dc");
	rules.add_rule<builtin_other_type>("Dn");
}

void add_function_type_rules(ProductionGrammar& rules) {
	rules.add_rule<function_type>(opt(cv_qualifiers), opt(exception_spec), opt("Dx"), "F", opt("Y"), bare_function_type, opt(ref_qualifier), "E");
	rules.add_rule<bare_function_type>(rep(type));
	rules.add_rule<exception_spec>("Do");
	rules.add_rule<exception_spec>("DO", expression, "E");
	rules.add_rule<exception_spec>("Dw", rep(type), "E");
	rules.add_rule<type>(pack_type);
	rules.add_rule<pack_type>("Dp", type);
}

void add_decltype_rules(ProductionGrammar& rules) {
	rules.add_rule<decltype_>("Dt", expression, "E");
	rules.add_rule<decltype_>("DT", expression, "E");
}

void add_class_rules(ProductionGrammar& rules) {
	rules.add_rule<class_enum_type>(name);
	rules.add_rule<class_enum_type>("Ts", name);
	rules.add_rule<class_enum_type>("Tu", name);
	rules.add_rule<class_enum_type>("Te", name);
	rules.add_rule<unnamed_type_name>("Ut_");
	rules.add_rule<unnamed_type_name>("Ut", nonneg_number, "_");
}

void add_array_rules(ProductionGrammar& rules) {
	rules.add_rule<array_type>("A_", type);
	rules.add_rule<array_type>("A", nonneg_number, "_", type);
	rules.add_rule<vector_type>("Dv", nonneg_number, "_", type);
	rules.add_rule<array_type>("A", expression, "_", type);
}

void add_pointer_to_member_rules(ProductionGrammar& rules) {
	rules.add_rule<pointer_to_member_type>("M", type, type);
}

void add_template_parameter_rules(ProductionGrammar& rules) {
	rules.add_rule<template_param>("T", opt(nonneg_number), "_");
	rules.add_rule<template_template_param>(template_param);
	rules.add_rule<template_template_param>(substitution);
	rules.add_rule<template_param>(template_param_decl_llvm);
	rules.add_rule<template_param_decl_llvm>("Ty");
	rules.add_rule<template_param_decl_llvm>("Tn", type);
	rules.add_rule<template_param_decl_llvm>("TtE");
	rules.add_rule<template_param_decl_llvm>("Tt", rep(template_param_decl_llvm), "E");
	rules.add_rule<template_param_decl_llvm>("Tp", template_param_decl_llvm);
	rules.add_rule<template_param_decl_llvm>("Tk", name, opt(template_args));
	rules.add_rule<template_param_decl_llvm>("TL", nonneg_number, "_", opt(nonneg_number), "_");
}

void add_function_parameter_reference_rules(ProductionGrammar& rules) {
	rules.add_rule<function_param>("fp", opt(cv_qualifiers), "_");
	rules.add_rule<function_param>("fp", opt(cv_qualifiers), nonneg_number, "_");
	rules.add_rule<function_param>("fL", nonneg_number, "p", opt(cv_qualifiers), "_");
	rules.add_rule<function_param>("fL", nonneg_number, "p", opt(cv_qualifiers), nonneg_number, "_");
	rules.add_rule<function_param>("fpT");
}

void add_template_argument_rules(ProductionGrammar& rules) {
	rules.add_rule<template_args>("I", rep(template_arg), "E");
	rules.add_rule<template_args>("I", rep(template_arg), requires_clause, "E");
	rules.add_rule<template_arg>(type);

	rules.add_rule<template_arg>("X", expression, "E");
	rules.add_rule<template_arg>(expr_primary);
	rules.add_rule<template_arg>(template_arg_pack);
	rules.add_rule<template_arg_pack>("J", opt_rep(template_arg), "E");
}

void add_expression_rules(ProductionGrammar& rules) {
	rules.add_rule<expression>(unary_operator_name, expression);
	rules.add_rule<expression>(binary_operator_name, expression, expression);
	rules.add_rule<expression>(ternary_operator_name, expression, expression, expression);
	rules.add_rule<expression>(prefix_operator_expression);
	rules.add_rule<prefix_operator_expression>("pp_", expression);
	rules.add_rule<prefix_operator_expression>("mm_", expression);
	rules.add_rule<expression>(call_expression);
	rules.add_rule<call_expression>("cp", base_unresolved_name, opt_rep(expression), "E");
	rules.add_rule<call_expression>("cl", base_unresolved_name, opt_rep(expression), "E");
	rules.add_rule<call_expression>("cl", rep(expression), "E");
	rules.add_rule<expression>(conversion_expression);
	rules.add_rule<conversion_expression>("cv", type, expression);
	rules.add_rule<conversion_expression>("cv", type, "_", opt_rep(expression), "E");
	rules.add_rule<expression>("tl", type, opt_rep(braced_expression), "E");
	rules.add_rule<expression>("il", opt_rep(braced_expression), "E");
	rules.add_rule<expression>(opt("gs"), "nw", opt_rep(expression), "_", type, "E");
	rules.add_rule<expression>(opt("gs"), "nw", opt_rep(expression), "_", type, initializer);
	rules.add_rule<expression>(opt("gs"), "na", opt_rep(expression), "_", type, "E");
	rules.add_rule<expression>(opt("gs"), "na", opt_rep(expression), "_", type, initializer);
	rules.add_rule<expression>(opt("gs"), "dl", expression);
	rules.add_rule<expression>(opt("gs"), "da", expression);
	rules.add_rule<expression>(pack_indexing_expression);
	rules.add_rule<pack_indexing_expression>("sy", template_param, expression);
	rules.add_rule<pack_indexing_expression>("sy", function_param, expression);
	rules.add_rule<pack_indexing_expression>("sy", template_arg_pack, expression);

	rules.add_rule<expression>(keyword_cast_expression);
	rules.add_rule<expression>(keyword_type_expression);
	rules.add_rule<expression>(keyword_expression);
	rules.add_rule<expression>(rethrow_expression);
	rules.add_rule<keyword_cast_expression>(keyword_cast_operator, type, expression);

	rules.add_rule<keyword_cast_operator>("dc");
	rules.add_rule<keyword_cast_operator>("sc");
	rules.add_rule<keyword_cast_operator>("cc");
	rules.add_rule<keyword_cast_operator>("rc");

	rules.add_rule<keyword_type_expression>(type_expression_operator, type);
	rules.add_rule<type_expression_operator>("ti");
	rules.add_rule<type_expression_operator>("st");
	rules.add_rule<type_expression_operator>("at");

	rules.add_rule<keyword_expression>(expression_operator, expression);
	rules.add_rule<expression_operator>("te");
	rules.add_rule<expression_operator>("sz");
	rules.add_rule<expression_operator>("az");
	rules.add_rule<expression_operator>("nx");
	rules.add_rule<expression_operator>("tw");
	rules.add_rule<rethrow_expression>("tr");

	rules.add_rule<expression>(template_param);
	rules.add_rule<expression>(function_param);
	rules.add_rule<expression>(bind_expression);
	rules.add_rule<bind_expression>("dt", expression, unresolved_name);
	rules.add_rule<bind_expression>("pt", expression, unresolved_name);
	rules.add_rule<bind_expression>("ds", expression, expression);

	rules.add_rule<expression>("sZ", template_param);
	rules.add_rule<expression>("sZ", function_param);
	rules.add_rule<expression>("sP", rep(template_arg), "E");
	rules.add_rule<expression>(pack_expression);
	rules.add_rule<pack_expression>("sp", expression);
	rules.add_rule<expression>(fold_expression);
	rules.add_rule<fold_expression>(fold_left_expression);
	rules.add_rule<fold_expression>(fold_right_expression);
	rules.add_rule<fold_left_expression>("fl", binary_operator_name, expression);
	rules.add_rule<fold_left_expression>("fL", binary_operator_name, expression, expression);
	rules.add_rule<fold_right_expression>("fr", binary_operator_name, expression);
	rules.add_rule<fold_right_expression>("fR", binary_operator_name, expression, expression);

	rules.add_rule<expression>("u", source_name, rep(template_arg), "E");
	rules.add_rule<expression>(unresolved_name);
	rules.add_rule<expression>(expr_primary);
	rules.add_rule<expression>(requires_expression);
	rules.add_rule<requires_expression>("rq", rep(requirement_llvm), "E");
	rules.add_rule<requires_expression>("rQ", bare_function_type, "_", rep(requirement_llvm), "E");
	rules.add_rule<requirement_llvm>("X", expression, "R", type);
	rules.add_rule<requirement_llvm>("X", expression);
	rules.add_rule<requirement_llvm>("T", type);

	rules.add_rule<unresolved_name>(opt("gs"), "sr", rep(unresolved_qualifier_level), "E", base_unresolved_name);
	rules.add_rule<unresolved_name>(base_unresolved_name);

	rules.add_rule<unresolved_name>("srN", unresolved_type, rep(unresolved_qualifier_level), "E", base_unresolved_name);
	rules.add_rule<unresolved_name>(opt("gs"), base_unresolved_name);
	rules.add_rule<unresolved_name>(unresolved_name_gcc);
	rules.add_rule<unresolved_name_gcc>("sr", type, base_unresolved_name);
	rules.add_rule<unresolved_name>("sr", unresolved_type, base_unresolved_name);

	rules.add_rule<unresolved_type>(template_param, opt(template_args));

	rules.add_rule<unresolved_type>(decltype_);
	rules.add_rule<unresolved_type>(substitution);

	rules.add_rule<unresolved_qualifier_level>(simple_id);
	rules.add_rule<simple_id>(source_name, opt(template_args));

	rules.add_rule<base_unresolved_name>(simple_id);
	rules.add_rule<base_unresolved_name>("on", operator_name, opt(template_args));
	rules.add_rule<base_unresolved_name>("dn", destructor_name);

	rules.add_rule<destructor_name>(unresolved_type);
	rules.add_rule<destructor_name>(simple_id);

	rules.add_rule<expr_primary>("L_Z", encoding, "E");
	rules.add_rule<expr_primary>("L_Z", data_encoding, "E");
	rules.add_rule<expr_primary>("L", type, number, "E");
	rules.add_rule<expr_primary>("L", type, float_, "E");
	rules.add_rule<expr_primary>("L", type, "E");
	rules.add_rule<expr_primary>("L", type, float_, "_", float_, "E");

	rules.add_rule<braced_expression>(expression);
	rules.add_rule<braced_expression>("di", source_name, braced_expression);
	rules.add_rule<braced_expression>("dx", expression, braced_expression);
	rules.add_rule<braced_expression>("dX", expression, expression, braced_expression);
	rules.add_rule<initializer>("pi", rep(expression), "E");
}

void add_scope_rules(ProductionGrammar& rules) {
	rules.add_rule<local_name>("Z", encoding, "E", name, opt(discriminator));

	rules.add_rule<local_name>("Z", encoding, "E", "s", opt(discriminator));
	rules.add_rule<local_name>("Z", encoding, "E", "s", opt(discriminator));
	rules.add_rule<discriminator>("_", digit);
	rules.add_rule<discriminator>("__", nonneg_number, "_");
}

void add_closure_rules(ProductionGrammar& rules) {
	rules.add_rule<unnamed_type_name>(closure_type_name);
	rules.add_rule<closure_type_name>("Ul", lambda_sig, "E", opt(nonneg_number), "_");
	rules.add_rule<lambda_sig>(rep(type));
	rules.add_rule<local_name>("Z", encoding, "Ed", number, "_", name);

	rules.add_rule<closure_prefix>("M", closure_type_name);
}

void add_substitution_rules(ProductionGrammar& rules) {
	rules.add_rule<substitution>("S", opt(seq_id), "_");
	rules.add_rule<substitution>("St");
	rules.add_rule<substitution>("Sa");
	rules.add_rule<substitution>("Sb");
	rules.add_rule<substitution>("Ss");
	rules.add_rule<substitution>("Si");
	rules.add_rule<substitution>("So");
	rules.add_rule<substitution>("Sd");
}
}

ProductionGrammar build_grammar() {
	ProductionGrammar rules;
	add_rules(rules);
	add_name_rules(rules);
	add_operator_rules(rules);
	add_special_rules(rules);
	add_ctor_rules(rules);
	add_type_rules(rules);
	add_guard_rules(rules);
	add_builtin_type_rules(rules);
	add_function_type_rules(rules);
	add_decltype_rules(rules);
	add_class_rules(rules);
	add_array_rules(rules);
	add_pointer_to_member_rules(rules);
	add_template_parameter_rules(rules);
	add_function_parameter_reference_rules(rules);
	add_template_argument_rules(rules);
	add_expression_rules(rules);
	add_scope_rules(rules);
	add_closure_rules(rules);
	add_substitution_rules(rules);

	rules.add_rule<nonneg_number>(rep(CharRanges(CharRange('0', '9'))));
	rules.add_rule<float_>(rep(CharRanges(CharRange('0', '9'), CharRange('a', 'f'))));
	rules.add_rule<digit>(CharRange('0', '9'));
	rules.add_rule<seq_id>(rep(CharRanges(CharRange('0', '9'), CharRange('A', 'Z'))));

	rules.add_rule<vendor_specfic_suffix>(rep(CharRanges({'0', '9'}, {'A', 'Z'}, '_', '.')));
	rules.add_rule<identifier>(rep(CharRanges({'0', '9'}, {'A', 'Z'}, '_', '.', '$')));

	rules.lookup = optimize(rules);

	return rules;
}

void extend_grammar(ProductionGrammar& rules) {
	rules.add_rule<overload_set>("_O", name);
	rules.add_rule<name>(overload_set);
	rules.lookup = optimize(rules);
}

}
