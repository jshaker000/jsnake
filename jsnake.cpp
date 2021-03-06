/* JOSEPH SHAKER 2018
** jsnake
** A c++ implementation of snake using the ncures library
**
** It is named after me, but now it humourously looks like it was written in java
*/

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

#include <ncurses.h>

// fill the grid with the snake's previous direction (or if it wasnt there)
enum class Direction: char { up, down, left, right, empt };

static constexpr int STARTING_LEN    = 5;
static constexpr int LENGTH_PER_FOOD = 1;

static constexpr int SCALING_FACT    = 2; // magnifies the game size, rows and cols, by this factor
static constexpr int COLS_PER_ROW    = 2; // number of colums per row, to try to make squares

static constexpr int STARTING_ROW    = 3; //Row that the tail spawns on, def 3
static constexpr int STARTING_COL    = 2; //Col that the tail spawns on, def 2

static constexpr int TICK_LENGTH     = 125;  // time in ms of each horiz frame
static constexpr double VERT_RATIO   = 1.00; // Ratio of Vert/horiz ticks
                                             // compensates that rows are longer than cols

static constexpr int STATS_HEIGHT    = 2;
static constexpr int STATS_WIDTH     = 59;

static constexpr int PAUSE_HEIGHT    = 7;
static constexpr int PAUSE_WIDTH     = 40;

static constexpr int HEAD_PAIR       = 1;
static constexpr int BODY_PAIR       = 2;
static constexpr int FOOD_PAIR       = 3;

static constexpr unsigned char SH    = ' '; // Character for snake head
static constexpr unsigned char SB    = ' '; // .. .. snake body
static constexpr unsigned char FD    = ' '; // .. .. food

static void draw_square (WINDOW*, const int, const int, const int, const char);
static void draw_border (WINDOW*, const int, const int, const int);
static void pause_menu  (WINDOW*, const int, const int);
static void update_stats(WINDOW*, const int, const int, const int, const int);
static int  lose_screen (WINDOW*, const int, const int, const int, const int);
static void victory     (WINDOW*, const int, const int);
static void redraw_snake(WINDOW*, const std::vector<Direction> &, const int , const int ,const int , const int);

