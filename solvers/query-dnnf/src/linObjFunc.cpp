#include "linObjFunc.hpp"


LinObjFunc::LinObjFunc(int nbVars) : weights(nbVars<<1, 0) {}


LinObjFunc::LinObjFunc(int nbVars, std::istream& in) : LinObjFunc(nbVars) {
  std::string line;
  while(getline(in, line)) {
    std::stringstream ss{line};
    std::vector<std::string> fields;
    std::string buf;
    while(ss >> buf) fields.push_back(buf);
    if(!fields.size()) continue;
    if(fields.size() != 2) {
      std::cerr << "warning: ignoring malformed line \"" << line << "\"" << std::endl;
      continue;
    }
    set(stoi(fields[0]), stoi(fields[1]));
  }
}


void LinObjFunc::set(int lit, int weight) {
  weights[lit>0 ? (lit-1)<<1 : 1+((-lit-1)<<1)] = weight;
}


std::pair<int, Model> LinObjFunc::optimizeUnderConstraint(std::shared_ptr<Graph> g) {
  int nbVars = (int)weights.size()>>1;
  Model model(nbVars);
  std::vector<bool> assigned(nbVars, false);
  std::unique_ptr<std::vector<int>> assigns = optimizeUnderConstraints(g->root);
  int value = 0;
  for(int i=0; i<assigns->size(); ++i) {
    int val = (*assigns)[i];
    value += get(val);
    int var = val>0 ? val : -val;
    assigned[var-1] = true;
    model.setBindingFor(var, val<0 ? Binding::False : Binding::True);
  }
  for(int i=0; i<assigned.size(); ++i) {
    if(!assigned[i]) {
      int valPos = get(i+1);
      int valNeg = get(-i-1);
      value += valPos>valNeg ? valNeg : valPos;
      model.setBindingFor(i+1, valPos>valNeg ? Binding::False : Binding::True);
    }
  }
  return std::make_pair<const int, Model>(std::move(value), std::move(model));
}


std::unique_ptr<std::vector<int>> LinObjFunc::optimizeUnderConstraints(std::shared_ptr<Node> root) {
  if(typeid(*root) == typeid(AndNode)) return optimizeUnderAndConstraint(root);
  else if(typeid(*root) == typeid(OrNode)) return optimizeUnderOrConstraint(root);
  else if(typeid(*root) == typeid(LitNode)) {
    LitNode& l = (LitNode&)*root;
    std::unique_ptr<std::vector<int>> res = std::make_unique<std::vector<int>>();
    res->push_back(l.lit);
    return res;
  }
  else if(typeid(*root) == typeid(TrueNode)) return std::unique_ptr<std::vector<int>>(new std::vector<int>);
  else if(typeid(*root) == typeid(FalseNode)) return nullptr;
  else return nullptr;
}


std::unique_ptr<std::vector<int>> LinObjFunc::optimizeUnderAndConstraint(std::shared_ptr<Node> root) {
  std::unique_ptr<std::vector<int>> assigns(new std::vector<int>);
  AndNode& n = (AndNode&)*root;
  std::vector<int> units = n.unitLits;
  for(int i=0; i<units.size(); ++i) {
    assigns->push_back(units[i]);
  }
  std::vector<std::shared_ptr<Node>> children = n.children;
  for(int i=0; i<children.size(); ++i) {
    std::unique_ptr<std::vector<int>> pChildAssigns = optimizeUnderConstraints(children[i]);
    if(pChildAssigns == nullptr) return nullptr;
    std::vector<int> childAssigns = *pChildAssigns;
    for(int j=0; j<childAssigns.size(); ++j) {
      assigns->push_back(childAssigns[j]);
    }
  }
  return assigns;
}


int LinObjFunc::minWeight() {
  std::vector<int> emptyVec;
  return minWeight(emptyVec);
}


int LinObjFunc::minWeight(int lit) {
  std::vector<int> vec;
  vec.push_back(lit);
  return minWeight(vec);
}


