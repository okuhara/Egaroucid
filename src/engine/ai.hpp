﻿/*
    Egaroucid Project

    @file ai.hpp
        Main algorithm of Egaroucid
    @date 2021-2022
    @author Takuto Yamana (a.k.a. Nyanyan)
    @license GPL-3.0 license
*/

#pragma once
#include <iostream>
#include <future>
#include <unordered_set>
#include "level.hpp"
#include "setting.hpp"
#include "midsearch.hpp"
#include "book.hpp"
#include "util.hpp"
#include "clogsearch.hpp"

#define SEARCH_BOOK -1

/*
    @brief Get a result of a search

    Firstly, if using MPC, execute clog search for finding special endgame.
    Then do some pre-search, and main search.

    @param board                board to solve
    @param depth                depth to search
    @param use_mpc              use MPC?
    @param mpct                 MPC probability
    @param show_log             show log?
    @param use_multi_thread     search in multi thread?
    @return the result in Search_result structure
*/
inline Search_result tree_search(Board board, int depth, bool use_mpc, double mpct, bool show_log, bool use_multi_thread){
    uint64_t strt;
    Search_result res;
    depth = std::min(HW2 - board.n_discs(), depth);
    bool is_end_search = (HW2 - board.n_discs() == depth);
    std::vector<Clog_result> clogs;
    res.clog_nodes = 0ULL;
    if (!is_end_search && use_mpc){
        strt = tim();
        clogs = first_clog_search(board, &res.clog_nodes);
        res.clog_time = tim() - strt;
        if (show_log){
            std::cerr << "clog search time " << res.clog_time << " nodes " << res.clog_nodes << " nps " << (res.clog_nodes / std::max(1ULL, res.clog_time)) << std::endl;
            for (int i = 0; i < (int)clogs.size(); ++i){
                std::cerr << i + 1 << "/" << clogs.size() << " " << idx_to_coord(clogs[i].pos) << " value " << clogs[i].val << std::endl;
            }
        }
    }
    Search search;
    int g = 0, alpha, beta, policy = -1;
    std::pair<int, int> result;
    search.init_board(&board);
    search.n_nodes = 0ULL;
    search.use_multi_thread = use_multi_thread;
    calc_features(&search);
    int search_depth;

    if (is_end_search){
        strt = tim();

        if (show_log)
            std::cerr << "start!" << std::endl;
        if (depth >= 23){
            search_depth = depth / 2;
            search.mpct = 1.0;
            search.use_mpc = true;
            result = first_nega_scout(&search, -SCORE_MAX, SCORE_MAX, search_depth, false, false, false, TRANSPOSITION_TABLE_UNDEFINED, clogs);
            g = result.first;
            if (show_log)
                std::cerr << "presearch depth " << search_depth << " mpct " << search.mpct << " value " << g << " policy " << idx_to_coord(result.second) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
        }
        if (depth >= 14){
            double presearch_mpct;
            if (use_mpc)
                presearch_mpct = mpct - 0.4;
            else
                presearch_mpct = std::min(2.5, 1.5 + 0.05 * (depth - 14));
            search_depth = depth;
            search.mpct = presearch_mpct;
            search.use_mpc = true;
            alpha = -SCORE_MAX;
            beta = SCORE_MAX;
            result = first_nega_scout(&search, alpha, beta, search_depth, false, true, false, result.second, clogs);
            g = result.first;
            if (show_log)
                std::cerr << "presearch depth " << search_depth << " mpct " << search.mpct << " value " << g << " policy " << idx_to_coord(result.second) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
        }

        search_depth = depth;
        search.use_mpc = use_mpc;
        search.mpct = mpct;
        result = first_nega_scout(&search, -SCORE_MAX, SCORE_MAX, search_depth, false, true, show_log, result.second, clogs);
        g = result.first;
        policy = result.second;
        if (show_log)
            std::cerr << "mainsearch depth " << search_depth << " mpct " << search.mpct << " value " << g << " policy " << idx_to_coord(policy) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
    
    } else{
        strt = tim();
        result.second = TRANSPOSITION_TABLE_UNDEFINED;
        search_depth = depth - 1;
        search.use_mpc = true;
        search.mpct = 0.7;
        g = -INF;
        if (depth - 1 >= 1){
            search_depth = depth - 1;
            result = first_nega_scout(&search, -SCORE_MAX, SCORE_MAX, search_depth, false, false, false, result.second, clogs);
            g = result.first;
            policy = result.second;
            if (show_log)
                std::cerr << "presearch depth " << search_depth << " mpct " << search.mpct << " value " << g << " policy " << idx_to_coord(policy) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
        }
        search_depth = depth;
        search.use_mpc = use_mpc;
        search.mpct = mpct;
        result = first_nega_scout(&search, -SCORE_MAX, SCORE_MAX, search_depth, false, false, show_log, result.second, clogs);
        if (g == -INF)
            g = result.first;
        else
            g = round((0.8 * g + 1.2 * result.first) / 2.0);
        policy = result.second;
        if (show_log)
            std::cerr << "mainsearch depth " << search_depth << " mpct " << search.mpct << " value " << g << " policy " << idx_to_coord(policy) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
    }
    res.depth = depth;
    res.nodes = search.n_nodes;
    res.time = tim() - strt;
    res.nps = search.n_nodes * 1000 / std::max(1ULL, res.time);
    res.policy = policy;
    res.value = g;
    res.is_end_search = is_end_search;
    res.probability = calc_probability(mpct);
    return res;
}

