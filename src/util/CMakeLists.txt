
add_library(swartalf-decolib OBJECT
Decorator.cxx
)

settings(swartalf-decolib)
add_library(swartalf::deco ALIAS swartalf-decolib)


add_library(swartalf-version OBJECT
version.cxx
)

settings(swartalf-version)
add_library(swartalf::version ALIAS swartalf-version)

add_library(swartalf-util STATIC
ascii.cxx
string.cxx
)

settings(swartalf-util)
add_library(swartalf::util ALIAS swartalf-util)