int main()
{
    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    start_color();

    //picks colors for the snake and food
    //NUMBER    TEXT_COLOR   HIGHLIGHT_COLOR
    init_pair(HEAD_PAIR, COLOR_WHITE, COLOR_RED   );
    init_pair(BODY_PAIR, COLOR_WHITE, COLOR_GREEN );
    init_pair(FOOD_PAIR, COLOR_WHITE, COLOR_YELLOW);

    // Settings to pipe through a software like lolcat for prettyness
    /*
    init_pair(HEAD_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(BODY_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(FOOD_PAIR, COLOR_WHITE, COLOR_BLACK);
    */

    //determine game size from terminal size
    int  ROWS = 0 ;
    int  COLS = 0 ;
    int  HIGH_SCORE = 0;
    bool PLAY_AGAIN = true;

    getmaxyx(stdscr, ROWS, COLS);

    int temp_cols  = COLS;
    while ((temp_cols - 2) % (COLS_PER_ROW * SCALING_FACT) != 0)
        temp_cols--;

    int temp_rows  = ROWS;
    while ((temp_rows - 3 - STATS_HEIGHT) % (SCALING_FACT) != 0)
        temp_rows--;

    //total size of the screen
    const int cols = temp_cols;
    const int rows = temp_rows;

    resizeterm(rows, cols);

    if (!has_colors())
    {
        endwin();
        printf("Terminal does not support color.\n");
        return 1;
    }

    if (rows < (2 + STATS_HEIGHT + PAUSE_HEIGHT)
         || cols < (std::max(STATS_WIDTH, PAUSE_WIDTH)+2))
    {
        endwin();
        printf("Terminal is too small to fit the stats\nMinimum Size:%3dx%2d\n", ( std::max( STATS_WIDTH, PAUSE_WIDTH ) + 2 + ( COLS - cols ) ),2 + STATS_HEIGHT + PAUSE_HEIGHT );
        return 1;
    }

    if (rows < (4+ STATS_HEIGHT + SCALING_FACT * STARTING_ROW)
        || cols < 3 + COLS_PER_ROW * SCALING_FACT * (STARTING_COL + STARTING_LEN))
    {
        endwin();
        printf("Terminal is too small to fit the starting snake\nMinimum Size:%3dx%2d\n",3 + COLS_PER_ROW * SCALING_FACT * ( STARTING_COL + STARTING_LEN ),4 + STATS_HEIGHT + SCALING_FACT * STARTING_ROW );
        return 1;
    }

    if (STARTING_LEN < 2)
    {
        endwin();
        printf( "Starting length must be >=2\n");
        return 1;
    }
    //'logical' size of the game screen
    const int game_rows = (rows - STATS_HEIGHT - 3) / SCALING_FACT;
    const int game_cols = (cols - 2) / (COLS_PER_ROW * SCALING_FACT);
    const int total_squares = game_rows * game_cols;

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dst_row(0,game_rows-1);
    std::uniform_int_distribution<int> dst_col(0,game_cols-1);

    std::vector <Direction> grid;    // rows are stored sequentially
                                     // row 0 col 5 is [ 5 ], row 1 col 5 is [ 1* numCols + 5]
                                     // lists all the places the snake is and what direction it was moving
                                     // for O(1) collision lookup and to 'follow' the tail every tick

    grid.resize(total_squares, Direction::empt);

    //main win is whole screen, game window is just the play field, stats window is just the stats
    WINDOW* main_win = newwin(rows, cols, 0, 0);
    WINDOW* stats_win= newwin(STATS_HEIGHT, cols - 2  , 1, 1);
    WINDOW* game_win = newwin(game_rows * SCALING_FACT, game_cols * COLS_PER_ROW * SCALING_FACT, 2 + STATS_HEIGHT, 1);

    draw_border(main_win, rows, cols, STATS_HEIGHT);
    pause_menu (game_win, game_rows, game_cols);

    while (PLAY_AGAIN)
    {
        std::fill(grid.begin(), grid.end(), Direction::empt);

        int food_col;
        int food_row;

        int score  = 0;
        int ch;
        int len_q  = 0;
        int tail_col;
        int tail_row;
        int head_col;
        int head_row;
        int old_head_row;
        int old_head_col;
        long long int time_so_far;

        Direction dir     = Direction::right;
        Direction old_dir = dir;

        bool quit      = false;
        bool tick_done = false;

        update_stats(stats_win, score, HIGH_SCORE, game_rows, game_cols);

        //initialize head and tail location of start snake
        //Then initialize gird with the direction the snake was going at each point
        // and draw the snake
        tail_col = STARTING_COL;
        tail_row = STARTING_ROW;
        head_col = STARTING_COL + STARTING_LEN - 1;
        head_row = STARTING_ROW;

        old_head_col = (STARTING_COL + STARTING_LEN - 2);
        old_head_row = STARTING_ROW;

        // draw initial snake
        std::for_each(grid.begin()+STARTING_ROW*game_cols+STARTING_COL,
                      grid.begin()+STARTING_ROW*game_cols+STARTING_COL+STARTING_LEN,
                      [](auto &g) { g = Direction::right; });

        do {
            food_col = dst_col(rng);
            food_row = dst_row(rng);
        } while ((grid[food_row * game_cols + food_col] != Direction::empt)
                 || (food_col == head_col && food_row == head_row));

        redraw_snake(game_win, grid,game_rows, game_cols, head_row, head_col);

        wrefresh(game_win);

        std::chrono::steady_clock::time_point tick_start = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point tick_end   = std::chrono::steady_clock::now();

        while(!quit)
        {
            // get user input until tick is over
            // do not allow user to go right if going left etc to kill themselves
            // support VIM, Arrows, WASD
            while(!tick_done)
            {
                ch = tolower(getch());

                switch(ch)
                {
                    case 'k': case 'w': case KEY_UP:
                        if (old_dir != Direction::down)
                            dir = Direction::up;
                        break;
                    case 'l': case 'd': case KEY_RIGHT:
                        if (old_dir != Direction::left)
                            dir = Direction::right;
                        break;
                    case 'j': case 's': case KEY_DOWN:
                        if (old_dir != Direction::up)
                            dir = Direction::down;
                        break;
                    case 'h': case 'a': case KEY_LEFT:
                        if (old_dir != Direction::right)
                            dir = Direction::left;
                        break;
                    case 'q':
                        quit = true;
                        break;
                    case 'p':
                        pause_menu  (game_win, game_rows, game_cols);
                        redraw_snake(game_win, grid,game_rows, game_cols, head_row, head_col);
                        tick_start = tick_end = std::chrono::steady_clock::now();
                        break;
                }

                tick_end = std::chrono::steady_clock::now();

                time_so_far = ((std::chrono::duration_cast<std::chrono::milliseconds> (tick_end - tick_start)).count());

                if (old_dir == Direction::left || old_dir == Direction::right)
                    tick_done =  time_so_far > TICK_LENGTH;

                else
                    tick_done = time_so_far > (TICK_LENGTH * VERT_RATIO);

                //sleeps for up to 95% of the tick while the OS waits for a keyboard input
                if (tick_done == 0 && TICK_LENGTH - time_so_far  > .05 * TICK_LENGTH)
                    std::this_thread::sleep_for(std::chrono::milliseconds((int)(.95*TICK_LENGTH - time_so_far)));

            }

            tick_start = std::chrono::steady_clock::now();

            tick_done = 0;
            old_dir   = dir;

            old_head_col = head_col;
            old_head_row = head_row;

            //store the direction the snake is going so we can follow its new tail when it eventually gets there
            grid[(head_row * game_cols + head_col)] = dir;
            switch (dir)
            {
                case Direction::up:    head_row--; break;
                case Direction::right: head_col++; break;
                case Direction::down:  head_row++; break;
                case Direction::left:  head_col--; break;
                default: break;
            }

            // eat food
            if(head_col == food_col && head_row == food_row)
            {
                len_q += LENGTH_PER_FOOD;
                if (score + STARTING_LEN + 1 != total_squares)
                {
                    do {
                        food_col = dst_col(rng);
                        food_row = dst_row(rng);
                    } while ((grid[food_row * game_cols + food_col] != Direction::empt)
                             || (food_col == head_col && food_row == head_row));
                }
            }

            //if the snake has some growing to do, increment score and dont clear tail
            if (len_q > 0)
            {
                len_q--;
                score++;
                if (score > HIGH_SCORE)
                    HIGH_SCORE = score;

                update_stats(stats_win, score, HIGH_SCORE, game_rows, game_cols);

                if (score + STARTING_LEN == total_squares)
                {
                    victory(game_win, game_rows, game_cols);
                    endwin();
                    return 0;
                }
            }
            // 'erase' the tail visually and from grid vector
            // and update the tail location based on the direction the snake was going (stored in grid vector)
            else
            {
                draw_square(game_win, 0, tail_row, tail_col, ' ');

                Direction tmp = grid[(tail_row * game_cols + tail_col)];
                grid[(tail_row * game_cols + tail_col)] = Direction::empt;
                switch(tmp)
                {
                    case Direction::up:    tail_row-=1; break;
                    case Direction::right: tail_col+=1; break;
                    case Direction::down:  tail_row+=1; break;
                    case Direction::left:  tail_col-=1; break;
                    default: break;
                }
            }

            // check for wall collision
            if(!quit && (head_row > (game_rows - 1) || head_col > (game_cols - 1) || head_row < 0 || head_col < 0))
                quit = true;

            // check for self collision
            if (!quit && (grid[(game_cols * head_row + head_col)] != Direction::empt))
                quit = true;

            // repaint old head with color of the body
            draw_square(game_win, BODY_PAIR, old_head_row, old_head_col, SB);

            // paint the new head head color
            draw_square(game_win, HEAD_PAIR, head_row, head_col, SH);

            // print food
            draw_square(game_win, FOOD_PAIR, food_row, food_col, FD);

            wrefresh(game_win);
        }

        ch = lose_screen(game_win, score, HIGH_SCORE, game_rows, game_cols);

        if (ch == 'q' || ch == 'Q')
            PLAY_AGAIN = 0;
    }
    endwin();
    return 0;
}

