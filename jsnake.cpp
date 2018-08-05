/*JOSEPH SHAKER 2018
**jsnake
**A c++ implementation of snake using the ncures library
*/
#include <chrono>
#include <ncurses.h>
#include <random>
#include <vector>

#define STATS_HEIGHT    2
#define STATS_WIDTH     59

#define PAUSE_HEIGHT    7
#define PAUSE_WIDTH     40

#define STARTING_LEN    5
#define LENGTH_PER_FOOD 1 

#define COLS_PER_ROW    2  //number of colums per row, to try to make squares

#define STARTING_ROW    3  //Row that the tail spawns on, def 3
#define STARTING_COL    2  //Col that the tail spawns on, def 2

#define HEAD_PAIR       1 //number of the color pair for the head 
#define BODY_PAIR       2 // .. body
#define FOOD_PAIR       3 // .. Food

#define TICK_LENGTH     105  //time in ms of each horiz frame
#define VERT_RATIO      1.00 //Ratio of Vert/horiz ticks 
                             //compensates that rows are longer than cols

void draw_border ( WINDOW*, int, int, int );
void pause_menu  ( WINDOW*, int, int );
void update_stats( WINDOW*, int, int, int, int );
int  lose_screen ( WINDOW*, int, int, int, int );
void victory     ( WINDOW*, int, int, int, int );
void redraw_snake( WINDOW*,  std::vector<char> , int , int ,int , int  );

