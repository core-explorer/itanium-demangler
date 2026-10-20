#pragma once
#include "ParseResult.hxx"
#include <expected>
namespace swartalf::itanium {
enum class ReturnSignature : uint8_t {
	no_return_type,
	return_type_but_not_mangled,
	mangled_return_type
};

struct FunctionClassification {
	ProductionType name_type;
	bool is_templated;
	ReturnSignature rs;
};

enum class ClassifyEncodingError : uint8_t {
	error
};
std::expected<FunctionClassification, ClassifyEncodingError> classify_encoding(const ParseResult& res);
std::expected<FunctionClassification, ClassifyEncodingError> classify_name(const ParseResult& res);
}