#ifndef NODE_HPP
#define NODE_HPP

#include "model.hpp"
#include "weightvector.hpp"
#include <set>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <gmpxx.h>

// GraphException is an exception that can be thrown during the use of the graph.
class GraphException {
public:
    GraphException(const std::string &message) : message{message} {}

    inline const std::string &getMessage() const { return message; }

private:
    const std::string message;
};

// NodeCodes are used when exporting nodes to the binary export format.
enum class NodeCode {FalseNode, TrueNode, LitNode, OrNode, AndNode};

class Node {
public:
    // printNNF prints on "out" a d-DNNF version of node, provided the lines of
    // potential children.
    virtual void printNNF(std::ostream& out, std::map<const Node*,int>& nodeToLine) const = 0;

    // returns nb of models that match the given weights. Already calculated nodes' values are stored into cache.
    virtual mpq_class modelCount(const WeightVector& weights, std::map<std::shared_ptr<Node>, mpq_class>& cache) const = 0;

    // validModel returns one model valid for the given partial model,
    // or nullptr if no valid model can be found.
    virtual std::unique_ptr<Model> validModel(const Model& partialModel) const = 0;

    virtual int nbDescendants() const = 0;

    virtual std::string toString() = 0;

    // The set of variables that are seen either in that node or in its children.
    virtual const std::set<int>& seenVars() const = 0;
};

// The empty set of variables, used in FalseNode and TrueNode.
const static std::set<int> emptySet{};

// FalseNode is the bottom node.
class FalseNode : public Node {
public:
    virtual mpq_class modelCount(const WeightVector& weights, std::map<std::shared_ptr<Node>, mpq_class>& cache) const override {
        return 0_mpq;
    }

    virtual std::unique_ptr<Model> validModel(const Model& partialModel) const override { return nullptr; }

    virtual void printNNF(std::ostream& out, std::map<const Node*,int>& nodeToLine) const override { out << "O 0 0" << std::endl; }

    inline int nbDescendants() const override {return 0;}

    inline const std::set<int>& seenVars() const override { return emptySet; }

    inline std::string toString() override {return "[FalseNode]\n";};
};

// TrueNode is the top node.
class TrueNode : public Node {
public:
    virtual mpq_class modelCount(const WeightVector& weights, std::map<std::shared_ptr<Node>, mpq_class>& cache) const override {
        return 1_mpq;
    }

    virtual std::unique_ptr<Model> validModel(const Model& partialModel) const override {
        return std::make_unique<Model>(partialModel);
    }

    virtual void printNNF(std::ostream& out, std::map<const Node*,int>& nodeToLine) const override { out << "A 0" << std::endl; }

    inline int nbDescendants() const override {return 0;}

    inline const std::set<int>& seenVars() const override { return emptySet; }

    inline std::string toString() override {return "[TrueNode]\n";};
};

// LitNode is a node containing a single literal.
// It is only used during parsing, since literals are considered unitLits
// withind AndNodes and OrNodes.
class LitNode : public Node {
public:
    LitNode(int lit) : lit{lit}, seenVar({abs(lit)}) { }

    virtual mpq_class modelCount(const WeightVector& weights, std::map<std::shared_ptr<Node>, mpq_class>& cache) const override {
        return weights.weightFor(lit);
    }

    virtual std::unique_ptr<Model> validModel(const Model& partialModel) const override;

    virtual void printNNF(std::ostream& out, std::map<const Node*,int>& nodeToLine) const override { out << "L " << lit << std::endl; }

    const int lit;
    const std::set<int> seenVar;

    inline int nbDescendants() const override {return 0;}

    inline const std::set<int>& seenVars() const override { return seenVar; }

  inline std::string toString() override {return "[LitNode lit="+ std::to_string(lit) +"]\n";};
};

// OrBranch is a branch in an OrNode.
struct OrBranch {
    std::vector<int> unitLits;
    std::shared_ptr<Node> child;
};

// OrNode is a decision, disjunction node.
class OrNode : public Node {
public:
    OrNode(int variable) : variable{variable} {
      branches[0].child = std::make_shared<TrueNode>();
      branches[1].child = std::make_shared<TrueNode>();
    }

    virtual mpq_class modelCount(const WeightVector& weights, std::map<std::shared_ptr<Node>, mpq_class>& cache) const override;

    virtual std::unique_ptr<Model> validModel(const Model& partialModel) const override;

    virtual void printNNF(std::ostream& out, std::map<const Node*,int>& nodeToLine) const override;

    const int variable;                // The variable the decision is made on
    std::array<OrBranch, 2> branches;  // Each branch (branch is 0 if negative, 1 if positive)
    std::set<int> _seenVars;

    inline int nbDescendants() const override;

    inline const std::set<int>& seenVars() const override { return _seenVars; }

    inline std::string toString() override {
      std::string result = "[OrNode\nvar="
	+ std::to_string(variable) + "\n"
	+ "[branch0\n";
      for(int i=0; i<branches[0].unitLits.size(); ++i) result = result + "[unitLit "+std::to_string(branches[0].unitLits[i])+"]\n";
      result = result + branches[0].child->toString();
      result = result + "]\n";
      result = result + "[branch1\n";
      for(int i=0; i<branches[1].unitLits.size(); ++i) result = result + "[unitLit "+std::to_string(branches[1].unitLits[i])+"]\n";
      result = result + branches[1].child->toString();
      result = result + "]\n]\n";
      return result;
    }
};

// AndNode is a deterministic, conjunction node.
class AndNode : public Node {
public:
    virtual mpq_class modelCount(const WeightVector& weights, std::map<std::shared_ptr<Node>, mpq_class>& cache) const override;

    virtual std::unique_ptr<Model> validModel(const Model& partialModel) const override;

    virtual void printNNF(std::ostream& out, std::map<const Node*,int>& nodeToLine) const override;

    inline int nbDescendants() const override {
        int cpt = unitLits.size();
        for(int i=0; i<children.size(); ++i) {
            cpt += 1+children[i]->nbDescendants();
        }
        return cpt;
    }

    inline const std::set<int>& seenVars() const override { return _seenVars; }

    std::vector<std::shared_ptr<Node>> children;
    std::vector<int> unitLits;
    std::set<int> _seenVars;

    inline std::string toString() override {
      std::string result = "[AndNode\n";
      for(int i=0; i<unitLits.size(); ++i) result = result + "[unitLit "+std::to_string(unitLits[i])+"]\n";
      for(int i=0; i<children.size(); ++i) result = result + children[i]->toString();
      result = result+"]\n";
      return result;
    }
};

// condition modifies root and all its subtres according to partialModel.
void condition(std::shared_ptr<Node>& node, const Model& partialModel, std::set<std::shared_ptr<Node>>& cache);

// Index all nodes starting from 'node', so as to be able to export the whole set of nodes as a d-DNNF
// file.
// nodeToLine indicates at what line in output each node will appear.
// allNodes is the list of all nodes, in the order they should be written in the output.
// lineIndex is the current index in lines, i.e at which line the next node will be written.
// When calling this function at the root, lines 0 and 1 should be reserved for false and true nodes, respectively, so lineIndex should start at 2.
void indexNodes(const std::shared_ptr<Node>& node, std::map<const Node*,int>& nodeToLine, std::vector<const Node*>& allNodes, int& lineIndex);

static const std::shared_ptr<TrueNode> trueNode{std::make_shared<TrueNode>()};
static const std::shared_ptr<FalseNode> falseNode{std::make_shared<FalseNode>()};

#endif
