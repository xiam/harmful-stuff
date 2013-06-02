/**
* edit.c - A very simple text editor
* ---------------------------------------------------------
* Written by:
* j. carlos nieto <xiam@users.sourceforge.net>
*
* This program is Free Software released under the terms of
* the GNU/GPL.
*
* Part of this program is based on the reading of busybox's
* less.c, read edit.h
*/


/*
 * Note: This program was written back in 2006 while I was
 * reading "The C Programming Language". I found it in an old
 * backup and decided to keep it online just for future
 * nostalgia.
 * */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "edit.h"

#define LINELENGTH  1024

#define ERR_MEMALLOC "Couldn't allocate memory!\n"

#define MODE_NORMAL 0
#define MODE_INSERT 1

#define ABS(x) (x >= 0 ? x : -1*x)

typedef struct textLine {
  struct textLine *next;
  struct textLine *prev;
  struct lineChar *line;
} TEXTLINE;

typedef struct lineChar {
  char code;
  struct lineChar *prev;
  struct lineChar *next;
} LINECHAR;

TEXTLINE *mainBuffer;
TEXTLINE *editLine;
LINECHAR *editChar;
char fileName[256] = {"noname.txt\0"};
int editMode;

void processKey(SCREEN *, int);
void initScreen(SCREEN *);
void buffLoad();
void buffPrint(SCREEN *);
void buffSave(char *);
void createLine(TEXTLINE **, LINECHAR **);
void createChar(LINECHAR **, char);

int fileOpen(SCREEN *, char *);
int main(int, char **);

