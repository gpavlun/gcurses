#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <signal.h>
#include <ctype.h>


#define $ break;


int terminal_cols;
int terminal_rows;

typedef struct tile_data{
    char symbol;
    char color[32];
}tile_t;
tile_t *terminal_display;
tile_t *prev_frame;

static inline void cursor_origin(void)                { fputs("\033[H",    stdout);}
static inline void clear_screen(void){
    int i;
    for(i=0;i<terminal_cols*terminal_rows;i++){
            terminal_display[i].symbol = ' ';
            strcpy(terminal_display[i].color,"\x1b[0m");
    }
}
static inline void clear_line(void)                 { fputs("\033[2K",   stdout);}
static inline void cursor_up(int n)                 { printf("\033[%dA", n);     }
static inline void cursor_down(int n)               { printf("\033[%dB", n);     }
static inline void cursor_forward(int n)            { printf("\033[%dC", n);     }
static inline void cursor_back(int n)               { printf("\033[%dD", n);     }
static inline void hide_cursor(void)                { fputs("\033[?25l", stdout);}
static inline void show_cursor(void)                { fputs("\033[?25h", stdout);}
static inline void cursor_bottom(void){
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cursor_down(ws.ws_row);
}
static inline void cursor_top(void){
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cursor_up(ws.ws_row);
}
static inline void cursor_leftend(void){
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cursor_back(ws.ws_col);
}
static inline void cursor_rightend(void){
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cursor_forward(ws.ws_col);
}



struct termios orig_term;
static int init_nonblocking_input(void){
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &orig_term) == -1) return -1;

    raw = orig_term;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) return -1;

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) return -1;


    setvbuf(stdout, NULL, _IONBF, 0);


    return fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

static void sigint_handler(int sig){
    (void)sig;                     /* unused */
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);   /* restore term */
    write(STDOUT_FILENO, "\nCtrl-C caught - restoring terminal and exiting.\n", 27);
    signal(SIGINT, SIG_DFL);       /* back to default */
    raise(SIGINT);                 /* now exit with the usual code */
}
static void segv_handler(int sig, siginfo_t *info, void *ctx) {
    (void)sig; (void)info; (void)ctx;
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);   /* restore term */
    write(STDOUT_FILENO, "\n*** SEGFAULT ***\n", 18);
    signal(SIGSEGV, SIG_DFL);   /* default action */
    raise(SIGSEGV);
}

typedef struct rectangle{
    int w; 
    int h;
    int r;
    int c;
}rect_t;

void draw_rectangle(rect_t rectangle){
    int i,j;
    for(i=rectangle.r;i<rectangle.h+rectangle.r;i++){
        for(j=rectangle.c;j<rectangle.w+rectangle.c;j++){
            (terminal_display + (i*terminal_cols) + j)->symbol = '#';
        }
    }
}
void draw_border(rect_t rectangle){
    int i,j;
    for(i=rectangle.r;i<rectangle.h+rectangle.r;i++){
        for(j=rectangle.c;j<rectangle.w+rectangle.c;j++){
            
            if(i==rectangle.r||i==rectangle.h+rectangle.r-1){
                (terminal_display + (i*terminal_cols) + j)->symbol = 'X';
            }
            if(j==rectangle.c||j==rectangle.w+rectangle.c-1){
                (terminal_display + (i*terminal_cols) + j)->symbol = 'X';
            }
        
        }
    }
}



int display_copy_h(tile_t *dest, char *source){
    int i=0;
    while(source[i]!=0){
        dest[i].symbol = source[i];
        i++;
    }
    return i;
}

int display_copy_v(tile_t *dest, char *source){
    int index = (dest - terminal_display);
    int r = index / terminal_cols;
    int c = index % terminal_cols;    

    int i=0;
    while(source[i]!=0&&r>=0&&r<terminal_rows && c>=0&&c<terminal_cols){
        dest->symbol = source[i];
        dest += terminal_cols;
        i++;
        r++;
    }   
    return i;
}

