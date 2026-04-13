#include "../header/vector2.h"

volatile int heapCounterVector2;

/* --- Lifecycle --- */

Vector2* V2create(void) {
    Vector2* v = (Vector2*)calloc(1, sizeof(Vector2));
    heapCounterVector2 += 1;

    /* Wire method pointers */
    v->shift          = V2shift;
    v->rotate         = V2rotate;
    v->scale          = V2scale;
    v->negate         = V2negate;
    v->zero           = V2zero;
    v->copy           = V2copy;
    v->reflectOrigin  = V2reflectOrigin;
    v->reflectYAxis   = V2reflectYAxis;
    v->reflectXAxis   = V2reflectXAxis;
    v->reflectDiagonal= V2reflectDiagonal;
    v->magnitude      = V2magnitude;
    v->dot            = V2dot;
    v->distance       = V2distance;
    v->equals         = V2equals;
    v->print          = V2print;

    return v;
}

Vector2* V2createXY(int x, int y) {
    Vector2* v = V2create();
    v->x = x;
    v->y = y;
    return v;
}

void V2delete(Vector2* v) {
    free(v);
    heapCounterVector2 -= 1;
}

/* --- Mutating operations --- */

void V2shift(struct _Vector2* self, struct _Vector2* target) {
    self->x += target->x;
    self->y += target->y;
}

void V2rotate(struct _Vector2* self, Angle angle) {
    double nx = (self->x * cos(angle)) - (self->y * sin(angle));
    double ny = (self->x * sin(angle)) + (self->y * cos(angle));
    self->x = (int)round(nx);
    self->y = (int)round(ny);
}

void V2scale(struct _Vector2* self, double scalar) {
    self->x = (int)round(self->x * scalar);
    self->y = (int)round(self->y * scalar);
}

void V2negate(struct _Vector2* self) {
    self->x = -self->x;
    self->y = -self->y;
}

void V2zero(struct _Vector2* self) {
    self->x = 0;
    self->y = 0;
}

void V2copy(struct _Vector2* dst, struct _Vector2* src) {
    dst->x = src->x;
    dst->y = src->y;
}

/* Point reflection through the origin: (x,y) -> (-x,-y) */
void V2reflectOrigin(struct _Vector2* self) {
    self->x = -self->x;
    self->y = -self->y;
}

void V2reflectYAxis(struct _Vector2* self) {
    self->x = -self->x;
}

void V2reflectXAxis(struct _Vector2* self) {
    self->y = -self->y;
}

/* Reflection over the line y=x: swap components */
void V2reflectDiagonal(struct _Vector2* self) {
    int temp = self->x;
    self->x  = self->y;
    self->y  = temp;
}

/* --- Non-mutating queries --- */

double V2magnitude(struct _Vector2* self) {
    return sqrt((double)(self->x * self->x) + (double)(self->y * self->y));
}

double V2dot(struct _Vector2* self, struct _Vector2* other) {
    return (double)(self->x * other->x) + (double)(self->y * other->y);
}

double V2distance(struct _Vector2* self, struct _Vector2* other) {
    double dx = (double)(self->x - other->x);
    double dy = (double)(self->y - other->y);
    return sqrt(dx * dx + dy * dy);
}

int V2equals(struct _Vector2* self, struct _Vector2* other) {
    return self->x == other->x && self->y == other->y;
}

void V2print(struct _Vector2* self) {
    printf("Vector2(%d, %d)\n", self->x, self->y);
}


#ifdef DEBUG_VECTOR2
int main(void) {
    Vector2* a = V2createXY(3, 0);
    Vector2* b = V2createXY(1, 2);

    printf("a = "); a->print(a);
    printf("b = "); b->print(b);

    a->shift(a, b);
    printf("a after shift by b: "); a->print(a);

    a->rotate(a, M_PI / 2);
    printf("a after 90-deg rotation: "); a->print(a);

    printf("magnitude of b: %.4f\n", b->magnitude(b));
    printf("dot(a, b): %.4f\n", a->dot(a, b));
    printf("distance(a, b): %.4f\n", a->distance(a, b));
    printf("a equals b: %d\n", a->equals(a, b));

    b->reflectOrigin(b);
    printf("b after reflectOrigin: "); b->print(b);

    b->reflectDiagonal(b);
    printf("b after reflectDiagonal: "); b->print(b);

    printf("heap live count: %d\n", heapCounterVector2);
    V2delete(a);
    V2delete(b);
    printf("heap live count after deletes: %d\n", heapCounterVector2);

    return 0;
}
#endif // DEBUG_VECTOR2