int main(int argc, char **argv) {
  SCREEN screen;
  int key;

  initScreen(&screen);

  if (argc > 1) {
    fileOpen(&screen, argv[1]);
    strcpy(fileName, argv[1]);
  }

  editMode = MODE_NORMAL;

  buffPrint(&screen);

  status(&screen, fileName);

  while (1) {
    if ((key = getKey()) != 0)
      processKey(&screen, key);
  }
  return 0;
}
void status(SCREEN *screen, char *message) {
  MOVE(0, screen->rows);
  HIGHLIGHT();
  //printf("(x=%d,y=%d,t=%d,l=%d,r=%d,c=%d,%d,%d) %s [%s]", screen->cursorx, screen->cursory, screen->top, screen->left, screen->abs_rows, screen->abs_cols, screen->rows, screen->cols, message, editMode ? "INS\0" : "NOR\0");
  printf("(%d,%d) \"%s\" - %s [%s]", screen->left+screen->cursorx, screen->top+screen->cursory, fileName, message, editMode ? "INS\0" : "NOR\0");

  NORMAL();
  MOVE(screen->cursorx, screen->cursory);
}
void processKey(SCREEN *screen, int key) {
  LINECHAR *newKey, *tmpKey;
  LINECHAR *tmpChr, *lstChr;
  TEXTLINE *newLine, *tmpLine, *lstLine;

  char answer[256];
  char statusmsg[256], *inputmsg;
  int i, dx, dy, dt, dl;
  statusmsg[0] = '\0';

  dx = dy = dt = dl = 0;

  switch (key) {
    case KEY_CTRL_Q:
      // Ctrl + Q = Quit
      do {
        strcpy(answer, inputRead(screen, "Do you want so save? ([Y]es/[N]o/[C]ancel)"));
        answer[0] = tolower(answer[0]);
      } while (answer[0] != 'n' && answer[0] != 'y' && answer[0] != 'c');

      switch (answer[0]) {
        case 'y':
          buffSave(0);
        break;
        case 'c':
          buffPrint(screen);
          return;
        break;
      }
      exit(0);
    break;
    case KEY_CTRL_O:
      // Ctrl + O = Open file
      while (fileOpen(screen, inputRead(screen, "File")) == 0) {
        buffPrint(screen);
        status(screen, "Failed to open file.\0");
      }
    break;
    case KEY_CTRL_W:
      do
        strcpy(fileName, inputRead(screen, "Save to"));
      while (fopen(fileName, "w") == 0);
      buffSave(0);
      status(screen, "Written.\0");
    break;
    case KEY_CTRL_S:
      // Ctrl + S = Save
      buffSave(0);
      status(screen, "Written.\0");
    break;
    case PAGE_DOWN:
      if (!editChar) return;

      if (screen->top < (screen->abs_rows - screen->rows)) {

        screen->top += (screen->rows-2);

        for (dy = screen->cursory; (editLine->next) && (dy < (screen->rows-2)); dy++)
          editLine = editLine->next;

        dy = -1 * screen->cursory;
      } else
        return;
    break;
    case PAGE_UP:
      if (!editChar) return;

      if (screen->top > 0) {

        screen->top -= (screen->rows-2);

        if (screen->cursory <= 1) {
          /**
           *  _---
           *  ----<--
           *  ----
           *  ----
           * */
          for (dy = screen->cursory; (editLine->next) && (dy <= 1); dy++)
            editLine = editLine->next;
        } else {
          /**
           * ----
           * ----<--
           * _---
           * ----
           * */
          for (dy = screen->cursory; (editLine->prev) && (dy > 2); dy--)
            editLine = editLine->prev;
        }

        dy = screen->rows - screen->cursory;
      } else
        return;
    break;
    case KEY_DOWN:
      if (!editChar) return;

      if (editLine && editLine->next) {
        editLine = editLine->next;
        dy = 1;
      } else
        return;
    break;
    case KEY_UP:
      if (!editChar) return;

      if (editLine->prev) {
        editLine = editLine->prev;
        dy = -1;
      } else
        return;
    break;
    case KEY_RIGHT:
      if (!editChar) return;

      if (editChar->next) {
        screen->cursorx++;
      } else if (editLine->next && (editLine->next)->line) {
        screen->cursory++;
        screen->cursorx = 1;
        screen->left = 0;
      }
    break;
    case KEY_LEFT:
      if (!editChar) return;

      if (editChar->prev) {
        screen->cursorx--;
      } else if (editLine->prev) {
        // jumping to the final of the previous line

        tmpChr = (editLine->prev)->line;

        for (i = 0; tmpChr; i++)
          tmpChr = tmpChr->next;

        screen->cursorx = i;
        dy--;
      }
    break;
    case KEY_HOME:
      if (!editChar) return;

      while (editLine->prev)
        editLine = editLine->prev;

      dy = -1*screen->cursory + 1;
      screen->top = 0;
    break;
    case KEY_END:
      if (!editChar) return;

      while (editLine && editLine->next && (editLine->next)->line)
        editLine = editLine->next;

      tmpKey = editLine->line;
      for (screen->cursorx = 1; tmpKey && tmpKey->next; screen->cursorx++)
        tmpKey = tmpKey->next;

      dy = (screen->abs_rows > (screen->rows-1) ? (screen->rows-1) : screen->abs_rows) - screen->cursory;

      screen->top = screen->abs_rows-(screen->rows-1);
    break;
    case KEY_BACKSPACE:
      if (!editChar) return;

      if (editChar->prev) {
        editChar = editChar->prev;

        if (editChar->prev)
          (editChar->prev)->next = editChar->next;
        else
          editLine->line = editChar->next;

        if (editChar->next)
          (editChar->next)->prev = editChar->prev;

        free(editChar);

        screen->cursorx--;

      } else if (editLine->prev) {
        tmpKey = (editLine->prev)->line;

        // repositioning cursor to the end of the previous line
        for (screen->cursorx = 1; tmpKey && tmpKey->next; screen->cursorx++)
          tmpKey = tmpKey->next;

        if (tmpKey->prev)
          (tmpKey->prev)->next = editLine->line;
        else
          (editLine->prev)->line = editLine->line;

        (editLine->line)->prev = tmpKey->prev;

        free(tmpKey);

        (editLine->prev)->next = editLine->next;

        if (editLine->next)
          (editLine->next)->prev = editLine->prev;

        tmpLine = editLine;

        editLine = editLine->prev;

        free (tmpLine);

        screen->abs_rows--;

        if (screen->top == 0 || screen->top < (screen->abs_rows-screen->rows))
          dy--;
      }
    break;
    case KEY_ENTER:
      // appending a \n for breaking the current line
      createChar(&newKey, '\n');

      if (editChar) {
        newKey->prev = editChar ? editChar->prev : 0;

        if (editChar->prev)
          (editChar->prev)->next = newKey;
        else
          editLine->line = newKey;

        editChar->prev = 0;
      }

      // creating a new line in where the remaining characters will be placed.
      createLine(&newLine, &editChar);

      newLine->prev = editLine;
      newLine->next = editLine ? editLine->next : 0;

      if (!newLine->line)
        newLine->line = newKey;

      if (newLine->next)
        (newLine->next)->prev = newLine;

      if (editLine) {
        editLine->next = newLine;
        screen->cursorx = 1;
        //screen->top = 0;
        screen->left = 0;
        dy++;
      } else
        mainBuffer = editLine = newLine;

      screen->abs_rows++;
    break;
    case KEY_DEL:
      if (!editChar) return;

      if (editChar->next) {
        (editChar->next)->prev = editChar->prev;

        if (editChar->prev)
          (editChar->prev)->next = editChar->next;
        else
          editLine->line = editChar->next;

        free (editChar);

      } else {
        if (editLine->next && (editLine->next)->line) {

          if (editChar->prev)
            (editChar->prev)->next = (editLine->next)->line;
          else
            editLine->line = (editLine->next)->line;

          ((editLine->next)->line)->prev = editChar->prev;

          free (editChar);

          tmpLine = editLine->next;

          editLine->next = (editLine->next)->next;

          if (editLine->next)
            (editLine->next)->prev = editLine;

          free (tmpLine);

          if (!editLine->prev)
            mainBuffer = editLine;

          if ((screen->top > 0) && ((screen->top+screen->rows) > screen->abs_rows))
            dy++;

          screen->abs_rows--;
        }
      }
    break;
    case KEY_INSERT:
      editMode = (editMode == MODE_INSERT) ? MODE_NORMAL : MODE_INSERT;
    break;
    default:

      if (key < 32)
        return;

      switch (editMode) {
        case MODE_INSERT:
          if (editChar) {
            if (!editChar->next) {
              createChar(&newKey, editChar->code);
              newKey->prev = editChar;
              editChar->next = newKey;
            }
            editChar->code = (char) key;
            screen->cursorx++;
          }
        break;
        case MODE_NORMAL:
          createChar(&newKey, (char) key);

          if (editChar) {

            newKey->next = editChar;
            newKey->prev = editChar->prev;

            editChar->prev = newKey;
            if (newKey->prev)
              (newKey->prev)->next = newKey;
            else
              editLine->line = newKey;

          } else {
            createChar(&tmpKey, '\n');
            tmpKey->prev = newKey;
            newKey->next = tmpKey;

            if (editLine)
              editLine->line = newKey;
            else {
              // I'm not sure if this will ever happen
              createLine (&newLine, &newKey);
              mainBuffer = newLine;
              editLine = newLine;
            }
            screen->abs_rows++;
          }

          screen->cursorx++;
        break;
      }
      break;
  }

  // diferentials
  if (dy) {
    tmpChr = editLine->line;

    i = -1*screen->left;
    screen->left = 0;

    while (tmpChr && i < screen->cursorx) {
      if (i > screen->cols)
        screen->left++;
      tmpChr = tmpChr->next;
      i++;
    }

    screen->cursorx = i;
    screen->cursory += dy;
  }

  // cursory
  if (screen->cursory > screen->rows-1) {
    screen->top += (screen->cursory - (screen->rows-1));
    screen->cursory = screen->rows-1;
  } else if (screen->cursory < 1) {
    screen->top -= (1 - screen->cursory);
    screen->cursory = 1;
  }

  // cursorx
  if (screen->cursorx > screen->cols) {
    screen->left += (screen->cursorx - screen->cols);
    screen->cursorx = screen->cols;
  }
  if (screen->cursorx < 1) {
    screen->left--;
    screen->cursorx = 1;
  }

  // top
  if (screen->top < 0) {
    screen->top = 0;
    strcpy(statusmsg, "Top.\0");
  }

  if (screen->top && (screen->top > screen->abs_rows - (screen->rows-1))) {
    screen->top = (screen->abs_rows - (screen->rows-1));
    strcpy(statusmsg, "Bottom.\0");
  }

  buffPrint(screen);

  status(screen, statusmsg);
}
void createChar(LINECHAR **newKey, char key) {
  (*newKey) = (LINECHAR *) malloc(sizeof(LINECHAR));
  if (*newKey) {
    (*newKey)->prev = 0;
    (*newKey)->next = 0;
    (*newKey)->code = key;
  } else {
    die(ERR_MEMALLOC);
  }
}
void createLine(TEXTLINE **newLine, LINECHAR **newKey) {
  (*newLine) = (TEXTLINE *) malloc(sizeof(TEXTLINE));
  if (*newLine) {

    (*newLine)->line = (newKey) ? *newKey : 0;
    (*newLine)->next = (*newLine)->prev = 0;
  } else {
    die(ERR_MEMALLOC);
  }
}
int fileOpen(SCREEN *screen, char *filename) {
  FILE *fp;
  if ((fp = fopen(filename, "r")) != 0) {
    buffLoad(screen, fp);
    fclose(fp);
    return 1;
  }
  return 0;
}

