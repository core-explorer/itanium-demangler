#include "../cli/ArgParser.hxx"

namespace swartalf {
struct FiltApp {
	ArgParser parser;
	std::string_view exe;
	int run(const ParsingResult& parseResult);
};
}