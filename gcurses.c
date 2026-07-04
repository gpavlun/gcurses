#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "gcurses.h"

term_w_t *selected_term;

void select_term(term_w_t *terminal){
    selected_term = terminal;
}

static void sigint_handler(int sig){
    (void)sig;                     /* unused */

    term_w_t *terminal = selected_term;
    if(terminal){
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal->old_conf);   /* restore term */
    }

    write(STDOUT_FILENO, "\nCtrl-C caught - restoring terminal and exiting.\n", 27);
    signal(SIGINT, SIG_DFL);       /* back to default */
    raise(SIGINT);                 /* now exit with the usual code */
}

static void segv_handler(int sig, siginfo_t *info, void *ctx) {
    (void)sig; (void)info; (void)ctx;

    term_w_t *terminal = selected_term;
    if(terminal){
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal->old_conf);   /* restore term */       
    }

    write(STDOUT_FILENO, "\n*** SEGFAULT ***\n", 18);
    signal(SIGSEGV, SIG_DFL);   /* default action */
    raise(SIGSEGV);
}

/*
    1/true to set non-blocking
    0/false to set blocking
*/
int io_blocking_handler(int setting){
    term_w_t *terminal = selected_term;

    if(setting){
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal->old_conf);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags != -1) fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        setvbuf(stdout, NULL, _IOLBF, 0); 
    }else{
        if (tcgetattr(STDIN_FILENO, &terminal->old_conf) == -1) return -1;

        terminal->new_conf = terminal->old_conf;
        terminal->new_conf.c_lflag &= ~(ICANON | ECHO);
        terminal->new_conf.c_cc[VMIN]  = 0;
        terminal->new_conf.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal->new_conf) == -1) return -1;

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags == -1) return -1;

        setvbuf(stdout, NULL, _IONBF, 0);

        return fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);        
    }

}

void clear_screen(void){

    term_w_t *terminal = selected_term;

    if(!terminal->cursor_data.hidden) hide_cursor();

    for(int i=0;i<terminal->ncols*terminal->nrows;i++){
            strcpy(terminal->term_frame[i].symbol, " ");
            strcpy(terminal->term_frame[i].color,"\x1b[0m");
    }

    if(!terminal->cursor_data.hidden) show_cursor();
}

void present(void){
    term_w_t *terminal = selected_term;

    if(!terminal->cursor_data.hidden) hide_cursor();
    cursor_origin();

    for(int i=0; i<terminal->nrows*terminal->ncols; i++){
        if(strcmp(terminal->term_frame[i].symbol, terminal->prev_frame[i].symbol)||strcmp(terminal->term_frame[i].color,terminal->prev_frame[i].color)){
            printf("\x1b[%d;%dH", i/terminal->ncols+1, i%terminal->ncols+1);
            printf("%s%s",terminal->term_frame[i].color,terminal->term_frame[i].symbol);
            usleep(1);
        }
    }

    printf("\x1b[%d;%dH", terminal->cursor_data.row+1, terminal->cursor_data.col+1);
    if(!terminal->cursor_data.hidden) show_cursor();

    fflush(stdout);
    memcpy(terminal->prev_frame, terminal->term_frame, sizeof(tile_t)*terminal->ncols*terminal->nrows);
}

void cursor_handler(int status){
    term_w_t *terminal = selected_term;
    if(status){
        show_cursor();
        terminal->cursor_data.hidden = 0;
    }else{
        hide_cursor();
        terminal->cursor_data.hidden = 1;
    }
}

int horz_strdisp(int r, int c, char *source){
    term_w_t *terminal = selected_term;

    int i=0;
    while(source[i]!=0){
        if(r>=0&&r<terminal->nrows && c>=0&&c<terminal->ncols){
            (terminal->term_frame+r*terminal->ncols + c)->symbol[0] = source[i];
            (terminal->term_frame+r*terminal->ncols + c)->symbol[1] = 0;
            strcpy((terminal->term_frame+r*terminal->ncols)->color,"\x1b[0m");
        }
        i++;
        c++;
    }
    return i;
}

