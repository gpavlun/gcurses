#ifndef GCURSES_H
#define GCURSES_H

#include <stdio.h>
#include <termios.h>


extern char GCS_VAR_COLOR_BLACK[];
extern char GCS_VAR_COLOR_RED[];
extern char GCS_VAR_COLOR_GREEN[];
extern char GCS_VAR_COLOR_YELLOW[];
extern char GCS_VAR_COLOR_BLUE[];
extern char GCS_VAR_COLOR_MAGENTA[];
extern char GCS_VAR_COLOR_CYAN[];
extern char GCS_VAR_COLOR_WHITE[];

extern char GCS_VAR_COLOR_BRIGHT_BLACK[];
extern char GCS_VAR_COLOR_BRIGHT_RED[];
extern char GCS_VAR_COLOR_BRIGHT_GREEN[];
extern char GCS_VAR_COLOR_BRIGHT_YELLOW[];
extern char GCS_VAR_COLOR_BRIGHT_BLUE[];
extern char GCS_VAR_COLOR_BRIGHT_MAGENTA[];
extern char GCS_VAR_COLOR_BRIGHT_CYAN[];
extern char GCS_VAR_COLOR_BRIGHT_WHITE[];

extern char GCS_VAR_COLOR_DEFAULT[];
extern char GCS_VAR_COLOR_RESET[];

extern char GCS_VAR_BG_COLOR_BLACK[];
extern char GCS_VAR_BG_COLOR_RED[];
extern char GCS_VAR_BG_COLOR_GREEN[];
extern char GCS_VAR_BG_COLOR_YELLOW[];
extern char GCS_VAR_BG_COLOR_BLUE[];
extern char GCS_VAR_BG_COLOR_MAGENTA[];
extern char GCS_VAR_BG_COLOR_CYAN[];
extern char GCS_VAR_BG_COLOR_WHITE[];

extern char GCS_VAR_BG_COLOR_BRIGHT_BLACK[];
extern char GCS_VAR_BG_COLOR_BRIGHT_RED[];
extern char GCS_VAR_BG_COLOR_BRIGHT_GREEN[];
extern char GCS_VAR_BG_COLOR_BRIGHT_YELLOW[];
extern char GCS_VAR_BG_COLOR_BRIGHT_BLUE[];
extern char GCS_VAR_BG_COLOR_BRIGHT_MAGENTA[];
extern char GCS_VAR_BG_COLOR_BRIGHT_CYAN[];
extern char GCS_VAR_BG_COLOR_BRIGHT_WHITE[];

extern char GCS_VAR_BG_COLOR_DEFAULT[];
extern char GCS_VAR_BG_COLOR_RESET[];


#define GCS_BLACK              GCS_VAR_COLOR_BLACK
#define GCS_RED                GCS_VAR_COLOR_RED
#define GCS_GREEN              GCS_VAR_COLOR_GREEN
#define GCS_YELLOW             GCS_VAR_COLOR_YELLOW
#define GCS_BLUE               GCS_VAR_COLOR_BLUE
#define GCS_MAGENTA            GCS_VAR_COLOR_MAGENTA
#define GCS_CYAN               GCS_VAR_COLOR_CYAN
#define GCS_WHITE              GCS_VAR_COLOR_WHITE

#define GCS_BRIGHT_BLACK       GCS_VAR_COLOR_BRIGHT_BLACK
#define GCS_BRIGHT_RED         GCS_VAR_COLOR_BRIGHT_RED
#define GCS_BRIGHT_GREEN       GCS_VAR_COLOR_BRIGHT_GREEN
#define GCS_BRIGHT_YELLOW      GCS_VAR_COLOR_BRIGHT_YELLOW
#define GCS_BRIGHT_BLUE        GCS_VAR_COLOR_BRIGHT_BLUE
#define GCS_BRIGHT_MAGENTA     GCS_VAR_COLOR_BRIGHT_MAGENTA
#define GCS_BRIGHT_CYAN        GCS_VAR_COLOR_BRIGHT_CYAN
#define GCS_BRIGHT_WHITE       GCS_VAR_COLOR_BRIGHT_WHITE

#define GCS_DEFAULT            GCS_VAR_COLOR_DEFAULT
#define GCS_RESET              GCS_VAR_COLOR_RESET

#define GCS_BG_BLACK              GCS_VAR_BG_COLOR_BLACK
#define GCS_BG_RED                GCS_VAR_BG_COLOR_RED
#define GCS_BG_GREEN              GCS_VAR_BG_COLOR_GREEN
#define GCS_BG_YELLOW             GCS_VAR_BG_COLOR_YELLOW
#define GCS_BG_BLUE               GCS_VAR_BG_COLOR_BLUE
#define GCS_BG_MAGENTA            GCS_VAR_BG_COLOR_MAGENTA
#define GCS_BG_CYAN               GCS_VAR_BG_COLOR_CYAN
#define GCS_BG_WHITE              GCS_VAR_BG_COLOR_WHITE

#define GCS_BG_BRIGHT_BLACK       GCS_VAR_BG_COLOR_BRIGHT_BLACK
#define GCS_BG_BRIGHT_RED         GCS_VAR_BG_COLOR_BRIGHT_RED
#define GCS_BG_BRIGHT_GREEN       GCS_VAR_BG_COLOR_BRIGHT_GREEN
#define GCS_BG_BRIGHT_YELLOW      GCS_VAR_BG_COLOR_BRIGHT_YELLOW
#define GCS_BG_BRIGHT_BLUE        GCS_VAR_BG_COLOR_BRIGHT_BLUE
#define GCS_BG_BRIGHT_MAGENTA     GCS_VAR_BG_COLOR_BRIGHT_MAGENTA
#define GCS_BG_BRIGHT_CYAN        GCS_VAR_BG_COLOR_BRIGHT_CYAN
#define GCS_BG_BRIGHT_WHITE       GCS_VAR_BG_COLOR_BRIGHT_WHITE

#define GCS_BG_DEFAULT            GCS_VAR_BG_COLOR_DEFAULT
#define GCS_BG_RESET              GCS_VAR_BG_COLOR_RESET



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
    char *color;
    char *bg_color;
}tile_t;
typedef struct rectangle{
    int w; 
    int h;
    int r;
    int c;
    tile_t tile;
}rect_t;
typedef struct term_frame_set{
    void (*w)(int value);
    void (*h)(int value);
    void (*max_w)(int value);
    void (*max_h)(int value);
    void (*min_w)(int value);
    void (*min_h)(int value);
    
}tframe_set_t;
typedef struct term_frame_constraints{
    int max_w;
    int min_w;
    int max_h;
    int min_h;
}tframe_con_t;
typedef struct term_frame_data{
    rect_t dim;
    tframe_set_t set;
    tframe_con_t con;
    void (*max_zz)(int value);
}tframe_t;



typedef struct term_window{
    int nrows;
    int ncols;
    term_w_t *self;
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
    void (*term_draw_frame)(term_w_t *terminal, rect_t rect);
    void (*frame_resize)(void);

    struct termios old_conf;
    struct termios new_conf;
    cursor_t cursor_data;
}term_w_t;

extern term_w_t *selected_term;


#define draw_frame(A)\
term_draw_farme()





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
void set_frame_heigth(int value);
void set_frame_max_width(int value);
void set_frame_min_width(int value);
void set_frame_max_heigth(int value);
void set_frame_min_heigth(int value);
int init_tframe(tframe_t *frame);
void select_frame(tframe_t *frame);
#endif