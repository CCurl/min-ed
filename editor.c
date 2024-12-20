// editor.c - A simple text editor

#include "editor.h"

// Change these as desired
#define MAX_LINES     500 // Maximum number of lines
#define LLEN          100 // Maximum width of a line
#define SCR_HEIGHT     35 // Number of lines to display

#define BLOCK_SZ      (MAX_LINES*LLEN)
#define EDCHAR(l,o)   edBuf[((l)*LLEN)+(o)]
#define EDCH(l,o)     EDCHAR(scrTop+l,o)
#define SHOW(l,v)     lineShow[(scrTop+l)]=v
#define DIRTY(l)      isDirty=1; SHOW(l,1)
#define RCASE         return; case
#define BTW(n,l,h)    ((l<=n) && (n<=h))
#define min(a,b)      ((a<b)?(a):(b))
#define max(a,b)      ((a>b)?(a):(b))

enum { NORMAL=1, INSERT, REPLACE, QUIT };

int scrLines = SCR_HEIGHT;
char theBlock[BLOCK_SZ], *edFileName;
int line, off, blkNum, edMode, scrTop;
int isDirty, lineShow[MAX_LINES];
char edBuf[BLOCK_SZ], tBuf[LLEN], mode[32], *msg = NULL;
char yanked[LLEN];

void mv(int l, int o);
void GotoXY(int x, int y) { printStringF("\x1B[%d;%dH", y, x); }
void CLS() { printString("\x1B[2J"); GotoXY(1, 1); }
void ClearEOL() { printString("\x1B[K"); }
void CursorOn() { printString("\x1B[?25h"); }
void CursorOff() { printString("\x1B[?25l"); }
void Color(int fg, int bg) { printStringF("\x1B[%d;%dm", (30+fg), bg?bg:40); }
void normalMode() { edMode=NORMAL; strcpy(mode, "normal"); }
void insertMode()  { edMode=INSERT;  strcpy(mode, "insert"); }
void replaceMode() { edMode=REPLACE; strcpy(mode, "replace"); }
int edKey() { return key(); }
void fill(char *buf, int ch, int sz) { for (int i=0; i<sz; i++) { buf[i]=ch; } }
int strEq(const char *a, const char *b) { return (strcmp(a,b)==0) ? 1 : 0; }
char lower(char c) { return BTW(c,'A','Z') ? c+32 : c; }

void NormLO() {
    line = min(max(line, 0), scrLines-1);
    off = min(max(off,0), LLEN-1);
    if (scrTop < 0) { scrTop=0; }
    if (scrTop > (MAX_LINES-scrLines)) { scrTop=MAX_LINES-scrLines; }
}

void showAll() {
    for (int i = 0; i < MAX_LINES; i++) { lineShow[i] = 1; }
}

char edChar(int l, int o) {
    char c = EDCH(l,o);
    if (c==0) { return c; }
    return BTW(c,32,126) ? c : ' ';
}

void showCursor() {
    char c = EDCH(line, off);
    c = max(c,32);
    GotoXY(off + 1, line + 1);
    Color(0, 47);
    printChar(c);
    Color(7, 0);
}

void showLine(int l) {
    int sl = scrTop+l;
    if (!lineShow[sl]) { return; }
    SHOW(l,0);
    GotoXY(1, l+1);
    for (int o = 0; o < LLEN; o++) {
        int c = edChar(l, o);
        if (c) { printChar(c); }
        else { ClearEOL(); break; }
    }
    if (l == line) { showCursor(); }
}

void showStatus() {
    static int cnt = 0;
    int c = EDCH(line,off);
    GotoXY(1, scrLines+1);
    printString("- Block Editor v0.1 - ");
    printStringF("Block# %03d%s", blkNum, isDirty ? " *" : "");
    printStringF("%s- %s", msg ? msg : " ", mode);
    printStringF(" [%d:%d]", (line+scrTop)+1, off+1);
    printStringF(" (#%d/$%02x)", c, c);
    ClearEOL();
    if (msg && (1 < ++cnt)) { msg = NULL; cnt = 0; }
}

void showEditor() {
    for (int i = 0; i < scrLines; i++) { showLine(i); }
}

void scroll(int amt) {
    int st = scrTop;
    scrTop += amt;
    if (st != scrTop) { line -= amt; showAll(); }
    NormLO();
}

void mv(int l, int o) {
    SHOW(line,1);
    line += l;
    off += o;
    if (line < 0) { scroll(line); line = 0; }
    if (scrLines <= line) { scroll(line-scrLines+1); line=scrLines-1; }
    NormLO();
    SHOW(line,1);
}

