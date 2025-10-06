#ifndef TUNE_H
#define TUNE_H

#ifdef TUNE

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct TunableParam {
    std::string name;
    int default_value;
    int curr_value;
    int min_value;
    int max_value;
    float cend;
    float rend;

    TunableParam() = delete;
    TunableParam(std::string name, int default_value, int min_value, int max_value, float cend, float rend)
        : name(name),
          default_value(default_value),
          curr_value(default_value),
          min_value(min_value),
          max_value(max_value),
          cend(cend),
          rend(rend) {}
    ~TunableParam() = default;

    // This function print the TunableParam in the UCI option format
    void print() const {
        std::cout << "option name " << name << " type spin default " << default_value << " min " << min_value << " max "
                  << max_value << "\n";
    }

    // This function print the TunableParam in the OpenBench input format
    void print_ob_format() const {
        std::cout << name << ", int, " << curr_value << ", " << min_value << ", " << max_value << ", " << cend << ", "
                  << rend << "\n";
    }
};

// Some parts of this implementation were inspired by Stormphrax's tune code
class TunableParamList {
  public:
    static TunableParamList &get() {
        static TunableParamList tunable_param_list;
        return tunable_param_list;
    }

    auto begin() { return m_params.begin(); }
    auto end() { return m_params.end(); }

    inline const TunableParam &insert(TunableParam param) {
        if (m_params.size() == m_params.capacity()) {
            std::cerr << "Attempted to add an element to a full TunableParamList, resizing it would invalidate "
                         "existing references.\n";
            exit(1);
        }

        return m_params.emplace_back(param);
    }

    inline TunableParam *find(const std::string &param_name) {
        for (TunableParam &param : m_params) {
            if (param.name == param_name) {
                return &param;
            }
        }

        return nullptr;
    }

  private:
    TunableParamList() {
        m_params.reserve(128); // This should be way more than enough
    }
    ~TunableParamList() = default;

    // Delete copy and move operations to enforce Singleton behavior
    TunableParamList(const TunableParamList &) = delete;
    TunableParamList operator=(const TunableParamList &) = delete;
    TunableParamList(const TunableParamList &&) = delete;
    TunableParamList operator=(const TunableParamList &&) = delete;

    // This could be a hash table to save lookup time but this is fine for now
    std::vector<TunableParam> m_params;
};

#define TUNABLE_PARAM(name, default, min, max, cend, rend)                                                            \
    static_assert(cend >= 0.5);                                                                                       \
    inline const TunableParam &tuned_##name = TunableParamList::get().insert({#name, default, min, max, cend, rend}); \
    inline int name() { return tuned_##name.curr_value; }

#else

#define TUNABLE_PARAM(name, default, min, max, cend, rend) \
    constexpr int name() { return default; }

#endif // #ifdef TUNE

// Aspiration Windows
TUNABLE_PARAM(aw_min_depth, 4, 1, 10, 0.5, 0.002)
TUNABLE_PARAM(aw_first_window, 35, 5, 200, 10, 0.002)
TUNABLE_PARAM(aw_widening_factor, 49, 1, 100, 5, 0.002)

// Null move pruning
TUNABLE_PARAM(nmp_base_reduction, 4, 1, 5, 0.5, 0.002)
TUNABLE_PARAM(nmp_depth_reduction_divisor, 3, 2, 8, 0.5, 0.002)
TUNABLE_PARAM(nmp_min_depth, 3, 2, 8, 0.5, 0.002)

// Reverse Futility Pruning
TUNABLE_PARAM(rfp_margin, 103, 50, 150, 5, 0.002)
TUNABLE_PARAM(rfp_max_depth, 10, 5, 15, 0.5, 0.002)

// Late Move Reductions
TUNABLE_PARAM(lmr_base, 107, 50, 150, 5, 0.002)
TUNABLE_PARAM(lmr_divisor, 210, 150, 350, 10, 0.002)

// Late Moves Pruning
TUNABLE_PARAM(lmp_base, 120, 100, 200, 5, 0.002)
TUNABLE_PARAM(lmp_scale, 30, 130, 5, 0.5, 0.002)

// Interval Iterative Reduction
TUNABLE_PARAM(iir_min_depth, 4, 3, 8, 0.5, 0.002)
TUNABLE_PARAM(iir_depth_reduction, 1, 1, 4, 0.5, 0.002)

#endif // #ifndef TUNE_H
