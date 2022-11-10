/*
    Egaroucid Project

    @file midsearch_nws.hpp
        Search midgame with NWS (Null Window Search)
    @date 2021-2022
    @author Takuto Yamana (a.k.a. Nyanyan)
    @license GPL-3.0 license
*/

#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <future>
#include "setting.hpp"
#include "common.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"
#include "transposition_table.hpp"
#include "endsearch.hpp"
#include "move_ordering.hpp"
#include "probcut.hpp"
#include "thread_pool.hpp"
#include "ybwc.hpp"
#include "util.hpp"
#include "stability.hpp"

inline bool ybwc_split_nws(const Search *search, int alpha, int depth, uint64_t legal, bool is_end_search, const bool *searching, uint_fast8_t policy, const int canput, const int pv_idx, const int split_count, std::vector<std::future<Parallel_task>> &parallel_tasks);
inline void ybwc_get_end_tasks(Search *search, std::vector<std::future<Parallel_task>> &parallel_tasks, int *v, int *best_move);
inline void ybwc_wait_all_nws(Search *search, std::vector<std::future<Parallel_task>> &parallel_tasks, int *v, int *best_move, int alpha, bool *searching, bool *mpc_used);

/*
    @brief Get a value with last move with Nega-Alpha algorithm (NWS)

    No move ordering. Just search it.

    @param search               search information
    @param alpha                alpha value (beta value is alpha + 1)
    @param skipped              already passed?
    @param searching            flag for terminating this search
    @return the value
*/
inline int nega_alpha_eval1_nws(Search *search, int alpha, bool skipped, const bool *searching){
    if (!global_searching || !(*searching))
        return SCORE_UNDEFINED;
    ++search->n_nodes;
    int v = -INF;
    uint64_t legal = search->board.get_legal();
    if (legal == 0ULL){
        if (skipped)
            return end_evaluate(&search->board);
        search->eval_feature_reversed ^= 1;
        search->board.pass();
            v = -nega_alpha_eval1_nws(search, -alpha - 1, true, searching);
        search->board.pass();
        search->eval_feature_reversed ^= 1;
        return v;
    }
    int g;
    Flip flip;
    for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){
        calc_flip(&flip, &search->board, cell);
        eval_move(search, &flip);
        search->move(&flip);
            g = -mid_evaluate_diff(search);
        search->undo(&flip);
        eval_undo(search, &flip);
        ++search->n_nodes;
        if (v < g){
            if (alpha < g){
                return g;
            }
            v = g;
        }
    }
    return v;
}

#if MID_FAST_DEPTH > 1
    /*
        @brief Get a value with last few moves with Nega-Alpha algorithm (NWS)

        No move ordering. Just search it.

        @param search               search information
        @param alpha                alpha value (beta value is alpha + 1)
        @param depth                remaining depth
        @param skipped              already passed?
        @param searching            flag for terminating this search
        @return the value
    */
    int nega_alpha_nws(Search *search, int alpha, int depth, bool skipped, const bool *searching){
        if (!global_searching || !(*searching))
            return SCORE_UNDEFINED;
        ++search->n_nodes;
        if (depth == 1)
            return nega_alpha_eval1_nws(search, alpha, skipped, searching);
        if (depth == 0)
            return mid_evaluate_diff(search);
        int v = -INF;
        uint64_t legal = search->board.get_legal();
        if (legal == 0ULL){
            if (skipped)
                return end_evaluate(&search->board);
            search->eval_feature_reversed ^= 1;
            search->board.pass();
                v = -nega_alpha_nws(search, -alpha - 1, depth, true, searching);
            search->board.pass();
            search->eval_feature_reversed ^= 1;
            return v;
        }
        Flip flip;
        int g;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){
            calc_flip(&flip, &search->board, cell);
            eval_move(search, &flip);
            search->move(&flip);
                g = -nega_alpha_nws(search, -alpha - 1, depth - 1, false, searching);
            search->undo(&flip);
            eval_undo(search, &flip);
            if (v < g){
                if (alpha < g)
                    return g;
                v = g;
            }
        }
        return v;
    }
