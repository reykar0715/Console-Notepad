#ifndef CURSOR_H
#define CURSOR_H

#include "buffer.h"

typedef struct Cursor
{
    int x;
    int y;
    int index;
} Cursor;

void cursor_init(Cursor *c);

void cursor_update_from_index(Buffer *buf, Cursor *c);

void cursor_move_left(Buffer *buf, Cursor *c);
void cursor_move_right(Buffer *buf, Cursor *c);
void cursor_move_up(Buffer *buf, Cursor *c);
void cursor_move_down(Buffer *buf, Cursor *c);

void cursor_move_word_left(Buffer *buf, Cursor *c);
void cursor_move_word_right(Buffer *buf, Cursor *c);

#endif