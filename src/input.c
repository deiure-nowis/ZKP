#include <stdio.h>
#include <string.h>
#include "input.h"

#ifdef _WIN32
#include <conio.h>
#include <stdlib.h>

int get_keypress(void) {
    int ch = _getch();
    if (ch == 0 || ch == 224) { // Funkční klávesy / šipky
        ch = _getch();
        if (ch == 72) return KEY_UP;
        if (ch == 80) return KEY_DOWN;
        return KEY_OTHER;
    }
    if (ch == '\r' || ch == '\n') return KEY_ENTER;
    if (ch == 27) return '0'; // Escape jako návrat
    return ch; // Vrátí přímo znak (např. '1', 'a', '+')
}

void clear_screen(void) { system("cls"); }

#else
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

int get_keypress(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    if (ch == 27) { // Escape sekvence
        struct termios stage2;
        tcgetattr(STDIN_FILENO, &stage2);
        stage2.c_cc[VMIN] = 0; stage2.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &stage2);
        int next = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        if (next == '[') {
            int arrow = getchar();
            if (arrow == 'A') ch = KEY_UP;
            else if (arrow == 'B') ch = KEY_DOWN;
            else ch = KEY_OTHER;
        } else if (next == -1) {
            ch = '0'; // Samotný Escape funguje jako '0' (Zpět)
        }
    } else if (ch == '\n' || ch == '\r') {
        ch = KEY_ENTER;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void clear_screen(void) { system("clear"); }
#endif

void read_string(const char *prompt, char *buffer, int max_len) {
    printf("%s", prompt);
    if (fgets(buffer, max_len, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
}
