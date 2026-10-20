#pragma once
#include "CompositeString.hxx"
#include "DemanglingDialect.hxx"
#include "../swartalf/RefPointer.hxx"
#include <string>
namespace swartalf {

struct ManglingDialect {
	bool nullpointer_value_implicit_zero = false;
};

class AbstractMangling : public swartalf::IntrusiveRefCounted<AbstractMangling> {
public:
	virtual ~AbstractMangling() = default;
	virtual std::string get_mangled_name(const ManglingDialect&) const = 0;
	virtual CompositeString get_demangled_name(const DemanglingDialect&) const = 0;
};

enum class ItaniumTypeType {
	none,
	basic,
	simple,
	templated,
	modified,
	template_parameter_reference,
	template_parameter_pack_expansion,
	unsized_array,
	sized_array,
	ellipsis,
	function_signature,
	closure_type,
	unnamed_type,
	scoped_type
};

enum class TypeModifier : char {
	none = 0,
	const_ = 'K',
	pointer = 'P',
	reference = 'R',
	rvalue = 'O',
	volatile_ = 'V',
	restrict_ = 'r',
	complex = 'C',
	imaginary = 'G',
};

class AbstractItaniumType : public AbstractMangling {
public:
	virtual ItaniumTypeType get_type() const = 0;
};

enum class ItaniumNameType {
	none,
	simple,
	anonymous_namespace,
	composite,
	standard,
	composite_standard,
	templated_name,
	type_name,
	template_argument_name,
	subsitution_name,
	operator_name,
	conversion_operator_name,
	ctor_dtor_name,
	tagged_name,
	vendor_name,
	dependent_name,
	local_name,

};

class AbstractItaniumName : public AbstractMangling {
public:
	virtual ItaniumNameType get_type() const = 0;
};

enum class TemplateArgumentType {
	none,
	type,
	value,
	pack,
	expression
};
class AbstractItaniumTemplateArgument : public AbstractMangling {
public:
	virtual TemplateArgumentType get_type() const = 0;
};

class AbstractItaniumSymbol : public AbstractMangling {
};

std::string to_string_mangled(const AbstractMangling&);
CompositeString to_string_demangled(const AbstractMangling&);

void print_mangled_name(const AbstractMangling&);
void print_demangled_name(const AbstractMangling&);
}