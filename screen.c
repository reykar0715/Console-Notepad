#include <stdio.h>
#include <windows.h>

#include "screen.h"

void gotoxy(int x, int y) //tüm konumları buradan yönetiyoruz
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(hOut, coord);
}

void clear_screen(void) //ekran temizleme
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD cellCount;
    DWORD count;
    COORD homeCoords = {0, 0};

    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    cellCount = (DWORD)(csbi.dwSize.X) * (DWORD)(csbi.dwSize.Y);

    FillConsoleOutputCharacter(hOut, (TCHAR)' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hOut, homeCoords);
}

void screen_clear_from_cursor(void) //toolbox ve buffer yazıları silinmesin diye imleçten sonrasını temizliyoruz
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); //koddaki bu satırların tümü terminale yazı yazma izni istemek için
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    COORD cursorPos;

    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    cursorPos = csbi.dwCursorPosition;

    int width = csbi.dwSize.X;
    int height = csbi.dwSize.Y;

    int startIndex = cursorPos.Y * width + cursorPos.X;
    int totalCells = width * height;

    if (startIndex >= totalCells)
        return;

    DWORD cellsToClear = (DWORD)(totalCells - startIndex);

    FillConsoleOutputCharacter(hOut, (TCHAR)' ', cellsToClear, cursorPos, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellsToClear, cursorPos, &count);
}