int display_copy_vc(int r, int c, char *source, char *color){

    int i=0;
    while(source[i]!=0){
        if(r>=0&&r<terminal_rows && c>=0&&c<terminal_cols){
            (terminal_display+r*terminal_cols+c)->symbol = source[i];
            strcpy((terminal_display+r*terminal_cols+c)->color, color);
        }
        i++;
        r++;
    }   
    return i;
}


typedef struct snake_node{
    uint32_t row;
    uint32_t col;
}snode_t;

typedef struct snake_head{
    snode_t *body;
    int length;
    uint8_t direction;
    uint32_t row;
    uint32_t col;
}snake_t;


typedef struct cursor_data{
    int row;
    int col;
    int show;
}cursor_t;
cursor_t cursor;


static void restore_input(void){
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1) fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    setvbuf(stdout, NULL, _IOLBF, 0); 
}

int getch_nb(void)
{
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1)           return (int)c;
    if (n == 0 || (n == -1)) return -1; 
    return -2;                          
}

void get_winsize(int *rows, int *cols){
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *cols = ws.ws_col;
    *rows = ws.ws_row;
}

void present(){
    cursor_origin();
    for(int i=0; i<terminal_rows*terminal_cols; i++){
        if(terminal_display[i].symbol!=prev_frame[i].symbol||strcmp(terminal_display[i].color,prev_frame[i].color)){
            printf("\x1b[%d;%dH", i/terminal_cols+1, i%terminal_cols+1);
            printf("%s%c",terminal_display[i].color,terminal_display[i].symbol);
        }
    }
    


    printf("\x1b[%d;%dH", cursor.row+1, cursor.col+1);
    fflush(stdout);
    memcpy(prev_frame, terminal_display, sizeof(tile_t)*terminal_cols*terminal_rows);
}


#define UP 0
#define LEFT 1
#define DOWN 2
#define RIGHT 3

typedef struct apple_data{
    int r;
    int c;
}apple_t;
apple_t apple;

char move_snake(snake_t *snake, char input){
    rect_t rectangle;
    rectangle.r = 0;
    rectangle.c = 0;
    rectangle.w = terminal_cols;
    rectangle.h = terminal_rows;    
    draw_border(rectangle);


    char counter[5];
    sprintf(counter,"%d",snake->length);
    display_copy_h(terminal_display+terminal_cols+2,counter);

    if(snake->direction==UP){
        snake->row--;
    }else if(snake->direction==LEFT){
        snake->col--;
    }else if(snake->direction==DOWN){
        snake->row++;
    }else{
        snake->col++;
    }

    if(snake->row<(terminal_rows-1) && snake->row>0 && snake->col<(terminal_cols-1) && snake->col>0 &&
    (terminal_display + snake->row*terminal_cols + snake->col)->symbol != '#'){

        if((terminal_display + snake->row*terminal_cols + snake->col)->symbol == '@'){
            snake->length++;
            snake->body = realloc(snake->body, sizeof(snode_t)*(snake->length));
            snake->body[snake->length-1].row = snake->body[snake->length-2].row;
            snake->body[snake->length-1].col = snake->body[snake->length-2].col;
            
            int ir = 1 + rand() % (terminal_rows-2);
            int ic = 1 + rand() % (terminal_cols-2);
            while((terminal_display+ir*terminal_cols+ic)->symbol != ' '){
                ir = 1 + rand() % (terminal_rows-2);
                ic = 1 + rand() % (terminal_cols-2);
            }
            apple.r = ir;
            apple.c = ic;
            (terminal_display+ir*terminal_cols+ic)->symbol = '@';
            strcpy((terminal_display+ir*terminal_cols+ic)->color,"\x1b[31m");
        }

        (terminal_display + snake->body[snake->length-1].row*terminal_cols + snake->body[snake->length-1].col)->symbol = ' ';
        strcpy((terminal_display + snake->body[snake->length-1].row*terminal_cols + snake->body[snake->length-1].col)->color,"\x1b[0m");
        for(int i=snake->length-1; i>0;i--){
            
            snake->body[i].row = snake->body[i-1].row;
            snake->body[i].col = snake->body[i-1].col;  
            (terminal_display + snake->body[i].row*terminal_cols + snake->body[i].col)->symbol = '#';
            strcpy((terminal_display + snake->body[i].row*terminal_cols + snake->body[i].col)->color,"\x1b[38;5;218m");
            
        }
        
        snake->body[0].row = snake->row;
        snake->body[0].col = snake->col;
        (terminal_display + snake->body[0].row*terminal_cols + snake->body[0].col)->symbol = '#';
        strcpy((terminal_display + snake->body[0].row*terminal_cols + snake->body[0].col)->color,"\x1b[38;5;218m");

    }else{
        display_copy_h(terminal_display+2*terminal_cols+2,"You Died!");
        return 'q';
    }
    return input;
}


