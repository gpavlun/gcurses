#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "gcurses.h"

void handleinput(int input, int *selected){
    switch(input){
        case 'w':
            if((*selected) > 0) (*selected)--;
            break;
        case 's':
            if((*selected) < 5) (*selected)++;
            break;
    }
}



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

    int selected = 0;

    
    tframe_t title_box;
    init_tframe(&title_box);
    
    title_box.set.max_w(strlen("GCURSES DEMO")*2 + 2);
    title_box.set.min_w(strlen("GCURSES DEMO") + 2);

    title_box.set.h(3);
    title_box.dim.tile.color = GCS_BLUE;

    while(input!='q'&&input!='\e'){
        terminal.frame_resize();

        handleinput(input, &selected);
        

        select_frame(&title_box);
        title_box.set.w(terminal.ncols/4);
        

        terminal.draw_frame(title_box.dim);

        terminal.horz_strdisp(title_box.dim.r+1, title_box.dim.c+1, "GCURSES DEMO");
        
        rect_t body_box;
        body_box.c = title_box.dim.c;
        body_box.r = title_box.dim.r+title_box.dim.h;
        body_box.h = terminal.nrows-title_box.dim.h;
        body_box.w = title_box.dim.w;
        body_box.tile.color = GCS_RED;
        body_box.tile.bg_color = GCS_BG_DEFAULT;

        terminal.draw_frame(body_box);

        rect_t ui_box;
        ui_box.c = title_box.dim.c+title_box.dim.w;
        ui_box.r = title_box.dim.r;
        ui_box.h = terminal.nrows;
        ui_box.w = terminal.ncols-title_box.dim.w;
        ui_box.tile.color = GCS_GREEN;
        ui_box.tile.bg_color = GCS_BG_DEFAULT;

        terminal.draw_frame(ui_box);

        terminal.horz_strdisp(ui_box.r+1,ui_box.c+1,"This is a demo of gcurses, my implementation of ncurses.");
        //terminal.vert_strdisp(body_box.r+2,body_box.c+body_box.w/2,"Vertical string");
        terminal.horz_strdisp(ui_box.r+2,ui_box.c+1,"Dynamic resizing is enabled, try moving the window around!");
        terminal.horz_strdisp(ui_box.r+3,ui_box.c+1,"There are also min and max widths, see the title box!");
        
        terminal.horz_strdisp(ui_box.r+6,ui_box.c+1,"Press q or esc to exit.");

        
        tile_t array[6][16] = {0};

        char set[6][16] = {
            "option 0",
            "option 1",
            "option 2",
            "option 3",
            "option 4",
            "option 5"
        };



        int j, i;
        for (j=0; j<6; j++) {
            for (i=0; set[j][i] != '\0'; i++) {

                if (selected == j) {
                    array[j][i].bg_color = GCS_BG_WHITE;
                    array[j][i].color = GCS_BLACK;
                } else {
                    array[j][i].bg_color = GCS_BG_DEFAULT;
                    array[j][i].color = GCS_DEFAULT;
                }
                array[j][i].symbol[0] = set[j][i];
                array[j][i].symbol[1] = '\0';
            }
            array[j][i].bg_color = GCS_BG_BLACK;
            array[j][i].color = GCS_WHITE;
            array[j][i].symbol[0] = 0;

        }
        for(int i=0;i<6;i++){
            terminal.horz_tiledisp(body_box.r+1+i,body_box.c+1,array[i]);
        }




        terminal.present();
        
        input = getch_nb();
        usleep(10000);
    }

    terminal.clear();
    terminal.present();
    terminal.io_block(1);
    
    return 0;
}