int main() 
{
    srand (  std::chrono::duration_cast<std::chrono::microseconds>
             ( std::chrono::system_clock::now().time_since_epoch()).count() );
    initscr();
    curs_set( 0 );
    noecho();
    keypad(stdscr, TRUE);
    
    start_color();

    //picks colors for the snake and food
    //NUMBER    TEXT_COLOR   HIGHLIGHT_COLOR
    init_pair( HEAD_PAIR, COLOR_BLACK, COLOR_RED    );
    init_pair( BODY_PAIR, COLOR_BLACK, COLOR_GREEN  );
    init_pair( FOOD_PAIR, COLOR_BLACK, COLOR_YELLOW );

    int ROWS = 0 ;
    int COLS = 0 ;
    int  HIGH_SCORE = 0;
    bool PLAY_AGAIN = 1;

    getmaxyx( stdscr, ROWS, COLS );

    int temp  = COLS;
    while ( ( temp - 2 ) % STATS_HEIGHT != 0 )
        temp--;
    
    const int cols = temp;
    const int rows = ROWS;
    resizeterm( rows, cols );

    if ( rows < ( 3 + STATS_HEIGHT + PAUSE_HEIGHT ) 
         || cols < ( std::max( STATS_WIDTH, PAUSE_WIDTH ) + 2 ) )
    {
        endwin();
        printf("Terminal is too small to fit the stats\nMinimum Size:%3dx%2d\n", cols < ( STATS_WIDTH + 3 ),( 3 + STATS_HEIGHT + 11 ) );
        return 1;

    }

    if ( rows <= ( 3 + STATS_HEIGHT + STARTING_ROW ) 
         || cols <= 2 + COLS_PER_ROW * ( STARTING_COL + STARTING_LEN ) )
    {
        endwin();
        printf("Terminal is too small to fit the starting snake\nMinimum Size:%3dx%2d\n", cols < ( STATS_WIDTH + 3 ),( 3 + STATS_HEIGHT + 11 ) );
        return 1;

    }

    if ( STARTING_LEN < 2 )
    {
        endwin();
        printf( "Starting length must be >=2\n");
        return 1;
    }

    int game_rows = rows - STATS_HEIGHT - 3;
    int game_cols = cols - 2;

    //height, width, starty,startx
    WINDOW* main_win = newwin( rows, cols, 0, 0 );
    WINDOW* stats_win = newwin( STATS_HEIGHT, cols - 2  , 1, 1 );
    WINDOW* game_win = newwin( game_rows , game_cols, 2 + STATS_HEIGHT, 1  );    

    draw_border( main_win, rows, cols, STATS_HEIGHT );
    update_stats( stats_win, 0, HIGH_SCORE, game_rows, game_cols );

    refresh();

    pause_menu( game_win, game_rows, game_cols );

    while ( PLAY_AGAIN )
    {    
        std::vector <char> grid;    //rows are stored sequentially
                                    //row 0 col 5 is [ 5 ], row 1 col 5 is [ 1* numCols + 5]

        grid.resize( game_rows * game_cols / COLS_PER_ROW, 0 );

        nodelay(stdscr, TRUE);      
        
        int FOOD_x = rand() % ( game_cols / COLS_PER_ROW );
        int FOOD_y = rand() % game_rows;
        int score  = 0;
        int ch     = 0;
        int queue  = 0;
        int tail_col, tail_row, head_col, head_row, old_head_row, old_head_col;

        char dir     = '>';
        char old_dir = '>';

        bool quit  = 0;
        bool tick_done = 0;

        update_stats( stats_win, score, HIGH_SCORE, game_rows, game_cols);

        std::chrono::steady_clock::time_point tick_start = std::chrono::steady_clock::now(); 
        std::chrono::steady_clock::time_point tick_end   = std::chrono::steady_clock::now();
        
        tail_col = STARTING_COL;
        tail_row = STARTING_ROW;
        head_col = ( STARTING_COL + STARTING_LEN - 1 );
        head_row = STARTING_ROW;
    
        old_head_col = ( STARTING_COL + STARTING_LEN - 2 );
        old_head_row = STARTING_ROW;

        //generate and draw start snake
        wattron( game_win, COLOR_PAIR( BODY_PAIR ) );
        for( int i = 0; i < STARTING_LEN; i++ )  
        {
            grid[ ( STARTING_ROW * game_cols/COLS_PER_ROW + ( STARTING_COL + i ) ) ] = '>';
            if ( i!=STARTING_LEN-1 )
            {
                for ( int j = 0; j < COLS_PER_ROW; j++)
                    mvwaddch( game_win, STARTING_ROW, COLS_PER_ROW * ( STARTING_COL + i ) + j, ' ' );
            }
            else
            {
                wattroff( game_win, COLOR_PAIR( BODY_PAIR ) );
                wattron ( game_win, COLOR_PAIR(HEAD_PAIR));
                for ( int j = 0; j < COLS_PER_ROW; j++)
                    mvwaddch( game_win, STARTING_ROW, COLS_PER_ROW*( STARTING_COL + i )+ j, ' ' );

                wattroff( game_win, COLOR_PAIR( HEAD_PAIR ) );
            }
        }
        
        wrefresh( game_win );

        while(!quit) 
        {
	        while( !tick_done )     //get user input until tick is over
            {
                ch = getch();
            
                switch(ch) 
                {
                    case KEY_UP:
                        if ( old_dir != 'v' )
                            dir = '^';
                        break;
                    case KEY_RIGHT:
                        if ( old_dir != '<' )
                            dir = '>';
                        break;
                    case KEY_DOWN:
                        if ( old_dir != '^' )
                            dir = 'v';
                        break;
                    case KEY_LEFT:
                        if ( old_dir != '>' )    
                            dir = '<';
                        break;
                    case 'q':
                        quit = 1;
                        break;
                    case 'p':
                        pause_menu( game_win, game_rows, game_cols );
                        redraw_snake( game_win, grid,game_rows, game_cols,  head_row,  head_col  );
                        tick_start = tick_end = std::chrono::steady_clock::now();
                        nodelay(stdscr, TRUE);      //key inputs are nonblocking
                        break;
                }
                
                tick_end = std::chrono::steady_clock::now();

                if ( old_dir == '>' || old_dir == '<' )
                {    
                    tick_done = ( ( std::chrono::duration_cast<std::chrono::milliseconds>
                        (tick_end - tick_start) ).count() ) > TICK_LENGTH ;
                }
                else
                {
                    tick_done = ( ( std::chrono::duration_cast<std::chrono::milliseconds>
                        (tick_end - tick_start) ).count() ) > ( TICK_LENGTH * VERT_RATIO ) ;

                }

            }
            
            tick_start = std::chrono::steady_clock::now();
            
            tick_done = 0;
            old_dir   = dir;
            
            old_head_col = head_col;
            old_head_row = head_row;

            //store the direction the snake is going so we can follow its new tail
            grid[ ( game_cols / COLS_PER_ROW * head_row + head_col ) ] = dir; 

            if     (dir == '^') head_row--;  // move up
            else if(dir == '>') head_col++;  // move right
            else if(dir == 'v') head_row++;  // move down
            else if(dir == '<') head_col--;  // move left
           
            if( head_col == FOOD_x && head_row == FOOD_y) 
            {
                queue += LENGTH_PER_FOOD;
            } 

            //if the snake has some growing to do, increment score
            if ( queue >= 1 )
            {
                queue--;
                score++;
                if ( score > HIGH_SCORE )
                    HIGH_SCORE = score;

                update_stats( stats_win, score, HIGH_SCORE, game_rows, game_cols );

                if ( score + STARTING_LEN == ( game_rows * game_cols ) / COLS_PER_ROW )
                {    
                    victory( game_win, rows, cols, game_rows, game_cols );
                    endwin();
                    return 0;
                }
            }

            //'erase' the tail and update the tail variable based on the direction
            else 
            {
                wattron( game_win, COLOR_PAIR( 0 ) );
                
                for ( int i = 0; i < COLS_PER_ROW; i++)
                    mvwaddch( game_win, tail_row, COLS_PER_ROW*tail_col + i, ' ' );
                
                wattroff( game_win, COLOR_PAIR( 0 ) );
                
                char temp = grid[ ( game_cols / COLS_PER_ROW * tail_row + tail_col ) ];
                grid[ ( game_cols / COLS_PER_ROW * tail_row + tail_col ) ] = 0;
                
                switch ( temp  )
                {
                    case '>':
                        tail_col+=1;
                        break;
                    case '<':
                        tail_col-=1;
                        break;
                    case '^':
                        tail_row-=1;
                        break;
                    case 'v':
                        tail_row+=1;
                        break;
                }
            }
            //check for wall collision
            if(head_row > ( game_rows - 1 ) || head_col > ( game_cols - 1 )/COLS_PER_ROW || head_row < 0 || head_col < 0 )
                quit = true;
                
            //make old head color of the body
            wattron( game_win, COLOR_PAIR( BODY_PAIR ) );
            for ( int i = 0; i < COLS_PER_ROW; i++)
                mvwaddch( game_win, old_head_row, COLS_PER_ROW*old_head_col + i, ' ' );
            wattroff( game_win, COLOR_PAIR( BODY_PAIR ) );

            //paint the new head
            wattron( game_win, COLOR_PAIR( HEAD_PAIR ) );
            for ( int i = 0; i < COLS_PER_ROW; i++)
                mvwaddch( game_win, head_row, COLS_PER_ROW*head_col + i, ' ' );
            wattroff( game_win, COLOR_PAIR( BODY_PAIR ) );

            //check for self collision
            if ( grid[ (game_cols/COLS_PER_ROW * head_row + head_col)]!=0)
                quit = 1;

            //generate new food if need be
            while ( grid[ game_cols/COLS_PER_ROW * FOOD_y + FOOD_x ] != 0 )
            {
                FOOD_x = rand() % ( game_cols / COLS_PER_ROW )  ;
                FOOD_y = rand() % game_rows;
            }
           
            //print food 
            wattron( game_win, COLOR_PAIR( FOOD_PAIR ) );
            for ( int i = 0; i < COLS_PER_ROW; i++)
                mvwaddch( game_win, FOOD_y, COLS_PER_ROW*FOOD_x + i, ' ' );
            wattroff( game_win, COLOR_PAIR( BODY_PAIR ) );

            wrefresh( game_win );
            
        }
        
        //wait for user input
        ch = lose_screen( game_win, score, HIGH_SCORE, game_rows, game_cols );
        
        if ( ch == 'q' )
            PLAY_AGAIN = 0;

    }

    endwin();   
    return 0;

}

