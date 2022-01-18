#include <vector>

// A WeightVector is a vector that associates, for each literal from a graph,
// a numeric weight.
class WeightVector {
public:
    WeightVector(int nbVars): vec(nbVars * 2, 1) {}

    // returns the weight for the given literal.
    // lit should be a valid literal since no checking is done here.
    inline double weightFor(int lit) const {
        const int v = (lit > 0? lit: -lit);
        const int idx = (v-1) * 2 + static_cast<int>(lit < 0);
        return vec[idx];
    }

    // returns the weight for the given variable.
    // var should be a valid variable since no checking is done here.
    inline double weightForVar(int var) const {
        const int idx = (var-1) * 2;
        return vec[idx] + vec[idx + 1];
    }

    // sets the weight for the given literal.
    // lit should be a valid literal since no checking is done here.
    inline void setWeightFor(int lit, double weight) {
        const int v = (lit > 0? lit: -lit);
        const int idx = (v-1) * 2 + static_cast<int>(lit < 0);
        vec[idx] = weight;
    }

    inline size_t nbVars() const { return vec.size() / 2; }

private:
    // Each literal's weight.
    // Weight of literal l is at index 2*(l-1).
    // Weight of literal -l is at index 2*(l-1) + 1.
    // So, the vector contains weights of literals 1, -1, 2, -2, etc.
    std::vector<double> vec;
};
