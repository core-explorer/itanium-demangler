#pragma once
#include "../util/Decorator.hxx"

namespace swartalf {

struct CliDecorator {
	using value_type = std::string;
	static std::string decorate(Decoration deco, std::string_view tn, unsigned level = 0);
};
using CliLinker = AsciiLinker;
using CliEscaper = AsciiEscaper;

using CliFormatter = Formatter<CliDecorator, CliEscaper, CliLinker>;
}