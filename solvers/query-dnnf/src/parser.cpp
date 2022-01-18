#include "parser.hpp"
#include <sstream>
#include <iostream>
#include <map>

using namespace std;

inline void parseLit(vector<shared_ptr<Node>>& allNodes, vector<string>& fields) {
	// if (fields.size() < 2) {
	// 	throw ParserException{"Invalid literal"};
	// }
    allNodes.push_back(make_shared<LitNode>(stoi(fields[1])));
}

void parseAnd(vector<shared_ptr<Node>>& allNodes, vector<string>& fields) {
	const int size = stoi(fields[1]);
	// if (size != static_cast<int>(fields.size()) - 2) {
	// 	throw ParserException{"Invalid size for \"A\" node"};
	// }
	if (size == 0) {
		allNodes.push_back(trueNode);
	} else {
        shared_ptr<AndNode> andNode = make_shared<AndNode>();
        for (size_t i = 2; i < fields.size(); i++) {
            const int idx = stoi(fields[i]);
            shared_ptr<Node>& child{allNodes[idx]};
            if (typeid(*child) == typeid(LitNode)) {
                const int lit{static_cast<LitNode&>(*child).lit};
                andNode->unitLits.push_back(lit);
                andNode->_seenVars.insert(abs(lit));
            } else {
                andNode->children.push_back(child);
                const auto seen = child->seenVars();
                andNode->_seenVars.insert(seen.begin(), seen.end());

            }
        }
        allNodes.push_back(andNode);
    }
}

void parseOr(vector<shared_ptr<Node>>& allNodes, vector<string>& fields) {
	// if (fields.size() != 3 && fields.size() != 5) {
	// 	throw ParserException{"Invalid size for \"O\" node"};
	// }
	const int var = stoi(fields[1]);
	// const int size = stoi(fields[2]);
	// if ((var < 0) || (var == 0 && size != 0) || (var > 0 && size != 2)) {
	// 	throw ParserException{"Invalid constraints for \"O\" node"};
	// }
	if (var == 0) {
		allNodes.push_back(falseNode);
	} else {
        const int left = stoi(fields[3]);
        const int right = stoi(fields[4]);
        array<shared_ptr<Node>, 2> children{{allNodes[left], allNodes[right]}};
        shared_ptr<OrNode> orNode = make_shared<OrNode>(var);
        for (int i = 0; i < 2; i++) {
            orNode->branches[i].child = children[i];
            const auto seen = children[i]->seenVars();
            orNode->_seenVars.insert(seen.begin(), seen.end());

        }
        allNodes.push_back(orNode);
    }
}

// Actual parsing of "in"'s content.
pair<const int, shared_ptr<Node>> doParseNNF(istream& in) {
    string line;
    getline(in, line);
    string buf;
    stringstream ss{line};
    vector<string> headers;
    while (ss >> buf) {
        headers.push_back(buf);
    }
    if (headers.size() != 4 || headers[0] != "nnf") {
        throw ParserException{"Invalid NNF header"};
    }
    const int nbVars = stoi(headers[3]);
    const int nbNodes = stoi(headers[1]);
    vector<shared_ptr<Node>> allNodes;
    allNodes.reserve(nbNodes);
    cout << "I will parse a graph of " << nbVars << " variables and " << nbNodes << " nodes..." << endl;
    while (getline(in, line)) {
        stringstream ss2{line}; // Insert the string into a stream
        vector<string> fields;  // Create vector to hold our words
        while (ss2 >> buf) {
            fields.push_back(buf);
        }
	    if(fields.size() == 0) continue;
        switch (fields[0][0]) {
        case 'A':
            parseAnd(allNodes, fields);
            break;
        case 'O':
            parseOr(allNodes, fields);
            break;
        case 'L':
            parseLit(allNodes, fields);
            break;
        default:
            throw ParserException{"Invalid node type"};
        }
        if (allNodes.size() % 2000000 == 0) {
            cout << "Parsed " << (allNodes.size() / 1000000 )<< " million nodes yet..." << endl;
        }
    }
    cout << "Done, returning item now..." << endl;
    return make_pair<const int, shared_ptr<Node>>(move(nbVars), move(allNodes.back()));
}

Graph parseNNF(istream& in) {
    auto p = doParseNNF(in);
    return Graph{p.first, p.second};
}

WeightVector parseWeights(istream& in, int nbVars) {
    WeightVector wv{nbVars};
    string line;
    while (getline(in, line)) {
        stringstream ss2{line};
        vector<string> fields;
        string buf;
    	while (ss2 >> buf) {
            fields.push_back(buf);
        }
	if(fields.size() == 0) continue;
    	const int lit = stoi(fields.at(0));
        const double weight = stof(fields.at(1));
        wv.setWeightFor(lit, weight);
    }
    return wv;
}

Model parseModel(istream& in) {
	string line;
	vector<string> headers;
	getline(in, line);
	string buf;
	stringstream ss{line};
	while (ss >> buf) {
		headers.push_back(buf);
	}
	if (headers.size() != 2 || headers[0] != "model") {
		throw ParserException{"Invalid model header"};
	}
	Model m{stoi(headers[1])};
	getline(in, line);
	stringstream ss2{line};
	while (ss2 >> buf) {
		const int lit = stoi(buf);
		const int var = lit > 0 ? lit : -lit;
		m.setBindingFor(var, static_cast<Binding>(lit > 0));
	}
	return m;
}
