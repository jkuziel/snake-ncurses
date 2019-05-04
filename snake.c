/*
Snake in NCurses
Copyright (c) 2019 Joseph Kuziel

Table of Contents:
1. Includes
2. Helper Functions
3. Game State
  3.1 Data Structures
  3.2 Functions
4. Input
5. Rendition
6. Main

*/

// 1. Includes =================================================================

#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>

// 2. Helper Functions =========================================================

int min(int a, int b) {
  return a < b ? a : b;
}

int max(int a, int b) {
  return a > b ? a : b;
}

// 3. Game State ===============================================================

// 3.1 Data Structures ---------------------------------------------------------

const int Board_x       = 2;
const int Board_y       = 1;
#define Board_width     20
#define Board_height    20
#define Board_size      (Board_width * Board_height)

typedef int Cell;

typedef enum CellType {
  CellType_empty  = 0x00,
  CellType_apple  = 0x04,
  CellType_snake  = 0x08,
  CellType_mask   = 0x0C,
} CellType;

typedef enum CellDirection {
  CellDirection_up,
  CellDirection_left,
  CellDirection_down,
  CellDirection_right,
  CellDirection_count,
  CellDirection_mask = 0x03,
} CellDirection;

typedef enum GameStatus {
  GameStatus_exit,
  GameStatus_running,
  GameStatus_over,
} GameStatus;

typedef enum Input {
  Input_none,
  Input_up,
  Input_left,
  Input_down,
  Input_right,
  Input_quit,
} Input;

typedef struct GameState {
  GameStatus status;
  Cell board[Board_size];
  int head;
  int tail;
  int applesEaten;
  int snakeSpeed;
} GameState;

// 3.2 Functions (Game State) --------------------------------------------------

int cell(int x, int y) {
  return Board_width * y + x;
}

int cellX(int c) {
  return c % Board_width;
}

int cellY(int c) {
  return c / Board_width;
}

int cellDir(int c, CellDirection dir) {

  int x = cellX(c);
  int y = cellY(c);

  switch (dir) {
    case CellDirection_up:    y = max(y - 1, 0); break;
    case CellDirection_left:  x = max(x - 1, 0); break;
    case CellDirection_down:  y = min(y + 1, Board_height - 1); break;
    case CellDirection_right: x = min(x + 1, Board_width - 1); break;
    default: /* Do nothing */ break;
  }

  return cell(x, y);
}

CellDirection mapCellDirection(Input input) {
  switch (input) {
    case Input_up:    return CellDirection_up;
    case Input_down:  return CellDirection_down;
    case Input_left:  return CellDirection_left;
    case Input_right: return CellDirection_right;
    default:          return CellDirection_up;
  }
}

void GameState_spawnApple(GameState* gs) {

  int new_pos;
  do {
    new_pos = cell(rand() % Board_width, rand() % Board_height);
  } while(gs->board[new_pos] != CellType_empty);

  gs->board[new_pos] = CellType_apple;
}

void GameState_init(GameState* gs) {
    
  gs->status = GameStatus_running;
  memset(gs->board, CellType_empty, Board_size * sizeof(Cell));
  gs->head = cell(Board_width / 2, Board_height / 2);
  gs->tail = gs->head - 2;
  gs->applesEaten = 0;
  gs->snakeSpeed = 850;

  GameState_spawnApple(gs);
  
  for (int i = gs->tail; i <= gs->head; i++) {
    gs->board[i] = CellType_snake | CellDirection_right;
  }
}

GameState GameState_step(Input input, const GameState* const cur_gs) {

  // Make next state
  GameState new_gs;
  memcpy(&new_gs, cur_gs, sizeof(GameState));

  switch (cur_gs->status) {

    case GameStatus_exit:
      // Do nothing
      break;

    case GameStatus_over: {
      switch (input) {
        case Input_quit:
          new_gs.status = GameStatus_exit;
          break;
        case Input_none:
          // Do nothing.
          break;
        default:
          GameState_init(&new_gs);
          break;
      }
      break;
    }

    case GameStatus_running: {

      // Get direction of snake head
      CellDirection headDir = cur_gs->board[cur_gs->head] & CellDirection_mask;

      switch (input) {
        case Input_quit:
          new_gs.status = GameStatus_exit;
          return new_gs;
        case Input_none:
          // This is a full step
          break;
        default: {
          // This is a mid step
          // Get direction of input
          CellDirection userDir = mapCellDirection(input);

          // Filter out requested directions that are not perpendicular to the
          // direction of the snake head
          if ((headDir % 2) != (userDir % 2)) {
            headDir = userDir;
          } else {
            return new_gs;
          }
          break;
        }
      }

      // Set current head cell to new direction
      new_gs.board[cur_gs->head] = CellType_snake | headDir;

      // Put head to new cell
      int nextHeadCell = cellDir(cur_gs->head, headDir);
      int nextCellType = cur_gs->board[nextHeadCell] & CellType_mask;
      new_gs.board[nextHeadCell] = CellType_snake | headDir;
      new_gs.head = nextHeadCell;

      switch (nextCellType) {
        case CellType_snake:
          new_gs.status = GameStatus_over;
          break;
        case CellType_empty:
          new_gs.board[cur_gs->tail] = CellType_empty;
          CellDirection tailDir = cur_gs->board[cur_gs->tail] & CellDirection_mask;
          new_gs.tail = cellDir(cur_gs->tail, tailDir);
          break;
        case CellType_apple:
          new_gs.applesEaten++;
          new_gs.snakeSpeed = 850 + new_gs.applesEaten;
          GameState_spawnApple(&new_gs);
          break;
        default:
          // Do nothing
          break;
      }

      break;
    } // GameStatus_running
  }

  return new_gs;
}

