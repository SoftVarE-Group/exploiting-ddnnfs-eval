#include "graph.hpp"
#include "linObjFunc.hpp"

class ParserException {
public:
	ParserException(const std::string& message): message{message} {}
	inline const std::string& getMessage() const noexcept { return message; }

private:
	const std::string message;
};

// Reads a NNF file through "in" and returns the corresponding Graph.
Graph parseNNF(std::istream& in);

// Reads a weight file and returns the weights associated with each literal.
WeightVector parseWeights(std::istream& in, int nbVars);

// Reads a binary file through "in" and returns the corresponding Graph.
Graph parseBin(std::istream& in);

// Reads a model from "in" and returns it.
Model parseModel(std::istream& in);
