#include <stdio.h>
#include "file.h"
#include "buffer.h"
#include "cursor.h"

int file_open(Editor *ed, const char *filename) //dosya açma ve içeriği buffera yükleme
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return 0;

    buffer_free(&ed->buffer);
    buffer_init(&ed->buffer);

    int c;

    while ((c = fgetc(fp)) != EOF)
    {
        buffer_insert(&ed->buffer, ed->buffer.length, (char)c);
    }

    fclose(fp);

    ed->cursor.index = 0;
    cursor_update_from_index(&ed->buffer, &ed->cursor);

    return 1;
}

int file_save(Editor *ed, const char *filename) //dosya kaydetme
{
    FILE *fp = fopen(filename, "w");
    if (!fp)
        return 0;

    fwrite(ed->buffer.data, sizeof(char), ed->buffer.length, fp);

    fclose(fp);

    return 1;
}