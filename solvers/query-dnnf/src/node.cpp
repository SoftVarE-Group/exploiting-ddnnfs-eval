#include "node.hpp"
#include <string>
#include <algorithm>

using namespace std;

unique_ptr<Model> LitNode::validModel(const Model& partialModel) const {
    const int var = lit > 0 ? lit : -lit;
	const Binding binding = partialModel.bindingFor(var);
	if (binding == Binding::Free) {
		Model m{partialModel};
		m.setBindingFor(var, static_cast<Binding>(lit > 0));
		return make_unique<Model>(m);
	} else if (binding == Binding::Unsat || binding == static_cast<Binding>(lit < 0)) {
		return nullptr;
	} else {
		return make_unique<Model>(Model{partialModel});
	}
}

static void printSet(const std::set<int>& s) {
    printf("seen vars: ");
    for (auto elt: s) {
        printf("%d ", elt);
    }
    printf("\n");
}

mpq_class OrNode::modelCount(const WeightVector& weights, map<shared_ptr<Node>, mpq_class>& cache) const {
    mpq_class res = 0_mpq;
    for (const OrBranch& branch: branches) {
		mpq_class local = branch.child->modelCount(weights, cache);
		if (_seenVars.size() != branch.child->seenVars().size()) {
			mpq_class factor = 1_mpq;
			const auto& seenChild = branch.child->seenVars();
			for (int var: _seenVars) {
				if (seenChild.count(var) == 0) {
					factor = factor * weights.weightFor(var) + factor * weights.weightFor(-var);
				}
				if (factor == 0_mpq) {
					break;
				} 
			}
			local *= factor;
		}
		res += local;
	}
	return res;
}

unique_ptr<Model> OrNode::validModel(const Model& partialModel) const {
    for (const OrBranch& branch: branches) {
		Model m{partialModel};
		bool failedBranch = false; // true iff a binding contradicts m
		for (int lit: branch.unitLits) {
			const int var   = lit > 0 ? lit : -lit;
			const Binding b = m.bindingFor(var);
            if (b == Binding::Unsat || b == static_cast<Binding>(lit < 0)) {  // Binding in model is opposite of binding in graph
				failedBranch = true;
				break;
			}
			m.setBindingFor(var, static_cast<Binding>(lit > 0));
		}
		if (!failedBranch) {
            unique_ptr<Model> res{branch.child->validModel(m)};
            if (res != nullptr) {
                return res;
            }
		}
	}
	return nullptr;
}

// Removes index's child from children.
// Replaces it with the last item from collection.
inline void removeChild(vector<shared_ptr<Node>>& children, int index) noexcept {
	children[index] = children.back();
	children.pop_back();
}

// Returns the line index of a child node.
inline int lineIndexFor(const map<const Node*,int> nodeToLine, const Node& node) noexcept {
	if (typeid(node) == typeid(FalseNode)) {
		return 0;
	} else if (typeid(node) == typeid(TrueNode)) {
		return 1;
	} else {
		if (nodeToLine.find(&node) == nodeToLine.end()) {
			throw "Invalid node!";
		}
		return nodeToLine.at(&node);
	}
}

void OrNode::printNNF(ostream& out, map<const Node*,int>& nodeToLine) const {
	const int line = nodeToLine[this];
	const int nbLines[2] = {static_cast<int>(branches[0].unitLits.size() + 1), static_cast<int>(branches[1].unitLits.size() + 1)};
	const int branchLines[2] = {line - 1, line - nbLines[0] - 1}; // Index of each branch's implicit AndNode's line.
	for (int i = branches.size() - 1; i >= 0; i--) {
		const int branchLine = branchLines[i];
		const OrBranch& branch{branches[i]};
		for (int lit: branch.unitLits) {
			out << "L " << lit << endl;
		}
		if(typeid(*branch.child) == typeid(TrueNode)) {
		  out << "A " << branch.unitLits.size();
		} else {
		  const int childLine = lineIndexFor(nodeToLine, *branch.child);
		  out << "A " << (branch.unitLits.size() + 1) << " " << childLine;
		}
		for (int j = 0; j < branch.unitLits.size(); j++) {
			const int litLine = branchLine - branch.unitLits.size() + j;
			out << " " << litLine;
		}
		out << endl;
	}
	out << "O " << variable << " 2 " << branchLines[0] << " " << branchLines[1] << endl;
}

mpq_class AndNode::modelCount(const WeightVector& weights, map<shared_ptr<Node>, mpq_class>& cache) const {
	mpq_class nb = 1_mpq;
	for (int lit: unitLits) {
		nb *= weights.weightFor(lit);
	}
	if (nb == 0_mpq) {
		return 0_mpq;
	}
	for (auto child: children) {
		auto it = cache.find(child);
		if (it == cache.end()) {
			auto mc = child->modelCount(weights, cache);
			cache[child] = mc;
			nb *= mc;
		} else {
			nb *= it->second;
		}
		if (nb == 0_mpq) {
			return 0_mpq;
		}
	}
	return nb;
}

