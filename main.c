#include "tool_box.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

#include "editor.h"

int main(void)
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    Editor editor;

    if (!editor_init(&editor))
    {
        printf("Failed to initialize editor.\n");
        return 1;
    }

    clear_screen();
    drawToolbox();
    editor_render(&editor); //metin editörünü ekrana çizdirme


    //program burada çalışır running 0 olunca program kapanır.
    int running = 1;
    while (running)
    {
        int ch = _getch(); //enter beklemez
        int needRender = editor_process_input(&editor, ch, &running); //basılan tuşa göre işleme yönlendirme

        if (!running)
            break;

        if (needRender) {//sadece GEREKLİ olduğunda ekranı tekrardan çizdirme
            drawToolbox(); 
            editor_render(&editor);
        }
    }

    editor_free(&editor);
    clear_screen();
    
    return 0;
}