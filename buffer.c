#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

#define INITIAL_CAPACITY 128

static int buffer_grow(Buffer *buf, int min_capacity)
{
    int new_capacity = buf->capacity;

    if (new_capacity == 0)
        new_capacity = INITIAL_CAPACITY;

    while (new_capacity < min_capacity)
        new_capacity *= 2;

    char *new_data = (char *)realloc(buf->data, new_capacity * sizeof(char)); //belleği yeniden boyutla,, eski veri *buf ile korunur
    if (!new_data)
        return 0;

    buf->data = new_data;
    buf->capacity = new_capacity;
    return 1;
}

int buffer_init(Buffer *buf) //ilk buffer işlemi için temizlik
{
    buf->data = NULL;
    buf->length = 0;
    buf->capacity = 0;

    return buffer_grow(buf, INITIAL_CAPACITY);
}

void buffer_free(Buffer *buf)
{
    if (buf->data)
        free(buf->data);
    buf->data = NULL;
    buf->length = 0;
    buf->capacity = 0;
}

int buffer_insert(Buffer *buf, int index, char c)//buffera karakter ekleme
{
    if (index < 0 || index > buf->length)
        return 0;

    if (buf->length + 1 > buf->capacity)
    {
        if (!buffer_grow(buf, buf->length + 1))
            return 0;
    }

    if (index < buf->length) //araya karakter ekleme işlemi
    {
        memmove(buf->data + index + 1, buf->data + index, (size_t)(buf->length - index)); //sağ tarafa kaydırma yapar
    }

    buf->data[index] = c;
    buf->length++;

    return 1;
}

int buffer_delete(Buffer *buf, int index) //karakter silme
{
    if (index < 0 || index >= buf->length)
        return 0;

    if (index < buf->length - 1) //son karakter değilse bir sola kaydırma
    {
        memmove(buf->data + index, buf->data + index + 1, (size_t)(buf->length - index - 1));
    }

    buf->length--;

    return 1;
}

char buffer_get_char(Buffer *buf, int index) //karakter okuma
{
    if (index < 0 || index >= buf->length)
        return '\0';
    return buf->data[index]; //karakteri döndürür
}