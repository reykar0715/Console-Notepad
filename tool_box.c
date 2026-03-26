#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "tool_box.h"

static int g_toolbox_height = 0;

static void gotoxy(short x, short y) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {x, y};
    SetConsoleCursorPosition(hOut, pos);
}

static short getConsoleWidth(void) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return (short)(info.srWindow.Right - info.srWindow.Left + 1);
}

typedef struct { //kategorileri tanımlıyoruz
    const char *category;
    struct {
        const char *key;
        const char *desc;
    } items[10];
    int itemCount;
} ToolboxGroup;

void drawToolbox(void) {
    ToolboxGroup groups[] = {
        {"[ DOSYA ]", {{"CTRL+S", "Kaydet"}, {"CTRL+SFT+S", "Frkli Kydt"}, {"CTRL+O", "Dosya Ac"}, {"Enter", "Yeni Satir"}}, 4},
        {"[ KISAYOLLAR ]", {{"CTRL+F", "Ara"}, {"CTRL+G", "Sonraki"}, {"CTRL+H", "Degistir"}, {"CTRL+Z", "Geri Al"}}, 4},
        {"[ DUZEN ]", {{"CTRL+C", "Kopyala"}, {"CTRL+X", "Kes"}, {"CTRL+V", "Yapistir"}}, 3},
        {"[ SECIM & SIL ]", {{"C+YON", "Krk.Sec"}, {"CTRL+SFT+YON", "Kel.Sec"}, {"BS", "Sil"}, {"C+BS", "Kel.Sil"}}, 4},
        {"[ CIKIS ]", {{"ESC", "Cikis"}}, 1}
    };

    const int groupCount = sizeof(groups) / sizeof(groups[0]);
    short width = getConsoleWidth();
    short y = 0;

    // üst Çerçeve
    gotoxy(0, y++);
    for (int i = 0; i < width; i++) putchar('=');

    // gruplar
    for (int i = 0; i < groupCount; i++) {
        // kategori Başlığı
        gotoxy(2, y);
        printf("\033[1;33m%s\033[0m", groups[i].category); // Sarı renk
        
        int currentX = 20; // komutların başlangıcı
        for (int j = 0; j < groups[i].itemCount; j++) {
            gotoxy((short)currentX, y);
            printf("%s: %s", groups[i].items[j].key, groups[i].items[j].desc);
            currentX += 26;
            
            // ekran genişliğini aşarsa alt satır
            if (currentX + 20 > width) break; 
        }
        y++;
    }

    // alt Çerçeve
    gotoxy(0, y++);
    for (int i = 0; i < width; i++) putchar('=');

    g_toolbox_height = y;
    
    fflush(stdout);
}

int toolboxHeight(void) {
    return g_toolbox_height;
}