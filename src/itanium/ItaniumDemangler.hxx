#pragma once
#include "ParseName.hxx"

namespace swartalf {

struct ItaniumDemangler {
	itanium::ProductionGrammar rules = itanium::build_grammar();
};

}