#![allow(non_camel_case_types)]

use std::io::{stdin, stdout, Write, Read};
use std::io::Result as StdResult;

//use std::time::Duration;

use rustix::termios;
use rustix::fd::AsFd;

macro_rules! cursor_origin {
    () => {
        print!("\x1b[H")
    };
}

macro_rules! hide_cursor {
    () => {
        print!("\x1b[?25l")
    };
}

macro_rules! show_cursor {
    () => {
        print!("\x1b[?25h")
    };
}


struct restore {
    old: termios::Termios,
}
impl Drop for restore {
    fn drop(&mut self) {
        let _ = termios::tcsetattr(
            std::io::stdin().as_fd(),
            termios::OptionalActions::Now,
            &self.old,
        );
    }
}


#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum color_t {
    Default = 0,
    Black = 1,
    Red = 2,
    Green = 3,
    Yellow = 4,
    Blue = 5,
    Magenta = 6,
    Cyan = 7,
    White = 8,
}

#[derive(Clone, Copy, PartialEq)]
pub struct cell_t{
    pub glyph: char,
    pub fg_color: color_t,
    pub bg_color: color_t
}

pub struct cursor_t{
    pub row: usize,
    pub col: usize,
    pub visible: bool
}

pub struct term_window_t{
    pub nrows: usize,
    pub ncols: usize,
    array: Vec<cell_t>,
    prev: Vec<cell_t>,
    cursor: cursor_t
}

impl term_window_t {
    pub fn new()->Result<Self, rustix::io::Errno>{
        
        let mut stdout = stdout();
        let dim = termios::tcgetwinsize(stdout.as_fd())?;
        let nrows = dim.ws_row as usize;
        let ncols = dim.ws_col as usize;

        cursor_origin!();
        let cursor: cursor_t = cursor_t { 
            row: 0, 
            col: 0, 
            visible: true 
        };

        let blank: cell_t = cell_t{
            glyph: ' ',
            fg_color: color_t::Default,
            bg_color: color_t::Default,
        };

        let array = vec![blank; nrows * ncols];


        for cell in &array{
                //print!("\x1b[{};{}H", i/ncols+1, i%ncols+1);
                // print!("{}{}{}",self.array[i].bg_color, 
                //                 self.array[i].fg_color,
                //                 self.array[i].glyph);
                print!("{}", cell.glyph);
        }

        print!("\x1b[0m");
        cursor_origin!();

        stdout.flush().unwrap();
        let prev:Vec<cell_t> = array.clone();

        Ok(Self { 
            nrows,
            ncols, 
            array,
            prev,
            cursor
        })
    }

    pub fn clear(&mut self){
        let blank: cell_t = cell_t {
            glyph: ' ',
            fg_color: color_t::Default,
            bg_color: color_t::Default,
        };

        self.array.fill(blank);
    }

    pub fn present(&mut self){

        if !self.cursor.visible {hide_cursor!()};
        cursor_origin!();

        for i in 0..(self.nrows*self.ncols){
            if self.array[i] != self.prev[i]{
                print!("\x1b[{};{}H", i/self.ncols+1, i%self.ncols+1);
                // print!("{}{}{}",self.array[i].bg_color, 
                //                 self.array[i].fg_color,
                //                 self.array[i].glyph);
                print!("{}", self.array[i].glyph);

            }
        }

        print!("\x1b[0m");

        println!("\x1b[{};{}H", self.cursor.row+1, self.cursor.col+1);
        if self.cursor.visible {show_cursor!()};

        stdout().flush().unwrap();
        self.prev.clone_from_slice(&self.array);
    }

    pub fn set(&mut self, row: usize, col: usize ,cell: cell_t){
        self.array[row*self.ncols + col] = cell;
    }


}














struct pos_t{
    r: usize,
    c: usize
}


fn main() -> StdResult<()> {
    let mut stdin = stdin();

    let mut cursor = pos_t{r:0,c:0};

    let mut term: term_window_t = term_window_t::new()?;
    term.clear();
    term.set(term.nrows/2, 
             term.ncols/2, 
             cell_t{ 
                    glyph: 'A', 
                    fg_color: color_t::Default,
                    bg_color: color_t::Default});
    term.present();

    let oldt = termios::tcgetattr(stdin.as_fd())?;
    let rest = restore{
        old: oldt.clone()
    };


    let mut rawt = oldt.clone();

    rawt.local_modes.remove(termios::LocalModes::ICANON);
    rawt.local_modes.remove(termios::LocalModes::ECHO);

    termios::tcsetattr(
        stdin.as_fd(),
        termios::OptionalActions::Now,
        &rawt,
    )?;

    let pointer = cell_t{ 
                    glyph: '+', 
                    fg_color: color_t::Default,
                    bg_color: color_t::Default};
    let clear = cell_t{ 
                    glyph: ' ', 
                    fg_color: color_t::Default,
                    bg_color: color_t::Default};    


    term.set(cursor.r, 
             cursor.c, 
             pointer);
    term.present();

    loop{
        let mut buffer = [0u8; 1];
        stdin.read_exact(&mut buffer)?;
        let ch = buffer[0] as char;
        
        term.set(cursor.r, 
                 cursor.c, 
                clear);

        match ch{

            'w'=>{
                if cursor.r>0 {
                    cursor.r-=1;
                }
            },
            's'=>{
                if cursor.r<term.nrows -1 {
                    cursor.r+=1;
                }            
            },
            'a'=>{
                if cursor.c>0 {
                    cursor.c-=1;
                }
            },
            'd'=>{
                if cursor.c<term.ncols -1 {
                    cursor.c+=1;
                }
            },
            'q'=> break,

            _=>()
        }

    term.set(cursor.r, 
             cursor.c, 
             pointer);
        
    term.present();

    };

    drop(rest);
    Ok(())
}
