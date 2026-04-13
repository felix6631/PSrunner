#ifndef RECT_H
#define RECT_H


/* ═══════════════════════════════════════════════════════════════════════
 *  Rect  —  axis-aligned rectangle in terminal coordinate space
 *
 *  Origin convention: x = column (1-based), y = row (1-based)
 *  Width/height are measured in character cells.
 *
 *  This is the layout primitive that every TUI panel, border, and
 *  widget will be built on top of.
 * ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    int x;   /* left edge   — column, 1-based */
    int w;   /* width  in columns             */
    int y;   /* top  edge   — row,    1-based */
    int h;   /* height in rows                */
} Rect;


/* ── Construction ─────────────────────────────────────────────────────── */

/* Return a Rect given top-left corner and size. */
Rect RectMake(int x, int y, int w, int h);

/* Return the smallest Rect covering two corner points (inclusive). */
Rect RectFromCorners(int x0, int y0, int x1, int y1);


/* ── Geometry queries ─────────────────────────────────────────────────── */

/* 1 if column cx / row cy lies inside r (inclusive edges), else 0. */
int  RectContains(Rect r, int cx, int cy);

/* 1 if two rects share any area, else 0. */
int  RectIntersects(Rect a, Rect b);

/* Return the overlapping region. Behaviour is undefined if !RectIntersects. */
Rect RectIntersection(Rect a, Rect b);


/* ── Layout helpers ───────────────────────────────────────────────────── */

/*  Split horizontally at row offset `at` (relative to r.y).
 *  *top  gets rows [r.y, r.y+at),  height = at
 *  *bot  gets rows [r.y+at, r.y+r.h), height = r.h - at
 *
 *  Example — split a 20-row rect, giving the top pane 3 rows:
 *    RectSplitH(r, 3, &header, &body);   // header.h==3, body.h==17
 */
void RectSplitH(Rect r, int at, Rect* top, Rect* bot);

/*  Split vertically at column offset `at` (relative to r.x).
 *  *left  gets columns [r.x, r.x+at),      width = at
 *  *right gets columns [r.x+at, r.x+r.w),  width = r.w - at
 */
void RectSplitV(Rect r, int at, Rect* left, Rect* right);

/*  Shrink r by `margin` cells on all four sides.
 *  Use this to get the inner content area of a bordered box:
 *    Rect inner = RectInset(outer, 1);
 */
Rect RectInset(Rect r, int margin);


#endif /* RECT_H */
