#define WIN32_LEAN_AND_MEAN //windows.h kütüphanesinden sadece gerekli fonksiyonları çekme

#include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_browser.h"
#include "screen.h"
#include "tool_box.h"

int is_save_mode = 0; //dosya açma ve dosya kaydetme işlemlerini ayırmak için tanımlandı

typedef struct BrowserItem
{
    char name[MAX_PATH];
    int is_dir;
} BrowserItem;

static int is_dots_name(const char *name)
{
    return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

static int item_cmp(const void *a, const void *b) //dosya ve klasörleri sıralama
{
    const BrowserItem *ia = (const BrowserItem *)a;
    const BrowserItem *ib = (const BrowserItem *)b;

    if (ia->is_dir != ib->is_dir) //klasörlere öncelik
        return ib->is_dir - ia->is_dir;

    return _stricmp(ia->name, ib->name); //alfabetik sıralama
}

static int list_directory(BrowserItem **out_items, int *out_count) //klasördeki tüm elemanları !dinamik diziye! ekleme
{
    *out_items = NULL;
    *out_count = 0;

    WIN32_FIND_DATAA ffd; //bulunan dosyaların bilgilerinin yazıldığı windows API'si
    HANDLE hFind = FindFirstFileA("*", &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    int cap = 64;
    BrowserItem *items = (BrowserItem *)malloc((size_t)cap * sizeof(BrowserItem));
    if (!items)
    {
        FindClose(hFind);
        return 0;
    }

    do
    {
        const char *name = ffd.cFileName;
        if (is_dots_name(name))
            continue;

        int is_dir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0; //klasör mü yoksa dosya mı kontrol eder klasörse isdir 1

        if (is_save_mode)
        {
            if (!is_dir)
                continue;
        }
        else
        {
            if (!is_dir)
            {
                char *ext = strrchr(name, '.');
                if (!(ext && _stricmp(ext, ".txt") == 0))
                    continue;
            }
        }

        if (*out_count >= cap) //kapasite arttırma
        {
            cap *= 2;
            BrowserItem *grown = (BrowserItem *)realloc(items, (size_t)cap * sizeof(BrowserItem));
            if (!grown)
            {
                free(items);
                FindClose(hFind);
                return 0;
            }
            items = grown;
        }

        strncpy(items[*out_count].name, name, MAX_PATH - 1);//bulunan dosyaların isimlerini kopyalama
        items[*out_count].name[MAX_PATH - 1] = '\0';
        items[*out_count].is_dir = is_dir;
        (*out_count)++;
    } while (FindNextFileA(hFind, &ffd) != 0);

    FindClose(hFind); //arama işlemini sonlandırma windows dosyalarını serbest bırakma

    if (*out_count > 1)
        qsort(items, (size_t)(*out_count), sizeof(BrowserItem), item_cmp);

    *out_items = items;
    return 1;
}

static void render_browser(const char *cwd, const BrowserItem *items, int count, int selected) //terminale dosyaları yazdırma
{
    clear_screen();
    drawToolbox();

    gotoxy(0, toolboxHeight());
    printf("%s\n\n", cwd);

    if (count <= 0)
    {
        printf("(empty)\n");

        if (is_save_mode)
        {
            const char *prefix = (selected == 0) ? "> " : "  ";
            printf("%s[Buraya Kaydet]\n", prefix);
        }

        printf("\n[Yukarı/Asağı] Seçim  [ENTER] Aç  [BACKSPACE] Üst klasör  [ESC] İptal\n");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        const char *prefix = (i == selected) ? "> " : "  ";
        if (items[i].is_dir)
            printf("%s[%s]\n", prefix, items[i].name);
        else
            printf("%s%s\n", prefix, items[i].name);
    }

    if (is_save_mode)
    {
        const char *prefix = (count == selected) ? "> " : "  ";
        printf("%s[Buraya Kaydet]\n", prefix);
    }

    printf("\n[Yukarı/Asağı] Seçim  [ENTER] Aç  [BACKSPACE] Üst klasör  [ESC] İptal\n");
}

static int build_full_path(const char *cwd, const char *name, char *out_path, int out_capacity) //klasör yolu ve dosya adını birleştirme
{
    if (!cwd || !name || !out_path || out_capacity <= 0)
        return 0;

    size_t cwd_len = strlen(cwd);
    int need_slash = (cwd_len > 0 && (cwd[cwd_len - 1] != '\\' && cwd[cwd_len - 1] != '/')) ? 1 : 0;

    int needed = (int)cwd_len + (need_slash ? 1 : 0) + (int)strlen(name) + 1;
    if (needed > out_capacity)
        return 0;

    if (need_slash)
        snprintf(out_path, (size_t)out_capacity, "%s\\%s", cwd, name);
    else
        snprintf(out_path, (size_t)out_capacity, "%s%s", cwd, name);

    return 1;
}

int file_browser_select(char *out_path, int out_capacity) //dosya seçme işlemi
{
    if (!out_path || out_capacity <= 0)
        return 0;

    out_path[0] = '\0';

    char original_dir[MAX_PATH]; //mevcut klasöre dönme durumu için kaydedilir (esc veya kaydetme işlemleri sonrası için)
    DWORD original_len = GetCurrentDirectoryA(MAX_PATH, original_dir);
    if (original_len == 0 || original_len >= MAX_PATH)
        original_dir[0] = '\0';

    BrowserItem *items = NULL;
    int count = 0;
    int selected = 0;

    for (;;)
    {
        char cwd[MAX_PATH];
        DWORD cwd_len = GetCurrentDirectoryA(MAX_PATH, cwd);
        if (cwd_len == 0 || cwd_len >= MAX_PATH)
            strcpy(cwd, "(unknown)");

        if (items)
        {
            free(items);
            items = NULL;
        }
        count = 0;
        selected = 0;

        if (!list_directory(&items, &count)) //mevcut klasördeki dosyaları tekrar tarama listeleme
        {
            if (items)
            {
                free(items);
                items = NULL;
            }
            if (original_dir[0] != '\0')
                SetCurrentDirectoryA(original_dir);
            return 0;
        }

        render_browser(cwd, items, count, selected); //yeni listeyi ekrana çizer

        for (;;) //işlem yapılabilmesi için girdi alma
        {
            int ch = _getch();

            if (ch == 27) // esc
            {
                if (items)
                    free(items);
                if (original_dir[0] != '\0')
                    SetCurrentDirectoryA(original_dir);
                clear_screen();
                return 0;
            }

            if (ch == 0 || ch == 224) //yön tuşları
            {
                int code = _getch();
                if (code == 72) //yukarı
                {
                    if (count > 0)
                        if (is_save_mode)
                            selected = (selected > 0) ? (selected - 1) : count;
                        else
                            selected = (selected > 0) ? (selected - 1) : (count - 1);
                    render_browser(cwd, items, count, selected);
                }
                else if (code == 80) //aşağı
                {
                    if (count > 0)
                        if (is_save_mode)
                            selected = (selected + 1) % (count + 1);
                        else
                            selected = (selected + 1) % count;
                    render_browser(cwd, items, count, selected);
                }
                continue;
            }

            if (ch == 8) // backspace, üst klasöre çıkma
            {
                SetCurrentDirectoryA(".."); //parent klasörleri listeleme
                break;
            }

            if (ch == 13) //enter
            {
                if (is_save_mode && selected == count) //save modu
                {
                    char cwd[MAX_PATH];
                    GetCurrentDirectoryA(MAX_PATH, cwd);

                    if (items) free(items);

                    if (original_dir[0] != '\0')
                        SetCurrentDirectoryA(original_dir);

                    strncpy(out_path, cwd, out_capacity - 1);
                    out_path[out_capacity - 1] = '\0';

                    clear_screen();
                    return 1;
                }

                if (!is_save_mode && count > 0 && selected < count && !items[selected].is_dir) //dosya açma modu
                {
                    char real_cwd[MAX_PATH];
                    GetCurrentDirectoryA(MAX_PATH, real_cwd);

                    int ok = build_full_path(real_cwd, items[selected].name, out_path, out_capacity);

                    if (items) free(items);

                    if (original_dir[0] != '\0')
                        SetCurrentDirectoryA(original_dir);
                    clear_screen();
                    return ok;
                }

                //her iki mod için de klasöre girme
                if (count > 0 && selected < count && items[selected].is_dir)
                {
                    if (SetCurrentDirectoryA(items[selected].name))
                        break;

                    render_browser(cwd, items, count, selected);
                }
            }
        }
    }
}
