#include "model.hpp"

using namespace std;

vector<CompactModel> Model::expanded() const {
	vector<CompactModel> res;
	vector<Model> models{*this};
	size_t i = 0;
	while (i < models.size()) {
		const Model& m{models[i]};
		const int fVar = m.firstFreeVar();
		if (fVar == -1) {
			res.push_back(CompactModel{m});
			i++;
		} else {
			Model mToTrue{m};
			Model mToFalse{m};
			mToTrue.setBindingFor(fVar, Binding::True);
			mToFalse.setBindingFor(fVar, Binding::False);
			models[i] = mToTrue; // Not push_back: we replace current model with mToTrue
			models.push_back(mToFalse);
			// No increment since we must test on mToTrue
		}
	}
	return res;
}

ostream& operator<<(std::ostream& os, const Model& m) {
	os << "Model{";
	bool firstVar = true;
	for (size_t i = 0; i < m.content.size(); i++) {
		if (m.content[i] != Binding::Free) {
			if (firstVar) {
				firstVar = false;
			} else {
    			os << ", ";
    		}
    		if (m.content[i] == Binding::False) {
    			os << "-";
    		}
    		os << (static_cast<int>(i+1));
    	}
    }
    os << "}";
    return os;
}

ostream& operator<<(ostream& os, const CompactModel& m) {
	for (size_t i = 0; i < m.content.size(); i++) {
		os << static_cast<int>(m.content[i] ? i+1 : -i-1) << " ";
	}
	os << "0";
	return os;
}
