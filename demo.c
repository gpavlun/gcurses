#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "gcurses.h"

int getch_nb(void)
{
    unsigned char c;
    size_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1)           return (int)c;
    if (n == 0 || (n == -1)) return -1; 
    return -2;                          
}

int main(void) {

    term_w_t terminal;
    init_tui(&terminal);
    terminal.io_block(0);
    terminal.cursor(0);

    char text[10];
    char input = 0;

    cursor_origin();

    terminal.clear();

    while(input!='q'&&input!='\e'){
        terminal.frame_resize();

        
        rect_t title_box;
        title_box.c = 0;
        title_box.r = 0;
        title_box.h = 3;
        title_box.w = terminal.ncols/4;
        title_box.tile.color = GCS_BLUE;
        title_box.tile.bg_color = GCS_BG_DEFAULT;

        terminal.draw_frame(title_box);

        terminal.horz_strdisp(title_box.r+1, title_box.c+1, "GCURSES DEMO");
        
        rect_t body_box;
        body_box.c = title_box.c;
        body_box.r = title_box.r+title_box.h;
        body_box.h = terminal.nrows-title_box.h;
        body_box.w = terminal.ncols/4;
        body_box.tile.color = GCS_RED;
        body_box.tile.bg_color = GCS_BG_DEFAULT;

        terminal.draw_frame(body_box);

        rect_t ui_box;
        ui_box.c = title_box.c+title_box.w;
        ui_box.r = title_box.r;
        ui_box.h = terminal.nrows;
        ui_box.w = terminal.ncols*3/4;
        ui_box.tile.color = GCS_GREEN;
        ui_box.tile.bg_color = GCS_BG_DEFAULT;

        terminal.draw_frame(ui_box);

        terminal.horz_strdisp(ui_box.r+1,ui_box.c+1,"This is a demo of gcurses, my implementation of ncurses.");
        terminal.vert_strdisp(body_box.r+2,body_box.c+body_box.w/2,"Vertical string");
        terminal.horz_strdisp(ui_box.r+2,ui_box.c+1,"Dynamic resizing is enabled, try moving the window around!");
        terminal.horz_strdisp(ui_box.r+3,ui_box.c+1,"Press q or esc to exit.");

        terminal.present();
        
        input = getch_nb();
        usleep(10000);
    }

    terminal.clear();
    terminal.present();
    terminal.io_block(1);
    return 0;
}