#include <iostream>
#include <string>
#include "./../ai.hpp"

using namespace std;

int main(){
    bit_init();
    flip_init();
    board_init();
    evaluate_init();
    book_init();
    parent_transpose_table.first_init();
    child_transpose_table.first_init();
    thread_pool.resize(16);
    constexpr int level = 1;
    constexpr int book_error = 0;
    Board board;
    Search_result result;
    while (true){
        board = input_board();
        result = ai(board, level, book_error);
        cout << result.value << " " << idx_to_coord(result.policy) << endl;
    }

    return 0;
}