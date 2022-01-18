#include "prompt.hpp"
#include "parser.hpp"
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

// Reads a partial model from fields.
// The first element of fields is ignored as it is supposed
// to be the command called.
// The read model is a list of literals, ended by an optional 0.
Model readPartialModel(int nbVars, const vector<string>& fields) {
    Model partialModel{nbVars};
    for (int i = 0; i < fields.size(); i++) {
        if (i == 0) {
            continue;
        }
        const string& field{fields[i]};
        const int lit = stoi(field);
        if (lit == 0) {
            break;
        }
        const int var = (lit > 0? lit: -lit);
        if (var > nbVars) {
            throw invalid_argument("invalid literal");
        }
        partialModel.setBindingFor(var, (lit > 0? Binding::True: Binding::False));
    }
    return partialModel;
}

Graph parseFromNNF(const string& path) {
    ifstream f;
    f.open(path);
    if (!f.is_open()) {
        throw ParserException{"ERROR: could not open file"};
    } else {
        return parseNNF(f);
    }
}

WeightVector parseFromWeights(const string& path, int nbVars) {
    ifstream f;
    f.open(path);
    if (!f.is_open()) {
        throw ParserException{"ERROR: could not open file"};
    } else {
        return parseWeights(f, nbVars);
    }
}

void parseMinimization(const std::shared_ptr<Graph> g, string path) {
  ifstream f;
  f.open(path);
  if (!f.is_open()) {
    throw ParserException{"ERROR: could not open file"};
  }
  LinObjFunc obj(g->nbVars, f);
  pair<int, Model> optModel = obj.optimizeUnderConstraint(g);
  cout << "o " << optModel.first << endl;
  cout << CompactModel{optModel.second} << endl;
}

std::shared_ptr<Graph> parseMinimizationAndCond(const std::shared_ptr<Graph> g, string path) {
  ifstream f;
  f.open(path);
  if (!f.is_open()) {
    throw ParserException{"ERROR: could not open file"};
  }
  LinObjFunc obj(g->nbVars, f);
  pair<int, Model> optModel = obj.optimizeUnderConstraint(g);
  cout << "o " << optModel.first << endl;
  cout << "c conditioning phase begins" << endl;
  std::shared_ptr<Graph> condResult = obj.keepBoundedWeightModels(g, optModel.first);
  return condResult;
  //cout << CompactModel{optModel.second} << endl;
}


void conditionGraph(std::shared_ptr<Graph> g, const vector<string>& fields) {
    if (g == nullptr) {
        cerr << "ERROR: load a graph first" << endl;
    }
    try {
        Model partialModel{readPartialModel(g->nbVars, fields)};
        g->conditionTo(partialModel);
    } catch (invalid_argument e) {
        cerr << "ERROR: invalid argument" << endl;
    }
}

void printModelCount(const shared_ptr<Graph>& g, const vector<string>& fields) {
    if (g == nullptr) {
        cerr << "ERROR: load a graph first" << endl;
        return;
    }
    try {
        Model partialModel{readPartialModel(g->nbVars, fields)};
        mpq_class mc = g->modelCount(partialModel);
        if (mc.get_den() == 1) {
            cout << mc << endl;
        } else {
            // This is required to avoid showing the result as a fraction.
            // The format %.Ff could be used insetad, to show a different but interesting output.
            gmp_printf("%Fe\n", mpf_class(mc).get_mpf_t());
        }
    } catch (invalid_argument e) {
        cerr << "ERROR: invalid argument" << endl;
    }
}

void printModel(const shared_ptr<Graph>& g, const vector<string>& fields) {
    if (g == nullptr) {
        cerr << "ERROR: load a graph first" << endl;
        return;
    }
    try {
        Model partialModel{readPartialModel(g->nbVars, fields)};
        auto m = g->validModel(partialModel);
        if (m == nullptr) {
            cout << "UNSAT" << endl;
        } else {
            cout << CompactModel{*m} << endl;
        }
    } catch (invalid_argument e) {
        cerr << "ERROR: invalid argument" << endl;
    }
}