void buffLoad(SCREEN *screen ,FILE *fp) {
  int i, j;
  char *c, line[LINELENGTH];
  TEXTLINE *newLine, *prevLine;
  LINECHAR *newChar, *prevChar;

  mainBuffer = 0;
  prevLine = 0;

  for (i = 0; !feof(fp); i++) {

    // getting a line from input file
    strcpy(line, "");
    fgets(line, LINELENGTH, fp);

    newLine = (TEXTLINE *) malloc(sizeof(TEXTLINE));

    if (newLine) {

      // default values for a new line
      newLine->line = 0;
      newLine->next = 0;
      newLine->prev = 0;

      // assigning the beginning of the line to mainBuffer
      if (!mainBuffer)
        mainBuffer = newLine;

      // is there a previous line?
      if (prevLine) {
        // linking lines
        prevLine->next = newLine;
        newLine->prev = prevLine;
      }

      // pointer to the first character of the text
      prevChar = 0;

      c = line;
      // Inserting the line
      for (j = 0; *c; j++, c++) {
        newChar = (LINECHAR *)malloc(sizeof(LINECHAR));
        newChar->next = 0;
        newChar->prev = 0;
        newChar->code = *c;

        if (newChar) {

          if (!newLine->line)
            newLine->line = newChar;

          if (prevChar) {
            prevChar->next = newChar;
            newChar->prev = prevChar;
          }

          prevChar = newChar;

        } else {
          die(ERR_MEMALLOC);
        }
      }

      // The maximum number of columns in a row
      if (j > screen->abs_cols)
        screen->abs_cols = j;

      prevLine = newLine;
    } else {
      die(ERR_MEMALLOC);
    }
  }

  // The maximum number of rows
  screen->abs_rows = i-1;
}