typedef struct trail_data{
    char *string;
    int r;
    int c;
}trail_t;

trail_t *get_trail(int min_length, int max_length){

    int trail_length = min_length + rand() % (max_length - min_length + 1);
    trail_t *trail = malloc(sizeof(trail_t));
    trail->string = calloc(1, trail_length + 2);
    trail->r = -(max_length*2) + rand() % (max_length - max_length*2 + 1);
    trail->c = 1 + rand() % (terminal_cols - 1);
    if(trail_length>9 && !(rand() % 51)){
        strcpy(trail->string,"NIGHTWING");
    }else{
        for(int i=0; i<trail_length; i++ )
            trail->string[i] = rand() % 2 + 0x30; 

    }

    return trail;
}

trail_t **get_set(int n_trails, int min_length, int max_length){

    trail_t **trail_set = malloc(sizeof(trail_t *)*n_trails);
    
    for(int i=0; i<n_trails; i++ )
        trail_set[i] = get_trail(min_length,max_length);

    return trail_set;
}



void matrix(void){

    int n_trails = 5+terminal_cols/50;
    trail_t ***trails;
    int max_length = terminal_rows;
    int min_length = 5;    
    int n_cycles = (2*max_length+terminal_rows)/4;
    int trail_length;
    int i,j;
    char input = 0;

    char color[32];
    int trail_set;
    int trail;
    int digit;

    display_copy_h(terminal_display+1*terminal_cols-2, "press enter to start");
    display_copy_h(terminal_display+2*terminal_cols-2, "press r,g,b,y,w to change color");
    display_copy_h(terminal_display+3*terminal_cols-2, "colors will vary based on terminal");
    display_copy_h(terminal_display+4*terminal_cols-2, "press x to clear the color");
    display_copy_h(terminal_display+5*terminal_cols-2, "press q or esc to quit");
    present();

    while(input!='\e'&&input!='q'&&input!='\n'){
        input = getch_nb(); 
        usleep(10000);
    }
       
    trails = calloc(sizeof(trail_t **), n_cycles);
    
    while(input!='q'&&input!='\e'){

        for(i=1;i<2*max_length+terminal_rows;i++){
            input = getch_nb();
            if(input=='q'||input=='\e') break;
            switch(input){
                case('r'):{
                    strcpy(color,"\x1b[31m");
                $}case('g'):{
                    strcpy(color,"\x1b[32m");
                $}case('b'):{
                    strcpy(color,"\x1b[34m");
                $}case('y'):{
                    strcpy(color,"\x1b[33m");
                $}case('w'):{
                    strcpy(color,"\x1b[37m");
                $}case('p'):{
                    strcpy(color,"\x1b[38;5;218m");
                $}case('x'):{
                    strcpy(color,"\x1b[0m");
                $}default:$
            }

            clear_screen();

            for( trail_set=0; trail_set<n_cycles; trail_set++ ){
                if(trails[trail_set]){
                    for( trail=0; trail<n_trails; trail++ ){
                        
                        display_copy_vc(trails[trail_set][trail]->r,trails[trail_set][trail]->c, trails[trail_set][trail]->string, color);
                        trails[trail_set][trail]->r++;

                    }
                }

            }
            if(!(i % (n_cycles))){
                for(j=0; j<n_cycles-1;j++){
                    if(trails[j+1]){
                        if(!j && trails[j]) free(trails[j]);
                        trails[j] = trails[j+1];
                    }
                }
                trails[j] = get_set(n_trails,min_length,max_length);
            }

            present();
            usleep(125000);
        }
    }
    for( trail_set=0; trail_set<n_cycles; trail_set++ ){
        if( trails[trail_set] ){
            for( trail=0; trail<n_trails; trail++ ){  
                free( trails[trail_set][trail]->string );
                free( trails[trail_set][trail] );
            }
            free( trails[trail_set] );
        }
        
    }
    free(trails);
}

