#include "FormatMangle.hxx"
#include "Grammar.hxx"
#include <format>
#include <string>

namespace fmt {
using namespace std;
}

namespace swartalf::itanium {

std::string format_mangled(const ParseResult& res) {
	if (res.type == ProductionType::source_name and res.comps.size() == 1) {
		if (const auto* p = std::get_if<ParseResult>(res.comps.data()); p and p->type == ProductionType::identifier and p->comps.size() == 1) {
			if (const auto* q = std::get_if<std::string_view>(p->comps.data()); q) {
				return fmt::format("{}{}", q->size(), *q);
			}
		}
	}

	std::string result;
	for (const auto& c : res.comps) {
		if (const auto* p = std::get_if<ParseResult>(&c); p) {
			result += format_mangled(*p);
		}
		if (const auto* q = std::get_if<std::string_view>(&c); q) {
			result += *q;
		}
	}

	return result;
}

}