void gotoEOL() {
    mv(0, -99);
    while (EDCH(line, off) != 10) { ++off; }
}

int toBlock() {
    fill(theBlock, 0, BLOCK_SZ);
    for (int l=0; l<MAX_LINES; l++) {
        char *y=&EDCHAR(l,0);
        strcat(theBlock,y);
    }
    return (int)strlen(theBlock);
}

void addLF(int l) {
    int o, lc = 0;
    for (o = 0; EDCH(l, o); ++o) { lc = EDCH(l, o); }
    if (lc != 10) { EDCH(l, o) = 10; }
}

void toBuf() {
    int o = 0, l = 0, ch;
    fill(edBuf, 0, BLOCK_SZ);
    for (int i = 0; i < BLOCK_SZ; i++) {
        ch = theBlock[i];
        if (ch == 0) { break; }
        if (ch == 9) { ch = 32; }
        if (ch ==10) {
            EDCHAR(l, o) = (char)ch;
            if (MAX_LINES <= (++l)) { return; }
            o=0;
            continue;
        } else if ((o < LLEN) && (ch!=13)) {
            EDCHAR(l,o++) = (char)ch;
        }
    }
    o = scrTop; scrTop = 0;
    for (int i = 0; i < MAX_LINES; i++) { addLF(i); }
    scrTop = o;
}

void edRdBlk(int force) {
    fill(theBlock, 0, BLOCK_SZ);
    FILE *fp = fopen(edFileName, "rb");
    if (fp) {
        fread(theBlock, BLOCK_SZ, 1, fp);
        fclose(fp);
    }
    toBuf();
    showAll();
    isDirty = 0;
}

void edSvBlk(int force) {
    if (isDirty || force) {
        int len = toBlock();
        while (1<len) {
            if (theBlock[len-2] == 10) { theBlock[len-1]=0; --len; }
            else { break; }
        }
        FILE *fp = fopen(edFileName, "wb");
        if (fp) {
            fwrite(theBlock, 1, len, fp);
            fclose(fp);
        }
        isDirty = 0;
    }
}

void deleteChar() {
    for (int o = off; o < (LLEN - 2); o++) {
        EDCH(line,o) = EDCH(line, o+1);
    }
    DIRTY(line);
    addLF(line);
}

void deleteWord() {
    int c = lower(EDCH(line,off));
    while (BTW(c,'a','z') || BTW(c,'0','9')) {
        deleteChar();
        c = lower(EDCH(line,off));
    }
    while (EDCH(line,off)==' ') { deleteChar(); }
}

void deleteLine() {
    EDCH(line,0) = 0;
    toBlock();
    toBuf();
    showAll();
    isDirty = 1;
}

void insertSpace() {
    for (int o=LLEN-1; off<o; o--) {
        EDCH(line,o) = EDCH(line, o-1);
    }
    EDCH(line, off)=32;
    DIRTY(line);
}

void insertLine() {
    insertSpace();
    EDCH(line, off)=10;
    toBlock();
    toBuf();
    showAll();
    isDirty = 1;
}

void joinLines() {
    gotoEOL();
    EDCH(line, off) = 0;
    toBlock();
    toBuf();
    showAll();
    isDirty = 1;
}

void replaceChar(char c, int force, int mov) {
    if (!BTW(c,32,126) && (!force)) { return; }
    for (int o=off-1; 0<=o; --o) {
        int ch = EDCH(line, o);
        if (ch && (ch != 10)) { break; }
        EDCH(line,o)=32;
    }
    EDCH(line, off)=c;
    DIRTY(line);
    addLF(line);
    if (mov) { mv(0, 1); }
}

int doInsertReplace(char c) {
    if (c==13) {
        if (edMode == REPLACE) { mv(1, -999); }
        else { insertLine(); mv(1,-99); }
        return 1;
    }
    if (!BTW(c,32,126)) { return 1; }
    if (edMode == INSERT) { insertSpace(); }
    replaceChar(c, 1, 1);
    return 1;
}

void edDelX(int c) {
    if (c==0) { c = key(); }
    if (c=='d') { strcpy(yanked, &EDCH(line, 0)); deleteLine(); }
    else if (c=='.') { deleteChar(); }
    else if (c=='w') { deleteWord(); }
    else if (c=='X') { if (0<off) { --off; deleteChar(); } }
    else if (c=='$') {
        EDCH(line,off) = 10;
        c=off+1; while ((c<LLEN) && EDCH(line,c)) { EDCH(line,c)=0; c++; }
        DIRTY(line);
    }
}

