// editor.c - A minimal text editor

#include "editor.h"

// Change these as desired
#define MAX_LINES     500 // Maximum number of lines
#define LLEN          100 // Maximum width of a line

#define BLOCK_SZ      (MAX_LINES*LLEN)
#define EDCHAR(l,o)   edBuf[((l)*LLEN)+(o)]
#define EDCH(l,o)     EDCHAR(scrTop+l,o)
#define SHOW(l,v)     lineShow[(scrTop+l)]=v
#define DIRTY(l)      isDirty=1; SHOW(l,1)
#define RCASE         return; case
#define min(a,b)      ((a<b)?(a):(b))
#define max(a,b)      ((a>b)?(a):(b))

enum { NORMAL=1, INSERT, REPLACE, QUIT };

int scrLines = 0;
char theBlock[BLOCK_SZ], *edFileName;
int line, off, edMode, scrTop;
int isDirty, lineShow[MAX_LINES];
char edBuf[BLOCK_SZ], tBuf[LLEN], mode[32];
char yanked[LLEN];

void mv(int l, int o);
void GotoXY(int x, int y) { printStringF("\x1B[%d;%dH", y, x); }
void CLS() { printString("\x1B[2J"); GotoXY(1, 1); }
void ClearEOL() { printString("\x1B[K"); }
void CursorOn() { printString("\x1B[?25h"); }
void CursorOff() { printString("\x1B[?25l"); }
void FG(int fg) { printStringF("\x1B[38;5;%dm", fg); }
void BG(int bg) { printStringF("\x1B[48;5;%dm", bg); }
void Color(int fg, int bg) { FG(fg); BG(bg); }
static void Green() { FG(40); }
static void Red() { FG(203); }
static void Yellow() { FG(226); }
static void White() { FG(231); }
static void Purple() { FG(213); }
void normalMode() { edMode=NORMAL; strcpy(mode, "normal"); }
void insertMode()  { edMode=INSERT;  strcpy(mode, "insert"); }
void replaceMode() { edMode=REPLACE; strcpy(mode, "replace"); }
void fill(char *buf, int ch, int sz) { for (int i=0; i<sz; i++) { buf[i]=ch; } }
int strEq(const char *a, const char *b) { return (strcmp(a,b)==0) ? 1 : 0; }
char lower(char c) { return BTW(c,'A','Z') ? c+32 : c; }
static int  winKey() { return (224 << 5) ^ key(); }
static int  winFKey() { return 0xF00 + key() - 58; }

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
    printString("- min-ed v0.1 - ");
    printStringF("%s%s", edFileName, isDirty ? " * " : " ");
    (edMode == NORMAL) ? Green() : Red();
    printString(mode);
    White();
    printStringF(" [%d:%d]", (line+scrTop)+1, off+1);
    printStringF(" (#%d/$%02x)", c, c);
    ClearEOL();
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

void edRdBlk() {
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
    replaceChar(c, 0, 1);
    return 1;
}

int doReplaceChar() {
    int c = key();
    replaceChar(c, 0, 1);
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
    else if (strEq(buf,"rl")) { edRdBlk(); }
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
        RCASE  Lt: mv(0, -1);
        RCASE  Home: mv(0,-99);
        RCASE  End:  gotoEOL();
        RCASE  PgUp: scroll(-scrLines/2);
        RCASE  PgDn: scroll(scrLines/2);
        RCASE  Ins: insertMode();
        RCASE  Del: edDelX('.');
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
        RCASE 'o': mv(1,-99); insertLine(); insertMode();
        RCASE 'O': mv(0,-99); insertLine(); insertMode();
        RCASE 'p': mv(1,-99); insertLine(); strcpy(&EDCH(line,0), yanked);
        RCASE 'P': mv(0,-99); insertLine(); strcpy(&EDCH(line,0), yanked);
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
    CursorOff();
    edRdBlk();
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
