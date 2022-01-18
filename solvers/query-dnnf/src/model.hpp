#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <cstdlib>
#include <iostream>

// Binding: a boolean variable binding.
// Unsat means there's a contradiction: variable is both true and false.
enum class Binding {False, True, Free, Unsat};

class CompactModel;

// A model is a list of bindings.
// Its size should be equivalent to the size of the original formula.
// It can be partial, i.e have Free variables.
class Model {
public:
	Model(int nbVars): content(nbVars, Binding::Free) {}

	inline int nbVars() const noexcept { return static_cast<int>(content.size()); }

	friend std::ostream& operator<<(std::ostream& os, const Model& m);

	inline Binding bindingFor(int var) const noexcept { return content[var-1]; }

	inline void setBindingFor(int var, Binding binding) noexcept {
		const Binding old = content[var-1];
		if ((old == Binding::True && binding == Binding::False) || (old == Binding::False && binding == Binding::True)) {
			content[var-1] = Binding::Unsat;
		} else {
			content[var-1] = binding;
		}
	}

	// hasFreeVars returns true iff model has at least one unbounded variable.
	inline bool hasFreeVars() const noexcept {
		for (Binding b: content) {
			if (b == Binding::Free) {
				return true;
			}
		}
		return false;
	}

	// firstFreeVar returns the first (in numeric order) var bound to Free.
	// If all vars are bound, returns -1.
	inline int firstFreeVar() const noexcept {
		for (size_t i = 0; i < content.size(); i++) {
			if (content[i] == Binding::Free) {
				return static_cast<int>(i) + 1;
			}
		}
		return -1;
	}

	// Mixes current model with bindings from m1 and m2 into mOut.
	// If m1 and m2 are not compatible (i.e binding for any var is true in m1, false in m2, or the opposite),
	// mOut becomes invalid and false is returned.
	// The same argument can be used for m1, m2 and/or mOut.
	inline static bool mix(const Model& m1, const Model& m2, Model& mOut) noexcept {
		for (size_t i = 0; i < m1.content.size(); i++) {
			const Binding b1{m1.content[i]};
			const Binding b2{m2.content[i]};
			if (b1 == Binding::Free) {
				mOut.content[i] = b2;
			} else if (b2 == Binding::Free || b1 == b2) {
				mOut.content[i] = b1;
			} else {
				return false;
			}
		}
		return true;
	}

	// expanded returns the list of all models that conform to this but
	// have no free vars.
	// This function will return 2**n models, where n is the number of free vars within current model.
	// Note this can be huge and shall only be called on models with a very limited number of free vars.
	std::vector<CompactModel> expanded() const;

private:
	std::vector<Binding> content;
};

// A CompactModel only holds bound variables. It is thus much more compact than a regular Model.
class CompactModel {
public:
	// Constructs a CompactModel of nbVars vars all bound to false
	inline CompactModel(int nbVars): content(nbVars, false) {}

	// Constructs a CompactModel from a Model.
	// Free variables from m will be bound a value.
	inline CompactModel(const Model& m): content(m.nbVars()) {
		for (size_t i = 0; i < content.size(); i++) {
			content[i] = m.bindingFor(static_cast<int>(i)+1) == Binding::True;
		}
	}

    friend std::ostream& operator<<(std::ostream& os, const CompactModel& m);

	inline bool hasFreeVars() const noexcept { return false; }

	inline int firstFreeVars() const noexcept { return -1; }

    inline Binding bindingFor(int var) const noexcept { return content[var-1] ? Binding::True : Binding::False ; }

private:
	std::vector<bool> content;
};

#endif