int edReadLine(char *buf, int sz) {
    int len = 0;
    CursorOn();
    while (len<(sz-1)) {
        char c = key();
        if (c==27) { len=0; break; }
        if (c==13) { break; }
        if ((c==127) || (c==8) && (len)) { --len; printStringF("%c %c",8,8); }
        if (BTW(c,32,126)) { buf[len++]=c; printChar(c); }
    }
    CursorOff();
    buf[len]=0;
    return len;
}

void edCommand() {
    char buf[32];
    GotoXY(1, scrLines+2); ClearEOL();
    printChar(':');
    edReadLine(buf, sizeof(buf));
    GotoXY(1, scrLines+2); ClearEOL();
    if (strEq(buf,"w")) { edSvBlk(0); }
    else if (strEq(buf,"w!")) { edSvBlk(1); }
    else if (strEq(buf,"wq")) { edSvBlk(0); edMode=QUIT; }
    else if (strEq(buf,"q!")) { edMode=QUIT; }
    else if (strEq(buf,"q")) {
        if (isDirty) { printString("(use 'q!' to quit without saving)"); }
        else { edMode=QUIT; }
    }
}

void doControl(int c) {
    if ((c == 8) || (c == 127)) {          // <backspace>
        c = (edMode==INSERT) ? 24 : 150;
    }
    if ((c == 13) && BTW(edMode,INSERT,REPLACE)) {
        doInsertReplace(c);
        return;
    }
    switch (c) {
        case    4: scroll(scrLines/2);     // <ctrl-d>
        RCASE   5: scroll(1);              // <ctrl-e>
        RCASE   9: mv(0,8);                // <tab>
        RCASE  10: mv(1,0);                // <ctrl-j>
        RCASE  11: mv(-1,0);               // <ctrl-k>
        RCASE  12: mv(0,1);                // <ctrl-l>
        RCASE  13: mv(1,-99);              // <ctrl-m>
        RCASE  21: scroll(-scrLines/2);    // <ctrl-u>
        RCASE  24: edDelX('X');            // <ctrl-x>
        RCASE  25: scroll(-1);             // <ctrl-y>
        RCASE  26: edDelX('.');            // <ctrl-z>
        RCASE  27: normalMode();           // <esc>
        RCASE 150: mv(0, -1);              // <left>
    }
}

void processEditorChar(int c) {
    if (!BTW(c,32,126)) { doControl(c); return; }
    if (BTW(edMode,INSERT,REPLACE)) {
        doInsertReplace((char)c);
        return;
    }

    switch (c) {
        case  ' ': mv(0,1);
        RCASE ':': edCommand();
        RCASE '$': gotoEOL();
        RCASE '+': ++scrLines; CLS(); showAll();
        RCASE '-': scrLines=max(scrLines-1,10); CLS(); showAll();
        RCASE '_': mv(0,-99);
        RCASE 'a': mv(0,1);  insertMode();
        RCASE 'A': gotoEOL(); insertMode();
        RCASE 'b': insertSpace();
        RCASE 'c': deleteChar(); insertMode();
        RCASE 'C': edDelX('$');  insertMode();
        RCASE 'd': edDelX(0);
        RCASE 'D': edDelX('$');
        RCASE 'g': mv(-line,-off);
        RCASE 'G': mv(scrLines,-99);
        RCASE 'h': mv(0,-1);
        RCASE 'i': insertMode();
        RCASE 'I': mv(0,-99); insertMode();
        RCASE 'j': mv(1,0);
        RCASE 'J': joinLines();
        RCASE 'k': mv(-1,0);
        RCASE 'l': mv(0,1);
        RCASE 'L': edRdBlk(1);
        RCASE 'o': mv(1,-99); insertLine(); insertMode();
        RCASE 'O': mv(0,-99); insertLine(); insertMode();
        RCASE 'p': mv(1,-99); insertLine(); strcpy(&EDCH(line,0), yanked);
        RCASE 'P': mv(0,-99); insertLine(); strcpy(&EDCH(line,0), yanked);
        RCASE 'r': replaceChar(edKey(), 0, 1);
        RCASE 'R': replaceMode();
        RCASE 'x': edDelX('.');
        RCASE 'X': edDelX('X');
        RCASE 'Y': strcpy(yanked, &EDCH(line, 0));
    }
}

void editFile(const char *fileName) {
    edFileName = (char *)fileName;
    line = off = scrTop = 0;
    msg = NULL;
    CLS();
    CursorOff();
    edRdBlk(0);
    normalMode();
    showAll();
    while (edMode != QUIT) {
        showEditor();
        showStatus();
        processEditorChar(edKey());
    }
    GotoXY(1, scrLines+2);
    CursorOn();
}