#endif

/*
    @brief Get a value with given depth with Nega-Alpha algorithm (NWS)

    Search with move ordering for midgame NWS
    Parallel search (YBWC: Young Brothers Wait Concept) used.

    @param search               search information
    @param alpha                alpha value (beta value is alpha + 1)
    @param depth                remaining depth
    @param skipped              already passed?
    @param legal                for use of previously calculated legal bitboard
    @param is_end_search        search till the end?
    @param searching            flag for terminating this search
    @return the value
*/
int nega_alpha_ordering_nws(Search *search, int alpha, int depth, bool skipped, uint64_t legal, bool is_end_search, const bool *searching, bool *mpc_used){
    if (!global_searching || !(*searching))
        return SCORE_UNDEFINED;
    if (is_end_search && depth <= MID_TO_END_DEPTH)
        return nega_alpha_end_nws(search, alpha, skipped, legal, searching);
    if (!is_end_search){
        #if MID_FAST_DEPTH > 1
            if (depth <= MID_FAST_DEPTH)
                return nega_alpha_nws(search, alpha, depth, skipped, searching);
        #else
            if (depth == 1)
                return nega_alpha_eval1_nws(search, alpha, skipped, searching);
            if (depth == 0)
                return mid_evaluate_diff(search);
        #endif
    }
    ++search->n_nodes;
    if (legal == LEGAL_UNDEFINED)
        legal = search->board.get_legal();
    int v = -INF;
    if (legal == 0ULL){
        if (skipped)
            return end_evaluate(&search->board);
        search->eval_feature_reversed ^= 1;
        search->board.pass();
            v = -nega_alpha_ordering_nws(search, -alpha - 1, depth, true, LEGAL_UNDEFINED, is_end_search, searching, mpc_used);
        search->board.pass();
        search->eval_feature_reversed ^= 1;
        return v;
    }
    uint32_t hash_code = search->board.hash();
    #if MID_TO_END_DEPTH < USE_TT_DEPTH_THRESHOLD
        int l = -INF, u = INF;
        if (search->n_discs <= HW2 - USE_TT_DEPTH_THRESHOLD){
            value_transposition_table.get(&search->board, hash_code, &l, &u, search->mpct, depth, mpc_used);
            if (u == l)
                return u;
            if (l < alpha && u <= alpha)
                return u;
            if (alpha < l && alpha + 1 < u)
                return l;
        }
    #else
        int l, u;
        value_transposition_table.get(&search->board, hash_code, &l, &u, search->mpct, depth, mpc_used);
        if (u == l)
            return u;
        if (u <= alpha)
            return u;
        if (alpha < l)
            return l;
    #endif
    #if USE_MID_MPC
        if (search->use_mpc){
            #if MID_TO_END_DEPTH < USE_MPC_ENDSEARCH_DEPTH
                if (!(is_end_search && depth < USE_MPC_ENDSEARCH_DEPTH)){
                    if (mpc_nws(search, alpha, depth, legal, is_end_search, &v, searching)){
                        *mpc_used = true;
                        return v;
                    }
                }
            #else
                if (mpc_nws(search, alpha, depth, legal, is_end_search, &v, searching)){
                    *mpc_used = true;
                    return v;
                }
            #endif
        }
    #endif
    int best_move = best_move_transposition_table.get(&search->board, hash_code);
    bool n_mpc_used;
    if (best_move != TRANSPOSITION_TABLE_UNDEFINED){
        if (1 & (legal >> best_move)){
            Flip flip_best;
            calc_flip(&flip_best, &search->board, best_move);
            eval_move(search, &flip_best);
            search->move(&flip_best);
            n_mpc_used = false;
                v = -nega_alpha_ordering_nws(search, -alpha - 1, depth - 1, false, LEGAL_UNDEFINED, is_end_search, searching, &n_mpc_used);
            *mpc_used |= n_mpc_used;
            search->undo(&flip_best);
            eval_undo(search, &flip_best);
            if (alpha < v)
                return v;
            legal ^= 1ULL << best_move;
        } else
            best_move = TRANSPOSITION_TABLE_UNDEFINED;
    }
    int g;
    if (legal){
        const int canput = pop_count_ull(legal);
        std::vector<Flip_value> move_list(canput);
        int idx = 0;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal))
            calc_flip(&move_list[idx++].flip, &search->board, cell);
        move_list_evaluate_nws(search, move_list, depth, alpha, is_end_search, searching);
        #if USE_ALL_NODE_PREDICTION
            const bool seems_to_be_all_node = predict_all_node(search, alpha, depth, LEGAL_UNDEFINED, is_end_search, searching);
        #else
            constexpr bool seems_to_be_all_node = false;
        #endif
        if (search->use_multi_thread && depth - 1 >= YBWC_MID_SPLIT_MIN_DEPTH){
            int pv_idx = 0, split_count = 0;
            if (best_move != TRANSPOSITION_TABLE_UNDEFINED)
                pv_idx = 1;
            std::vector<std::future<Parallel_task>> parallel_tasks;
            bool n_searching = true;
            for (int move_idx = 0; move_idx < canput; ++move_idx){
                swap_next_best_move(move_list, move_idx, canput);
                eval_move(search, &move_list[move_idx].flip);
                search->move(&move_list[move_idx].flip);
                    if (ybwc_split_nws(search, -alpha - 1, depth - 1, move_list[move_idx].n_legal, is_end_search, &n_searching, move_list[move_idx].flip.pos, canput, pv_idx++, seems_to_be_all_node, split_count, parallel_tasks)){
                        ++split_count;
                    } else{
                        n_mpc_used = false;
                        g = -nega_alpha_ordering_nws(search, -alpha - 1, depth - 1, false, move_list[move_idx].n_legal, is_end_search, searching, &n_mpc_used);
                        *mpc_used |= n_mpc_used;
                        if (v < g){
                            v = g;
                            if (alpha < v){
                                search->undo(&move_list[move_idx].flip);
                                eval_undo(search, &move_list[move_idx].flip);
                                break;
                            }
                        }
                        if (split_count){
                            ybwc_get_end_tasks(search, parallel_tasks, &v, &best_move);
                            if (alpha < v){
                                search->undo(&move_list[move_idx].flip);
                                eval_undo(search, &move_list[move_idx].flip);
                                break;
                            }
                        }
                    }
                search->undo(&move_list[move_idx].flip);
                eval_undo(search, &move_list[move_idx].flip);
            }
            if (split_count){
                if (alpha < v || !(*searching)){
                    n_searching = false;
                    ybwc_wait_all(search, parallel_tasks);
                } else
                    ybwc_wait_all_nws(search, parallel_tasks, &v, &best_move, alpha, &n_searching, mpc_used);
            }
        } else{
            for (int move_idx = 0; move_idx < canput; ++move_idx){
                swap_next_best_move(move_list, move_idx, canput);
                eval_move(search, &move_list[move_idx].flip);
                search->move(&move_list[move_idx].flip);
                n_mpc_used = false;
                    g = -nega_alpha_ordering_nws(search, -alpha - 1, depth - 1, false, move_list[move_idx].n_legal, is_end_search, searching, &n_mpc_used);
                *mpc_used |= n_mpc_used;
                search->undo(&move_list[move_idx].flip);
                eval_undo(search, &move_list[move_idx].flip);
                if (v < g){
                    v = g;
                    if (alpha < v)
                        break;
                }
            }
        }
    }
    if (*mpc_used)
        register_tt_nws(search, depth, hash_code, alpha, v, best_move, l, u, searching);
    else
        register_tt_nws_nompc(search, depth, hash_code, alpha, v, best_move, l, u, searching);
    return v;
}