// 4. Input ====================================================================

int getLastCh() {
  int result = ERR;
  int ch;
  while ((ch = getch()) != ERR) {
    result = ch;
  }
  return result;
}

Input mapInput(int ch) {
  switch (ch) {
    case KEY_UP:    return Input_up;
    case KEY_DOWN:  return Input_down;
    case KEY_LEFT:  return Input_left;
    case KEY_RIGHT: return Input_right;
    case 27:
    case 'q':       return Input_quit;
    default:        return Input_none;
  }
}

// 5. Rendition ================================================================

typedef enum ColorPair {
  ColorPair_bg    = 1,
  ColorPair_fg,
  ColorPair_snake,
  ColorPair_apple,
  ColorPair_alert
} ColorPair;

void init_draw() {

  start_color();
  init_pair(ColorPair_bg, COLOR_WHITE, COLOR_BLACK);
  init_pair(ColorPair_fg, COLOR_BLACK, COLOR_WHITE);
  init_pair(ColorPair_snake, COLOR_BLACK, COLOR_GREEN);
  init_pair(ColorPair_apple, COLOR_RED, COLOR_RED);
  init_pair(ColorPair_alert, COLOR_WHITE, COLOR_RED);

  int x1 = Board_x - 1;
  int x2 = x1 + Board_width * 2 + 1;
  int y1 = Board_y - 1;
  int y2 = y2 + Board_height + 1;

  attron(COLOR_PAIR(ColorPair_bg));

  attron(A_DIM);
  move(y1 + 1, x1); vline('|', y2 - 1);
  move(y1, x1 + 1); hline('-', x2 - 2);
  move(y1 + 1, x2); vline('|', y2 - 1);
  move(y2, x1 + 1); hline('-', x2 - 2);
  attroff(A_DIM);

  attron(A_UNDERLINE);
  mvprintw(y2 - 6, x2 + 2, "Move Snake");
  mvprintw(y2 - 2, x2 + 2, "Quit");
  attroff(A_UNDERLINE);

  mvprintw(y2 - 5, x2 + 2, "Arrow Keys");
  mvprintw(y2 - 1, x2 + 2, "q or ESC");

  attroff(COLOR_PAIR(ColorPair_bg));

  attron(COLOR_PAIR(ColorPair_fg) | A_BOLD);
  mvprintw(Board_y, Board_x + Board_width * 2 + 2, "SCORE   ");
  attroff(COLOR_PAIR(ColorPair_fg) | A_BOLD);
}

void draw(const GameState* const gs) {

  attron(COLOR_PAIR(ColorPair_bg));

  for (int y = 0; y < Board_height; y++) {

    move(y + Board_y, Board_x);

    for (int x = 0; x < Board_width; x++) {

      Cell value = gs->board[cell(x, y)];
      switch (value & CellType_mask) {
        case CellType_empty: printw("  "); break;
        case CellType_apple:
          attron(COLOR_PAIR(ColorPair_apple));
          printw("  ");
          attron(COLOR_PAIR(ColorPair_bg));
          break;
        case CellType_snake:
          attron(COLOR_PAIR(ColorPair_snake));
          if (gs->head == cell(x, y)) {
            switch (value & CellDirection_mask) {
              case CellDirection_up:    printw("''"); break;
              case CellDirection_left:  printw(": "); break;
              case CellDirection_down:  printw(".."); break;
              case CellDirection_right: printw(" :"); break;
              default: /* Do nothing */ break;
            }            
          } else {
            printw("  ");
          }
          attron(COLOR_PAIR(ColorPair_bg));
          break;
      }
    }
  }

  attron(A_BOLD);
  mvprintw(Board_y + 1, Board_x + Board_width * 2 + 2, "%08d", gs->applesEaten * 100);
  attroff(A_BOLD);

  attroff(COLOR_PAIR(ColorPair_bg));
  
  if (gs->status == GameStatus_over) {
    attron(COLOR_PAIR(ColorPair_alert) | A_BOLD | A_BLINK);
    mvprintw(Board_y + Board_height / 2, Board_x + Board_width - 5, "GAME OVER!");
    attroff(COLOR_PAIR(ColorPair_alert) | A_BOLD | A_BLINK);
  }
}

// 6. Main =====================================================================

int main(int argc, char* argv[]) {

  srand(time(0));

  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);

  clear();
  curs_set(0);

  init_draw();

  // Setup game state
  GameState gs;
  GameState_init(&gs);

  // Main loop
  int clock = 1000;
  int frame = 0;
  int didStep = 0;
  Input input;

  while(gs.status != GameStatus_exit) {

    input = mapInput(getLastCh());
    
    if (frame == 0 || input != Input_none) {
      gs = GameState_step(frame == 0 ? Input_none : input, &gs);
      draw(&gs);
      refresh();
    }

    if (input != Input_none) {
      didStep = 1;
    }

    usleep(1000000 / clock);

    if ((frame = ((frame + 1) % max(clock - gs.snakeSpeed, 1))) == 0) {
      didStep = 0;
    }
  }

  endwin();

  return 0;
}