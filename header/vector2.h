#ifndef VECTOR2_H
#define VECTOR2_H
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "angle.h"

typedef struct _Vector2 {
    int x, y;

    /* Mutating method pointers */
    void   (*shift)(struct _Vector2*, struct _Vector2*);
    void   (*rotate)(struct _Vector2*, Angle);
    void   (*scale)(struct _Vector2*, double);
    void   (*negate)(struct _Vector2*);
    void   (*zero)(struct _Vector2*);
    void   (*copy)(struct _Vector2* self, struct _Vector2* src);
    void   (*reflectOrigin)(struct _Vector2*);   /* point reflection: (-x, -y)  */
    void   (*reflectYAxis)(struct _Vector2*);    /* flip x                      */
    void   (*reflectXAxis)(struct _Vector2*);    /* flip y                      */
    void   (*reflectDiagonal)(struct _Vector2*); /* swap x and y (y = x line)   */

    /* Non-mutating query method pointers */
    double (*magnitude)(struct _Vector2*);
    double (*dot)(struct _Vector2*, struct _Vector2*);
    double (*distance)(struct _Vector2*, struct _Vector2*);
    int    (*equals)(struct _Vector2*, struct _Vector2*);
    void   (*print)(struct _Vector2*);
} Vector2;

/* Heap counter — tracks live Vector2 allocations */
extern volatile int heapCounterVector2;

/* Lifecycle */
Vector2* V2create(void);
Vector2* V2createXY(int x, int y);
void     V2delete(Vector2*);

/* Mutating operations */
void   V2shift(struct _Vector2*, struct _Vector2*);
void   V2rotate(struct _Vector2*, Angle);
void   V2scale(struct _Vector2*, double scalar);
void   V2negate(struct _Vector2*);
void   V2zero(struct _Vector2*);
void   V2copy(struct _Vector2* dst, struct _Vector2* src);
void   V2reflectOrigin(struct _Vector2*);
void   V2reflectYAxis(struct _Vector2*);
void   V2reflectXAxis(struct _Vector2*);
void   V2reflectDiagonal(struct _Vector2*);

/* Non-mutating queries */
double V2magnitude(struct _Vector2*);
double V2dot(struct _Vector2*, struct _Vector2*);
double V2distance(struct _Vector2*, struct _Vector2*);
int    V2equals(struct _Vector2*, struct _Vector2*);
void   V2print(struct _Vector2*);

#endif // VECTOR2_H
