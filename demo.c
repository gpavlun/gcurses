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

    while(input!='q'&&input!='\e'){
        terminal.frame_resize();
        terminal.clear();
        
        rect_t edge;
        edge.c = 0;
        edge.r = 0;
        edge.h = terminal.nrows;
        edge.w = terminal.ncols;
        strcpy(edge.tile.color, GCS_WHITE);

        terminal.draw_frame(edge);

        rect_t title_box;
        title_box.c = edge.r+1;
        title_box.r = edge.c+1;
        title_box.h = 5;
        title_box.w = terminal.ncols - 2;
        strcpy(title_box.tile.color, GCS_BLUE);

        terminal.draw_frame(title_box);

        terminal.horz_strdisp(title_box.r+2, title_box.c+2, "GCURSES DEMO");
        
        rect_t body_box;
        body_box.c = title_box.c;
        body_box.r = title_box.r+title_box.h;
        body_box.h = terminal.nrows-title_box.h-2;
        body_box.w = terminal.ncols - 2;
        strcpy(body_box.tile.color, GCS_RED);
        terminal.draw_frame(body_box);

        terminal.horz_strdisp(body_box.r+2,body_box.c+2,"This is a demo of gcurses, my implementation of ncurses.");
        terminal.vert_strdisp(body_box.r+4,body_box.c+4,"Vertical string");
        terminal.horz_strdisp(body_box.r+4,body_box.c+8,"Dynamic resizing is enabled, try moving the window around!");
        terminal.horz_strdisp(body_box.r+6,body_box.c+8,"Press q or esc to exit.");

        terminal.present();
        
        input = getch_nb();
        usleep(10000);
    }
    input = 0;





    terminal.clear();
    terminal.present();
    terminal.io_block(1);
    return 0;
}