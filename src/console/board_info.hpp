/*
    Egaroucid Project

    @file board_info.hpp
        Board structure of Egaroucid
    @date 2021-2022
    @author Takuto Yamana (a.k.a. Nyanyan)
    @license GPL-3.0 license
*/

#pragma once
#include "./../engine/engine_all.hpp"

#define INVALID_CELL -1

struct Board_info{
    Board board;
    uint_fast8_t player;
    int put_cells[HW2 - 4];
    Board first_board;
    uint_fast8_t first_player;

    void reset(){
        board.reset();
        player = BLACK;
        for (int i = 0; i < HW2 - 4; ++i)
            put_cells[i] = INVALID_CELL;
        first_board.reset();
        first_player = BLACK;
    }
};