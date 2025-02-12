// editor.c - A minimal text editor

#include "editor.h"

#define BLOCK_SZ      (MAX_LINES*LLEN)
#define MAX_LINE      (MAX_LINES-1)
#define MAX_OFF       (LLEN-1)
#define EDCHAR(ln,o)  edBuf[((ln)*LLEN)+(o)]
#define EDCH(ln,o)    EDCHAR(scrTop+ln,o)
#define RCASE         return; case
#ifndef min
    #define min(a,b)      ((a<b)?(a):(b))
    #define max(a,b)      ((a>b)?(a):(b))
#endif

enum { NORMAL=1, INSERT, REPLACE, QUIT };

int scrLines, line, off, edMode, scrTop, isDirty;
char theBlock[BLOCK_SZ], edBuf[BLOCK_SZ];
char yanked[LLEN], mode[32], *edFileName;

static void mv(int ln, int o);
static void GotoXY(int x, int y) { printStringF("\x1B[%d;%dH", y, x); }
static void CLS() { printString("\x1B[2J"); GotoXY(1, 1); }
static void ClearEOL() { printString("\x1B[K"); }
static void CursorOn() { printString("\x1B[?25h"); }
static void CursorOff() { printString("\x1B[?25l"); }
static void CursorBlock() { printString("\x1B[2 q"); }
static void FG(int fg) { printStringF("\x1B[38;5;%dm", fg); }
static void BG(int bg) { printStringF("\x1B[48;5;%dm", bg); }
static void Color(int fg, int bg) { FG(fg); BG(bg); }
static void Green()  { FG(40); }
static void Red()    { FG(203); }
static void Yellow() { FG(226); }
static void White()  { FG(231); }
static void Purple() { FG(213); }
static void Cyan()   { FG(111); }
static void normalMode()  { edMode=NORMAL;  strcpy(mode, "normal"); }
static void insertMode()  { edMode=INSERT;  strcpy(mode, "insert"); }
static void replaceMode() { edMode=REPLACE; strcpy(mode, "replace"); }
static void fill(char *buf, int ch, int sz) { for (int i=0; i<sz; i++) { buf[i]=ch; } }
static char lower(char c) { return BTW(c,'A','Z') ? c+32 : c; }
static void gotoEOL() { off = strlen(&EDCH(line, 0)); }
static int  winKey() { return (224 << 5) ^ key(); }
static int  winFKey() { return 0xF00 + key() - 58; }
int  strEq(const char *a, const char *b) { return (strcmp(a,b)==0) ? 1 : 0; }

// VT key mapping, after <escape>, '['
enum { Up=7240, Dn=7248, Rt=7245, Lt=7243, Home=7239, PgUp=7241, PgDn=7249,
    End=7247, Ins=7250, Del=7251, CHome=7287, CEnd=7285,
    STab=12333, F1=0xF01, F5=0xF05, F6=0xF06, F7=0xF07
};
#define NUM_VTK 16
static int vks[NUM_VTK][7] = {
        { 0, 49, 53, 126, 999, F5 },
        { 0, 49, 55, 126, 999, F6 },
        { 0, 49, 56, 126, 999, F7 },
        { 0, 49, 59, 53, 72, 999, CHome },
        { 0, 49, 59, 53, 70, 999, CEnd },
        { 0, 50, 126, 999, Ins },
        { 0, 51, 126, 999, Del },
        { 0, 53, 126, 999, PgUp },
        { 0, 54, 126, 999, PgDn },
        { 0, 65, 999, Up },
        { 0, 66, 999, Dn },
        { 0, 67, 999, Rt },
        { 0, 68, 999, Lt },
        { 0, 70, 999, End },
        { 0, 72, 999, Home },
        { 0, 90, 999, STab },
    };

static int vtKey() {
    if (key() != '[') { return 27; }
    int ndx = 0, k, m;
    for (int i=0; i<NUM_VTK; i++) { vks[i][0] = 1; }
    while (++ndx < 5) {
        m = 0;
        k = key();
        for (int i=0; i<NUM_VTK; i++) {
            if ((vks[i][0] == ndx) && (vks[i][ndx] == k)) {
                if (vks[i][ndx+1] == 999) { return vks[i][ndx+2]; }
                vks[i][0] = ndx+1;
                m++;
            }
        }
        if (m == 0) { return 27; }
    }
    return 27;
}

static int edKey() {
    int x = key();
    if (x ==   0) { return winFKey(); }  // Windows: Function key
    if (x ==  27) { return vtKey(); }    // Possible VT control sequence
    if (x == 224) { return winKey(); }   // Windows: start char
    return x;
}

