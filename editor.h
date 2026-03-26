#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include "cursor.h"
#include "undo.h"
#include "screen.h"

typedef struct
{
    Buffer buffer;
    Cursor cursor;
    UndoStack undo;

    char filename[260];
    int file_loaded;

    char *clipboard;
    int clipboard_len;

    char search_query[100];
    char replace_query[100];
    int search_len;
    int *search_results;  
    int search_count;      
    int search_current;  

    int selection_start;
    int selection_end;
    int selection_active;

    int modified;

} Editor;

int editor_init(Editor *ed);

void editor_free(Editor *ed);

void editor_render(Editor *ed);

int editor_process_input(Editor *ed, int ch, int *running);

void editor_open_file(Editor *ed);
void editor_save_file(Editor *ed, int force_save_as);

#endif