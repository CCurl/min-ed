// editor.cpp - A simple block editor

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VERSION "25.02.11"

// Change these as desired
#define MAX_LINES     500 // Maximum number of lines
#define LLEN          100 // Maximum width of a line

#define BTW(num,lo,hi)    ((lo<=num) && (num<=hi))

// These are defined by editor.c
extern int  scrLines;
extern void editFile(const char *fileName);
extern int  strEq(const char *a, const char *b);

// These are needed by editor.c
extern void printStringF(const char *fmt, ...);
extern void printString(const char *str);
extern void printChar(const char ch);
extern int  key();
