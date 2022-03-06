#pragma once
#include <iostream>
#include <fstream>
#include <algorithm>
#include "setting.hpp"
#include "common.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "transpose_table.hpp"

using namespace std;

#define MID_MPC_MIN_DEPTH 2
#define MID_MPC_MAX_DEPTH 25
#define END_MPC_MIN_DEPTH 5
#define END_MPC_MAX_DEPTH 40
#define MPC_MPCT 1.0

#define MID_FAST_DEPTH 3
#define END_FAST_DEPTH 7
#define MID_TO_END_DEPTH 12

#define SCORE_UNDEFINED -INF

constexpr int cell_weight[HW2] = {
    18,  4,  16, 12, 12, 16,  4, 18,
     4,  2,   6,  8,  8,  6,  2,  4,
    16,  6,  14, 10, 10, 14,  6, 16,
    12,  8,  10,  0,  0, 10,  8, 12,
    12,  8,  10,  0,  0, 10,  8, 12,
    16,  6,  14, 10, 10, 14,  6, 16,
     4,  2,   6,  8,  8,  6,  2,  4,
    18,  4,  16, 12, 12, 16,  4, 18
};

constexpr int mpcd[41] = {
    0, 1, 0, 1, 2, 3, 2, 3, 4, 3, 
    4, 3, 4, 5, 4, 5, 6, 5, 6, 7, 
    6, 7, 6, 7, 8, 7, 8, 9, 8, 9, 
    10, 9, 10, 11, 12, 11, 12, 13, 14, 13, 
    14
};

