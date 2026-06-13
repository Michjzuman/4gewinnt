#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define WINNING 4
#define W 7
#define H 6
#define SHOW_PROGRESS false

#define NEW_PEAK_BOT_MEMORY_SYSTEM true

enum Cell { EMPTY, RED, YELLOW };
enum RelativeCell { NOBODY, ME, YOU };

void enable_raw_mode();
void disable_raw_mode();

struct Situation {
    enum RelativeCell board[H][W];
    int move;
};

struct Situation *peak_bot_memory = NULL;
int peak_bot_memory_length = 0;
int peak_bot_memory_capacity = 0;
bool peak_bot_memory_set = false;

struct MemoryContainer {
    struct MemoryContainer *content;
    int move;
    bool initialized;
};

struct MemoryContainer peak_bot_memory2 = {
    .initialized = false
};

typedef struct {
    uintptr_t data;
} MemoryBranch;

MemoryBranch *new_fork() {
    MemoryBranch *fork = calloc(3, sizeof(MemoryBranch));
    if (fork == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return fork;
}

MemoryBranch peak_bot_memory3 = {0};

void add_peak_bot_memory(struct Situation situation) {
    if (NEW_PEAK_BOT_MEMORY_SYSTEM) {
        
        if (peak_bot_memory3.data == 0) {
	        peak_bot_memory3.data = (uintptr_t)new_fork();
	    }

        MemoryBranch *layer = &peak_bot_memory3;
        for (int y = H - 1; y >= 0; y--) {
            for (int x = W - 1; x >= 0; x--) {
                MemoryBranch *next_layer = &((MemoryBranch *)layer->data)[situation.board[y][x]];

                if (next_layer->data == 0) {
                    if (!(x == 0 && y == 0)) {
                        next_layer->data = (uintptr_t)new_fork();
                    }
                }

                if (x == 0 && y == 0) {
                    next_layer->data = situation.move + 1;
                }

                layer = next_layer;
            }
        }

    } else {
        if (peak_bot_memory_length >= peak_bot_memory_capacity) {
            int new_capacity;
            if (peak_bot_memory_capacity == 0) {
                new_capacity = 1024;
            } else {
                new_capacity = peak_bot_memory_capacity * 2;
            }
    
            struct Situation *new_memory = realloc(
                peak_bot_memory,
                new_capacity * sizeof(struct Situation)
            );
            if (new_memory == NULL) {
                fprintf(stderr, "Could not allocate peak_bot_memory\n");
                exit(1);
            }
    
            peak_bot_memory = new_memory;
            peak_bot_memory_capacity = new_capacity;
        }
    
        peak_bot_memory[peak_bot_memory_length] = situation;
        peak_bot_memory_length++;
    }
}

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

int bot_recursion(enum Cell board[H][W], enum Cell player, int depth, int level, bool multicore, bool peak);

struct BotArgs {
    int i;
    struct BotReport *reports;
    enum Cell (*board)[W];
    enum Cell player;
    int depth;
    int level;
    bool multicore;
    bool peak;
};

void bot_recursion_line(struct BotArgs args);

void *bot_worker(void *arg) {
    struct BotArgs *args = (struct BotArgs *)arg;
    bot_recursion_line(*args);
    return NULL;
}

void bot_recursion_line(struct BotArgs args) {
    enum Cell test_board[H][W];
    memcpy(test_board, args.board, sizeof(test_board));

    enum Cell test_player = args.player;

    bool moved = move(test_board, test_player, args.i);
    int count = 1;
    bool wins = false;
    bool loses = false;

    if (moved) {
        enum Cell test_winner = winner(test_board);
        while (test_winner == EMPTY && !board_is_full(test_board)) {
            if (test_player == RED) test_player = YELLOW;
            else test_player = RED;

            int test_move;
            if (args.depth < args.level || args.peak) {
                test_move = bot_recursion(test_board, test_player, args.depth + 1, args.level, args.multicore, args.peak);
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
        if (test_winner == args.player) {
            wins = true;
        } else if (test_winner != EMPTY) {
            loses = true;
        }
    }

    args.reports[args.i] = (struct BotReport){
        .wins = wins,
        .loses = loses,
        .legal = moved,
        .moves = count,
        .move = args.i
    };
}

int bot_recursion(enum Cell board[H][W], enum Cell player, int depth, int level, bool multicore, bool peak) {
    char name[] = "memoryX.67";
    name[6] = level + '0';

    // search memory
    if (depth == 0) {
        FILE *rfile = fopen(name, "r");
        if (rfile != NULL) {
            char line[256] = {0};
            while (fgets(line, sizeof(line), rfile) != NULL) {
                if (peak && !peak_bot_memory_set) {
                    struct Situation situation;

                    for (int i = 0; i < W * H; i++) {
                        if (line[i] == 'M') {
                            situation.board[i / W][i % W] = ME;
                        } else if (line[i] == 'Y') {
                            situation.board[i / W][i % W] = YOU;
                        } else {
                            situation.board[i / W][i % W] = NOBODY;
                        }
                    }

                    situation.move = line[W * H] - '0';

                    add_peak_bot_memory(situation);
                }
                bool is_the_same = true;
                bool is_mirrored = true;
                for (int y = 0; y < H; y++) {
                    for (int x = 0; x < W; x++) {
                        if (
                            !(line[y * W + x] == 'M' && board[y][x] == player) &&
                            !(
                                line[y * W + x] == 'Y' &&
                                board[y][x] != EMPTY &&
                                board[y][x] != player
                            ) &&
                            !(line[y * W + x] == '.' && board[y][x] == EMPTY)
                        ) {
                            is_the_same = false;
                        }
                        if (
                            !(line[y * W + (W - 1 - x)] == 'M' && board[y][x] == player) &&
                            !(
                                line[y * W + (W - 1 - x)] == 'Y' &&
                                board[y][x] != EMPTY &&
                                board[y][x] != player
                            ) &&
                            !(line[y * W + (W - 1 - x)] == '.' && board[y][x] == EMPTY)
                        ) {
                            is_mirrored = false;
                        }
                    }
                    if (!is_the_same && !is_mirrored) break;
                }
                if (is_the_same || is_mirrored) {
                    //usleep(200000);
                    fclose(rfile);
                    if (is_the_same) {
                        return line[W * H] - '0';
                    }
                    if (is_mirrored) {
                        return W - 1 - (line[W * H] - '0');
                    }
                }
            }
            fclose(rfile);
        }
    }
    peak_bot_memory_set = true;

    // peak_bot search stored memory
    if (peak && depth > 0) {
        if (NEW_PEAK_BOT_MEMORY_SYSTEM) {
            enum Cell mirrored_board[H][W];

            for (int y = 0; y < H; y++) {
                for (int x = 0; x < W; x++) {
                    mirrored_board[y][x] = board[y][W - 1 - x];
                }
            }

            for (int i = 0; i < 2; i++) {
                MemoryBranch *layer = &peak_bot_memory3;
                bool mirrored = i == 1;

                if (peak_bot_memory3.data != 0) {
                    bool stop = false;
                    for (int y = H - 1; y >= 0; y--) {
                        for (int x = W - 1; x >= 0; x--) {
    
                            enum Cell this_one = (
                                mirrored ? mirrored_board[y][x] : board[y][x]
                            );
    
                            enum RelativeCell this_one_but_relative = (
                                this_one == player ? ME : (
                                    this_one == EMPTY ? NOBODY : YOU
                                )
                            );
                            
                            layer = &((MemoryBranch *)layer->data)[this_one_but_relative];
                            
	  		                if (layer->data == 0) {
                                stop = true;
                                break;
                            }
                            
                            if (x == 0 && y == 0) {
                                return (
                                    mirrored ?
                                    W - (int)layer->data : (int)layer->data - 1
                                );
                            }
                        }
                        if (stop) break;
                    }
                }
            }

        } else {
            for (int i = 0; i < peak_bot_memory_length; i++) {
                bool is_the_same = true;
                bool is_mirrored = true;
                
                // peak_bot_memory[i].board == board
                for (int y = 0; y < H; y++) {
                    for (int x = 0; x < W; x++) {
                        if (
                            !(peak_bot_memory[i].board[y][x] == ME && board[y][x] == player) &&
                            !(peak_bot_memory[i].board[y][x] == YOU && board[y][x] == player % 2 + 1) &&
                            !(peak_bot_memory[i].board[y][x] == NOBODY && board[y][x] == EMPTY)
                        ) {
                            is_the_same = false;
                        }
                        if (
                            !(peak_bot_memory[i].board[y][W - 1 - x] == ME && board[y][x] == player) &&
                            !(peak_bot_memory[i].board[y][W - 1 - x] == YOU && board[y][x] == player % 2 + 1) &&
                            !(peak_bot_memory[i].board[y][W - 1 - x] == NOBODY && board[y][x] == EMPTY)
                        ) {
                            is_mirrored = false;
                        }
                        if (!is_the_same && !is_mirrored) break;
                    }
                    if (!is_the_same && !is_mirrored) break;
                }
                
                if (is_the_same) {
                    return peak_bot_memory[i].move;
                }
                if (is_mirrored) {
                    return W - 1 - peak_bot_memory[i].move;
                }
            }
        }
    }


    // think
    struct BotReport reports[W];
    if (depth == 0 && multicore) {
        pthread_t threads[W];
        struct BotArgs args[W];
        for (int i = 0; i < W; i++) {
            if (SHOW_PROGRESS) printf("%d\n", i + 1);
            args[i] = (struct BotArgs){
                .reports = reports,
                .board = board,
                .i = i,
                .level = level,
                .player = player,
                .depth = depth,
                .multicore = multicore,
                .peak = peak
            };
            pthread_create(&threads[i], NULL, bot_worker, &args[i]);
        }
        for (int i = 0; i < W; i++) {
            pthread_join(threads[i], NULL);
        }
    } else {
        for (int i = 0; i < W; i++) {
            if (SHOW_PROGRESS && depth == 0) printf("%d\n", i + 1);

            struct BotArgs args = {
                .reports = reports,
                .board = board,
                .i = i,
                .level = level,
                .player = player,
                .depth = depth,
                .multicore = multicore,
                .peak = peak
            };

            bot_recursion_line(args);
        }
    }

    // decide
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

    bool hope = false;
    bool danger = false;
    for (int i = 0; i < W; i++) {
        struct BotReport this_one = reports[i];
        if (this_one.wins) {
            hope = true;
        }
        if (this_one.loses) {
            danger = true;
        }
    }

    if (depth == 0 && SHOW_PROGRESS) {
        printf("\n\n\n\n\n\n\n\n\n");
    }

    // memorize
    if (depth == 0 || peak) {
        FILE *afile = fopen(name, "a");
        struct Situation situation;
        situation.move = best.move;
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                if (board[y][x] == 0) {
                    fprintf(afile, ".");
                    situation.board[y][x] = NOBODY;
                } else if (board[y][x] == player) {
                    fprintf(afile, "M");
                    situation.board[y][x] = ME;
                } else {
                    fprintf(afile, "Y");
                    situation.board[y][x] = YOU;
                }
            }
        }
        if (peak) {
            add_peak_bot_memory(situation);
        }

        fprintf(afile, "%d", best.move);
        //if (danger && !hope) {
        //    fprintf(afile, " 💀");
        //} else if (hope && !danger) {
        //    fprintf(afile, " 🔥");
        //}
        fprintf(afile, "\n");
        fclose(afile);
    }

    return best.move;
}

int recursive_bot(enum Cell board[H][W], enum Cell player, int level, bool multicore, bool peak) {
    return bot_recursion(board, player, 0, level, multicore, peak);
}


// player options

int human(enum Cell board[H][W], enum Cell player) {
    int pointer_x = 3;
    draw(board, player, pointer_x);
    enable_raw_mode();
    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);
        if (c == '\r' || c == '\n' || c == ' ') {
            bool legal = is_legal(board, pointer_x);
            disable_raw_mode();
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
    disable_raw_mode();
    return 0;
}

int bot2(enum Cell board[H][W], enum Cell player) {
    return recursive_bot(board, player, 2, false, false);
}

int bot3_mini(enum Cell board[H][W], enum Cell player) {
    return recursive_bot(board, player, 3, false, false);
}

int bot3(enum Cell board[H][W], enum Cell player) {
    return recursive_bot(board, player, 3, true, false);
}

int bot4(enum Cell board[H][W], enum Cell player) {
    return recursive_bot(board, player, 4, true, false);
}

int bot5(enum Cell board[H][W], enum Cell player) {
    return recursive_bot(board, player, 5, true, false);
}

int peak_bot(enum Cell board[H][W], enum Cell player) {
    return recursive_bot(board, player, 0, false, true);
}

int jonkler(enum Cell board[H][W], enum Cell player) {
    int result = rand() % W;
    while (!is_legal(board, result)) {
        result = rand() % W;
    }
    return result;
}

#include "ai-slop.c"

struct Player {
    char name[20];
    int (*controller)(enum Cell (*board)[W], enum Cell);
};

struct Player players[] = {
    {"human", human},
    {"jonkler", jonkler},
    {"gemma4", gemma4},
    {"bot2", bot2},
    {"bot3", bot3},
    {"bot4", bot4},
    {"bot5", bot5},
    {"slop_bot", slop_bot},
    {"peak_bot", peak_bot}
};

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

void menu() {
    int pointer_x = 0;
    int pointer_y[] = {0, 0};
    int max_len = 0;
    int amount = sizeof(players) / sizeof(struct Player);
    for (int i = 0; i < amount; i++) {
        if (max_len < strlen(players[i].name)) {
            max_len = strlen(players[i].name);
        }
        printf("\n");
    }
    printf("\n");

    enable_raw_mode();
    while (true) {
        printf("\033[%dF\n", amount + 1);
        for (int y = 0; y < amount; y++) {
            for (int x = 0; x < 2; x++) {
                if (pointer_y[x] == y) {
                    if (pointer_x == x) {
                        printf("\033[44m\033[1m");
                    } else {
                        printf("\033[100m");
                    }
                }
                printf("%s", players[y].name);
                for (int x = 0; x < max_len - strlen(players[y].name); x++) {
                    printf(" ");
                }
                printf("\033[0m  ");
            }
            printf("\n");
        }
        char c;
        read(STDIN_FILENO, &c, 1);
        if (c == '\r' || c == '\n' || c == ' ') {
            int result = play(
                players[pointer_y[0]].controller,
                players[pointer_y[1]].controller
            );
            printf("\n\033[32m");
            if (result == 0) {
                printf("nobody");
            } else {
                printf("%s", players[pointer_y[result - 1]].name);
            }
            printf(" won!\033[0m\n\n");
            for (int y = 0; y < amount; y++) {
                printf("\n");
            }
        } else if (c == '\033') {
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    pointer_y[pointer_x] = (
                        pointer_y[pointer_x] - 1 + amount
                    ) % amount;
                } else if (seq[1] == 'B') {
                    pointer_y[pointer_x] = (
                        pointer_y[pointer_x] + 1 + amount
                    ) % amount;
                } else if (seq[1] == 'C') {
                    pointer_x = (
                        pointer_x + 1 + 2
                    ) % 2;
                } else if (seq[1] == 'D') {
                    pointer_x = (
                        pointer_x - 1 + 2
                    ) % 2;
                }
            }
        }
    }
    disable_raw_mode();
}

int main() {
    srand(time(NULL));

    // printf("%d\n", play(bot5, bot5));

    menu();

    return 0;
}
