#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "node.hpp"

class Graph {
public:
	Graph(int nbVars, std::shared_ptr<Node> root): nbVars{nbVars}, root{root}, weights(nbVars) {}

	// nbNodes returns the total number of nodes in the graph.
	int nbNodes() const;

	// prints the content of the graph in NNF format on the provided output stream.
	void print(std::ostream& out) const;

	// modelCount returns the number of models for that graph that verify the
	// given constraints.
	mpq_class modelCount(const Model& partialModel) const;

    // Returns a valid model, if any, for the graph.
    // Return is null iff modelCount() == 0.
    inline std::unique_ptr<Model> validModel() const { return root->validModel(Model{nbVars}); }

    // Returns a valid model, if any, for the graph and given partial model.
    // Return is null iff modelCount(partialModel) == 0.
    std::unique_ptr<Model> validModel(const Model& partialModel) const { return root->validModel(partialModel); }

	// Modifies the graph so that the partialModel is satisfied.
	// Way more efficient than its const counterpart.
	void conditionTo(const Model& partialModel);

	// setWeight sets the weight of 'literal' to 'weight'.
	inline void setWeights(const WeightVector& newWeights) {
		weights = newWeights;
	}

	const int nbVars;

	std::shared_ptr<Node> root;

private:
	WeightVector weights;
};

#endif