int vert_strdisp(int r, int c, char *source){
    term_w_t *terminal = selected_term;

    int i=0;
    while(source[i]!=0){
        if(r>=0&&r<terminal->nrows && c>=0&&c<terminal->ncols){
            (terminal->term_frame+r*terminal->ncols + c)->symbol[0] = source[i];
            (terminal->term_frame+r*terminal->ncols + c)->symbol[1] = 0;
            strcpy((terminal->term_frame+r*terminal->ncols)->color,"\x1b[0m");
        }
        i++;
        r++;
    }
    return i;    
}

int horz_tiledisp(int r, int c, tile_t *source){
    term_w_t *terminal = selected_term;

    int i=0;
    while(source[i].symbol!=0){
        if(r>=0&&r<terminal->nrows && c>=0&&c<terminal->ncols){
            strcpy((terminal->term_frame+r*terminal->ncols + c)->symbol, source[i].symbol);
            strcpy((terminal->term_frame+r*terminal->ncols)->color,source[i].color);
        }
        i++;
        c++;
    }
    return i;
}

int vert_tiledisp(int r, int c, tile_t *source){
    term_w_t *terminal = selected_term;

    int i=0;
    while(source[i].symbol!=0){
        if(r>=0&&r<terminal->nrows && c>=0&&c<terminal->ncols){
            strcpy((terminal->term_frame+r*terminal->ncols + c)->symbol, source[i].symbol);
            strcpy((terminal->term_frame+r*terminal->ncols)->color,source[i].color);
        }
        i++;
        r++;
    }
    return i;
}

void draw_rect(rect_t rectangle){
    term_w_t *terminal = selected_term;

    int i,j;
    for(i=rectangle.r;i<rectangle.h+rectangle.r;i++){
        for(j=rectangle.c;j<rectangle.w+rectangle.c;j++){
            strcpy((terminal->term_frame + (i*terminal->ncols) + j)->symbol, rectangle.tile.symbol);
            strcpy((terminal->term_frame + (i*terminal->ncols) + j)->color, rectangle.tile.color);
        }
    }
}

void draw_border(rect_t rectangle){
    term_w_t *terminal = selected_term;

    int i,j;
    for(i=rectangle.r;i<rectangle.h+rectangle.r;i++){
        for(j=rectangle.c;j<rectangle.w+rectangle.c;j++){
            
            if(i==rectangle.r||i==rectangle.h+rectangle.r-1){
                strcpy((terminal->term_frame + (i*terminal->ncols) + j)->symbol, rectangle.tile.symbol);
                strcpy((terminal->term_frame + (i*terminal->ncols) + j)->color,rectangle.tile.color);
            }
            if(j==rectangle.c||j==rectangle.w+rectangle.c-1){
                strcpy((terminal->term_frame + (i*terminal->ncols) + j)->symbol , rectangle.tile.symbol);
                strcpy((terminal->term_frame + (i*terminal->ncols) + j)->color,rectangle.tile.color);
            }
        
        }
    }
}
void draw_frame(rect_t rectangle){
    term_w_t *terminal = selected_term;

    int i;
    
    strcpy((terminal->term_frame + (rectangle.r*terminal->ncols) + rectangle.c)->symbol, "┌");
    strcpy((terminal->term_frame + (rectangle.r*terminal->ncols) + rectangle.c)->color, rectangle.tile.color);
    strcpy((terminal->term_frame + (rectangle.r*terminal->ncols) + rectangle.c + rectangle.w - 1)->symbol, "┐");
    strcpy((terminal->term_frame + (rectangle.r*terminal->ncols) + rectangle.c + rectangle.w - 1)->color, rectangle.tile.color);
    strcpy((terminal->term_frame + ((rectangle.r + rectangle.h - 1)*terminal->ncols) + rectangle.c)->symbol, "└");
    strcpy((terminal->term_frame + ((rectangle.r + rectangle.h - 1)*terminal->ncols) + rectangle.c)->color, rectangle.tile.color);
    strcpy((terminal->term_frame + ((rectangle.r + rectangle.h - 1)*terminal->ncols) + rectangle.c + rectangle.w - 1)->symbol, "┘");
    strcpy((terminal->term_frame + ((rectangle.r + rectangle.h - 1)*terminal->ncols) + rectangle.c + rectangle.w - 1)->color, rectangle.tile.color);

    for(i=1;i<rectangle.w-1;i++){
        strcpy((terminal->term_frame + (rectangle.r*terminal->ncols) + rectangle.c + i)->symbol, "─");
        strcpy((terminal->term_frame + (rectangle.r*terminal->ncols) + rectangle.c + i)->color, rectangle.tile.color);
        strcpy((terminal->term_frame + ((rectangle.r+rectangle.h - 1)*terminal->ncols) + rectangle.c + i)->symbol, "─");
        strcpy((terminal->term_frame + ((rectangle.r+rectangle.h - 1)*terminal->ncols) + rectangle.c + i)->color, rectangle.tile.color);
    }
    for(i=1;i<rectangle.h-1;i++){
        strcpy((terminal->term_frame + ((rectangle.r+i)*terminal->ncols) + rectangle.c)->symbol, "│");
        strcpy((terminal->term_frame + ((rectangle.r+i)*terminal->ncols) + rectangle.c)->color,rectangle.tile.color);
        strcpy((terminal->term_frame + ((rectangle.r+i)*terminal->ncols) + rectangle.c + rectangle.w - 1)->symbol, "│");
        strcpy((terminal->term_frame + ((rectangle.r+i)*terminal->ncols) + rectangle.c + rectangle.w - 1)->color,rectangle.tile.color);
    }

}

