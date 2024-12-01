#include <stdlib.h>
#include "pd_api.h"

#define CELL_SIZE 8
#define GRID_WIDTH (LCD_COLUMNS / CELL_SIZE)
#define GRID_HEIGHT (LCD_ROWS / CELL_SIZE)
#define WALL_THICKNESS 2

typedef struct {
    int x;
    int y;
} Point;

static PlaydateAPI* pd = NULL;
static int* maze = NULL;

// directions: north, east, south, west
static const int dx[] = {0, 1, 0, -1};
static const int dy[] = {-1, 0, 1, 0};

static inline int isValid(int x, int y) {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

static inline int getCell(int x, int y) {
    return maze[y * GRID_WIDTH + x];
}

static inline void setCell(int x, int y, int value) {
    maze[y * GRID_WIDTH + x] = value;
}

static void generateMaze(int x, int y) {
    setCell(x, y, 1);  // mark as visited

    // create array of directions and shuffle them
    int dirs[4] = {0, 1, 2, 3};
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = dirs[i];
        dirs[i] = dirs[j];
        dirs[j] = temp;
    }

    // try each direction
    for (int i = 0; i < 4; i++) {
        int dir = dirs[i];
        int nx = x + dx[dir] * 2;  // move two cells in direction
        int ny = y + dy[dir] * 2;

        if (isValid(nx, ny) && !getCell(nx, ny)) {
            // mark the cell between as visited (remove wall)
            setCell(x + dx[dir], y + dy[dir], 1);
            generateMaze(nx, ny);
        }
    }
}

static void drawMaze(void) {
    pd->graphics->clear(kColorWhite);
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!getCell(x, y)) {
                pd->graphics->fillRect(
                    x * CELL_SIZE, 
                    y * CELL_SIZE, 
                    CELL_SIZE, 
                    CELL_SIZE, 
                    kColorBlack
                );
            }
        }
    }
    
    pd->graphics->markUpdatedRows(0, LCD_ROWS-1);
}

static void initMaze(void) {
    // allocate maze grid
    maze = (int*)malloc(GRID_WIDTH * GRID_HEIGHT * sizeof(int));
    if (!maze) {
        pd->system->logToConsole("Failed to allocate maze memory");
        return;
    }

    // initialize all cells as walls
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        maze[i] = 0;
    }

    // generate maze starting from top-left
    generateMaze(0, 0);
    drawMaze();
}

static void cleanup(void) {
    if (maze) {
        free(maze);
        maze = NULL;
    }
}

static int update(void* ud) {
    PDButtons pushed, held, released;
    pd->system->getButtonState(&held, &pushed, &released);
    
    if (pushed & kButtonA) { // generate new maze
        for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
            maze[i] = 0;
        }
        generateMaze(0, 0);
        drawMaze();
    }
    
    if (pushed & kButtonB) {
        return 1; 
    }

    return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
    if (event == kEventInit) {
        pd = playdate;
        pd->display->setRefreshRate(20);
        pd->system->setUpdateCallback(update, NULL);
        
        srand(pd->system->getCurrentTimeMilliseconds());
        initMaze();
    } else if (event == kEventTerminate) {
        cleanup();
    }
    
    return 0;
}