constexpr double mpcsd[N_PHASES][MID_MPC_MAX_DEPTH - MID_MPC_MIN_DEPTH + 1]={
{0.512, 1.026, 1.0, 1.026, 1.0, 0.513, 1.026, 1.0, 1.539, 1.535, 1.026, 1.024, 0.513, 0.512, 0.513, 0.513, 0.5, 0.5, 0.5, 0.513, 0.512, 0.513, 1.539, 0.512},
{0.839, 1.24, 0.61, 1.0, 1.049, 0.999, 1.182, 1.916, 0.988, 0.907, 1.861, 1.543, 0.51, 0.839, 1.433, 0.945, 1.457, 0.979, 1.297, 0.826, 1.309, 0.734, 1.669, 0.953},
{1.155, 4.204, 1.335, 0.489, 1.532, 2.51, 0.933, 1.241, 0.968, 1.32, 1.75, 2.329, 1.276, 0.853, 1.725, 1.281, 1.478, 2.254, 1.798, 2.174, 2.012, 2.671, 1.447, 2.748},
{1.729, 3.79, 0.941, 2.118, 5.734, 1.555, 1.777, 1.224, 1.322, 1.129, 2.142, 2.045, 2.395, 1.861, 1.627, 1.954, 1.902, 1.465, 1.618, 1.86, 1.395, 1.217, 0.999, 1.445},
{3.303, 2.653, 1.814, 1.146, 2.845, 2.278, 1.372, 3.571, 1.75, 2.384, 1.731, 1.563, 1.542, 1.852, 1.174, 1.847, 1.368, 0.951, 1.177, 1.089, 1.261, 1.046, 0.979, 1.125},
{3.028, 3.633, 1.626, 1.765, 2.585, 2.202, 1.517, 1.552, 1.603, 1.72, 1.609, 1.269, 1.559, 1.235, 1.11, 1.447, 1.916, 1.572, 1.5, 1.21, 1.508, 1.726, 0.801, 1.231},
{2.742, 2.796, 1.751, 1.593, 1.981, 1.81, 1.41, 2.181, 0.923, 1.944, 2.145, 1.232, 1.86, 1.461, 1.253, 1.694, 1.46, 2.231, 1.553, 1.141, 1.473, 2.189, 1.031, 1.588},
{3.39, 2.007, 1.535, 1.581, 4.128, 1.733, 1.277, 2.013, 1.804, 2.665, 2.405, 1.991, 1.504, 1.572, 1.615, 1.536, 2.196, 1.257, 1.683, 1.356, 1.86, 1.323, 1.605, 1.836},
{2.416, 2.305, 1.878, 1.891, 3.22, 1.751, 1.732, 2.599, 1.553, 2.268, 1.609, 2.366, 1.289, 2.291, 2.056, 1.501, 1.859, 1.04, 1.849, 1.478, 1.731, 1.601, 1.317, 1.399},
{2.177, 2.498, 1.82, 1.781, 2.282, 1.925, 1.917, 3.131, 1.234, 1.483, 1.893, 1.49, 2.952, 1.701, 1.362, 1.791, 1.495, 1.432, 1.785, 1.676, 1.572, 1.68, 1.418, 1.095},
{2.861, 4.089, 1.761, 2.056, 3.12, 1.699, 2.314, 3.492, 1.991, 2.139, 1.453, 1.732, 3.745, 1.49, 1.364, 1.395, 1.991, 1.605, 2.634, 1.29, 1.338, 1.852, 2.183, 1.565},
{3.104, 3.861, 1.959, 1.736, 4.758, 2.404, 2.259, 4.077, 1.711, 2.929, 1.961, 1.373, 1.812, 3.103, 1.386, 2.118, 1.785, 2.793, 1.947, 1.797, 1.268, 1.814, 2.042, 1.322},
{2.427, 3.122, 2.212, 1.579, 4.736, 1.932, 1.701, 4.332, 2.383, 1.944, 1.555, 1.731, 2.196, 2.3, 1.444, 1.75, 2.186, 1.819, 2.317, 2.1, 2.164, 3.161, 1.804, 2.621},
{2.982, 2.741, 2.408, 1.84, 2.36, 2.475, 2.059, 2.523, 2.396, 2.188, 1.402, 2.732, 3.015, 2.013, 2.287, 1.777, 1.774, 1.182, 1.619, 2.251, 1.424, 2.189, 2.228, 2.212},
{1.729, 3.2, 2.234, 2.009, 1.622, 2.686, 1.606, 2.961, 2.193, 2.164, 2.404, 1.971, 3.169, 1.701, 2.599, 2.139, 4.315, 2.202, 2.118, 1.761, 1.552, 2.212, 2.919, 1.701},
{2.789, 2.15, 2.215, 1.766, 2.368, 1.997, 1.677, 3.617, 2.069, 2.534, 2.062, 2.762, 2.06, 1.85, 2.137, 2.114, 2.991, 1.786, 2.186, 2.085, 5.326, 2.479, 2.336, 2.331},
{3.782, 2.374, 2.618, 1.959, 2.585, 2.523, 2.045, 2.371, 2.572, 2.56, 2.153, 3.395, 2.571, 1.905, 1.959, 1.845, 2.088, 2.319, 2.564, 1.845, 2.235, 1.905, 1.651, 3.066},
{2.726, 2.356, 3.977, 1.708, 3.775, 2.074, 2.097, 3.13, 3.007, 2.837, 3.304, 2.349, 2.036, 3.488, 2.062, 3.363, 3.242, 2.329, 2.523, 2.239, 2.925, 3.034, 2.624, 2.014},
{3.045, 2.543, 2.074, 2.253, 2.656, 2.519, 1.79, 2.371, 2.26, 2.668, 2.585, 3.014, 2.965, 2.221, 2.534, 2.492, 3.935, 3.332, 1.642, 2.482, 4.533, 2.989, 0.0, 0.0},
{3.117, 3.257, 2.452, 1.652, 2.328, 2.484, 2.013, 2.438, 2.175, 2.56, 2.548, 2.593, 3.333, 2.838, 2.606, 3.152, 3.588, 3.116, 3.291, 3.035, 0.0, 0.0, 0.0, 0.0},
{2.434, 3.007, 3.407, 2.114, 2.781, 2.585, 3.342, 2.858, 2.486, 2.594, 4.493, 3.034, 3.477, 3.364, 2.618, 4.212, 3.886, 3.27, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{4.221, 2.938, 2.1, 1.849, 3.776, 2.892, 2.627, 3.924, 2.771, 3.283, 3.596, 3.468, 3.832, 4.429, 3.11, 3.899, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{2.827, 3.474, 2.972, 1.861, 4.032, 3.252, 2.627, 1.875, 2.857, 3.776, 4.118, 2.943, 3.96, 5.605, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{3.789, 4.363, 3.392, 2.873, 3.258, 3.496, 5.082, 4.774, 3.205, 4.135, 4.883, 2.953, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{3.202, 3.528, 4.378, 3.252, 6.536, 3.859, 6.849, 5.996, 4.419, 3.009, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{3.993, 3.576, 3.051, 2.732, 4.434, 5.381, 5.202, 5.087, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{4.06, 3.988, 3.569, 3.886, 7.146, 4.377, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{3.556, 6.732, 6.214, 3.836, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{4.571, 4.243, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};

constexpr int mpcd_final[41] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 
    4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 
    6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 
    9
};

constexpr double mpcsd_final[END_MPC_MAX_DEPTH - END_MPC_MIN_DEPTH + 1] = {
    4.627, 4.714, 5.038, 5.266, 5.647, 5.41, 5.503, 5.429, 5.268, 4.996, 5.22, 5.105, 5.268, 5.007, 4.744, 4.552, 4.566, 4.468, 4.323, 4.396, 4.397, 4.291, 4.18, 4.564, 4.666, 4.718, 5.475, 5.97, 6.471, 6.83, 7.18, 7.648, 8.564, 8.987, 9.374, 9.742
};

unsigned long long can_be_flipped[HW2];

struct Search_result{
    int policy;
    int value;
    int depth;
    int nps;
    uint64_t nodes;
};

struct Search{
    Board board;
    bool use_mpc;
    double mpct;
    uint64_t n_nodes;
};

inline int stability_cut(Search *search, int *alpha, int *beta){
    int stab_player, stab_opponent;
    calc_stability(&search->board, &stab_player, &stab_opponent);
    int n_alpha = 2 * stab_player - HW2;
    int n_beta = HW2 - 2 * stab_opponent;
    if (*beta <= n_alpha)
        return n_alpha;
    if (n_beta <= *alpha)
        return n_beta;
    if (n_beta <= n_alpha)
        return n_alpha;
    *alpha = max(*alpha, n_alpha);
    *beta = min(*beta, n_beta);
    return SCORE_UNDEFINED;
}
