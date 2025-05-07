#include <time.h>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void millisecond_sleep(int n);
void init_screen();
void update_screen();
void close_screen();
void handle_input(int input);
void increase_speed();

void millisecond_sleep(int n) {
    struct timespec t = { 0, n * 1000000 };
    nanosleep(&t, NULL);
}

void update_screen() { refresh(); }

void init_screen() {
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
}

void close_screen() { endwin(); }

#define w 80
#define h 40

int board[w * h];
int head;
enum Dir { N, E, S, W } dir;
int quit;

enum State { SPACE=0, FOOD=1, BORDER=2 };

int score = 0;
int snake_speed = 100;
time_t start_time;

void age() {
    for (int i = 0; i < w * h; ++i) {
        if (board[i] < 0)
            board[i]++;
    }
}

void plant() {
    int r;
    do
        r = rand() % (w * h);
    while(board[r] != SPACE);
    board[r] = FOOD;
}

void start(void) {
    for(int i = 0; i < w; ++i)
        board[i] = board[i + (h - 1) * w] = BORDER;
    for(int i = 0; i < h; ++i)
        board[i * w] = board[i * w + w - 1] = BORDER;
    head = w * (h - 1 - h % 2) / 2;
    board[head] = -5;
    dir = N;
    quit = 0;
    srand(time(0));
    plant();
    time(&start_time);
}

void step() {
    int len = board[head];
    switch(dir) {
        case N: head -= w; break;
        case S: head += w; break;
        case W: --head; break;
        case E: ++head; break;
    }
    switch(board[head]) {
        case FOOD: 
            board[head] = len - 1; // Extend the snake
            score++; 
            plant(); 
            break;
        case SPACE: 
            board[head] = len - 1; // Extend the snake
            age(); // Age all other segments
            break;
        default: 
            quit = 1;
            return;
    }
}


void increase_speed() {
    time_t current_time;
    time(&current_time);
    double elapsed_time = difftime(current_time, start_time);
    if (elapsed_time >= 15) {
        snake_speed -= 10;
        time(&start_time);
    }
}

void show() {
    const char * symbol = " @.";
    for(int i = 0; i < w * h; ++i)
        mvaddch(i / w, i % w, board[i] < 0 ? '#' : symbol[board[i]]);
    update_screen();
}

int main (int argc, char * argv[]) {
    init_screen();

    char studentID[9] = "";
    int studentIDEntered = 0;
    int studentIDIndex = 0;
    int cursorPos = 0; // Track cursor position within studentID

    clear();
    mvprintw(0, 0, "Welcome! Please enter your Student ID: ");
    refresh();

    while (!studentIDEntered) {
        int ch = getch();
        switch(ch) {
            case '\n':
            case '\r':
                if (strlen(studentID) == 8) {
                    studentIDEntered = 1;
                } else {
                    mvprintw(1, 0, "Error: Student ID must be exactly 8 digits. Try again: ");
                    memset(studentID, 0, sizeof(studentID)); // Reset studentID
                    studentIDIndex = 0;
                    cursorPos = 0;
                }
                break;
            case KEY_BACKSPACE:
            case 127:
                if (studentIDIndex > 0 && cursorPos > 0) {
                    memmove(&studentID[cursorPos - 1], &studentID[cursorPos], strlen(studentID) - cursorPos + 1);
                    studentIDIndex--;
                    cursorPos--;
                    mvprintw(0, 38, "        "); // Clear the input line
                    mvprintw(0, 38, "%s", studentID); // Update the input line
                    refresh();
                }
                break;
            case KEY_RIGHT:
                if (cursorPos < studentIDIndex) {
                    cursorPos++;
                    move(0, 38 + cursorPos);
                }
                break;
            case KEY_LEFT:
                if (cursorPos > 0) {
                    cursorPos--;
                    move(0, 38 + cursorPos);
                }
                break;
            default:
                if (ch >= '0' && ch <= '9' && studentIDIndex < 8) {
                    memmove(&studentID[cursorPos + 1], &studentID[cursorPos], strlen(studentID) - cursorPos + 1);
                    studentID[cursorPos] = ch;
                    studentIDIndex++;
                    cursorPos++;
                    mvprintw(0, 38, "        "); // Clear the input line
                    mvprintw(0, 38, "%s", studentID); // Update the input line
                    refresh();
                }
                break;
        }
    }

    clear();
    start();

    do {
        show();
        int ch = getch();
        handle_input(ch);
        step();
        millisecond_sleep(snake_speed);
        increase_speed();
    } while (!quit);

    mvprintw(h / 2, w / 2 - 6, "Game Over! Score = %d", score);
    refresh();
    sleep(5);

    clear();
    mvprintw(0, 0, "Press any key to exit...");
    refresh();
    getch();
    close_screen();
    return 0;
}

void handle_input(int input) {
    switch(input) {
        case KEY_UP:
            if (dir != S) dir = N;
            break;
        case KEY_DOWN:
            if (dir != N) dir = S;
            break;
        case KEY_LEFT:
            if (dir != E) dir = W;
            break;
        case KEY_RIGHT:
            if (dir != W) dir = E;
            break;
        case 'q':
            quit = 1;
            break;
    }
}