unique_ptr<Model> AndNode::validModel(const Model& partialModel) const {
	Model res{partialModel};
	for (int lit: unitLits) {
		const int var   = lit > 0 ? lit : -lit;
		const Binding b = res.bindingFor(var);
		if (b == Binding::Unsat || b == static_cast<Binding>(lit < 0)) {  // Binding in model is opposite of binding in graph
			return nullptr;
		}
		res.setBindingFor(var, static_cast<Binding>(lit > 0));
	}
    for (auto child: children) {
        const unique_ptr<Model> m{child->validModel(res)};
        if (m == nullptr|| !Model::mix(res, *m, res)) {
            return nullptr;
        }
    }
    return make_unique<Model>(move(res));
}

void AndNode::printNNF(ostream& out, map<const Node*,int>& nodeToLine) const {
	const int line = nodeToLine[this];
	for (int lit: unitLits) {
		out << "L " << lit << endl;
	}
	out << "A " << (unitLits.size() + children.size());
	for (const auto& child: children) {
		const int childLine = lineIndexFor(nodeToLine, *child);
		out << " " << childLine;
	}
	for (int i = 0; i < unitLits.size(); i++) {
		const int litLine = line - unitLits.size() + i;
		out << " " << litLine;
	}
	out << endl;
}

// conditions node to partialModel.
void conditionOr(OrNode& node, const Model& partialModel, set<shared_ptr<Node>>& cache) {
	for (int i = 0; i < node.branches.size(); i++) {
		OrBranch& branch{node.branches[i]};
		int j = 0;
		for (int lit: branch.unitLits) {
			const int var = (lit > 0? lit: -lit);
			const Binding binding{partialModel.bindingFor(var)};
			if (binding != Binding::Free && (binding == Binding::True) != (lit > 0)) { // branch becomes UNSAT
				branch.child = falseNode;
				branch.unitLits.clear();
				break;
			}
		}
		for (int i = 0; i < partialModel.nbVars(); i++) { // All vars from the model must be removed from seenVars
			const int var = i + 1;
			if (partialModel.bindingFor(var) != Binding::Free) {
				node._seenVars.erase(var);
			}
		}
		condition(branch.child, partialModel, cache);
	}
}

void conditionAnd(AndNode& node, const Model& partialModel, set<shared_ptr<Node>>& cache) {
	int j = 0;
	for (int i = 0; i < partialModel.nbVars(); i++) { // All vars from the model must be removed from seenVars
		const int var = i + 1;
		if (partialModel.bindingFor(var) != Binding::Free) {
			node._seenVars.erase(var);
		}
	}
	for (const int lit: node.unitLits) {
		const int var = (lit > 0? lit: -lit);
		const Binding binding{partialModel.bindingFor(var)};
		if (binding != Binding::Free && (binding == Binding::True) != (lit > 0)) { // Incompatible assignment: no need to go any further
			node.children.clear();
			node.children.push_back(falseNode);
			node.unitLits.clear();
			return;
		}
	}
	for (int i = 0; i < node.children.size(); i++) {
		condition(node.children[i], partialModel, cache);
	}
}

void condition(shared_ptr<Node>& node, const Model& partialModel, set<shared_ptr<Node>>& cache) {
	auto it = cache.find(node);
	if (it != cache.end()) { // We aleardy visited this node, ignore
		return;
	}
	cache.insert(node);
	if (typeid(*node) == typeid(TrueNode&) || typeid(*node) == typeid(FalseNode&)) {
		return;
	}
	if (typeid(*node) == typeid(OrNode)) {
		conditionOr(static_cast<OrNode&>(*node), partialModel, cache);
	} else if (typeid(*node) == typeid(AndNode)) { // AndNode
		conditionAnd(static_cast<AndNode&>(*node), partialModel, cache);
	}
}

void indexNodes(const shared_ptr<Node>& node, map<const Node*,int>& nodeToLine, vector<const Node*>& allNodes, int& lineIndex) {
	Node* ptr = node.get();
	if (nodeToLine.find(ptr) != nodeToLine.end()) { // Node was already indexed
		return;
	}
	if (typeid(*node) == typeid(LitNode)) {
		nodeToLine[ptr] = lineIndex;
		lineIndex++;
		allNodes.push_back(ptr);
	} else if (typeid(*node) == typeid(AndNode)) {
		const AndNode& aNode{static_cast<AndNode&>(*node)};
		for (const auto& child: aNode.children) {
			indexNodes(child, nodeToLine, allNodes, lineIndex);
		}
		lineIndex += aNode.unitLits.size();
		nodeToLine[ptr] = lineIndex;
		lineIndex++;
		allNodes.push_back(ptr);
	} else if (typeid(*node) == typeid(OrNode)) {
		const OrNode& oNode{static_cast<OrNode&>(*node)};
		for (const OrBranch& branch: oNode.branches) {
			indexNodes(branch.child, nodeToLine, allNodes, lineIndex);
		}
		for (const OrBranch& branch: oNode.branches) {
			lineIndex += branch.unitLits.size() + 1; // One line for each lit, one for the implicit "and node".
		}
		nodeToLine[ptr] = lineIndex;
		lineIndex++;
		allNodes.push_back(ptr);
	}
}


int OrNode::nbDescendants() const {
  int cpt=0;
  for(int i=0; i<2; ++i) {
    OrBranch b = branches[i];
    ++cpt;
    cpt += b.unitLits.size();
    if((b.child != nullptr) && (typeid(*b.child) != typeid(TrueNode))) {
      ++cpt;
      cpt += b.child->nbDescendants();
    }
  }
  return cpt;
}
