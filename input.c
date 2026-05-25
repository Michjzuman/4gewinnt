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