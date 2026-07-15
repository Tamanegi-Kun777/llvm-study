CXX := clang++
LLVM_FLAGS := $(shell llvm-config --cxxflags --ldflags --system-libs --libs core)
SRCS := dcc.cpp AST.cpp TokenStream.cpp lexer.cpp parser.cpp codegen.cpp option_parser.cpp

dcc: $(SRCS)
	$(CXX) $(SRCS) $(LLVM_FLAGS) -o dcc

clean:
	rm -f dcc