void NormLO() {
    line = min(max(line, 0), scrLines-1);
    off = min(max(off,0), MAX_OFF);
    if (scrTop < 0) { scrTop=0; }
    if (scrTop > (MAX_LINES-scrLines)) { scrTop=MAX_LINES-scrLines; }
}

static void showStatus() {
    int c = EDCH(line,off);
    GotoXY(1, scrLines+1);
    Cyan(); printStringF("- min-ed v%s - ", VERSION);
    Yellow(); printStringF("%s%s", edFileName, isDirty ? " * " : " ");
    if (edMode == NORMAL) { Green(); } else { Red(); }
    printString(mode);
    Cyan(); printStringF(" [%d:%d]", scrTop+line+1, off+1);
    printStringF(" (#%d/$%02x)", c, c);
    ClearEOL();
}

static void showEditor() {
    White();
    for (int i = 0; i < scrLines; i++) {
        char *cp = &EDCH(i,0);
        GotoXY(1, i+1);
        printString(cp);
        ClearEOL();
    }
}

static void scroll(int amt) {
    int st = scrTop;
    scrTop += amt;
    if (st != scrTop) { line -= amt; }
    NormLO();
}

static void mv(int ln, int o) {
    line += ln;
    off += o;
    if (line < 0) { scroll(line); line = 0; }
    if (scrLines <= line) { scroll(line-scrLines+1); line=scrLines-1; }
    NormLO();
}

static int toBlock() {
    char cr[] = {10,0};
    fill(theBlock, 0, BLOCK_SZ);
    for (int ln = 0; ln < MAX_LINES; ln++) {
        char *y = &EDCHAR(ln,0);
        strcat(theBlock,y);
        strcat(theBlock,cr);
    }
    return (int)strlen(theBlock);
}

static void toBuf() {
    int o = 0, ln = 0, ch;
    fill(edBuf, 0, BLOCK_SZ);
    for (int i = 0; i < BLOCK_SZ; i++) {
        ch = theBlock[i];
        if (ch == 0) { break; }
        if (ch == 9) { ch = 32; }
        if (ch ==10) {
            EDCHAR(ln, o) = 0;
            if (MAX_LINES <= (++ln)) { return; }
            o = 0;
        } else if ((o < LLEN) && (ch!=13)) {
            EDCHAR(ln,o++) = (char)ch;
        }
    }
}

static void edRdBlk() {
    fill(theBlock, 0, BLOCK_SZ);
    FILE *fp = fopen(edFileName, "rb");
    if (fp) {
        fread(theBlock, BLOCK_SZ, 1, fp);
        fclose(fp);
    }
    toBuf();
    isDirty = 0;
}

