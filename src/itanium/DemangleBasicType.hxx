#pragma once
#include "DemanglingDialect.hxx"
#include <string_view>
namespace swartalf::itanium {
std::string_view demangleBasicType(char mangledName, const DemanglingDialect& v);
std::string_view demangleBasicDType(char mangledName, const DemanglingDialect& v);
std::string_view demangleBasicType(std::string_view mangledName, const DemanglingDialect& v);

}