/*
    @brief Get a result of a search with no pre-search

    No clog search needed because this function is used for iterative deepning.
    This function is used for hint calculation.

    @param board                board to solve
    @param depth                depth to search
    @param use_mpc              use MPC?
    @param mpct                 MPC probability
    @param show_log             show log?
    @param use_multi_thread     search in multi thread?
    @return the result in Search_result structure
*/
inline Search_result tree_search_iterative_deepening(Board board, int depth, bool use_mpc, double mpct, bool show_log, bool use_multi_thread){
    Search search;
    int g = 0, alpha, beta, policy = -1;
    std::pair<int, int> result;
    depth = std::min(HW2 - board.n_discs(), depth);
    bool is_end_search = (HW2 - board.n_discs() == depth);
    search.init_board(&board);
    search.n_nodes = 0ULL;
    search.use_mpc = use_mpc;
    search.mpct = mpct;
    search.use_multi_thread = use_multi_thread;
    calc_features(&search);
    std::vector<Clog_result> clogs;
    uint64_t strt = tim();
    result = first_nega_scout(&search, -SCORE_MAX, SCORE_MAX, depth, false, is_end_search, show_log, result.second, clogs);
    g = result.first;
    policy = result.second;
    if (show_log){
        if (is_end_search)
            std::cerr << "depth " << depth << " value " << g << " policy " << idx_to_coord(policy) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
        else
            std::cerr << "midsearch time " << tim() - strt << " depth " << depth << " value " << g << " policy " << idx_to_coord(policy) << " nodes " << search.n_nodes << " time " << (tim() - strt) << " nps " << search.n_nodes * 1000 / std::max(1ULL, tim() - strt) << std::endl;
    }
    Search_result res;
    res.depth = depth;
    res.nodes = search.n_nodes;
    res.time = tim() - strt;
    res.nps = search.n_nodes * 1000 / std::max(1ULL, res.time);
    res.policy = policy;
    res.value = g;
    res.is_end_search = is_end_search;
    res.probability = calc_probability(mpct);
    return res;
}

/*
    @brief Get a result of a search with given window with no pre-search

    This function is used for book expanding.

    @param board                board to solve
    @param depth                depth to search
    @param alpha                alpha of the search window
    @param beta                 beta of the search window
    @param use_mpc              use MPC?
    @param mpct                 MPC probability
    @param use_multi_thread     search in multi thread?
    @return the score of the board
*/
inline int tree_search_window(Board board, int depth, int alpha, int beta, bool use_mpc, double mpct, bool use_multi_thread){
    Search search;
    depth = std::min(HW2 - board.n_discs(), depth);
    bool is_end_search = (HW2 - board.n_discs() == depth);
    search.init_board(&board);
    search.n_nodes = 0ULL;
    search.use_mpc = use_mpc;
    search.mpct = mpct;
    search.use_multi_thread = use_multi_thread;
    calc_features(&search);
    uint64_t strt = tim();
    bool searching = true;
    uint64_t clog_n_nodes = 0ULL;
    int clog_res = clog_search(board, &clog_n_nodes);
    if (clog_res != CLOG_NOT_FOUND)
        return clog_res;
    return nega_scout(&search, alpha, beta, depth, false, LEGAL_UNDEFINED, is_end_search, &searching);
}

