#include "DemangleBasicType.hxx"
namespace swartalf::itanium {
using namespace std::string_view_literals;
std::string_view demangleBasicType(char mangledName, const DemanglingDialect& v) {
	switch (mangledName) {
	case 'v':
		return "void"sv;
	case 'z':
		return "..."sv;
	case 'b':
		return "bool"sv;
	case 's': {
		if (v.fundamental_name == FundamentalName::explicit_int) {
			return "short int"sv;
		}
		return "short"sv;
	}
	case 't': {
		if (v.fundamental_name == FundamentalName::explicit_int) {
			if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
				return "short int unsigned"sv;
			}
			if (v.unsigned_orientation == UnsignedOrientation::central_unsigned) {
				return "short unsigned int"sv;
			}
			return "unsigned short int"sv;
		}
		if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
			return "short unsigned"sv;
		}
		return "unsigned short"sv;
	}
	case 'i':
		return "int"sv;
	case 'j': {
		if (v.fundamental_name == FundamentalName::explicit_int or v.fundamental_name == FundamentalName::unsigned_int) {
			if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
				return "int unsigned"sv;
			}
			return "unsigned int"sv;
		}
		return "unsigned"sv;
	}
	case 'l': {
		if (v.fundamental_name == FundamentalName::explicit_int) {
			return "long int"sv;
		}
		return "long"sv;
	}
	case 'm': {
		if (v.fundamental_name == FundamentalName::explicit_int) {
			if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
				return "long int unsigned"sv;
			}
			if (v.unsigned_orientation == UnsignedOrientation::central_unsigned) {
				return "long unsigned int"sv;
			}
			return "unsigned long int"sv;
		}
		if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
			return "long unsigned"sv;
		}
		return "unsigned long"sv;
	}
	case 'n': {
		return "__int128"sv;
	}
	case 'o': {
		if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
			return "__int128 unsigned"sv;
		}
		return "unsigned __int128"sv;
	}
	case 'x': {
		if (v.fundamental_name == FundamentalName::explicit_int) {
			return "long long int"sv;
		}
		return "long long"sv;
	}
	case 'y': {
		if (v.fundamental_name == FundamentalName::explicit_int) {
			if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
				return "long long int unsigned"sv;
			}
			if (v.unsigned_orientation == UnsignedOrientation::central_unsigned) {
				return "long long unsigned int"sv;
			}
			return "unsigned long long int"sv;
		}
		if (v.unsigned_orientation == UnsignedOrientation::east_unsigned) {
			return "long long unsigned"sv;
		}
		return "unsigned long long"sv;
	}
	case 'c':
		return "char"sv;
	case 'h':
		return "unsigned char"sv;
	case 'a':
		return "signed char"sv;
	case 'f':
		return "float"sv;
	case 'g':
		return "__float128"sv;
	case 'd':
		return "double"sv;
	case 'e':
		return "long double"sv;
	case 'w':
		return "wchar_t"sv;
	default:
		return "_Unknown"sv;
	}
}
std::string_view demangleBasicDType(char mangledName, const DemanglingDialect& v) {
	switch (mangledName) {
	case 'a':
		return "auto"sv;
	case 'c':
		return "decltype(auto)"sv;
	case 'd':
		return "decimal64"sv;
	case 'e':
		return "decimal128"sv;
	case 'f':
		return "decimal32"sv;
	case 'h':
		return "half"sv;
	case 'i':
		return "char32_t"sv;
	case 's':
		return "char16_t"sv;
	case 'u':
		return "char8_t"sv;
	case 'n': {
		if (v.nullptr_type == NullptrType::decltype_nullptr) {
			return "decltype(nullptr)"sv;
		}
		return "std::nullptr_t"sv;
	}

	default:
		return "_Unknown"sv;
	}
}
std::string_view demangleBasicType(std::string_view mangledName, const DemanglingDialect& v) {
	if (mangledName.size() == 1) {
		return demangleBasicType(mangledName[0], v);
	}
	if (mangledName.size() == 2 and mangledName[0] == 'D') {
		return demangleBasicDType(mangledName[1], v);
	}
	if (mangledName == "DF16b") {
		return "std::bfloat16_t";
	}
	if (mangledName == "DF16_") {
		return "_Float16";
	}
	if (mangledName == "DF32_") {
		return "_Float32";
	}
	if (mangledName == "DF64_") {
		return "_Float64";
	}
	if (mangledName == "DF128_") {
		return "_Float128";
	}
	return "_Unknown"sv;
}
}