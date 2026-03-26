#include <stdio.h>
#include <stdlib.h>

#include "undo.h"

#define UNDO_INITIAL_CAPACITY 64 //yığın için ayırdığımız kapasite

void undo_init(UndoStack *st) //yığını oluşturma
{
    st->data = (UndoAction *)malloc(UNDO_INITIAL_CAPACITY * sizeof(UndoAction));
    if (st->data == NULL)
    {
        st->capacity = 0;
        st->top = -1;
        return;
    }
    st->capacity = UNDO_INITIAL_CAPACITY;
    st->top = -1;
}

void undo_free(UndoStack *st) //yığının belleğini serbest bırakma
{
    if (st->data)
        free(st->data);
    st->data = NULL;
    st->capacity = 0;
    st->top = -1;
}

int undo_push(UndoStack *st, UndoAction action) //hafızaya ekleme
{
    if (st->capacity == 0)
        return 0;

    if (st->top + 1 >= st->capacity)
    {
        int new_capacity = st->capacity * 2;
        UndoAction *new_data = (UndoAction *)realloc(st->data, new_capacity * sizeof(UndoAction));
        if (!new_data)
            return 0;

        st->data = new_data;
        st->capacity = new_capacity;
    }

    st->top++;
    st->data[st->top] = action;
    return 1;
}

int undo_pop(UndoStack *st, UndoAction *action) //hafızadan geri çağırma
{
    if (st->top < 0)
        return 0;

    *action = st->data[st->top];
    st->top--;
    return 1;
}