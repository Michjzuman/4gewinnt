// made by GPT-5.5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

struct termios old_terminal;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_terminal);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &old_terminal);
    atexit(disable_raw_mode);
    struct termios raw = old_terminal;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void json_escape(const char *input, char *output, size_t output_size) {
    size_t j = 0;

    for (size_t i = 0; input[i] != '\0' && j + 2 < output_size; i++) {
        char c = input[i];

        if (c == '"' || c == '\\') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = c;
        } else if (c == '\n') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = 'n';
        } else if (c == '\r') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = 'r';
        } else if (c == '\t') {
            if (j + 2 >= output_size) break;
            output[j++] = '\\';
            output[j++] = 't';
        } else {
            output[j++] = c;
        }
    }

    output[j] = '\0';
}

static int parse_ollama_move(const char *response) {
    char *start = strstr(response, "\"response\":\"");
    if (start == NULL) {
        return -1;
    }

    start += strlen("\"response\":\"");

    for (int i = 0; start[i] != '\0' && start[i] != '"'; i++) {
        if (start[i] >= '0' && start[i] <= '6') {
            return start[i] - '0';
        }
    }

    return -1;
}

static int parse_text_move(const char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] >= '0' && text[i] <= '6') {
            return text[i] - '0';
        }
    }

    return -1;
}

int ask_ollama_for_move(const char *prompt) {
    char escaped_prompt[4096];
    char json_data[8192];
    char filename[128];
    char command[512];
    char response[8192];

    json_escape(prompt, escaped_prompt, sizeof(escaped_prompt));

    snprintf(json_data, sizeof(json_data),
        "{\"model\":\"gemma4:e2b-mlx\",\"prompt\":\"%s\",\"stream\":false}",
        escaped_prompt
    );

    snprintf(filename, sizeof(filename), "/tmp/ollama_request_%d.json", getpid());

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return -1;
    }

    fputs(json_data, file);
    fclose(file);

    snprintf(command, sizeof(command),
        "curl -s --max-time 30 http://localhost:11434/api/generate "
        "-H 'Content-Type: application/json' "
        "-d @%s",
        filename
    );

    FILE *pipe = popen(command, "r");
    if (pipe == NULL) {
        remove(filename);
        return -1;
    }

    size_t length = fread(response, 1, sizeof(response) - 1, pipe);
    response[length] = '\0';

    pclose(pipe);
    remove(filename);

    return parse_ollama_move(response);
}

int ask_hermes_for_move(const char *prompt) {
    char filename[128];
    char command[512];
    char response[8192];

    snprintf(filename, sizeof(filename), "/tmp/hermes_prompt_%d.txt", getpid());

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return -1;
    }

    fputs(prompt, file);
    fclose(file);

    snprintf(command, sizeof(command),
        "hermes -z \"$(cat %s)\"",
        filename
    );

    FILE *pipe = popen(command, "r");
    if (pipe == NULL) {
        remove(filename);
        return -1;
    }

    size_t length = fread(response, 1, sizeof(response) - 1, pipe);
    response[length] = '\0';

    pclose(pipe);
    remove(filename);

    return parse_text_move(response);
}

void build_ai_prompt(enum Cell board[H][W], enum Cell player, char prompt[2048]) {
    char line[128];

    snprintf(prompt, 2048,
        "Du spielst 4 Gewinnt. Gib nur eine einzelne Zahl von 0 bis 6 zurück. "
        "Keine Erklärung, kein Text. Du bist Spieler %d.\n"
        "Board von oben nach unten. 0=leer, 1=rot, 2=gelb:\n",
        player
    );

    for (int y = H - 1; y >= 0; y--) {
        for (int x = 0; x < W; x++) {
            snprintf(line, sizeof(line), "%d ", board[y][x]);
            strncat(prompt, line, 2048 - strlen(prompt) - 1);
        }
        strncat(prompt, "\n", 2048 - strlen(prompt) - 1);
    }
}

int first_legal_move(enum Cell board[H][W]) {
    for (int x = 0; x < W; x++) {
        if (is_legal(board, x)) {
            return x;
        }
    }

    return 0;
}

int gemma4(enum Cell board[H][W], enum Cell player) {
    char prompt[2048];
    build_ai_prompt(board, player, prompt);

    int ollama_move = ask_ollama_for_move(prompt);
    if (ollama_move >= 0 && ollama_move < W && is_legal(board, ollama_move)) {
        return ollama_move;
    }

    return first_legal_move(board);
}

int hermes(enum Cell board[H][W], enum Cell player) {
    char prompt[2048];
    build_ai_prompt(board, player, prompt);

    int hermes_move = ask_hermes_for_move(prompt);
    if (hermes_move >= 0 && hermes_move < W && is_legal(board, hermes_move)) {
        return hermes_move;
    }

    return first_legal_move(board);
}

static enum Cell other_player(enum Cell player) {
    if (player == RED) return YELLOW;
    return RED;
}

static void copy_board(enum Cell to[H][W], enum Cell from[H][W]) {
    memcpy(to, from, sizeof(enum Cell) * H * W);
}

static int center_distance(int x) {
    int middle = W / 2;
    if (x > middle) return x - middle;
    return middle - x;
}

