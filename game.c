#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "input.c"

#define WINNING 4
#define W 7
#define H 6

enum Cell { EMPTY, RED, YELLOW };

void draw(enum Cell board[H][W], enum Cell player, int pointer_x) {
    printf("\033[%dF  ", H + 3);
    for (int i = 0; i < W; i++) {
        if (pointer_x == i) {
            if (player == RED) {
                printf("\033[31m");
            } else {
                printf("\033[33m");
            }
            printf("◉");
        } else {
            printf(" ");
        }
        printf("\033[0m ");
    }
    
    printf("\n╒═");
    for (int i = 0; i < W; i++) printf("══");
    printf("╕\n");

    bool pointer_set = false;
    for (int y = H - 1; y >= 0; y--) {
        printf("│ ");
        for (int x = 0; x < W; x++) {
            enum Cell player = board[y][x];
            if (player == EMPTY) {
                printf("\033[90m◌");
            } else {
                if (player == RED) {
                    printf("\033[31m");
                } else if (player == YELLOW) {
                    printf("\033[33m");
                }
                printf("◉");
            }
            printf("\033[0m ");
        }
        printf("│\n");
    }
    printf("╰");
    for (int i = 0; i < W * 2 + 1; i++) {
        printf("─");
    }
    printf("╯\n");
}

enum Cell winner(enum Cell board[H][W]) {
    for (int y = 0; y < H; y++) {
        enum Cell player = EMPTY;
        int count = 0;
        for (int x = 0; x < W; x++) {
            if (board[y][x] != EMPTY && board[y][x] == player) {
                count++;
            } else {
                player = board[y][x];
                count = (player == EMPTY) ? 0 : 1;
            }
            if (count >= WINNING) {
                return player;
            }
        }
    }
    for (int x = 0; x < W; x++) {
        enum Cell player = EMPTY;
        int count = 0;
        for (int y = 0; y < H; y++) {
            if (board[y][x] != EMPTY && board[y][x] == player) {
                count++;
            } else {
                player = board[y][x];
                count = (player == EMPTY) ? 0 : 1;
            }
            if (count >= WINNING) {
                return player;
            }
        }
    }
    for (int x = 0; x <= W - WINNING; x++) {
        for (int y = 0; y <= H - WINNING; y++) {
            if (board[y][x] != EMPTY) {
                bool found = true;
                for (int i = 0; i < WINNING; i++) {
                    if (board[y][x] != board[y + i][x + i]) {
                        found = false;
                    }
                }
                if (found) return board[y][x];
            }
        }
    }
    for (int x = WINNING - 1; x < W; x++) {
        for (int y = 0; y <= H - WINNING; y++) {
            if (board[y][x] != EMPTY) {
                bool found = true;
                for (int i = 0; i < WINNING; i++) {
                    if (board[y][x] != board[y + i][x - i]) {
                        found = false;
                    }
                }
                if (found) return board[y][x];
            }
        }
    }
    return EMPTY;
}

bool move(enum Cell board[H][W], enum Cell player, int x) {
    for (int y = 0; y < H; y++) {
        if (board[y][x] == EMPTY) {
            board[y][x] = player;
            return true;
        }
    }
    return false;
}

void human(enum Cell board[H][W], enum Cell player) {
    int pointer_x = 3;
    draw(board, player, pointer_x);
    enable_raw_mode();
    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);
        if (c == 'q') {
            break;
        } else if (c == '\r' || c == '\n' || c == ' ') {
            bool moved = move(board, player, pointer_x);
            draw(board, player, pointer_x);
            if (moved) return;
        } else if (c == '\033') {
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            if (seq[0] == '[') {
                if (seq[1] == 'C') {
                    pointer_x++;
                    pointer_x %= W;
                    draw(board, player, pointer_x);
                } else if (seq[1] == 'D') {
                    pointer_x = (pointer_x - 1 + W )% W;
                    draw(board, player, pointer_x);
                }
            }
        }
    }
}

bool bot_recursion(enum Cell board[H][W], enum Cell player, int depth) {

    for (int i = 0; i < W; i++) {
    
        enum Cell test_board[H][W];
        memcpy(test_board, board, sizeof(test_board));
        int test_player;
        if (player == 1) test_player = 2;
        else test_player = 1;
        
        move(test_board, test_player, i);
    }

    return true;
}

void bot(enum Cell board[H][W], enum Cell player) {
    bot_recursion(board, player, 0);
    move(board, player, 3);
}

int main() {
    enum Cell board[H][W] = {EMPTY};
    enum Cell turn = RED;

    for (int i = 0; i < H + 3; i++) printf("\n");

    while (winner(board) == EMPTY) {
        if (turn == RED) {
            human(board, turn);
        } else {
            bot(board, turn);
        }
        draw(board, EMPTY, -1);

        if (turn == 1) turn = 2;
        else turn = 1;
    }

    printf("%d\n", winner(board));

    return 0;
}
