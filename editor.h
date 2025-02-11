// editor.cpp - A simple block editor

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BTW(n,l,h)    ((l<=n) && (n<=h))

extern int scrLines;
void editFile(const char *fileName);

extern void printStringF(const char *fmt, ...);
extern void printString(const char *str);
extern void printChar(const char ch);
extern int  strEq(const char *a, const char *b);

extern int key();
