/*
    Egaroucid Project

    @date 2021-2022
    @author Takuto Yamana (a.k.a Nyanyan)
    @license GPL-3.0 license
*/

#pragma once
#include <iostream>
#include "setting.hpp"
#include "common.hpp"
#include "transpose_table.hpp"
#include "search.hpp"
#include "midsearch.hpp"
#include "endsearch.hpp"
#include "parallel.hpp"
#include "thread_pool.hpp"

#define YBWC_MID_SPLIT_MIN_DEPTH 6
//#define YBWC_REMAIN_TASKS 1

int nega_alpha_ordering(Search *search, int alpha, int beta, int depth, bool skipped, uint64_t legal, bool is_end_search, const bool *searching);
int nega_alpha_end(Search *search, int alpha, int beta, bool skipped, uint64_t legal, const bool *searching);
int nega_scout(Search *search, int alpha, int beta, int depth, bool skipped, uint64_t legal, bool is_end_search, const bool *searching);

Parallel_task ybwc_do_task(int id, uint64_t player, uint64_t opponent, int_fast8_t n_discs, uint_fast8_t parity, bool use_mpc, double mpct, int first_depth, int alpha, int beta, int depth, uint64_t legal, bool is_end_search, uint_fast8_t policy, const bool *searching){
    Search search;
    search.board.player = player;
    search.board.opponent = opponent;
    search.n_discs = n_discs;
    search.parity = parity;
    search.use_mpc = use_mpc;
    search.mpct = mpct;
    search.first_depth = first_depth;
    search.n_nodes = 0ULL;
    calc_features(&search);
    int g = -nega_alpha_ordering(&search, alpha, beta, depth, false, legal, is_end_search, searching);
    Parallel_task task;
    if (*searching)
        task.value = g;
    else
        task.value = SCORE_UNDEFINED;
    task.n_nodes = search.n_nodes;
    task.cell = policy;
    return task;
}

inline bool ybwc_split(const Search *search, const Flip *flip, int alpha, int beta, int depth, uint64_t legal, bool is_end_search, const bool *searching, uint_fast8_t policy, const int canput, const int pv_idx, const int split_count, vector<future<Parallel_task>> &parallel_tasks){
    if (thread_pool.n_idle() &&
        pv_idx > 0 && 
        depth >= YBWC_MID_SPLIT_MIN_DEPTH){
            bool pushed;
            parallel_tasks.emplace_back(thread_pool.push(&pushed, &ybwc_do_task, search->board.player, search->board.opponent, search->n_discs, search->parity, search->use_mpc, search->mpct, search->first_depth, alpha, beta, depth, legal, is_end_search, policy, searching));
            if (!pushed)
                parallel_tasks.pop_back();
            return pushed;
    }
    return false;
}

inline void ybwc_get_end_tasks(Search *search, vector<future<Parallel_task>> &parallel_tasks, int *v, int *best_move, int *alpha){
    Parallel_task got_task;
    for (future<Parallel_task> &task: parallel_tasks){
        if (task.valid()){
            if (task.wait_for(chrono::seconds(0)) == future_status::ready){
                got_task = task.get();
                if (*v < got_task.value){
                    *v = got_task.value;
                    *best_move = got_task.cell;
                }
                search->n_nodes += got_task.n_nodes;
            }
        }
    }
    *alpha = max((*alpha), (*v));
}

inline void ybwc_wait_all(Search *search, vector<future<Parallel_task>> &parallel_tasks, int *v, int *best_move, int *alpha, int beta, bool *searching){
    ybwc_get_end_tasks(search, parallel_tasks, v, best_move, alpha);
    if (beta <= (*alpha))
        *searching = false;
    Parallel_task got_task;
    for (future<Parallel_task> &task: parallel_tasks){
        if (task.valid()){
            got_task = task.get();
            search->n_nodes += got_task.n_nodes;
            if ((*v) < got_task.value && (*searching)){
                *best_move = got_task.cell;
                *v = got_task.value;
                if (beta <= (*v))
                    *searching = false;
            }
        }
    }
    *alpha = max((*alpha), (*v));
}

inline void ybwc_wait_all(Search *search, vector<future<Parallel_task>> &parallel_tasks){
    Parallel_task got_task;
    for (future<Parallel_task> &task: parallel_tasks){
        if (task.valid()){
            got_task = task.get();
            search->n_nodes += got_task.n_nodes;
        }
    }
}
