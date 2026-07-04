#ifndef GCURSES_H
#define GCURSES_H

#include <stdio.h>
#include <termios.h>

#define GCS_RED "\x1b[31m"
#define GCS_GREEN "\x1b[32m"
#define GCS_BLUE "\x1b[34m"
#define GCS_YELLOW "\x1b[33m"
#define GCS_WHITE "\x1b[37m"

#define cursor_origin() fputs("\033[H",    stdout);
#define hide_cursor() fputs("\033[?25l", stdout);
#define show_cursor() fputs("\033[?25h", stdout);

typedef struct cursor_location{
    int row;
    int col;
    int hidden;
}cursor_t;
typedef struct tile_data{
    char symbol[5];
    char color[32];
}tile_t;
typedef struct rectangle{
    int w; 
    int h;
    int r;
    int c;
    tile_t tile;
}rect_t;
typedef struct term_window{
    int nrows;
    int ncols;
    tile_t *term_frame;
    tile_t *prev_frame;
    void (*clear)(void);
    int (*io_block)(int set);
    void (*present)(void);
    void (*cursor)(int set);
    int (*horz_strdisp)(int r, int c, char *src);
    int (*vert_strdisp)(int r, int c, char *src);
    int (*horz_tiledisp)(int r, int c, tile_t *src);
    int (*vert_tiledisp)(int r, int c, tile_t *src);
    void (*draw_rect)(rect_t rect);
    void (*draw_border)(rect_t rect);
    void (*draw_frame)(rect_t rect);
    void (*frame_resize)(void);

    struct termios old_conf;
    struct termios new_conf;
    cursor_t cursor_data;
}term_w_t;

extern term_w_t *selected_term;

void select_term(term_w_t *terminal);
int io_blocking_handler(int setting);
void clear_screen(void);
void present(void);
void cursor_handler(int status);
int horz_strdisp(int r, int c, char *source);
int vert_strdisp(int r, int c, char *source);
int horz_tiledisp(int r, int c, tile_t *source);
int vert_tiledisp(int r, int c, tile_t *source);
void draw_rect(rect_t rectangle);
void draw_border(rect_t rectangle);
int init_tui(term_w_t *terminal);

#endif