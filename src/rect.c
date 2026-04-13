#include "../header/rect.h"


/* ═══════════════════════════════════════════════════════════════════════
 *  rect.c  —  fully implemented
 *
 *  No TODOs here: this file is meant to be read as reference code.
 *  The logic is identical to coordinate-geometry problems you already
 *  know from PS — just in a struct wrapper.
 * ═══════════════════════════════════════════════════════════════════════ */


/* ── Construction ─────────────────────────────────────────────────────── */

Rect RectMake(int x, int y, int w, int h) {
    Rect r = { x, w, y, h };
    return r;
}

Rect RectFromCorners(int x0, int y0, int x1, int y1) {
    /* Normalise so (x0,y0) is always the top-left corner */
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    return RectMake(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}


/* ── Geometry queries ─────────────────────────────────────────────────── */

int RectContains(Rect r, int cx, int cy) {
    return cx >= r.x && cx < r.x + r.w &&
           cy >= r.y && cy < r.y + r.h;
}

int RectIntersects(Rect a, Rect b) {
    /* Two intervals [a, a+w) and [b, b+w) overlap iff a < b+bw && b < a+aw.
     * Apply the same check independently on both axes. */
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}

Rect RectIntersection(Rect a, Rect b) {
    /* Take the max of the left edges and the min of the right edges */
    int x0 = a.x > b.x ? a.x : b.x;
    int y0 = a.y > b.y ? a.y : b.y;
    int x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    int y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
    return RectMake(x0, y0, x1 - x0, y1 - y0);
}


/* ── Layout helpers ───────────────────────────────────────────────────── */

void RectSplitH(Rect r, int at, Rect* top, Rect* bot) {
    *top = RectMake(r.x, r.y,      r.w, at);
    *bot = RectMake(r.x, r.y + at, r.w, r.h - at);
}

void RectSplitV(Rect r, int at, Rect* left, Rect* right) {
    *left  = RectMake(r.x,      r.y, at,       r.h);
    *right = RectMake(r.x + at, r.y, r.w - at, r.h);
}

Rect RectInset(Rect r, int margin) {
    return RectMake(r.x + margin,
                    r.y + margin,
                    r.w - 2 * margin,
                    r.h - 2 * margin);
}