void draw_border( WINDOW* win, int rows, int cols, int stats_size )
{   
    refresh();
    wmove( win, 0, 0 );
    waddch( win, '+' );
    for ( int j = 1; j < cols - 1; j++ )
        waddch ( win, '-' );
    waddch( win, '+' );

    for ( int j = 1; j < 1 + stats_size; j++ )
    {
        wmove ( win, j, 0 );
        waddch( win, '|' );
        wmove ( win, j, cols - 1 );
        waddch( win, '|' );        
    }

    wmove ( win, 1 + stats_size, 0 );
    waddch( win, '+' );
    for ( int j = 1; j < cols - 1; j++ )
        waddch ( win,'-' );
    waddch( win, '+' );

    for ( int j = 2 + stats_size; j < rows - 1 ; j++ )
    {
        wmove ( win, j, 0 );
        waddch( win, '|' );
        wmove ( win, j, cols - 1 );
        waddch( win, '|' );
    }

    wmove ( win, rows - 1, 0 ); 
    waddch( win, '+' );  
    for ( int j = 1; j < cols - 1; j++ )
        waddch ( win, '-' );
    waddch( win, '+' );
    wrefresh( win );
    refresh();
}

void pause_menu( WINDOW* win, int rows, int cols )
{     
    timeout( 3600000 );
    mvwprintw( win, rows/2 -3, cols/2 - PAUSE_WIDTH/2,"========================================");
    mvwprintw( win, rows/2 -2, cols/2 - PAUSE_WIDTH/2,"+++++++++++Welcome to  jSnake+++++++++++");
    mvwprintw( win, rows/2 -1, cols/2 - PAUSE_WIDTH/2,"++++++++++++++Eat the Food++++++++++++++");
    mvwprintw( win, rows/2 -0, cols/2 - PAUSE_WIDTH/2,"+++++++Use the Arrow Keys to Move+++++++");
    mvwprintw( win, rows/2 +1, cols/2 - PAUSE_WIDTH/2,"+++Dont Crash into  Walls or Yourself+++");
    mvwprintw( win, rows/2 +2, cols/2 - PAUSE_WIDTH/2,"++++++++Press  any Key to Resume++++++++");
    mvwprintw( win, rows/2 +3, cols/2 - PAUSE_WIDTH/2,"========================================");
    wrefresh( win );
    char a = getch();
    wclear( win );
}   
    