int LinObjFunc::minWeight(std::vector<int>& lits) {
  int nbVars = (int)weights.size()>>1;
  std::vector<bool> assigned(nbVars, false);
  int value = 0;
  for(int i=0; i<lits.size(); ++i) {
    int val = lits[i];
    value += get(val);
    assigned[val>0 ? val-1 : -val-1] = true;
  }
  for(int i=0; i<assigned.size(); ++i) {
    if(!assigned[i]) {
      int valPos = get(i+1);
      int valNeg = get(-i-1);
      value += valNeg > valPos ? valPos : valNeg;
    }
  }
  return value;
}


std::unique_ptr<std::vector<int>> LinObjFunc::branchAssigns(OrBranch &b) {
  std::unique_ptr<std::vector<int>> assigns(new std::vector<int>);
  for(int i=0; i<b.unitLits.size(); ++i) {
    assigns->push_back(b.unitLits[i]);
  }
  std::unique_ptr<std::vector<int>> childAssigns = optimizeUnderConstraints(b.child);
  if(childAssigns != nullptr) {
    for(int i=0; i<childAssigns->size(); ++i) {
      assigns->push_back(childAssigns->at(i));
    }
  }
  return assigns;
}


std::unique_ptr<std::vector<int>> LinObjFunc::optimizeUnderOrConstraint(std::shared_ptr<Node> root) {
  OrNode& o = (OrNode&)*root;
  std::unique_ptr<std::vector<int>> assigns0 = branchAssigns(o.branches[0]);
  std::unique_ptr<std::vector<int>> assigns1 = branchAssigns(o.branches[1]);
  return minWeight(*assigns0) <= minWeight(*assigns1) ? std::move(assigns0) : std::move(assigns1);
}


std::shared_ptr<Graph> LinObjFunc::keepBoundedWeightModels(std::shared_ptr<Graph> g, int bound) {
  int nbVars = (int)weights.size()>>1;
  VecNodePair newRoot = keepBoundedWeightModels(g->root, bound);
  if(newRoot.second == nullptr) return nullptr;
  return std::make_shared<Graph>(nbVars, newRoot.second);
}


LinObjFunc::VecNodePair LinObjFunc::makeVecNodePair(std::shared_ptr<std::vector<int>> vec, std::shared_ptr<Node> node) {
  return std::make_pair<std::shared_ptr<std::vector<int>>, std::shared_ptr<Node>>(std::move(vec), std::move(node));
}


LinObjFunc::VecNodePair LinObjFunc::makeFalseNodePair() {
  std::vector<int> emptyVec;
  return makeVecNodePair(std::make_shared<std::vector<int>>(emptyVec), falseNode);
}
int nodeCount = 0;

LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModels(std::shared_ptr<Node> n, int bound) {
  int nIndex=++nodeCount;
  VecNodePair v;
  if(typeid(*n) == typeid(AndNode)) v = keepBoundedWeightModelsAndRooted(n, bound);
  else if(typeid(*n) == typeid(OrNode)) v = keepBoundedWeightModelsOrRooted(n, bound);
  else if(typeid(*n) == typeid(LitNode)) v = keepBoundedWeightModelsLitRooted(n, bound);
  else if(typeid(*n) == typeid(TrueNode)) v = keepBoundedWeightModelsTrueRooted(n, bound);
  else if(typeid(*n) == typeid(FalseNode)) v = keepBoundedWeightModelsFalseRooted(n, bound);
  else v = makeVecNodePair(nullptr, nullptr);
  return v;
}


LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModelsAndRooted(std::shared_ptr<Node> root, int bound) {
  AndNode& n = (AndNode&)*root;
  std::vector<int> assigns;
  std::shared_ptr<AndNode> newAndNode = std::make_shared<AndNode>();
  std::vector<int> units = n.unitLits;
  for(int i=0; i<units.size(); ++i) {
    assigns.push_back(units[i]);
    newAndNode->unitLits.push_back(units[i]);
  }
  std::vector<std::shared_ptr<Node>> children = n.children;
  for(int i=0; i<children.size(); ++i) {
    VecNodePair child = keepBoundedWeightModels(children[i], bound);
    if(typeid(*child.second) == typeid(FalseNode)) {
      return makeFalseNodePair();
    }
    std::shared_ptr<std::vector<int>> childAssigns = child.first;
    for(int j=0; j<childAssigns->size(); ++j) {
      assigns.push_back(childAssigns->at(j));
    }
    newAndNode->children.push_back(child.second);
  }
  int minW = minWeight(assigns);
  if(minW <= bound) {
    return makeVecNodePair(std::make_shared<std::vector<int>>(assigns), newAndNode);
  }
  return makeFalseNodePair();
}


LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModelsOrBranch(OrBranch &b, int bound) {
  int nIndex=++nodeCount;
  VecNodePair p;
  if(typeid(*b.child) == typeid(LitNode)) {
    p = keepBoundedWeightModelsLitRooted(b.child, bound);
  } else if(typeid(*b.child) == typeid(TrueNode)) {
    p = keepBoundedWeightModelsTrueRooted(b.child, bound);
  } else if(typeid(*b.child) == typeid(FalseNode)) {
    p = keepBoundedWeightModelsFalseRooted(b.child, bound);
  } else if(typeid(*b.child) == typeid(OrNode)) {
    p = keepBoundedWeightModelsOrRooted(b.child, bound);
  } else if(typeid(*b.child) == typeid(AndNode)) {
    p = keepBoundedWeightModelsAndRooted(b.child, bound);
  } else {
    return makeFalseNodePair();
  }
  if(b.unitLits.size() > 0) {
    std::shared_ptr<AndNode> n = std::make_shared<AndNode>();
    n->children.push_back(p.second);
    for(int i=0; i<b.unitLits.size(); ++i) {
      n->unitLits.push_back(b.unitLits[i]);
      p.first->push_back(b.unitLits[i]);
    }
    p.second = n;
  }
  if(minWeight(*p.first) > bound) {
    p = makeFalseNodePair();
  }
  return p;
}


LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModelsOrRooted(std::shared_ptr<Node> root, int bound) {
  OrNode& o = (OrNode&)*root;
  VecNodePair p0 = keepBoundedWeightModelsOrBranch(o.branches[0], bound);
  VecNodePair p1 = keepBoundedWeightModelsOrBranch(o.branches[1], bound);
  int mw0 = minWeight(*p0.first);
  int mw1 = minWeight(*p1.first);
  if(typeid(*p0.second) == typeid(FalseNode)) {
    return p1;
  }
  if(typeid(*p1.second) == typeid(FalseNode)) {
    return p0;
  }
  if(mw0 > mw1) {
    return p1;
  }
  if(mw1 > mw0) {
    return p0;
  }
  std::shared_ptr<OrNode> newOrNode = std::make_shared<OrNode>(o.variable);
  newOrNode->branches[0].child = p0.second;
  newOrNode->branches[1].child = p1.second;
  return makeVecNodePair(p0.first, newOrNode);
}


LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModelsTrueRooted(std::shared_ptr<Node> root, int bound) {
  if(minWeight() <= bound) {
    std::vector<int> emptyVec;
    return makeVecNodePair(std::make_shared<std::vector<int>>(emptyVec), trueNode);
  }
  return makeFalseNodePair();
}


LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModelsFalseRooted(std::shared_ptr<Node> root, int bound) {
  return makeFalseNodePair();
}


LinObjFunc::VecNodePair LinObjFunc::keepBoundedWeightModelsLitRooted(std::shared_ptr<Node> n, int bound) {
  LitNode& l = (LitNode&)*n;
  int minW = minWeight(l.lit);
  std::vector<int> litVec;
  litVec.push_back(l.lit);
  return minW <= bound
    ? makeVecNodePair(std::make_shared<std::vector<int>>(litVec), std::make_shared<LitNode>(l.lit))
    : makeFalseNodePair();
}