static int ordered_column(int index) {
    int order[W] = {3, 2, 4, 1, 5, 0, 6};
    return order[index];
}

static int immediate_win_move(enum Cell board[H][W], enum Cell player) {
    for (int i = 0; i < W; i++) {
        int x = ordered_column(i);
        if (is_legal(board, x)) {
            enum Cell test_board[H][W];
            copy_board(test_board, board);
            move(test_board, player, x);
            if (winner(test_board) == player) {
                return x;
            }
        }
    }

    return -1;
}

static int count_immediate_wins(enum Cell board[H][W], enum Cell player) {
    int count = 0;

    for (int x = 0; x < W; x++) {
        if (is_legal(board, x)) {
            enum Cell test_board[H][W];
            copy_board(test_board, board);
            move(test_board, player, x);
            if (winner(test_board) == player) {
                count++;
            }
        }
    }

    return count;
}

static int score_window(int mine, int theirs, int empty) {
    if (mine > 0 && theirs > 0) return 0;
    if (mine == 4) return 1000000;
    if (theirs == 4) return -1000000;
    if (mine == 3 && empty == 1) return 1200;
    if (mine == 2 && empty == 2) return 80;
    if (mine == 1 && empty == 3) return 4;
    if (theirs == 3 && empty == 1) return -1600;
    if (theirs == 2 && empty == 2) return -90;
    if (theirs == 1 && empty == 3) return -3;
    return 0;
}

static int evaluate_board(enum Cell board[H][W], enum Cell me) {
    enum Cell enemy = other_player(me);
    enum Cell won = winner(board);

    if (won == me) return 10000000;
    if (won == enemy) return -10000000;

    int score = 0;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (board[y][x] == me) {
                score += (W / 2 + 1 - center_distance(x)) * 6;
            } else if (board[y][x] == enemy) {
                score -= (W / 2 + 1 - center_distance(x)) * 6;
            }
        }
    }

    int directions[4][2] = {
        {1, 0},
        {0, 1},
        {1, 1},
        {1, -1}
    };

    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];

        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                int end_x = x + dx * (WINNING - 1);
                int end_y = y + dy * (WINNING - 1);

                if (end_x < 0 || end_x >= W || end_y < 0 || end_y >= H) {
                    continue;
                }

                int mine = 0;
                int theirs = 0;
                int empty = 0;

                for (int i = 0; i < WINNING; i++) {
                    enum Cell cell = board[y + dy * i][x + dx * i];
                    if (cell == me) mine++;
                    else if (cell == enemy) theirs++;
                    else empty++;
                }

                score += score_window(mine, theirs, empty);
            }
        }
    }

    score += count_immediate_wins(board, me) * 5000;
    score -= count_immediate_wins(board, enemy) * 7000;

    return score;
}

static int minimax_bot(enum Cell board[H][W], enum Cell turn, enum Cell me, int depth, int alpha, int beta) {
    enum Cell won = winner(board);
    if (won == me) return 10000000 + depth;
    if (won != EMPTY) return -10000000 - depth;
    if (depth == 0 || board_is_full(board)) return evaluate_board(board, me);

    int forced = immediate_win_move(board, turn);
    if (forced != -1) {
        if (turn == me) return 9000000 + depth;
        return -9000000 - depth;
    }

    if (turn == me) {
        int best = -20000000;

        for (int i = 0; i < W; i++) {
            int x = ordered_column(i);
            if (!is_legal(board, x)) continue;

            enum Cell test_board[H][W];
            copy_board(test_board, board);
            move(test_board, turn, x);

            int score = minimax_bot(test_board, other_player(turn), me, depth - 1, alpha, beta);
            if (score > best) best = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        }

        return best;
    } else {
        int best = 20000000;

        for (int i = 0; i < W; i++) {
            int x = ordered_column(i);
            if (!is_legal(board, x)) continue;

            enum Cell test_board[H][W];
            copy_board(test_board, board);
            move(test_board, turn, x);

            int score = minimax_bot(test_board, other_player(turn), me, depth - 1, alpha, beta);
            if (score < best) best = score;
            if (score < beta) beta = score;
            if (alpha >= beta) break;
        }

        return best;
    }
}

int skibidi(enum Cell board[H][W], enum Cell player) {
    enum Cell enemy = other_player(player);

    int win = immediate_win_move(board, player);
    if (win != -1) return win;

    int block = immediate_win_move(board, enemy);
    if (block != -1) return block;

    int best_move = first_legal_move(board);
    int best_score = -20000000;
    int depth = 7;

    for (int i = 0; i < W; i++) {
        int x = ordered_column(i);
        if (!is_legal(board, x)) continue;

        enum Cell test_board[H][W];
        copy_board(test_board, board);
        move(test_board, player, x);

        int opponent_wins = count_immediate_wins(test_board, enemy);
        int score = minimax_bot(test_board, enemy, player, depth - 1, -20000000, 20000000);

        if (opponent_wins >= 2) {
            score -= 3000000;
        }

        score -= center_distance(x);

        if (score > best_score) {
            best_score = score;
            best_move = x;
        }
    }

    return best_move;
}

int slop_bot(enum Cell board[H][W], enum Cell player) {
    return skibidi(board, player);
}

