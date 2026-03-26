#ifndef UNDO_H
#define UNDO_H

typedef struct UndoAction
{
    int type;
    int position;
    char character;
} UndoAction;

typedef struct UndoStack
{
    UndoAction *data;
    int top;
    int capacity;
} UndoStack;

void undo_init(UndoStack *st);

void undo_free(UndoStack *st);

int undo_push(UndoStack *st, UndoAction action);

int undo_pop(UndoStack *st, UndoAction *action);

#endif