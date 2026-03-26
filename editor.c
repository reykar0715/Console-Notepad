#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <string.h>

#include "editor.h"
#include "buffer.h"
#include "cursor.h"
#include "undo.h"
#include "screen.h"
#include "tool_box.h"
#include "file.h"
#include "file_browser.h"

#define UNDO_TYPE_INSERT 1 //undo karakter ekleme işlemleri için
#define UNDO_TYPE_DELETE 2 //undo silme işlemleri için

void editor_prompt(Editor *ed, const char *prompt, char *buf, int buf_len);
void editor_search(Editor *ed);
static void editor_delete_selection(Editor *ed);

static void editor_clear_selection(Editor *ed) //seçimi temizleme
{
    ed->selection_active = 0;
}

void editor_replace(Editor *ed) { //CTRL + H, kelime değiştirme
    editor_prompt(ed, "Aranan Kelime:", ed->search_query, sizeof(ed->search_query));
    if (ed->search_query[0] == '\0') return;

    editor_prompt(ed, "Yeni Kelime:", ed->replace_query, sizeof(ed->replace_query));

    int s_len = (int)strlen(ed->search_query);
    int r_len = (int)strlen(ed->replace_query);
    int count = 0;

    //tüm eşleşmeleri bulup değiştiriyoruz bu işlemi de kelimedeki karakter sayısı değişebileceğinden manuel yapıyoruz.
    int i = 0;
    while (i <= ed->buffer.length - s_len) {
        if (strncmp(&ed->buffer.data[i], ed->search_query, s_len) == 0) {
            for (int j = 0; j < s_len; j++) {
                buffer_delete(&ed->buffer, i);
            }
            for (int j = 0; j < r_len; j++) {
                buffer_insert(&ed->buffer, i + j, ed->replace_query[j]);
            }
            
            i += r_len;
            count++;
        } else {
            i++;
        }
    }

    ed->search_len = 0; 
    
    cursor_update_from_index(&ed->buffer, &ed->cursor);
}

void editor_open_file(Editor *ed) //dosya açma
{
    char path[260];

    extern int is_save_mode;
    is_save_mode = 0;

    if (!file_browser_select(path, (int)sizeof(path)))
        return;

    if (file_open(ed, path))
    {
        strcpy(ed->filename, path);
        ed->file_loaded = 1;
    }
}


void editor_save_file(Editor *ed, int force_save_as) //dosya kaydetme
{
    char dir[260];
    char filename[260];

    if (!ed->file_loaded || force_save_as)
    {
        extern int is_save_mode;
        is_save_mode = 1;

        if (!file_browser_select(dir, sizeof(dir)))
            return;

        editor_prompt(ed, "Dosya adı:", filename, sizeof(filename));
        if (filename[0] == '\0') return;

        char *ext = strrchr(filename, '.'); //.txt yoksa ekleme
        if (!(ext && _stricmp(ext, ".txt") == 0))
            strcat(filename, ".txt");

        char full[260];
        snprintf(full, sizeof(full), "%s\\%s", dir, filename);

        int counter = 1;
        while (1) //aynı isimde dosya varsa 1, 2 gibi ek ekler
        {
            FILE *fp = fopen(full, "r");
            if (!fp) break;
            fclose(fp);

            char base[260];
            char *dot = strrchr(filename, '.');

            int len = dot ? (int)(dot - filename) : (int)strlen(filename);
            strncpy(base, filename, len);
            base[len] = '\0';

            snprintf(full, sizeof(full), "%s\\%s(%d).txt", dir, base, counter++);
        }

        strcpy(ed->filename, full);
    }

    file_save(ed, ed->filename);
    ed->file_loaded = 1;
    ed->modified = 0;
}

void editor_prompt(Editor *ed, const char *prompt, char *buf, int buf_len) //girdi alma, (buffer için)
{
    int n = 0;
    buf[0] = '\0';

    while (1) {
        gotoxy(0, toolboxHeight() + 1); //toolboxın altından yazdırma
        printf("\x1b[1;33m%s\x1b[0m %s_ ", prompt, buf);

        int ch = _getch();
        if (ch == 8) { // Backspace
            if (n > 0) buf[--n] = '\0';
        } else if (ch == 27) { // esc iptal
            buf[0] = '\0';
            break;
        } else if (ch == 13) { // enter
            break;
        } else if (ch >= 32 && n < buf_len - 1) {
            buf[n++] = (char)ch;
            buf[n] = '\0';
        }
    }
}

