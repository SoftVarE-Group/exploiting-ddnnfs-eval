bin_PROGRAMS = query-dnnf

query_dnnf_SOURCES = graph.cpp graph.hpp linObjFunc.cpp linObjFunc.hpp main.cpp model.cpp model.hpp node.cpp node.hpp parser.cpp parser.hpp prompt.cpp prompt.hpp weightvector.hpp

all: all-am
	(test -e ../query-dnnf) || $(LN_S) src/query-dnnf ../query-dnnf
