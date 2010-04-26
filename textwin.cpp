#include "solvespace.h"
#include <stdarg.h>

const TextWindow::Color TextWindow::fgColors[] = {
    { 'd', RGB(255, 255, 255) },
    { 'l', RGB(100, 100, 255) },
    { 't', RGB(255, 200,   0) },
    { 'h', RGB( 90,  90,  90) },
    { 's', RGB( 40, 255,  40) },
    { 'm', RGB(200, 200,   0) },
    { 'r', RGB(  0,   0,   0) },
    { 'x', RGB(255,  20,  20) },
    { 'i', RGB(  0, 255, 255) },
    { 'g', RGB(160, 160, 160) },
    { 0, 0 },
};
const TextWindow::Color TextWindow::bgColors[] = {
    { 'd', RGB(  0,   0,   0) },
    { 't', RGB( 34,  15,  15) },
    { 'a', RGB( 20,  20,  20) },
    { 'r', RGB(255, 255, 255) },
    { 0, 0 },
};

void TextWindow::MakeColorTable(const Color *in, float *out) {
    int i;
    for(i = 0; in[i].c != 0; i++) {
        int c = in[i].c;
        if(c < 0 || c > 255) oops();
        out[c*3 + 0] = REDf(in[i].color);
        out[c*3 + 1] = GREENf(in[i].color);
        out[c*3 + 2] = BLUEf(in[i].color);
    }
}

void TextWindow::Init(void) {
    ClearSuper();
}

void TextWindow::ClearSuper(void) {
    HideTextEditControl();

    memset(this, 0, sizeof(*this));
    MakeColorTable(fgColors, fgColorTable);
    MakeColorTable(bgColors, bgColorTable);

    ClearScreen();
    Show();
}

void TextWindow::ClearScreen(void) {
    int i, j;
    for(i = 0; i < MAX_ROWS; i++) {
        for(j = 0; j < MAX_COLS; j++) {
            text[i][j] = ' ';
            meta[i][j].fg = 'd';
            meta[i][j].bg = 'd';
            meta[i][j].link = NOT_A_LINK;
        }
        top[i] = i*2;
    }
    rows = 0;
}

void TextWindow::Printf(bool halfLine, char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);

    if(rows >= MAX_ROWS) return;

    int r, c;
    r = rows;
    top[r] = (r == 0) ? 0 : (top[r-1] + (halfLine ? 3 : 2));
    rows++;

    for(c = 0; c < MAX_COLS; c++) {
        text[r][c] = ' ';
        meta[r][c].link = NOT_A_LINK;
    }

    int fg = 'd', bg = 'd';
    int link = NOT_A_LINK;
    DWORD data = 0;
    LinkFunction *f = NULL, *h = NULL;

    c = 0;
    while(*fmt) {
        char buf[1024];

        if(*fmt == '%') {
            fmt++;
            if(*fmt == '\0') goto done;
            strcpy(buf, "");
            switch(*fmt) {
                case 'd': {
                    int v = va_arg(vl, int);
                    sprintf(buf, "%d", v);
                    break;
                }
                case 'x': {
                    DWORD v = va_arg(vl, DWORD);
                    sprintf(buf, "%08x", v);
                    break;
                }
                case '@': {
                    double v = va_arg(vl, double);
                    sprintf(buf, "%.2f", v);
                    break;
                }
                case '2': {
                    double v = va_arg(vl, double);
                    sprintf(buf, "%s%.2f", v < 0 ? "" : " ", v);
                    break;
                }
                case '3': {
                    double v = va_arg(vl, double);
                    sprintf(buf, "%s%.3f", v < 0 ? "" : " ", v);
                    break;
                }
                case '#': {
                    double v = va_arg(vl, double);
                    sprintf(buf, "%.3f", v);
                    break;
                }
                case 's': {
                    char *s = va_arg(vl, char *);
                    memcpy(buf, s, min(sizeof(buf), strlen(s)+1));
                    break;
                }
                case 'c': {
                    char v = va_arg(vl, char);
                    sprintf(buf, "%c", v);
                    break;
                }
                case 'E':
                    fg = 'd';
                    // leave the background, though
                    link = NOT_A_LINK;
                    data = 0;
                    f = NULL;
                    h = NULL;
                    break;

                case 'F':
                case 'B': {
                    int color;
                    if(fmt[1] == '\0') goto done;
                    if(fmt[1] == 'p') {
                        color = va_arg(vl, int);
                    } else {
                        color = fmt[1];
                    }
                    if((color < 0 || color > 255) && !(color & 0x80000000)) {
                        color = 0;
                    }
                    if(*fmt == 'F') {
                        fg = color;
                    } else {
                        bg = color;
                    }
                    fmt++;
                    break;
                }
                case 'L':
                    if(fmt[1] == '\0') goto done;
                    fmt++;
                    if(*fmt == 'p') {
                        link = va_arg(vl, int);
                    } else {
                        link = *fmt;
                    }
                    break;

                case 'f':
                    f = va_arg(vl, LinkFunction *);
                    break;

                case 'h':
                    h = va_arg(vl, LinkFunction *);
                    break;

                case 'D':
                    data = va_arg(vl, DWORD);
                    break;
                    
                case '%':
                    strcpy(buf, "%");
                    break;
            }
        } else {
            buf[0] = *fmt;
            buf[1]= '\0';
        }

        for(unsigned i = 0; i < strlen(buf); i++) {
            if(c >= MAX_COLS) goto done;
            text[r][c] = buf[i];
            meta[r][c].fg = fg;
            meta[r][c].bg = bg;
            meta[r][c].link = link;
            meta[r][c].data = data;
            meta[r][c].f = f;
            meta[r][c].h = h;
            c++;
        }

        fmt++;
    }
    while(c < MAX_COLS) {
        meta[r][c].fg = fg;
        meta[r][c].bg = bg;
        c++;
    }