int snake_ai(snake_t snake,int diffr, int diffc, int *snaketimer){
    int direction;
    if(!*snaketimer){
        if(diffr < 0){
            if(snake.direction!=UP){
                if( ((terminal_display+(snake.row-1)*terminal_cols+snake.col)->symbol==' ' ||
                    (terminal_display+(snake.row-1)*terminal_cols+snake.col)->symbol=='@')  &&
                    (snake.length <= 1 || snake.direction!=DOWN)){ 
                    direction = UP;
                }else{
                    if(((terminal_display+snake.row*terminal_cols+snake.col-1)->symbol==' ' ||
                        (terminal_display+snake.row*terminal_cols+snake.col-1)->symbol=='@') &&
                        (snake.length <= 1 || snake.direction!=RIGHT)){
                        direction = LEFT;
                    }else if(((terminal_display+snake.row*terminal_cols+snake.col+1)->symbol==' ' ||
                            (terminal_display+snake.row*terminal_cols+snake.col+1)->symbol=='@')&&
                                (snake.length <= 1 || snake.direction!=LEFT)){
                            direction = RIGHT;
                    }else{
                            direction = DOWN;
                    }
                }
            }
        }else if(diffr > 0){
            if(snake.direction!=DOWN){
                if( ((terminal_display+(snake.row+1)*terminal_cols+snake.col)->symbol==' ' ||
                    (terminal_display+(snake.row+1)*terminal_cols+snake.col)->symbol=='@')&&
                    (snake.length <= 1 || snake.direction!=UP)){ 
                    direction = DOWN;
                }else{
                    if(((terminal_display+snake.row*terminal_cols+snake.col-1)->symbol==' ' || 
                        (terminal_display+snake.row*terminal_cols+snake.col-1)->symbol=='@')&&
                    (snake.length <= 1 || snake.direction!=RIGHT)){
                        direction = LEFT;
                    }else if(((terminal_display+snake.row*terminal_cols+snake.col+1)->symbol==' ' || 
                            (terminal_display+snake.row*terminal_cols+snake.col+1)->symbol=='@')&&
                                (snake.length <= 1 || snake.direction!=LEFT)){
                            direction = RIGHT;
                    }else{
                            direction = UP;
                    }
                }
            }
        }else{
            if(diffc < 0){
                if(snake.direction!=LEFT){
                    if( ((terminal_display+snake.row*terminal_cols+snake.col-1)->symbol==' ' ||
                        (terminal_display+snake.row*terminal_cols+snake.col-1)->symbol=='@') &&
                        (snake.length <= 1 || snake.direction!=RIGHT)){ 
                        direction = LEFT;
                    }else{
                        if(((terminal_display+(snake.row-1)*terminal_cols+snake.col)->symbol==' ' ||
                            (terminal_display+(snake.row-1)*terminal_cols+snake.col)->symbol=='@')&&
                        (snake.length <= 1 || snake.direction!=DOWN)){
                            direction = UP;
                        }else if(((terminal_display+(snake.row+1)*terminal_cols+snake.col)->symbol==' ' ||
                                (terminal_display+(snake.row+1)*terminal_cols+snake.col)->symbol=='@')&&
                                (snake.length <= 1 || snake.direction!=UP)){
                            direction = DOWN;
                        }else{
                            direction = RIGHT;
                        }
                    }
                }
            }else if(diffc > 0){
                if(snake.direction!=RIGHT){
                    if( ((terminal_display+snake.row*terminal_cols+snake.col+1)->symbol==' ' ||
                        (terminal_display+snake.row*terminal_cols+snake.col+1)->symbol=='@') &&
                        (snake.length <= 1 || snake.direction!=LEFT)){ 
                        direction = RIGHT;  
                    }else{
                        if(((terminal_display+(snake.row-1)*terminal_cols+snake.col)->symbol==' ' ||
                            (terminal_display+(snake.row-1)*terminal_cols+snake.col)->symbol=='@')&&
                            (snake.length <= 1 || snake.direction!=DOWN)){
                            direction = UP;
                        }else if(((terminal_display+(snake.row+1)*terminal_cols+snake.col)->symbol==' ' ||
                                (terminal_display+(snake.row+1)*terminal_cols+snake.col)->symbol=='@')&&
                                (snake.length <= 1 || snake.direction!=UP)){
                            direction = DOWN;
                        }else{
                            direction = LEFT;
                        }
                    }
                }                    
            }
        }
    }
    return direction;
}


