#ifndef __LIN_OBJ_FUNC_HPP__
#define __LIN_OBJ_FUNC_HPP__


#include <iostream>
#include <sstream>
#include <vector>

#include "graph.hpp"
#include "model.hpp"

/**
 * \class LinObjFunc
 * \brief A class handling linear objective functions.
 * \brief Provides minimization under d-DNNF constraints.
 *
 * \author Emmanuel Lonca - lonca@cril.fr
 */
class LinObjFunc {
  
public:
  /** 
   * \fn LinObjFunc(int nbVars)
   * \brief Builds a new empty (=0) linear objective function given the number of variables it is able to handle.
   *
   * \param nbVars the number of boolean variables
   */
  LinObjFunc(int nbVars);

  /**
   * \fn LinObjFunc(int nbVars, std::istream& in)
   * \brief Builds a new linear objective function given the number of variables it is able to handle and a stream in which the function is encoded.
   *
   * \param nbVars the number of boolean variables
   * \param in the input stream
   */
  LinObjFunc(int nbVars, std::istream& in);

  /**
   * \fn void set(int lit, int weight)
   * \brief Associates a weight with a literal.
   *
   * \param lit the literal
   * \param weight the weight
   */
  void set(int lit, int weight);

  /**
   * \fn inline int  get(int lit)
   * \brief Returns the weight associated with a literal.
   *
   * \param lit the literal
   *
   * \return the weight associated with the literal.
   */
  inline int get(int lit) {return weights[lit>0 ? (lit-1)<<1 : 1+((-lit-1)<<1)];}

  /**
   * \fn std::pair<int, Model> optimizeUnderConstraint(std::shared_ptr<Graph> g)
   * \brief Computes the minimal value of the function under a d-DNNF constraint.
   * \brief It Also computes a model of the constraint that give the function its minimal value.
   *
   * \param g the d-DNNF constraint
   *
   * \return a pair containing both the minimal value and a model of g that minimizes the function
   */
  std::pair<int, Model> optimizeUnderConstraint(std::shared_ptr<Graph> g);

  /**
   * \fn std::shared_ptr<Graph> keepBoundedWeightModels(std::shared_ptr<Graph> g, int weight)
   * \brief Remove the costly models from the graph.
   * \brief A model is said "costly" if it gives a higher value to the objective function thqat the one given as parameter.
   *
   * \param g the d-DNNF constraint
   * \param bound the maximal allowed value given to the objective function by a model to keep
   *
   * \return the transformed graph
   */
  std::shared_ptr<Graph> keepBoundedWeightModels(std::shared_ptr<Graph> g, int bound);

  
private:
  int minWeight();
  int minWeight(int lit);
  int minWeight(std::vector<int>& lits);

  std::unique_ptr<std::vector<int>> branchAssigns(OrBranch &b);
  std::unique_ptr<std::vector<int>> optimizeUnderConstraints(std::shared_ptr<Node> root);
  std::unique_ptr<std::vector<int>> optimizeUnderAndConstraint(std::shared_ptr<Node> root);
  std::unique_ptr<std::vector<int>> optimizeUnderOrConstraint(std::shared_ptr<Node> root);

  using VecNodePair = std::pair<std::shared_ptr<std::vector<int>>, std::shared_ptr<Node>>;
  VecNodePair makeVecNodePair(std::shared_ptr<std::vector<int>> vec, std::shared_ptr<Node> node);
  VecNodePair makeFalseNodePair();
  VecNodePair keepBoundedWeightModels(std::shared_ptr<Node> n, int bound);
  VecNodePair keepBoundedWeightModelsAndRooted(std::shared_ptr<Node> n, int bound);
  VecNodePair keepBoundedWeightModelsOrRooted(std::shared_ptr<Node> n, int bound);
  VecNodePair keepBoundedWeightModelsOrBranch(OrBranch &b, int bound);
  VecNodePair keepBoundedWeightModelsLitRooted(std::shared_ptr<Node> n, int bound);
  VecNodePair keepBoundedWeightModelsTrueRooted(std::shared_ptr<Node> n, int bound);
  VecNodePair keepBoundedWeightModelsFalseRooted(std::shared_ptr<Node> n, int bound);
  
  
private:
  // Each literal's weight.
  // Weight of literal l is at index 2*(l-1).
  // Weight of literal -l is at index 2*(l-1) + 1.
  // So, the vector contains weights of literals 1, -1, 2, -2, etc.
  std::vector<int> weights;
  
};


#endif