done:
    va_end(vl);
}

#define gs (SS.GW.gs)
void TextWindow::Show(void) {
    if(!(SS.GW.pending.operation)) SS.GW.ClearPending();

    SS.GW.GroupSelection();

    if(SS.GW.pending.description) {
        // A pending operation (that must be completed with the mouse in
        // the graphics window) will preempt our usual display.
        HideTextEditControl();
        ShowHeader(false);
        Printf(false, "");
        Printf(false, "%s", SS.GW.pending.description);
        Printf(true, "%Fl%f%Ll(cancel operation)%E",
            &TextWindow::ScreenUnselectAll);
    } else if((gs.n > 0 || gs.constraints > 0) && 
                                    shown.screen != SCREEN_PASTE_TRANSFORMED)
    {
        if(edit.meaning != EDIT_TTF_TEXT) HideTextEditControl();
        ShowHeader(false);
        DescribeSelection();
    } else {
        if(edit.meaning == EDIT_TTF_TEXT) HideTextEditControl();
        ShowHeader(true);
        switch(shown.screen) {
            default:
                shown.screen = SCREEN_LIST_OF_GROUPS;
                // fall through
            case SCREEN_LIST_OF_GROUPS:     ShowListOfGroups();     break;
            case SCREEN_GROUP_INFO:         ShowGroupInfo();        break;
            case SCREEN_GROUP_SOLVE_INFO:   ShowGroupSolveInfo();   break;
            case SCREEN_CONFIGURATION:      ShowConfiguration();    break;
            case SCREEN_STEP_DIMENSION:     ShowStepDimension();    break;
            case SCREEN_LIST_OF_STYLES:     ShowListOfStyles();     break;
            case SCREEN_STYLE_INFO:         ShowStyleInfo();        break;
            case SCREEN_PASTE_TRANSFORMED:  ShowPasteTransformed(); break;
            case SCREEN_EDIT_VIEW:          ShowEditView();         break;
        }
    }
    Printf(false, "");
    InvalidateText();
}

