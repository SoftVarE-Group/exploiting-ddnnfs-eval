#include "graph.hpp"
#include <algorithm>
#include <map>

using namespace std;

inline vector<bool> differenceOf(const vector<bool>& b1, const vector<bool>& b2) noexcept {
    vector<bool> res(b1.size());
    for (size_t i = 0; i < b1.size(); i++) {
        res[i] = b1[i] && !b2[i];
    }
    return res;
}

inline void unionWith(vector<bool>& vec, const vector<bool>& other) noexcept {
    for (size_t i = 0; i < vec.size(); i++) {
        vec[i] = vec[i] || other[i];
    }
}

inline int nbTrue(const vector<bool>& vec) noexcept {
    return count(vec.begin(), vec.end(), true);
}

// updateWeights updates the weights by setting all literals that are falsified by the model to 0.
void updateWeights(WeightVector& weights, const Model& partialModel) {
    for (int i = 1; i <= weights.nbVars(); i++) {
        switch (partialModel.bindingFor(i)) {
        case Binding::True:
            weights.setWeightFor(-i, 0);
            break;
        case Binding::False:
            weights.setWeightFor(i, 0);
            break;
        case Binding::Unsat:
            weights.setWeightFor(-i, 0);
            weights.setWeightFor(i, 0);
            break;
        default:
            break;
        }
    }
}

mpq_class Graph::modelCount(const Model& partialModel) const {
    map<shared_ptr<Node>, mpq_class> cache;
    WeightVector localWeights = weights;
    updateWeights(localWeights, partialModel);
    mpq_class mc = root->modelCount(localWeights, cache);
    const auto& vars = root->seenVars();
    if (vars.size() == nbVars) {
        return mc;
    }
    mpq_class factor = 1_mpq;
	for (int var = 1; var <= nbVars; var++) {
		if (vars.count(var) == 0) {
			factor = factor * localWeights.weightFor(var) + factor * localWeights.weightFor(-var);
		}
		if (factor == 0_mpq) {
			break;
		} 
	}
	return mc * factor;
}

void Graph::conditionTo(const Model& partialModel) {
    updateWeights(weights, partialModel);
    shared_ptr<AndNode> node = make_shared<AndNode>();
    for (int var = 1; var <= weights.nbVars(); var++) {
        switch (partialModel.bindingFor(var)) {
        case Binding::True:
            node->unitLits.push_back(var);
            node->_seenVars.insert(var);
            break;
        case Binding::False:
            node->unitLits.push_back(-var);
            node->_seenVars.insert(var);
            break;
        case Binding::Unsat:
            node->unitLits.push_back(var);
            node->unitLits.push_back(-var);
            node->_seenVars.insert(var);
            break;
        default:
            break;
        }
    }
    node->children.push_back(root);
    const auto seen = root->seenVars();
    node->_seenVars.insert(seen.begin(), seen.end());
    root = move(node);
}

int nbNodesRec(const shared_ptr<Node>& node) {
	if (shared_ptr<AndNode> aNode = dynamic_pointer_cast<AndNode>(node)) {
		int nb = 1;
		for (auto child: aNode->children) {
			nb += nbNodesRec(child);
		}
		return nb;
	} else if (shared_ptr<OrNode> oNode = dynamic_pointer_cast<OrNode>(node)) {
		return 1 + nbNodesRec(oNode->branches[0].child) + nbNodesRec(oNode->branches[1].child);
	} else { // LitNode, TrueNode or FalseNode
        return 1;
    }
	throw GraphException{"Invalid node"};
}

int Graph::nbNodes() const {
	return nbNodesRec(root);
}

void Graph::print(ostream& out) const {
    map<const Node*, int> nodeToLine;
    vector<const Node*> allNodes;
    int lineIndex{2};
    indexNodes(root, nodeToLine, allNodes, lineIndex);
    out << "nnf " << lineIndex << " " << root->nbDescendants() << " " << nbVars << endl;
    // Bottom and top must be written as the very first lines
    out << "O 0 0" << endl;
    out << "A 0" << endl;
    for (const Node* node: allNodes) {
        node->printNNF(out, nodeToLine);
    }
}