void editor_search(Editor *ed) //CTRL + F ve + G, arama 
{
    editor_prompt(ed, "Aranacak:", ed->search_query, sizeof(ed->search_query));
     
    if (ed->search_query[0] == '\0') {
        ed->search_len = 0;
        return;
    }
    ed->search_len = (int)strlen(ed->search_query);

    //ilk eşleşmeyi bulup imleci oraya taşıyoruz
    char *match = strstr(ed->buffer.data, ed->search_query);
    if (match) {
        ed->cursor.index = (int)(match - ed->buffer.data);
        cursor_update_from_index(&ed->buffer, &ed->cursor);
    }
}

void editor_find_all_matches(Editor *ed) //tüm eşleşmeleri bulabilmek için
{
    if (ed->search_results) free(ed->search_results);
    ed->search_results = NULL;
    ed->search_count = 0;
    ed->search_current = -1;

    if (ed->search_query[0] == '\0') return;

    int query_len = (int)strlen(ed->search_query);
    
    for (int i = 0; i <= ed->buffer.length - query_len; i++) {
        if (strncmp(&ed->buffer.data[i], ed->search_query, query_len) == 0) {
            ed->search_count++;
            ed->search_results = realloc(ed->search_results, sizeof(int) * ed->search_count);
            ed->search_results[ed->search_count - 1] = i;
        }
    }

    if (ed->search_count > 0) {
        ed->search_current = 0;
        ed->cursor.index = ed->search_results[0];
        cursor_update_from_index(&ed->buffer, &ed->cursor);
    }
}

void editor_search_interactive(Editor *ed) //üstteki iki fonksiyonu kullanarak arama işlemini tamamlama
{
    editor_prompt(ed, "Aranacak Kelime:", ed->search_query, sizeof(ed->search_query));
    
    if (ed->search_query[0] == '\0') {
        ed->search_len = 0;
        return;
    }
    ed->search_len = (int)strlen(ed->search_query);
    editor_find_all_matches(ed);
}

void editor_search_next(Editor *ed, int direction) //CTRL + G bulunan eşleşmeler arası gezinme
{
    if (ed->search_count <= 0){
        return;
    }

    ed->search_current += direction;

    if (ed->search_current >= ed->search_count) ed->search_current = 0; //sonuncudan sonra baştakine gönderir
    if (ed->search_current < 0) ed->search_current = ed->search_count - 1; //ilkteyken öncekine basınca sonuncuya gönderir

    ed->cursor.index = ed->search_results[ed->search_current];
    cursor_update_from_index(&ed->buffer, &ed->cursor);
}

static void editor_insert_char(Editor *ed, char c) //yeni karakter girdisi alma
{
    if (!buffer_insert(&ed->buffer, ed->cursor.index, c))
        return;

    UndoAction act;
    act.type = UNDO_TYPE_INSERT;
    act.position = ed->cursor.index;
    act.character = c; //eklenen karakter
    undo_push(&ed->undo, act); //undo yapısına ekleme

    ed->cursor.index++;
    cursor_update_from_index(&ed->buffer, &ed->cursor);
    ed->modified = 1;
}

static void editor_copy_selection(Editor *ed) //CTRL + C, kopyalama
{
    if (!ed->selection_active)
        return;

    int start = ed->selection_start;
    int end   = ed->selection_end;

    if (start > end) { //ters çevirme
        int tmp = start;
        start = end;
        end = tmp;
    }

    if (start < 0) start = 0; //seçim 0 veya daha küçükse veya geçersizse kontrol yapıyoruz
    if (end > ed->buffer.length) end = ed->buffer.length;

    int len = end - start;
    if (len <= 0 || len > ed->buffer.length)
        return;

    if (ed->clipboard)
        free(ed->clipboard);

    ed->clipboard = (char*)malloc(len); //clipboard temizleme
    ed->clipboard_len = len;

    for (int i = 0; i < len; i++) { //secili alanı temiz clipboarda yazma
        ed->clipboard[i] = ed->buffer.data[start + i];
    }
}