void TextWindow::Paint(int width, int height) {
    // We would like things pixel-exact, to avoid shimmering.
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3d(1, 1, 1);
   
    glTranslated(-1, 1, 0);
    glScaled(2.0/width, -2.0/height, 1);
    glTranslated(0, 0, 0);

    halfRows = height / (LINE_HEIGHT/2);

    int bottom = SS.TW.top[SS.TW.rows-1] + 2;
    scrollPos = min(scrollPos, bottom - halfRows);
    scrollPos = max(scrollPos, 0);

    // Let's set up the scroll bar first
    MoveTextScrollbarTo(scrollPos, SS.TW.top[SS.TW.rows - 1] + 1, halfRows);

    // Create the bitmap font that we're going to use.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // Now paint the window.
    int r, c, a;
    for(a = 0; a < 3; a++) {
        if(a == 0) {
            glBegin(GL_QUADS);
        } else if(a == 1) {
            glEnable(GL_TEXTURE_2D);
            glxCreateBitmapFont();
            glBegin(GL_QUADS);
        } else {
            glBegin(GL_LINES);
        }

        for(r = 0; r < SS.TW.rows; r++) {
            int top = SS.TW.top[r];
            if(top < (scrollPos-1)) continue;
            if(top > scrollPos+halfRows) break;

            for(c = 0; c < min((width/CHAR_WIDTH)+1, SS.TW.MAX_COLS); c++) {
                int x = LEFT_MARGIN + c*CHAR_WIDTH;
                int y = (top-scrollPos)*(LINE_HEIGHT/2) + 2;

                int fg = SS.TW.meta[r][c].fg;
                int bg = SS.TW.meta[r][c].bg;

                // On the first pass, all the background quads; on the next
                // pass, all the foreground (i.e., font) quads.
                if(a == 0) {
                    int bh = LINE_HEIGHT, adj = 0;
                    if(bg & 0x80000000) {
                        glColor3f(REDf(bg), GREENf(bg), BLUEf(bg));
                        bh = CHAR_HEIGHT;
                        adj = 2;
                    } else {
                        glColor3fv(&(bgColorTable[bg*3]));
                    }

                    if(!(bg == 'd')) {
                        // Move the quad down a bit, so that the descenders
                        // still have the correct background.
                        y += adj;
                        glBegin(GL_QUADS);
                            glVertex2d(x,              y);
                            glVertex2d(x + CHAR_WIDTH, y);
                            glVertex2d(x + CHAR_WIDTH, y + bh);
                            glVertex2d(x,              y + bh);
                        glEnd();
                        y -= adj;
                    }
                } else if(a == 1) {
                    glColor3fv(&(fgColorTable[fg*3]));
                    glxBitmapCharQuad(SS.TW.text[r][c], x, y + CHAR_HEIGHT);
                } else {
                    if(SS.TW.meta[r][c].link && SS.TW.meta[r][c].link != 'n') {
                        glColor3fv(&(fgColorTable[fg*3]));
                        y += CHAR_HEIGHT + 1;
                        glVertex2d(x, y);
                        glVertex2d(x + CHAR_WIDTH, y);
                    }
                }
            }
        }

        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
}

void TextWindow::MouseEvent(bool leftClick, double x, double y) {
    if(TextEditControlIsVisible() || GraphicsEditControlIsVisible()) {
        if(leftClick) {
            HideTextEditControl();
            HideGraphicsEditControl();
        } else {
            SetMousePointerToHand(false);
        }
        return;
    }

    GraphicsWindow::Selection ps = SS.GW.hover;
    SS.GW.hover.Clear();

    // Find the corresponding character in the text buffer
    int c = (int)((x - LEFT_MARGIN) / CHAR_WIDTH);
    int hh = (LINE_HEIGHT)/2;
    y += scrollPos*hh;
    int r;
    for(r = 0; r < SS.TW.rows; r++) {
        if(y >= SS.TW.top[r]*hh && y <= (SS.TW.top[r]+2)*hh) {
            break;
        }
    }
    if(r >= SS.TW.rows) {
        SetMousePointerToHand(false);
        goto done;
    }

#define META (SS.TW.meta[r][c])
    if(leftClick) {
        if(META.link && META.f) {
            (META.f)(META.link, META.data);
            SS.TW.Show();
            InvalidateGraphics();
        }
    } else {
        if(META.link) {
            SetMousePointerToHand(true);
            if(META.h) {
                (META.h)(META.link, META.data);
            }
        } else {
            SetMousePointerToHand(false);
        }
    }

done:
    if(!ps.Equals(&(SS.GW.hover))) {
        InvalidateGraphics();
    }
}

void TextWindow::ScrollbarEvent(int newPos) {
    int bottom = SS.TW.top[SS.TW.rows-1] + 2;
    newPos = min(newPos, bottom - halfRows);
    newPos = max(newPos, 0);

    if(newPos != scrollPos) {
        scrollPos = newPos;
        MoveTextScrollbarTo(scrollPos, SS.TW.top[SS.TW.rows - 1] + 1, halfRows);

        if(TextEditControlIsVisible()) {
            extern int TextEditControlCol, TextEditControlHalfRow;
            ShowTextEditControl(
                TextEditControlHalfRow, TextEditControlCol, NULL);
        }
        InvalidateText();
    }
}

