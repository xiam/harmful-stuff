/**
* edit.h - Terminal functions for GNU/Linux.
* ---------------------------------------------------------
* Written by:
* j. carlos nieto <xiam@users.sourceforge.net>
*
* This program is Free Software released under the terms of
* the GNU/GPL.
*
* This library is based on the reading of busybox's less.c
*/

#include <termios.h>
#include <sys/ioctl.h>

// scape sequences corresponding to special keys
#define REAL_KEY_UP 'A'
#define REAL_KEY_DOWN 'B'
#define REAL_KEY_RIGHT 'C'
#define REAL_KEY_LEFT 'D'

#define REAL_END 'F'
#define REAL_HOME 'H'
#define REAL_INSERT '2'
#define REAL_DEL '3'
#define REAL_PAGE_UP '5'
#define REAL_PAGE_DOWN '6'

// scape codes assigned by this program
#define KEY_ENTER 13
#define PAGE_DOWN 305
#define KEY_UP 300
#define KEY_DOWN 301
#define KEY_LEFT 302
#define KEY_RIGHT 303
#define PAGE_UP 304
#define KEY_BACKSPACE 127
#define KEY_HOME 307
#define KEY_END 308
#define KEY_DEL 309
#define KEY_INSERT 310

#define KEY_CTRL_O 15
#define KEY_CTRL_Q 17
#define KEY_CTRL_S 19
#define KEY_CTRL_W 23

#define CLEAR() printf("%s", "\033[H\033[J")
#define MOVE(x,y) printf("\033[%i;%iH", y, x)
#define HIGHLIGHT() printf("\033[7m")
#define NORMAL() printf("\033[0m")

// termios structures
static struct termios term_orig, term_vi;

// terminal window
typedef struct screen {
  int cols;
  int rows;
  int abs_cols;
  int abs_rows;
  int cursorx;
  int cursory;
  int top;
  int left;
} SCREEN;

int getKey();

char *inputRead(SCREEN *, char *);

void move(SCREEN *, int, int);
void status(SCREEN *, char *);
void tty_raw();
void tty_cooked();
void initScreen(SCREEN *);
void tty_get_size(int, int *, int *);
void die(char *);

// borrowed from busybox/miscutils/less.c
// taken from busybox's less.c, they took it from vi.c
void tty_cooked(void) {
  fflush(stdout);
  tcsetattr(fileno(stdin), TCSANOW, &term_orig);
}
void tty_raw(void) {
  tcsetattr(fileno(stdin), TCSANOW, &term_vi);
}
void tty_get_size(int fd, int *width, int *height) {
  struct winsize win = { 0, 0, 0, 0};

  ioctl(fd, TIOCGWINSZ, &win);

  if (win.ws_col < 1)
    win.ws_col = 80;

  if (win.ws_row < 1)
    win.ws_row = 24;

  *width = (int) win.ws_col;
  *height = (int) win.ws_row;
}
int getKey() {

  int input;

  tty_raw();

  input = getc(stdin);

  //printf("%d\n", input); exit(0);

  if (input == 27 && getc(stdin) == 91) {

    input = getc(stdin);

    switch (input) {
      case REAL_KEY_UP:
        input = KEY_UP;
      break;
      case REAL_KEY_DOWN:
        input = KEY_DOWN;
      break;
      case REAL_KEY_LEFT:
        input = KEY_LEFT;
      break;
      case REAL_KEY_RIGHT:
        input = KEY_RIGHT;
      break;
      case REAL_PAGE_UP:
        input = PAGE_UP;
        getc(stdin);
      break;
      case REAL_PAGE_DOWN:
        input = PAGE_DOWN;
        getc(stdin);
      break;
      case REAL_HOME:
        input = KEY_HOME;
      break;
      case REAL_END:
        input = KEY_END;
      break;
      case REAL_DEL:
        input = KEY_DEL;
        getc(stdin);
      break;
      case REAL_INSERT:
        input = KEY_INSERT;
        getc(stdin);
      break;
      default:
        //printf("%d %c\n", input, input);
        //exit(0);
        input = 0;
      break;
    }
  }

  tty_cooked();

  return input;
}

char *inputRead(SCREEN *screen, char *message) {
  int i;
  char c;
  char *answer;
  answer = (char *) malloc(sizeof(char)*256);

  MOVE(0, screen->rows);
  HIGHLIGHT();
  printf("%s: ", message);
  MOVE(strlen(message)+3, screen->rows);

  NORMAL();

  for (i = 0; c = getchar(); i++) {
    if (c == '\n') break;
    answer[i] = c;
  }
  answer[i] = '\0';

  return answer;
}

void move(SCREEN *screen, int x, int y) {
  MOVE(screen->cursorx = x, screen->cursory = y);
}

void initScreen(SCREEN *screen) {
  screen->cols = 0;
  screen->rows = 0;
  screen->abs_cols = 0;
  screen->abs_rows = 0;
  screen->cursorx = 1;
  screen->cursory = 1;
  screen->top = 0;
  screen->left = 0;

  // termio stuff borrowed from less.c (busybox)
  tcgetattr(fileno(stdin), &term_orig);
  term_vi = term_orig;
  term_vi.c_lflag &= (~ICANON & ~ECHO);
  term_vi.c_iflag &= (~IXON & ~ICRNL);
  term_vi.c_oflag &= (~ONLCR);
  term_vi.c_cc[VMIN] = 1;
  term_vi.c_cc[VTIME] = 0;
}

void die(char *c) {
  printf("%s", c);
  exit(0);
}