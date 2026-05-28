#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define WINNING 4
#define W 7
#define H 6
#define SHOW_PROGRESS false

enum Cell { EMPTY, RED, YELLOW };

void enable_raw_mode();
void disable_raw_mode();

int BOT_MAX_DEPTH;

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

    for (int y = H - 1; y >= 0; y--) {
        printf("│ ");
        for (int x = 0; x < W; x++) {
            enum Cell cell = board[y][x];
            if (cell == EMPTY) {
                printf("\033[90m◌");
            } else {
                if (cell == RED) {
                    printf("\033[31m");
                } else if (cell == YELLOW) {
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

bool is_legal(enum Cell board[H][W], int x) {
    for (int y = 0; y < H; y++) {
        if (board[y][x] == EMPTY) {
            return true;
        }
    }
    return false;
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

bool board_is_full(enum Cell board[H][W]) {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (board[y][x] == EMPTY) {
                return false;
            }
        }
    }
    return true;
}

struct BotReport {
    bool wins;
    bool loses;
    bool legal;
    int moves;
    int move;
};

int bot_recursion(enum Cell board[H][W], enum Cell player, int depth) {
    struct BotReport reports[W];

    for (int i = 0; i < W; i++) {
        if (depth == 0 && SHOW_PROGRESS) {
            printf("%d\n", i);
        }

        enum Cell test_board[H][W];
        memcpy(test_board, board, sizeof(test_board));

        enum Cell test_player = player;

        bool moved = move(test_board, test_player, i);
        int count = 1;
        bool wins = false;
        bool loses = false;

        if (moved) {
            enum Cell test_winner = EMPTY;
            while (test_winner == EMPTY && !board_is_full(test_board)) {
                if (test_player == RED) test_player = YELLOW;
                else test_player = RED;

                int test_move;
                if (depth < BOT_MAX_DEPTH) {
                    test_move = bot_recursion(test_board, test_player, depth + 1);
                } else {
                    for (int i = 0; i < W; i++) {
                        if (is_legal(test_board, i)) {
                            test_move = i;
                        }
                    }
                }
                move(test_board, test_player, test_move);

                count++;
                test_winner = winner(test_board);
            }
            if (test_winner == player) {
                wins = true;
            } else if (test_winner != EMPTY) {
                loses = true;
            }
        }

        reports[i] = (struct BotReport){
            .wins = wins,
            .loses = loses,
            .legal = moved,
            .moves = count,
            .move = i
        };
    }

    struct BotReport best = reports[0];
    for (int i = 1; i < W; i++) {
        struct BotReport this_one = reports[i];
        if (this_one.legal) {
            if (!best.legal) {
                best = this_one;
            } else if (this_one.wins) {
                if (!best.wins || this_one.moves < best.moves) {
                    best = this_one;
                }
            } else if (best.loses) {
                if(!this_one.loses || this_one.moves > best.moves) {
                    best = this_one;
                }
            }
        }
    }

    if (depth == 0 && SHOW_PROGRESS) {
        printf("\n\n\n\n\n\n\n\n\n");
    }

    return best.move;
}

int human(enum Cell board[H][W], enum Cell player) {
    int pointer_x = 3;
    draw(board, player, pointer_x);
    enable_raw_mode();
    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);
        if (c == '\r' || c == '\n' || c == ' ') {
            bool legal = is_legal(board, pointer_x);
            if (legal) return pointer_x;
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
    return 0;
}

int recursive_bot(enum Cell board[H][W], enum Cell player) {
    return bot_recursion(board, player, 0);
}

#include "ai-slop.c"

enum Cell play(
    int (*p1)(enum Cell (*board)[W], enum Cell),
    int (*p2)(enum Cell (*board)[W], enum Cell)
) {
    enum Cell board[H][W] = {{EMPTY}};
    enum Cell turn = RED;

    for (int i = 0; i < H + 3; i++) printf("\n");

    while (winner(board) == EMPTY && !board_is_full(board)) {
        int the_move;
        if (turn == RED) {
            the_move = p1(board, turn);
        } else {
            the_move = p2(board, turn);
        }
        move(board, turn, the_move);
        draw(board, EMPTY, -1);

        if (turn == 1) turn = 2;
        else turn = 1;
    }

    return winner(board);
}



int main(int argc, char *argv[]) {

    BOT_MAX_DEPTH = atoi(argv[1]);

    printf("%d\n", play(human, recursive_bot));

    return 0;
}
