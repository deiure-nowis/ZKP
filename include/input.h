#ifndef INPUT_H
#define INPUT_H

typedef enum {
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER,
    KEY_OTHER
} Key;

int get_keypress(void);
void clear_screen(void);
void read_string(const char *prompt, char *buffer, int max_len);

#endif // INPUT_H