static void edSvBlk(int force) {
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

static void deleteChar() {
    for (int o = off; o < (LLEN - 2); o++) {
        EDCH(line, o) = EDCH(line, o+1);
    }
    isDirty = 1;
}

static void deleteWord() {
    int c = lower(EDCH(line,off));
    while (BTW(c,'a','z') || BTW(c,'0','9')) {
        deleteChar();
        c = lower(EDCH(line,off));
    }
    while (EDCH(line,off) == ' ') { deleteChar(); }
}

static void deleteLine(int ln) {
    for (int i = ln; i < MAX_LINE; i++) {
        char *t = &EDCH(i, 0);
        char *f = &EDCH(i+1, 0);
        strcpy(t, f);
    }
    isDirty = 1;
}

static void insertSpace() {
    for (int o = MAX_OFF; off < o; o--) {
        EDCH(line, o) = EDCH(line, o-1);
    }
    EDCH(line, off) = 32;
    isDirty = 1;
}

static void insertLineAt(int ln, int o) {
    char x[LLEN];
    ln += scrTop;
    strcpy(x, &EDCHAR(ln,o));
    for (int i = MAX_LINE; i > ln; i--) {
        char *t = &EDCHAR(i, 0);
        char *f = &EDCHAR(i-1, 0);
        for (int c = 0; c < LLEN; c++) { t[c] = f[c]; }
    }
    EDCHAR(ln,o) = 0;
    strcpy(&EDCHAR(ln+1, 0), x);
    isDirty = 1;
}

static void joinLines() {
    char *f = &EDCH(line+1, 0);
    char *t = &EDCH(line, 0);
    strcat(t, f);
    deleteLine(line+1);
}

static void replaceChar(char c, int force, int mov) {
    if (!BTW(c,32,126) && (!force)) { return; }
    char *cp = &EDCH(line, 0);
    if ((int)strlen(cp) < off) {
        for (int i = strlen(cp); i <= off; i++) { cp[i] = 32; }
        cp[off+1] = 0;
    }
    EDCH(line, off)=c;
    isDirty = 1;
    if (mov) { mv(0, 1); }
}

static int doInsertReplace(char c) {
    if (c==13) {
        if (edMode == REPLACE) { mv(1, -999); }
        else { insertLineAt(line, off); mv(1,-99); }
        return 1;
    }
    if (!BTW(c,32,126)) { return 1; }
    if (edMode == INSERT) { insertSpace(); }
    replaceChar(c, 0, 1);
    return 1;
}

static void doReplaceChar() {
    Purple(); CursorOn(); printStringF("?%c",8);
    replaceChar(key(), 0, 1);
}

static void edDelX(int c) {
    if (c == 0) { c = key(); }
    if (c == 'd') { strcpy(yanked, &EDCH(line, 0)); deleteLine(line); }
    else if (c == '.') { deleteChar(); }
    else if (c == 'w') { deleteWord(); }
    else if (c == 'X') { if (0<off) { --off; deleteChar(); } }
    else if (c == '$') {
        EDCH(line, off) = 0;
        isDirty = 1;
    }
}

static int edReadLine(char *buf, int sz) {
    int len = 0;
    CursorOn();
    while (len < (sz-1)) {
        char c = key();
        if (c == 27) { len=0; break; }
        if (c == 13) { break; }
        if ((c == 127) || (c == 8) && (len)) { --len; printStringF("%c %c",8,8); }
        if (BTW(c,32,126)) { buf[len++] = c; printChar(c); }
    }
    CursorOff();
    buf[len] = 0;
    return len;
}

static void edCommand() {
    char buf[32];
    GotoXY(1, scrLines+2); ClearEOL();
    CursorOn(); White(); printChar(':');
    edReadLine(buf, sizeof(buf));
    GotoXY(1, scrLines+2); ClearEOL();
    if (strEq(buf,"w")) { edSvBlk(0); }
    else if (strEq(buf,"rl")) { edRdBlk(); }
    else if (strEq(buf,"w!")) { edSvBlk(1); }
    else if (strEq(buf,"wq")) { edSvBlk(0); edMode = QUIT; }
    else if (strEq(buf,"q!")) { edMode = QUIT; }
    else if (strEq(buf,"q")) {
        if (isDirty) { printString("(use 'q!' to quit without saving)"); }
        else { edMode = QUIT; }
    }
}

static void doControl(int c) {
    if ((c == 8) || (c == 127)) {          // <backspace>
        c = (edMode==INSERT) ? 24 : Lt;
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
        RCASE  Dn: mv(1,0);
        RCASE  Up: mv(-1,0);
        RCASE  Rt: mv(0,1);
        RCASE  Lt: mv(0,-1);
        RCASE  CHome: scrTop = line = off = 0;
        RCASE  Home: mv(0,-99);
        RCASE  End:  gotoEOL();
        RCASE  PgUp: scroll(-scrLines/2);
        RCASE  PgDn: scroll(scrLines/2);
        RCASE  Ins:  insertMode();
        RCASE  Del:  edDelX('.');
    }
}

static void processEditorChar(int c) {
    CursorOff();
    if (!BTW(c,32,126)) { doControl(c); return; }
    if (BTW(edMode,INSERT,REPLACE)) {
        doInsertReplace((char)c);
        return;
    }

    switch (c) {
        case  ' ': mv(0,1);
        RCASE ':': edCommand();
        RCASE '$': gotoEOL();
        RCASE '+': ++scrLines;
        RCASE '-': scrLines=max(scrLines-1,10); CLS();
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
        RCASE 'o': mv(1,-99); insertLineAt(line, off); insertMode();
        RCASE 'O': mv(0,-99); insertLineAt(line, off); insertMode();
        RCASE 'p': mv(1,-99); insertLineAt(line, off); strcpy(&EDCH(line,0), yanked);
        RCASE 'P': mv(0,-99); insertLineAt(line, off); strcpy(&EDCH(line,0), yanked);
        RCASE 'r': doReplaceChar();
        RCASE 'R': replaceMode();
        RCASE 'x': edDelX('.');
        RCASE 'X': edDelX('X');
        RCASE 'Y': strcpy(yanked, &EDCH(line, 0));
    }
}

void editFile(const char *fileName) {
    edFileName = (char *)fileName;
    line = off = scrTop = 0;
    if (scrLines == 0) { scrLines = 35; }
    CLS();
    CursorBlock();
    edRdBlk();
    normalMode();
    White();
    while (edMode != QUIT) {
        CursorOff();
        showEditor();
        showStatus();
        GotoXY(off+1, line+1);
        CursorOn();
        processEditorChar(edKey());
    }
    GotoXY(1, scrLines+2);
    CursorOn();
}