/*
    @brief Get a result of a search with book or search

    Firstly, check if the given board is in the book.
    Then search the board and get the result.

    @param board                board to solve
    @param level                level of AI
    @param use_book             use book?
    @param use_multi_thread     search in multi thread?
    @param show_log             show log?
    @return the result in Search_result structure
*/
Search_result ai(Board board, int level, bool use_book, bool use_multi_thread, bool show_log){
    Search_result res;
    int value_sign = 1;
    if (board.get_legal() == 0ULL){
        board.pass();
        if (board.get_legal() == 0ULL){
            res.policy = -1;
            res.value = -board.score_player();
            res.depth = 0;
            res.nps = 0;
            res.is_end_search = true;
            res.probability = 100;
            return res;
        } else{
            value_sign = -1;
        }
    }
    Book_value book_result = book.get_random(&board, 0);
    if (book_result.policy != -1 && use_book){
        if (show_log)
            std::cerr << "book " << idx_to_coord(book_result.policy) << " " << book_result.value << std::endl;
        res.policy = book_result.policy;
        res.value = value_sign * book_result.value;
        res.depth = SEARCH_BOOK;
        res.nps = 0;
        res.is_end_search = false;
        res.probability = 100;
    } else if (level == 0){
        uint64_t legal = board.get_legal();
        std::vector<int> move_lst;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal))
            move_lst.emplace_back(cell);
        res.policy = move_lst[myrandrange(0, (int)move_lst.size())];
        res.value = value_sign * mid_evaluate(&board);
        res.depth = 0;
        res.nps = 0;
        res.is_end_search = false;
        res.probability = 0;
    } else{
        int depth;
        bool use_mpc, is_mid_search;
        double mpct;
        get_level(level, board.n_discs() - 4, &is_mid_search, &depth, &use_mpc, &mpct);
        if (show_log)
            std::cerr << "level status " << level << " " << board.n_discs() - 4 << " " << depth << " " << use_mpc << " " << mpct << std::endl;
        res = tree_search(board, depth, use_mpc, mpct, show_log, use_multi_thread);
        res.value *= value_sign;
    }
    return res;
}

/*
    @brief Get a result of a search with book or search

    This function is used for hint calculation.

    @param board                board to solve
    @param level                level of AI
    @param use_book             use book?
    @param use_multi_thread     search in multi thread?
    @param show_log             show log?
    @return the result in Search_result structure
*/
Search_result ai_hint(Board board, int level, bool use_book, bool use_multi_thread, bool show_log){
    Search_result res;
    int value_sign = 1;
    if (board.get_legal() == 0ULL){
        board.pass();
        if (board.get_legal() == 0ULL){
            res.policy = -1;
            res.value = -board.score_player();
            res.depth = 0;
            res.nps = 0;
            res.is_end_search = true;
            res.probability = 100;
            return res;
        } else{
            value_sign = -1;
        }
    }
    int book_result = book.get(&board);
    if (book_result != -INF && use_book){
        if (show_log)
            std::cerr << "book " << idx_to_coord(book_result) << std::endl;
        res.policy = -1;
        res.value = -value_sign * book_result;
        res.depth = SEARCH_BOOK;
        res.nps = 0;
        res.is_end_search = false;
        res.probability = 100;
    } else if (level == 0){
        uint64_t legal = board.get_legal();
        std::vector<int> move_lst;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal))
            move_lst.emplace_back(cell);
        res.policy = move_lst[myrandrange(0, (int)move_lst.size())];
        res.value = value_sign * mid_evaluate(&board);
        res.depth = 0;
        res.nps = 0;
        res.is_end_search = false;
        res.probability = 0;
    } else{
        int depth;
        bool use_mpc, is_mid_search;
        double mpct;
        get_level(level, board.n_discs() - 4, &is_mid_search, &depth, &use_mpc, &mpct);
        if (show_log)
            std::cerr << "level status " << level << " " << board.n_discs() - 4 << " " << depth << " " << use_mpc << " " << mpct << std::endl;
        res = tree_search_iterative_deepening(board, depth, use_mpc, mpct, show_log, use_multi_thread);
        res.value *= value_sign;
    }
    return res;
}

/*
    @brief Get a result of a search with given window with book or search

    This function is used for book expanding.

    @param board                board to solve
    @param level                level of AI
    @param alpha                alpha of the search window
    @param beta                 beta of the search window
    @param use_multi_thread     search in multi thread?
    @return the score of the board
*/
int ai_window(Board board, int level, int alpha, int beta, bool use_multi_thread){
    int value_sign = 1;
    if (board.get_legal() == 0ULL){
        board.pass();
        if (board.get_legal() == 0ULL)
            return -board.score_player();
        else
            value_sign = -1;
    }
    int book_result = book.get(&board);
    if (book_result != -INF)
        return -value_sign * book_result;
    else if (level == 0)
        return value_sign * mid_evaluate(&board);
    int depth;
        bool use_mpc, is_mid_search;
        double mpct;
        get_level(level, board.n_discs() - 4, &is_mid_search, &depth, &use_mpc, &mpct);
    return value_sign * tree_search_window(board, depth, alpha, beta, use_mpc, mpct, use_multi_thread);
}