int detect_resize(term_w_t *terminal){
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    if(terminal->nrows!=ws.ws_row||terminal->ncols!=ws.ws_col){
        return 1;
    }else{
        return 0;
    }
}

void frame_resize(void){
    term_w_t *terminal = selected_term;

    if(detect_resize(terminal)){
        puts("\x1b[2J");

        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);  
        terminal->nrows = ws.ws_row;
        terminal->ncols = ws.ws_col;

        free(terminal->term_frame);
        free(terminal->prev_frame);
        terminal->term_frame = malloc(sizeof(tile_t)*terminal->nrows*terminal->ncols);
        terminal->prev_frame = malloc(sizeof(tile_t)*terminal->nrows*terminal->ncols);
        int i,j;
        for(i=0;i<terminal->nrows*terminal->ncols;i++){
            strcpy(terminal->term_frame[i].symbol, " ");
            strcpy(terminal->term_frame[i].color, "\x1b[0m");
            strcpy(terminal->prev_frame[i].symbol, " ");
            strcpy(terminal->term_frame[i].color, "\x1b[0m");
        }
        usleep(125000);
    }
}

int init_tui(term_w_t *terminal){
    if(!terminal) return 1;

    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    terminal->nrows = ws.ws_row;
    terminal->ncols = ws.ws_col;

    terminal->term_frame = malloc(sizeof(tile_t)*terminal->nrows*terminal->ncols);
    if(!terminal->term_frame) return 1;

    terminal->prev_frame = malloc(sizeof(tile_t)*terminal->nrows*terminal->ncols);
    if(!terminal->prev_frame) return 1;

    int i,j;
    for(i=0;i<terminal->nrows*terminal->ncols;i++){
        strcpy(terminal->term_frame[i].symbol, " ");
        strcpy(terminal->term_frame[i].color, "\x1b[0m");
        strcpy(terminal->prev_frame[i].symbol, " ");
        strcpy(terminal->term_frame[i].color, "\x1b[0m");
    }
    terminal->clear = clear_screen;
    terminal->io_block = io_blocking_handler;
    terminal->present = present;
    terminal->cursor = cursor_handler;
    terminal->horz_strdisp = horz_strdisp;
    terminal->vert_strdisp = vert_strdisp;
    terminal->horz_tiledisp = horz_tiledisp;
    terminal->vert_tiledisp = vert_tiledisp;
    terminal->draw_rect = draw_rect;
    terminal->draw_border = draw_border;
    terminal->draw_frame = draw_frame;
    terminal->frame_resize = frame_resize;
    cursor_origin();
    terminal->cursor_data.row = 0;
    terminal->cursor_data.col = 0;

    if(tcgetattr(STDIN_FILENO, &terminal->old_conf) == -1) return -1;

    struct sigaction sa = {0};
    sa.sa_handler = sigint_handler;
    sa.sa_flags   = SA_RESTART;
    sigaction(SIGINT, &sa, NULL); 
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);


    selected_term = terminal;

    return 0;
}