int utf8_char_length_backward(const char *data, int index) //türkçe karakter için karakter byte kontrolü
{
    if (index <= 0) return 1;

    int i = index - 1;

    // UTF-8 uzunluk kontrolü
    while (i > 0 && (data[i] & 0xC0) == 0x80)
        i--;

    return index - i;
}

static void editor_backspace(Editor *ed) //BACKSPACE işlemi
{
    if (ed->cursor.index <= 0)
        return;

    int len = utf8_char_length_backward(ed->buffer.data, ed->cursor.index);

    for (int i = 0; i < len; i++) //karakterin byteı kadar silme
    {
        char removed = buffer_get_char(&ed->buffer, ed->cursor.index - 1);

        if (!buffer_delete(&ed->buffer, ed->cursor.index - 1))
            return;

        UndoAction act;
        act.type = UNDO_TYPE_DELETE;
        act.position = ed->cursor.index - 1;
        act.character = removed;
        undo_push(&ed->undo, act); //undo yığınına karakteri gönderme

        ed->cursor.index--;
    }

    cursor_update_from_index(&ed->buffer, &ed->cursor);
    ed->modified = 1;
}

static void editor_delete_selection(Editor *ed) //seçili alanı silme işlemi
{
    if (!ed->selection_active)
        return;

    int start = ed->selection_start;
    int end   = ed->selection_end;

    if (start > end) { //terse çevirme
        int tmp = start;
        start = end;
        end = tmp;
    }

    int count = end - start;

    for (int i = 0; i < count; i++) {
        buffer_delete(&ed->buffer, start);
    }

    ed->cursor.index = start;

    cursor_update_from_index(&ed->buffer, &ed->cursor);

    ed->selection_active = 0;
    ed->modified = 1;
}

static void editor_paste(Editor *ed) //CTRL + V, yapıştırma
{
    if (!ed->clipboard || ed->clipboard_len <= 0)
        return;

    if (ed->selection_active) {
        editor_delete_selection(ed);
    }

    for (int i = 0; i < ed->clipboard_len; i++) {
        buffer_insert(&ed->buffer, ed->cursor.index, ed->clipboard[i]);

        UndoAction act;
        act.type = UNDO_TYPE_INSERT;
        act.position = ed->cursor.index;
        act.character = ed->clipboard[i];
        undo_push(&ed->undo, act); //undo yığınına gönderme

        ed->cursor.index++;
        ed->modified = 1;
    }

    cursor_update_from_index(&ed->buffer, &ed->cursor);
}

static void editor_delete(Editor *ed) //silme
{
    if (ed->cursor.index >= ed->buffer.length)
        return;

    char removed = buffer_get_char(&ed->buffer, ed->cursor.index);
    if (!buffer_delete(&ed->buffer, ed->cursor.index))
        return;

    UndoAction act;
    act.type = UNDO_TYPE_DELETE;
    act.position = ed->cursor.index;
    act.character = removed;
    undo_push(&ed->undo, act);

    cursor_update_from_index(&ed->buffer, &ed->cursor);
    ed->modified = 1;
}

static void editor_delete_word_left(Editor *ed) //SHIFT BACKSPACE, kelime silme
{
    if (ed->cursor.index <= 0)
        return;

    while (ed->cursor.index > 0) //boşluk varsa önce boşluğu siliyoruz
    {
        char c = buffer_get_char(&ed->buffer, ed->cursor.index - 1);

        if (c != ' ' && c != '\t' && c != '\n')
            break;

        editor_backspace(ed);
    }

    while (ed->cursor.index > 0) //kelime silme
    {
        char c = buffer_get_char(&ed->buffer, ed->cursor.index - 1);

        if (c == ' ' || c == '\t' || c == '\n')
            break;

        editor_backspace(ed);
    }
}

