#include <stdio.h>

#include "cursor.h"
#include "buffer.h"
#include "tool_box.h"

static int find_line_start(Buffer *buf, int index) //satır başı bulma
{
    if (index <= 0)
        return 0;

    int i = index - 1;
    for (i; i >= 0; --i)
    {
        if (buf->data[i] == '\n') //new line bulunca satır başı yapar
            return i + 1;
    }
    return 0;
}

static int find_line_end(Buffer *buf, int index) //satır sonunu döndürme
{
    int i = index;
    for (; i < buf->length; ++i)
    {
        if (buf->data[i] == '\n')
            return i;
    }
    return buf->length;
}

void cursor_init(Cursor *c) //imleci başlatma
{
    c->x = 0; //x satır y sutun olarak cursorün yerini tutar
    c->y = 0;
    c->index = 0; //ekrandaki yerini tutar
}

void cursor_update_from_index(Buffer *buf, Cursor *c) //x,yden ekran koordinatı dönüşümü
{
    int i;
    const int marginWidth = 6;
    
    int x = marginWidth;
    int y = toolboxHeight() + 1;

    if (c->index < 0) c->index = 0;
    if (c->index > buf->length) c->index = buf->length;

    for (i = 0; i < c->index && i < buf->length; ++i)
    {
        unsigned char ch = (unsigned char)buf->data[i];

        if (ch == '\n')
        {
            y++;
            x = marginWidth;
        }
        else //eğer ki türkçe karakterse 2 karakter olduğu için birini saymıyoruz.
        {
            if ((ch & 0xC0) != 0x80)
            {
                x++;
            }
        }
    }

    c->x = x;
    c->y = y;
}

//cursor hareketleri
void cursor_move_left(Buffer *buf, Cursor *c)
{
    if (c->index <= 0)
        return;
    c->index--;
    cursor_update_from_index(buf, c);
}

void cursor_move_right(Buffer *buf, Cursor *c)
{
    if (c->index >= buf->length)
        return;
    c->index++;
    cursor_update_from_index(buf, c);
}

void cursor_move_up(Buffer *buf, Cursor *c) //alt ve üst hareketlerinde imleç konumu x'in indeksine kadar ilerletilir.
{
    if (c->index <= 0)
        return;

    int target_x = c->x;
    int line_start = find_line_start(buf, c->index);
    int prev_line_end = (line_start > 0) ? line_start - 1 : 0;
    int prev_line_start = find_line_start(buf, prev_line_end);

    int i = prev_line_start;
    int new_index = prev_line_start;
    int x = 0;

    while (i < buf->length && buf->data[i] != '\n')
    {
        if (x >= target_x)
            break;
        x++;
        i++;
        new_index++;
    }

    c->index = new_index;
    cursor_update_from_index(buf, c);
}

void cursor_move_down(Buffer *buf, Cursor *c)
{
    if (c->index >= buf->length)
        return;

    int target_x = c->x;
    int line_end = find_line_end(buf, c->index);

    if (line_end >= buf->length)
        return;

    int next_line_start = line_end + 1;
    int i = next_line_start;
    int new_index = next_line_start;
    int x = 0;

    while (i < buf->length && buf->data[i] != '\n')
    {
        if (x >= target_x)
            break;
        x++;
        i++;
        new_index++;
    }

    c->index = new_index;
    cursor_update_from_index(buf, c);
}

void cursor_move_word_left(Buffer *buf, Cursor *c) //Kelime seçimlerinde cursorün konumunu ayarlama
{
    if (c->index <= 0)
        return;

    int i = c->index;

    if (i > 0)
        i--;

    while (i > 0 && (buf->data[i] == ' ' || buf->data[i] == '\t' || buf->data[i] == '\n'))
        i--;

    while (i > 0 && buf->data[i - 1] != ' ' && buf->data[i - 1] != '\t' && buf->data[i - 1] != '\n')
        i--;

    c->index = i;
    cursor_update_from_index(buf, c);
}

void cursor_move_word_right(Buffer *buf, Cursor *c)
{
    if (c->index >= buf->length)
        return;

    int i = c->index;

    while (i < buf->length && buf->data[i] != ' ' && buf->data[i] != '\t' && buf->data[i] != '\n')
        i++;

    while (i < buf->length && (buf->data[i] == ' ' || buf->data[i] == '\t' || buf->data[i] == '\n'))
        i++;

    c->index = i;
    cursor_update_from_index(buf, c);
}