void buffPrint(SCREEN *screen) {

  TEXTLINE *curLine;
  LINECHAR *lineChar;

  int i, j, p;

  CLEAR();

  tty_get_size(0, &(screen->cols), &(screen->rows));

  curLine = mainBuffer;

  //editLine = editChar = 0;

  // positioning curLine
  for (i = 0; (curLine) && (i < screen->top); i++)
    curLine = curLine->next;

  // Drawing in terminal
  for (i = 0; i < (screen->rows-1); i++) {


    if (curLine) {

      if ((screen->cursory-1) == i) {
        editLine = curLine;
        editChar = editLine->line;
      }

      lineChar = curLine->line;

      // If the screen is moved from the left edge
      for (j = 0; j < screen->left; j++)
        if (lineChar)
          lineChar = lineChar->next;

      p = 0;
      for (j = 0; lineChar && j < screen->cols; j++) {

        if (((screen->cursory-1) == i) && (j == (screen->cursorx-1)))
          editChar = lineChar;

        putchar(lineChar->code);

        lineChar = lineChar->next;
        p = 1;
      }

      if (!p)
        putchar('\n');

      curLine = curLine->next;

    } else {
      // Beyond the end of the file
      printf("~\n");
    }
  }

  move(screen, screen->cursorx, screen->cursory);
}

void buffSave(char *file) {
  FILE *fp;
  TEXTLINE *curLine;
  LINECHAR *lineChar;
  int i, j, p;

  curLine = mainBuffer;

  fp = fopen(file ? file : fileName, "w");

  while (curLine) {
    lineChar = curLine->line;
    while (lineChar) {
      fputc(lineChar->code, fp);
      lineChar = lineChar->next;
    }
    curLine = curLine->next;
  }

  fclose(fp);
}
