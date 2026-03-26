#ifndef BUFFER_H
#define BUFFER_H

typedef struct Buffer
{
    char *data;
    int length;
    int capacity;
} Buffer;

int buffer_init(Buffer *buf);

void buffer_free(Buffer *buf);

int buffer_insert(Buffer *buf, int index, char c);

int buffer_delete(Buffer *buf, int index);

char buffer_get_char(Buffer *buf, int index);

#endif