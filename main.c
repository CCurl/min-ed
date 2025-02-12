#include "editor.h"
#include <stdarg.h>

#if (defined _WIN32 || defined _WIN64)

// Support for Windows
#include <conio.h>
int qKey() { return _kbhit(); }
int key() { return _getch(); }

#elif (defined __i386 || defined __x86_64)

// Support for Linux
#include <unistd.h>
#include <termios.h>
#define isPC
static struct termios normT, rawT;
static int isTtyInit = 0;
void ttyInit() {
    tcgetattr( STDIN_FILENO, &normT);
    cfmakeraw(&rawT);
    isTtyInit = 1;
}
void ttyModeNorm() {
    if (!isTtyInit) { ttyInit(); }
    tcsetattr( STDIN_FILENO, TCSANOW, &normT);
}
void ttyModeRaw() {
    if (!isTtyInit) { ttyInit(); }
    tcsetattr( STDIN_FILENO, TCSANOW, &rawT);
}
int qKey() {
    struct timeval tv;
    fd_set rdfs;
    ttyModeRaw();
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rdfs);
    FD_SET(STDIN_FILENO, &rdfs);
    select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
    int x = FD_ISSET(STDIN_FILENO, &rdfs);
    ttyModeNorm();
    return x;
}
int key() {
    ttyModeRaw();
    int x = getchar();
    ttyModeNorm();
    return x;
}

#endif

void printChar(const char c) { fputc(c, stdout); }
void printString(const char *s) { fputs(s, stdout); }

void printStringF(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printString(buf);
}

int parseNum(char *num) {
    int ret = 0;
    while (BTW(*num,'0','9')) {
        ret = (ret * 10) + (*num - '0');
        ++num;
    }
    return ret ? ret : scrLines;
}

int main(int argc, char *argv[]) {
    int fnNdx = 0;
    scrLines = 35;
    if (1 < argc) {
        fnNdx = 1;
        if (strEq(argv[1], "-h")) {
            scrLines = parseNum(argv[2]);
            fnNdx = (3 < argc) ? 3 : 0;
        }
    }
    if (0 < fnNdx) {
        editFile(argv[fnNdx]);
    } else {
        printString("usage: min-ed [-h height] FileName\n");
    }
    return 0;
}