#define MOVING 0
#define TYPING 1
#define SNAKING 2
#define MATRIX 3
int mode;
int mouse;


int main(void) {

    struct sigaction sa = {0};
    sa.sa_handler = sigint_handler;
    sa.sa_flags   = SA_RESTART;
    sigaction(SIGINT, &sa, NULL); 
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    init_nonblocking_input();
    cursor_origin();
    cursor.show = 1;
    get_winsize(&terminal_rows, &terminal_cols);

    terminal_display = malloc(sizeof(tile_t)*terminal_rows*terminal_cols);
    prev_frame = malloc(sizeof(tile_t)*terminal_rows*terminal_cols);
    
    clear_screen();   

    int temp_dir;
    snake_t snake;
    int fast_snake = 1;
    int snaketimer;
    char text[10];
    char input = 0;
    int buff = {0};
    int autosnake = 0;
    cursor_origin();
    while(input!='q'&&input!='\e'){
        if(!autosnake){
            clear_screen();
            display_copy_h(terminal_display+1*terminal_cols+2, "press wasd to move");
            display_copy_h(terminal_display+2*terminal_cols+2, "press i to type and esc to stop");
            display_copy_h(terminal_display+3*terminal_cols+2, "press k to snake");
            display_copy_h(terminal_display+4*terminal_cols+2, "press m to matrix");
            present();
        }
        input = getch_nb();
            switch(mode){
                case(MOVING):{
                   switch(input){
                        case('w'):{
                            if(cursor.row>0){
                                cursor.row--;
                                cursor_up(1);
                            }
                        $}case('a'):{
                            if(cursor.col>0){
                                cursor.col--;
                                cursor_back(1);
                            }
                        $}case('s'):{
                            if(cursor.row<terminal_rows-1){
                                cursor.row++;
                                cursor_down(1);
                            }
                        $}case('d'):{
                            if(cursor.col<terminal_cols-1){
                                cursor.col++;
                                cursor_forward(1);
                            }
                        $}case('c'):{
                            clear_line();
                        $}case('x'):{
                            clear_screen();
                        $}case('i'):{
                            mode=TYPING;
                        $}case('k'):{
                            mode=SNAKING;
                        $}case('m'):{
                            mode=MATRIX; 
                        $}default:$
                    }
                $}case(TYPING):{
                    switch(input){
                       case('\e'):{
                            mode=MOVING;
                        $}case('\n'):{
                            cursor_down(1);
                            cursor.row++;
                            cursor_back(cursor.col);
                            cursor.col = 0;
                        $}default:{
                            if(input>=0x21&&input<=0x7E){
                                display_copy_h(terminal_display+(cursor.row)*terminal_cols+cursor.col,&input);
                                cursor.col++;
                                cursor_forward(1);      
                            }
                        $}
                    }
                    input = 0;
                $}case(SNAKING):{

                    clear_screen();
                    if(!autosnake){
                        fast_snake = 1;
                        display_copy_h(terminal_display+1*terminal_cols+2, "press enter to start");
                        display_copy_h(terminal_display+2*terminal_cols+2, "press wasd to move");
                        display_copy_h(terminal_display+3*terminal_cols+2, "press g to autosnake");
                        display_copy_h(terminal_display+4*terminal_cols+2, "press f to increase speed");
                        display_copy_h(terminal_display+5*terminal_cols+2, "press l to get longer");
                        display_copy_h(terminal_display+6*terminal_cols+2, "press q or esc to quit");
                        present();
                        while(input!='\e'&&input!='q'&&input!='\n'){
                            input = input = getch_nb();
                        }
                        clear_screen();                        
                    }

                    
                    int ir = 1 + rand() % (terminal_rows-2);
                    int ic = 1 + rand() % (terminal_cols-2);
                    while((terminal_display+ir*terminal_cols+ic)->symbol != ' '){
                        ir = 1 + rand() % (terminal_rows-2);
                        ic = 1 + rand() % (terminal_cols-2);
                    }
                    apple.r = ir;
                    apple.c = ic;
                    (terminal_display+ir*terminal_cols+ic)->symbol = '@';
                    strcpy((terminal_display+ir*terminal_cols+ic)->color,"\x1b[31m");

                    snake.body = calloc(1,sizeof(snake_t));
                    snake.length = 1;
                    snake.direction = 0;

                    snake.row = terminal_rows/2;
                    snake.col = terminal_cols/2;
                    snake.body[0].row = snake.row;
                    snake.body[0].col = snake.col;
                    snaketimer = 0;
                    int move_flag = 1;
                    int all_wait = 1;
                    int diffr;
                    int diffc;
                    hide_cursor();
                    while(input!='q'&&input!='\e'){
                        input = getch_nb();
                        if(input>0) input = (char)tolower((unsigned char)input);
                        switch(input){
                            case('w'):{
                                if((snake.length <= 1 || snake.direction!=DOWN)&&snake.direction!=UP){ 
                                    snake.direction = UP;
                                    snaketimer = 0;
                                }
                            $}case('a'):{
                                if((snake.length <= 1 || snake.direction!=RIGHT)&&snake.direction!=LEFT){ 
                                    snake.direction = LEFT;
                                    snaketimer = 0;
                                }
                            $}case('s'):{
                               if((snake.length <= 1 || snake.direction!=UP)&&snake.direction!=DOWN){ 
                                    snake.direction = DOWN;
                                    snaketimer = 0;
                                }
                            $}case('d'):{
                                if((snake.length <= 1 || snake.direction!=LEFT)&&snake.direction!=RIGHT){ 
                                    snake.direction = RIGHT;
                                    snaketimer = 0;
                                }
                            $}case('l'):{
                               snake.length++;
                                snake.body = realloc(snake.body, sizeof(snode_t)*(snake.length));
                                snake.body[snake.length-1].row = snake.body[snake.length-2].row;
                                snake.body[snake.length-1].col = snake.body[snake.length-2].col;
                            $}case('f'):{
                               if(fast_snake<20) fast_snake+=2;
                               else fast_snake = 1;
                            $}case('g'):{
                                autosnake = !autosnake;
                            $}default:$
                        }
                        if(autosnake){
                            diffr = apple.r-snake.row;
                            diffc = apple.c-snake.col;
                            snake.direction = snake_ai(snake,diffr,diffc,&snaketimer);
                        }
                        if(snaketimer){
                            snaketimer--;
                        }else{
                            input = move_snake(&snake, input);
                            if(move_flag>2){
                                move_flag -= 2;
                            }else{
                                move_flag = 0;
                            }
                            all_wait = 1;
                            snaketimer = 15 - snake.length/5;
                            if(snaketimer<0) snaketimer = 1;
                        }
                        usleep(10000/fast_snake);
                        
                        present();

                    }
                    input = 0;
                    free(snake.body);
                    snake.length = 1;
                    snake.direction = 0;

                    snake.row = terminal_rows/2;
                    snake.col = terminal_cols/2;
                    snaketimer = 0;
                    if(!autosnake) mode = MOVING;
                    show_cursor();
                $}case(MATRIX):{

                    hide_cursor();
                    clear_screen();
                    matrix(); 
                    

                    input = 0;
                    mode = MOVING;
                    show_cursor();
                $}default:$

            }
    usleep(10000);
    }
    clear_screen();
    cursor_bottom();
    cursor_leftend();
    restore_input();
    return 0;
}