static void draw_square(WINDOW* win, const int color_pair, const int start_row, const int start_col, const char ch)
{
    wattron(win, COLOR_PAIR(color_pair));
    for (int i = 0; i < SCALING_FACT; i++)
        for (int j = 0; j < COLS_PER_ROW*SCALING_FACT; j++)
            mvwaddch(win, start_row*SCALING_FACT+i, start_col*COLS_PER_ROW*SCALING_FACT + j, ch);
    wattroff(win, COLOR_PAIR(color_pair));
}

static void draw_border(WINDOW* win, const int rows, const int cols, const int stats_size)
{
    refresh();
    wmove(win, 0, 0);
    waddch(win, '+');
    for (int j = 1; j < cols - 1; j++)
        waddch ( win, '-' );
    waddch(win, '+');

    for (int j = 1; j < 1 + stats_size; j++)
    {
        wmove (win, j, 0);
        waddch(win, '|');
        wmove (win, j, cols - 1);
        waddch(win, '|');
    }

    wmove (win, 1 + stats_size, 0);
    waddch(win, '+' );
    for (int j = 1; j < cols - 1; j++)
        waddch (win,'-');
    waddch( win, '+');

    for (int j = 2 + stats_size; j < rows - 1 ; j++)
    {
        wmove (win, j, 0);
        waddch(win, '|');
        wmove (win, j, cols - 1);
        waddch(win, '|');
    }

    wmove (win, rows - 1, 0);
    waddch(win, '+');
    for (int j = 1; j < cols - 1; j++)
        waddch (win, '-');
    waddch(win, '+');
    wrefresh(win);
    refresh();
}

