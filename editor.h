// editor.cpp - A simple block editor

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void editFile(const char *fileName);

extern void printStringF(const char *fmt, ...);
extern void printString(const char *str);
extern void printChar(const char ch);

extern int key();