void saveGraph(const shared_ptr<Graph>& g, const string& path) {
    if (g == nullptr) {
        cerr << "ERROR: load a graph first" << endl;
        return;
    }
    ofstream out{path};
    if (!out.is_open()) {
        throw ParserException{"ERROR: could not open file"};
    } else {
        g->print(out);
    }
}

void printHelp() {
    cout << "cond [partial model] - conditions the graph according to partial model" << endl;
    cout << "h - displays current help" << endl;
    cout << "help - displays current help" << endl;
    cout << "load filename - loads a graph from file" << endl;
    cout << "mc [partial model] - count models" << endl;
    cout << "min filename - minimize objective function in file under the graph" << endl;
    cout << "mintr filename - keep models that minimizes objective function in file only" << endl;
    cout << "model [partial model] - display a valid model, if any" << endl;
    cout << "nodes - display number of nodes" << endl;
    cout << "p - prints graph on standard output in d-DNNF format" << endl;
    cout << "q - quits program" << endl;
    cout << "store filename - saves graph in d-DNNF format in filename" << endl;
    cout << "vars - display number of vars" << endl;
    cout << "w filename - loads weighs from file" << endl;
}

void prompt() {
    shared_ptr<Graph> g{nullptr};
    string line;
    while (!cin.eof()) {
        cout << "> ";
        getline(cin, line);
        if (line == "") {
            continue;
        }
        if (line == "q") {
            return;
        }
        string buf;
        stringstream ss{line};
        vector<string> fields;
        while (ss >> buf) {
            fields.push_back(buf);
        }
        if (fields[0] == "mc") {
            printModelCount(g, fields);
        } else if (fields[0] == "model") {
            printModel(g, fields);
        } else if (fields[0] == "nodes") {
            if (g == nullptr) {
                cerr << "ERROR: load a graph first" << endl;
            } else {
                cout << g->nbNodes() << endl;
            }
        } else if (fields[0] == "vars") {
            if (g == nullptr) {
                cerr << "ERROR: load a graph first" << endl;
            } else {
                cout << g->nbVars << endl;
            }
        } else if (fields[0] == "load") {
            if (fields.size() != 2) {
                cerr << "ERROR: invalid call" << endl;
            } else {
                try {
                    g = make_shared<Graph>(parseFromNNF(fields[1]));
                } catch (ParserException e) {
                    cerr << e.getMessage() << endl;
                }
            }
        } else if (fields[0] == "w") {
            if (g == nullptr) {
                cerr << "ERROR: load a graph first" << endl;
            } else if (fields.size() != 2) {
                cerr << "ERROR: invalid call" << endl;
            } else {
                try {
                    g->setWeights(parseFromWeights(fields[1], g->nbVars));
                } catch (ParserException e) {
                    cerr << e.getMessage() << endl;
                }
            }
        } else if (fields[0] == "cond") {
            conditionGraph(g, fields);
        } else if (fields[0] == "h" || fields[0] == "help") {
            printHelp();
        } else if (fields[0] == "p") {
            if (g == nullptr) {
                cerr << "ERROR: load a graph first" << endl;
            } else {
                g->print(cout);
            }
        } else if (fields[0] == "store") {
            if (fields.size() != 2) {
                cerr << "ERROR: invalid call" << endl;
            } else {
                try {
                    saveGraph(g, fields[1]);
                } catch (GraphException e) {
                    cerr << e.getMessage() << endl;
                }
            }
	} else if (fields[0] == "min") {
	  if(g == nullptr) {
	    cerr << "ERROR: no graph loaded" << endl;
	  } else {
	    parseMinimization(g, fields[1]);
	  }
        } else if (fields[0] == "mintr") {
	  if(g == nullptr) {
	    cerr << "ERROR: no graph loaded" << endl;
	  } else {
	    g = parseMinimizationAndCond(g, fields[1]);
	  }
        } else {
            cerr << "Invalid command" << endl;
        }
    }
}
