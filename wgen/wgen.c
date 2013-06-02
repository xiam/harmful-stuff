/*
 * wgen2.c - bruteforce algorithm
 * ------------------------------
 * Author: J. Carlos Nieto  <xiam@users.sourceforge.net>
 * Copyright (C) 2005, J. Carlos Nieto
 * License: GPL 2 <http://www.gnu.org/licenses/gpl.txt>
 *
 * Tested under GNU/Linux with GCC 3.3.5, Celeron 500MHz
 *
 * Compile:
 * gcc -o wgen2 wgen2.c
 *
 * */

/*
 * I don't remember exactly why I made this program but I wasn't officially
 * learning C at the time.
 * */

#include <stdio.h>
#include <time.h>

// max key size (the very limit)
#define KEY_SIZE 8

// key generator
#define KEY_MAXL 5
#define KEY_MINL 0

// configure for engine 1
#define ENGINE_1 0
#define KEY_CHARSET_BGN 33
#define KEY_CHARSET_END 126

// configure for engine 2
#define ENGINE_2 1
#define KEY_CHARSET "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//#define KEY_CHARSET "321"
// debug level
#define DEBUG 1

int main()
{
  char key[KEY_SIZE] = "";
  char *kp;
  int i,keylen = 0;

  long words = 0;

  clock_t stime, etime;
  double gtime;


  stime = clock();

#if DEBUG
  printf("wgen.c - bruteforce algorithm written by J. Carlos Nieto\n");
#endif

#if ENGINE_2
#if DEBUG
  printf("Using engine: 2...\n");
#endif
  char *dkey[KEY_MAXL]; // each dkey element is pointing to one char from KEY_CHARSET
  char *mp;

  mp = KEY_CHARSET; // KEY_CHARSET could be replaced by mp... I'm in doubt which is speeder

  i = 0;
  do {
    dkey[i] = KEY_CHARSET;
    key[i] = *dkey[i];
    i++;
    keylen++;
  } while (i < KEY_MINL);

  keylen--;

  while (keylen < KEY_MAXL) {
    /*
     * This loop always starts when dkey[0] is null, this means dkey[0] pointer
     * has reached the end of KEY_CHARSET
     * */
    for (i = 0; !*dkey[i]; i++) {
      /*
       * Setting the lower significant character as the beggining of
       * KEY_CHARSET
       * */
      dkey[i] = KEY_CHARSET;
      key[i] = *dkey[i];

      if (!key[i+1]) {
        /*
         * When the lower significant character has reached the limit, and
         * the next character in our order is null, whe have to increment
         * one character to the rigth and set it as the beggining of
         * KEY_CHARSET
         * */
        dkey[i+1] = KEY_CHARSET-1;
        keylen++;
      #if DEBUG
        printf("* Finished key length: %d\n", keylen);
      #endif
      }
      /*
       * Incrementing our next significant character by one
       * */
      dkey[i+1]++;
      key[i+1] = *dkey[i+1];
    }

    // Lower significant character
    key[0] = *dkey[0]++;
  //#if DEBUG > 1
  //  printf("%s\n", key);
  //#endif

    words++;
  }
#if DEBUG
  printf("\n");
#endif

#endif

#if ENGINE_1
#if DEBUG
  printf("Using engine: 1...\n");
#endif

  i = 0;
  do {
    key[i] = KEY_CHARSET_BGN;
    i++;
    keylen++;
  } while (i < KEY_MINL);
  keylen--;

  /*
   * Starting one position below the lower limit
   * */
  key[0] = KEY_CHARSET_BGN-1;

  while (keylen < KEY_MAXL) {
    // Increasing lower significative char
    key[0]++;

    /*
     * Starts when key[0] reaches the limit. increment by one
     * */
    for (i = 0; *(key+i) > KEY_CHARSET_END; i++) {
      key[i] = KEY_CHARSET_BGN;
      // when the next character is null, whe have to set as the lower
      if (key[i+1]++ < KEY_CHARSET_BGN) {
        key[++keylen] = KEY_CHARSET_BGN;
      #if (DEBUG)
        printf("* Finished key length: %d\n", keylen);
      #endif
      }
    }
  #if (DEBUG > 1)
    printf("%s\n", key);
  #endif
    words++;
  }
#if DEBUG
  printf("\n");
#endif

#endif

  etime = clock();

  gtime = ((double) (etime - stime)) / CLOCKS_PER_SEC;
  printf("** Statistics\n");
  printf("\tGenerated words: %d\n", words);
  printf("\tTime taken: %f\n", gtime);
  printf("\tAverage speed: %f w/s\n", words/gtime);

}