void update_stats( WINDOW* win, int score, int high_score, int rows, int cols )
{
    for ( int i = 0;i < STATS_HEIGHT; i++ )
    {   
        if ( i == 0)
            mvwprintw( win, i, 0, "Length:     %5d || Score:     %5d | High Score:  %5d", STARTING_LEN + score, score, high_score );
        else if ( i == 1 )
            mvwprintw( win, 1, 0, "Game Size: %3dx%2d || Move: Arrow Keys | Quit: q | pause: p ",  cols / COLS_PER_ROW, rows);
        //etc
    }
    wrefresh( win );
}

int lose_screen( WINDOW* win, int score, int high_score, int rows, int cols)
{
    int a = 0;
    mvwprintw( win, rows/2 -2, cols/2 - PAUSE_WIDTH/2,"========================================");
    mvwprintw( win, rows/2 -1, cols/2 - PAUSE_WIDTH/2,"++You lost with a final score of %5d++", score);
    mvwprintw( win, rows/2 -0, cols/2 - PAUSE_WIDTH/2,"+Press Ret to  play again, or q to quit+" );
    mvwprintw( win, rows/2 +1, cols/2 - PAUSE_WIDTH/2,"+++++++++++High Score:  %5d+++++++++++", high_score);
    mvwprintw( win, rows/2 +2, cols/2 - PAUSE_WIDTH/2,"========================================");
    wrefresh( win );
    while ( a != 0x0A && a != 0x0D && a != 'q' )
        a = getch();
    wclear ( win );
    wrefresh( win );
    return a;
}

void victory( WINDOW* win, int rows, int cols, int game_rows, int game_cols )
{
    wclear ( win );
    timeout( 3600000 );
    mvwprintw( win, rows/2 -2, cols/2 - PAUSE_WIDTH/2,"========================================");
    mvwprintw( win, rows/2 -1, cols/2 - PAUSE_WIDTH/2,"++++++++++++A WINNER  IS YOU++++++++++++");
    mvwprintw( win, rows/2 -0, cols/2 - PAUSE_WIDTH/2,"+YOU FILLED THE ENTIRE  %3dx%2d field!+", game_cols/COLS_PER_ROW, game_rows );
    mvwprintw( win, rows/2 +1, cols/2 - PAUSE_WIDTH/2,"++++++++PRESS ANY BUTTON TO EXIT++++++++");
    mvwprintw( win, rows/2 +2, cols/2 - PAUSE_WIDTH/2,"========================================");
    wrefresh( win );
    refresh();
    char a = getch();
}

void redraw_snake( WINDOW* win, std::vector<char> grid,  int game_rows, 
                   int game_cols, int head_row, int head_col  )
{

    wattron( win, COLOR_PAIR( BODY_PAIR ) );

    for ( int i = 0; i < game_rows*game_cols/COLS_PER_ROW; i++ )
    {
        if ( grid[ i ] != 0 )
        {
            for ( int j = 0; j < COLS_PER_ROW; j++ )
            {
                mvwaddch( win, i / ( game_cols/COLS_PER_ROW ), 
                          i % ( game_cols/COLS_PER_ROW ) * COLS_PER_ROW + j, ' ' );
            }
        }
    }

    wattroff(win, COLOR_PAIR( BODY_PAIR ) );

    wattron( win, COLOR_PAIR( HEAD_PAIR ) );
    for ( int j = 0; j < COLS_PER_ROW; j++ )
    {
        mvwaddch( win, head_row, 
                  COLS_PER_ROW*head_col + j, ' ' );

    }
    wattroff(win, COLOR_PAIR( HEAD_PAIR ) );

}