static void pause_menu(WINDOW* win, const int game_rows, const int game_cols)
{
    nodelay(stdscr, FALSE);
    const int cols = game_cols * COLS_PER_ROW * SCALING_FACT;
    const int rows = game_rows * SCALING_FACT;
    mvwprintw(win, rows/2 -3, cols/2 - PAUSE_WIDTH/2,"========================================");
    mvwprintw(win, rows/2 -2, cols/2 - PAUSE_WIDTH/2,"+++++++++++Welcome to  jSnake+++++++++++");
    mvwprintw(win, rows/2 -1, cols/2 - PAUSE_WIDTH/2,"++++++++++++++Eat the Food++++++++++++++");
    mvwprintw(win, rows/2 -0, cols/2 - PAUSE_WIDTH/2,"+++++++Use the Arrow Keys to Move+++++++");
    mvwprintw(win, rows/2 +1, cols/2 - PAUSE_WIDTH/2,"+++Dont Crash into  Walls or Yourself+++");
    mvwprintw(win, rows/2 +2, cols/2 - PAUSE_WIDTH/2,"++++++++Press  any Key to Resume++++++++");
    mvwprintw(win, rows/2 +3, cols/2 - PAUSE_WIDTH/2,"========================================");
    wrefresh(win);
    refresh();
    getch();
    nodelay(stdscr, TRUE);
    wclear(win);
}

static void update_stats(WINDOW* win, const int score, const int high_score, const int game_rows, const int game_cols)
{
    for (int i = 0; i < STATS_HEIGHT; i++)
    {
        if (i == 0)
            mvwprintw(win, i, 0, "Length:     %5d || Score:     %5d | High Score:  %5d", STARTING_LEN + score, score, high_score );
        else if (i == 1)
            mvwprintw( win, 1, 0, "Game Size: %3dx%2d || Move: Arrow Keys | Quit: q | Pause: p ", game_cols, game_rows );
        //etc
    }
    wrefresh(win);
}

static int lose_screen(WINDOW* win, const int score, const int high_score, const int game_rows, const int game_cols)
{
    nodelay(stdscr, FALSE);
    const int cols = game_cols * COLS_PER_ROW * SCALING_FACT;
    const int rows = game_rows * SCALING_FACT;
    mvwprintw(win, rows/2 -2, cols/2 - PAUSE_WIDTH/2,"========================================");
    mvwprintw(win, rows/2 -1, cols/2 - PAUSE_WIDTH/2,"++You lost with a final score of %5d++", score);
    mvwprintw(win, rows/2 -0, cols/2 - PAUSE_WIDTH/2,"+Press Ret to  play again, or q to quit+" );
    mvwprintw(win, rows/2 +1, cols/2 - PAUSE_WIDTH/2,"+++++++++++High Score:  %5d+++++++++++", high_score);
    mvwprintw(win, rows/2 +2, cols/2 - PAUSE_WIDTH/2,"========================================");
    wrefresh(win);
    int a = 0;
    while (a != '\n' && a != '\r' && a != 'q' && a != 'Q')
        a = getch();
    wclear  (win);
    wrefresh(win);
    nodelay(stdscr, TRUE);
    return a;
}

static void victory(WINDOW* win, const int game_rows, const int game_cols)
{
    wclear (win);
    nodelay(stdscr, FALSE);
    const int cols = game_cols * COLS_PER_ROW * SCALING_FACT;
    const int rows = game_rows * SCALING_FACT;
    mvwprintw(win, rows/2 -2, cols/2 - PAUSE_WIDTH/2,"========================================");
    mvwprintw(win, rows/2 -1, cols/2 - PAUSE_WIDTH/2,"++++++++++++A WINNER  IS YOU++++++++++++");
    mvwprintw(win, rows/2 -0, cols/2 - PAUSE_WIDTH/2,"++YOU FILLED THE ENTIRE  %3dx%2d FIELD!++", game_cols, game_rows );
    mvwprintw(win, rows/2 +1, cols/2 - PAUSE_WIDTH/2,"++++++++PRESS ANY BUTTON TO EXIT++++++++");
    mvwprintw(win, rows/2 +2, cols/2 - PAUSE_WIDTH/2,"========================================");
    wrefresh(win);
    refresh();
    getch();
    nodelay(stdscr, TRUE);
}

static void redraw_snake(WINDOW* win, const std::vector<Direction> &grid,  const int game_rows,
                         const int game_cols, const int head_row, int head_col)
{
    wattron(win, COLOR_PAIR(BODY_PAIR));

    for (int i = 0; i < game_rows*game_cols; i++)
    {
        if (grid[i] != Direction::empt)
        {
            for (int j = 0; j < SCALING_FACT; j++)
                for (int k = 0; k < COLS_PER_ROW*SCALING_FACT; k++)
                    mvwaddch(win, (i/(game_cols))*SCALING_FACT+j, i%(game_cols)*COLS_PER_ROW*SCALING_FACT + k, SB);
        }
    }

    wattroff(win, COLOR_PAIR(BODY_PAIR));

    draw_square(win, HEAD_PAIR, head_row, head_col, SH);
}