static void editor_undo(Editor *ed) //CTRL + Z, geri alma
{
    UndoAction act;
    if (!undo_pop(&ed->undo, &act))
        return;

    if (act.type == UNDO_TYPE_INSERT) //eklenen karakteri silme
    {
        if (act.position < 0 || act.position >= ed->buffer.length + 1)
            return;

        if (act.position < ed->buffer.length)
        {
            buffer_delete(&ed->buffer, act.position);
            if (ed->cursor.index > act.position)
            {
                ed->cursor.index--;
            }
        }
    }
    else if (act.type == UNDO_TYPE_DELETE) //silinen karakteri ekleme
    {
        if (act.position < 0 || act.position > ed->buffer.length)
            return;

        buffer_insert(&ed->buffer, act.position, act.character);
        if (ed->cursor.index >= act.position)
        {
            ed->cursor.index++;
        }
    }

    if (ed->cursor.index < 0)
        ed->cursor.index = 0;
    if (ed->cursor.index > ed->buffer.length)
        ed->cursor.index = ed->buffer.length;

    cursor_update_from_index(&ed->buffer, &ed->cursor);
}

int editor_init(Editor *ed) //program başlayınca temizleme
{
    ed->modified = 0;
    ed->filename[0] = '\0';
    ed->file_loaded = 0;

    if (!buffer_init(&ed->buffer))
        return 0;

    cursor_init(&ed->cursor);
    undo_init(&ed->undo);
    
    ed->clipboard = NULL;
    ed->clipboard_len = 0;
    
    ed->selection_active = 0;
    ed->selection_start = 0;
    ed->selection_end = 0;
    ed->search_results = NULL;
    ed->search_count = 0;
    ed->search_current = -1;
    ed->search_len = 0;
    ed->search_query[0] = '\0';

    return 1;
}

void editor_free(Editor *ed)
{
    buffer_free(&ed->buffer);
    undo_free(&ed->undo);
    free(ed->clipboard);
}

void editor_render(Editor *ed) //bufferdaki verileri düzenleyip (renk vs) ekrana yazdırma
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    //ekran çizilirken cursor hareket etmesin diye gizliyoruz
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    drawToolbox();
    
    int startY = toolboxHeight() + 1;

    gotoxy(0, startY);
    screen_clear_from_cursor();

    int lineNum = 1;
    printf("%3d | ", lineNum); //satır numaralarını yazdırma

    int sel_start = ed->selection_start;
    int sel_end   = ed->selection_end;

    if (sel_start > sel_end) {
        int tmp = sel_start;
        sel_start = sel_end;
        sel_end = tmp;
    }

    for (int i = 0; i < ed->buffer.length; i++) {
        char c = ed->buffer.data[i];

        if (ed->selection_active && i >= sel_start && i < sel_end) {
            printf("\x1b[43m"); //sarı
            putchar(c);
            printf("\x1b[0m");
            continue;
        }

        if (ed->search_len > 0 && i <= ed->buffer.length - ed->search_len && strncmp(&ed->buffer.data[i], ed->search_query, ed->search_len) == 0)
        {
            printf("\x1b[44m"); // mavi arka plan

            for (int j = 0; j < ed->search_len; j++) {
                putchar(ed->buffer.data[i + j]);
            }

            printf("\x1b[0m");

            i += (ed->search_len - 1);
            continue;
        }

        putchar(c);

        if (c == '\n') {
            lineNum++;
            printf("%3d | ", lineNum);
        }
    }

    gotoxy(ed->cursor.x, ed->cursor.y);

    //cursorü tekrar görünür yapıyoruz
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

static void handle_arrow_keys(Editor *ed, int code) //TUŞ girişi alıyoruz yön için
{
    int shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0; //shift

    int old_index = ed->cursor.index;

    int move_word = 0;

    if (shift) //tön tuşlarına basılırsa cursorü hareket ettiriyor
    {
        if (code == 116 || code == 115) {
            move_word = 1;
        }
    }

    if (move_word)
    {
        if (code == 116) // sağ
            cursor_move_word_right(&ed->buffer, &ed->cursor);
        else if (code == 115) // sol
            cursor_move_word_left(&ed->buffer, &ed->cursor);
    }
    else
    {
        if (code == 75 || code == 115) // sol
            cursor_move_left(&ed->buffer, &ed->cursor);
        else if (code == 77 || code == 116) // sağ
            cursor_move_right(&ed->buffer, &ed->cursor);
        else if (code == 72)
            cursor_move_up(&ed->buffer, &ed->cursor);
        else if (code == 80)
            cursor_move_down(&ed->buffer, &ed->cursor);
    }

    if (GetKeyState(VK_CONTROL) & 0x8000) //başlangıç noktasını kaydedip kelime olarak imleci atlatır
    {
        if (!ed->selection_active)
        {
            ed->selection_start = old_index;
            ed->selection_active = 1;
        }
        ed->selection_end = ed->cursor.index;
    }
    else
    {
        ed->selection_active = 0; //burayı silersek seçim alanı sonsuza kadar duruyor
    }
}

