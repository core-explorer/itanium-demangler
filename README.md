A demangler for the C++ Itanium ABI.

No dependencies besides the C++23 standard library.

Available as a command line tool or online.

Highlights:
- colored output
- can shorted output
- the parser accepts more code than c++filt or llvm-cxxfilt, including mangled requirements and constraints and pack indexing

Downsides:
- This is not a production quality implementation. Other demanglers will be faster and use less memory.