int editor_process_input(Editor *ed, int ch, int *running) //normal karakter, kısayollar ve enter,esc,BS girdilerini alma
{
    if (ch >= 1 && ch <= 26 || ch == 127) 
    {
        switch (ch) {
            case 3:  //CTRL + C 
                if (ed->selection_active) {
                    editor_copy_selection(ed);
                }
                return 0;

            case 6:  // CTRL + F
                editor_search(ed); 
                return 1;
            
            case 7:  //CTRL + G
                editor_search_interactive(ed);
                return 1;

            case 8:  // Backspace ve CTRL + H
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) { //ctrl h için
                    editor_replace(ed);
                } 
                else if (ed->selection_active) { //seçim varsa silme
                    editor_delete_selection(ed); 
                }
                else {
                    editor_backspace(ed);
                }
                return 1;

            case 13: //ENTER
                editor_insert_char(ed, '\n');
                ed->search_len = 0;
                ed->search_count = 0;
                return 1;

            case 15:  //CTRL + O
                editor_open_file(ed);
                return 1;

            case 19: //CTRL+S
                {
                    int shift = GetAsyncKeyState(VK_SHIFT) & 0x8000;
                
                    if (shift)
                        editor_save_file(ed, 1); // CTRL+SHIFT+S farklı kaydet
                    else
                        editor_save_file(ed, 0); // normal CTRL+S
                
                    return 1;
                }

            case 22:  //CTRL + V
                editor_paste(ed);
                return 1;

            case 24:  //CTRL + X
                if (ed->selection_active) {
                    editor_copy_selection(ed);
                    editor_delete_selection(ed);
                }
                return 1;

            case 26: //CTRL + Z
                editor_undo(ed);
                return 1;
            
            case 127: //CTRL + Backspace
                editor_delete_word_left(ed);
                if (ed->selection_active) {
                    editor_delete_selection(ed);
                } else {
                    editor_delete_word_left(ed);
                }
                return 1;
           
        }
    }

    if (ch == 27) { //ESC
        if (ed->search_count > 0) {
            ed->search_len = 0;
            ed->search_count = 0;
            return 1;
        }
    
        if (ed->modified) {
            printf("\nKaydetmek istiyor musunuz? (y/n): ");
            char c = _getch();
    
            if (c == 'y' || c == 'Y') {
                editor_save_file(ed,0);
            }
        }
    
        *running = 0; 
        return 1; 
    }

    if (ch == 0 || ch == 224)
    {
        int code = _getch();

        if (code == 83)//backspace
        {
            if (ed->selection_active) {
                editor_delete_selection(ed);
            } else {
                editor_delete(ed);
            }
            return 1;
        }
        
        if (ed->search_count > 0) {//arama yapılınca eşleşmeler arası gezinti
            if (code == 77 || code == 80) { //sağ veya aşağı tuşları
                editor_search_next(ed, 1);
                return 1;
            } else if (code == 75 || code == 72) { //sol veya yukarı
                editor_search_next(ed, -1);
                return 1;
            }
        }

        // Arama yoksa veya ok tuşları dışında bir özel tuşsa normal imleç hareketi
        handle_arrow_keys(ed, code);
        return 1; 
    }
    
    if (ch >= 32 || ch < 0) //normal karakter girişi
    {
        ed->search_len = 0;
        ed->search_count = 0;

        if (ed->selection_active) {
            editor_delete_selection(ed);
        }

        editor_insert_char(ed, (char)ch);
        return 1;
    }

    return 1;
}