/*******************************************************************************************
*
*   raylib - classic game: tetris
*
*   Sample game developed by Marc Palau and Ramon Santamaria
*
*   This game has been created using raylib v1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
// #define SQUARE_SIZE             20

#define GRID_HORIZONTAL_SIZE    12
#define GRID_VERTICAL_SIZE      20

#define LATERAL_SPEED           10
#define TURNING_SPEED           12
#define FAST_FALL_AWAIT_COUNTER 30

#define FADING_TIME             33

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum GridSquare { EMPTY, MOVING, FULL, BLOCK, FADING } GridSquare;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static int screenWidth = 1920;
static int screenHeight = 1080;
// static int screenWidth = 400;
// static int screenHeight = 225;

static int SQUARE_SIZE;
static int MAX_PLAYERS = 2;
static int Gr = 0;
static int masterOffsetX = 0;
static int masterOffsetY = 0;

static bool gameOver [4] = {false, false, false, false};
static bool pause = false;

// Matrices
static GridSquare grid [4][GRID_HORIZONTAL_SIZE][GRID_VERTICAL_SIZE];
static GridSquare piece [4][4][4];
static GridSquare incomingPiece [4][4][4];

// Theese variables keep track of the active piece position
static int piecePositionX[4] = {0, 0, 0, 0};
static int piecePositionY[4] = {0, 0, 0, 0};

// Game parameters
static Color fadingColor[4];
//static int fallingSpeed;           // In frames

static bool beginPlay [4] = {true, true, true, true};      // This var is only true at the begining of the game, used for the first matrix creations
static bool pieceActive [4] = {false, false, false, false};
static bool detection [4] = {false, false, false, false};
static bool lineToDelete [4] = {false, false, false, false};

// Statistics
static int level[4] = {1, 1, 1, 1};
static int lines[4] = {0, 0, 0, 0};

// Counters
static int gravityMovementCounter [4] = {0, 0, 0, 0};
static int lateralMovementCounter [4] = {0, 0, 0, 0};
static int turnMovementCounter [4] = {0, 0, 0, 0};
static int fastFallMovementCounter [4] = {0, 0, 0, 0};

static int fadeLineCounter [4] = {0, 0, 0, 0};

// Based on level
static int gravitySpeed = 30;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(Color C1, Color C2, Color C3);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

// Additional module functions
static bool Createpiece();
static void GetRandompiece();
static void ResolveFallingMovement(bool *detection, bool *pieceActive, int Gr);
static bool ResolveLateralMovement();
static bool ResolveTurnMovement();
static void CheckDetection(bool *detection, int Gr);
static void CheckCompletion(bool *lineToDelete, int Gr);
static int DeleteCompleteLines();

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "classic game: tetris");

    for (int p = 0; p < MAX_PLAYERS; p++)
    {
        Gr = p;
        InitGame();
    }
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
#endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame();         // Unload loaded data (textures, sounds, models...)

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------
// Game Module Functions Definition
//--------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Initialize game statistics
    level[Gr] = 1;
    lines[Gr] = 0;

    SQUARE_SIZE = screenWidth / 40;

    fadingColor[Gr] = GRAY;

    piecePositionX[Gr] = 0;
    piecePositionY[Gr] = 0;

    pause = false;

    beginPlay[Gr] = true;
    pieceActive[Gr] = false;
    detection[Gr] = false;
    lineToDelete[Gr] = false;

    // Counters
    gravityMovementCounter[Gr] = 0;
    lateralMovementCounter[Gr] = 0;
    turnMovementCounter[Gr] = 0;
    fastFallMovementCounter[Gr] = 0;

    fadeLineCounter[Gr] = 0;
    gravitySpeed = 30;

    // Initialize grid matrices
    for (int i = 0; i < GRID_HORIZONTAL_SIZE; i++)
    {
        for (int j = 0; j < GRID_VERTICAL_SIZE; j++)
        {
            if ((j == GRID_VERTICAL_SIZE - 1) || (i == 0) || (i == GRID_HORIZONTAL_SIZE - 1)) grid[Gr][i][j] = BLOCK;
            else grid[Gr][i][j] = EMPTY;
        }
    }

    // Initialize incoming piece matrices
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j< 4; j++)
        {
            incomingPiece[Gr][i][j] = EMPTY;
        }
    }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver[Gr])
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (!pause)
        {
            if (!lineToDelete[Gr])
            {
                if (!pieceActive[Gr])
                {
                    // Get another piece
                    pieceActive[Gr] = Createpiece();

                    // We leave a little time before starting the fast falling down
                    fastFallMovementCounter[Gr] = 0;
                }
                else    // Piece falling
                {
                    // Counters update
                    fastFallMovementCounter[Gr]++;
                    gravityMovementCounter[Gr]++;
                    lateralMovementCounter[Gr]++;
                    turnMovementCounter[Gr]++;

                    if (Gr == 1)
                    {
                        // We make sure to move if we've pressed the key this frame
                        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) lateralMovementCounter[Gr] = LATERAL_SPEED;
                        if (IsKeyPressed(KEY_UP)) turnMovementCounter[Gr] = TURNING_SPEED;

                        // Fall down
                        if (IsKeyDown(KEY_DOWN) && (fastFallMovementCounter[Gr] >= FAST_FALL_AWAIT_COUNTER))
                        {
                            // We make sure the piece is going to fall this frame
                            gravityMovementCounter[Gr] += gravitySpeed;
                        }
                    }

                    if (Gr == 0)
                    {
                        // We make sure to move if we've pressed the key this frame
                        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D)) lateralMovementCounter[Gr] = LATERAL_SPEED;
                        if (IsKeyPressed(KEY_W)) turnMovementCounter[Gr] = TURNING_SPEED;

                        // Fall down
                        if (IsKeyDown(KEY_S) && (fastFallMovementCounter[Gr] >= FAST_FALL_AWAIT_COUNTER))
                        {
                            // We make sure the piece is going to fall this frame
                            gravityMovementCounter[Gr] += gravitySpeed;
                        }
                    }

                    if (gravityMovementCounter[Gr] >= gravitySpeed)
                    {
                        // Basic falling movement
                        CheckDetection(&detection[0], Gr);

                        // Check if the piece has collided with another piece or with the boundings
                        ResolveFallingMovement(&detection[0], &pieceActive[0], Gr);

                        // Check if we fullfilled a line and if so, erase the line and pull down the the lines[Gr] above
                        CheckCompletion(&lineToDelete[0], Gr);

                        gravityMovementCounter[Gr] = 0;
                    }

                    // Move laterally at player's will
                    if (lateralMovementCounter[Gr] >= LATERAL_SPEED)
                    {
                        // Update the lateral movement and if success, reset the lateral counter
                        if (!ResolveLateralMovement()) lateralMovementCounter[Gr] = 0;
                    }

                    // Turn the piece at player's will
                    if (turnMovementCounter[Gr] >= TURNING_SPEED)
                    {
                        // Update the turning movement and reset the turning counter
                        if (ResolveTurnMovement()) turnMovementCounter[Gr] = 0;
                    }
                }

                // Game over logic
                for (int j = 0; j < 2; j++)
                {
                    for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
                    {
                        if (grid[Gr][i][j] == FULL)
                        {
                            gameOver[Gr] = true;
                        }
                    }
                }
            }
            else
            {
                // Animation when deleting lines[Gr]
                fadeLineCounter[Gr]++;

                if (fadeLineCounter[Gr]%8 < 4) fadingColor[Gr] = MAROON;
                else fadingColor[Gr] = GRAY;

                if (fadeLineCounter[Gr] >= FADING_TIME)
                {
                    int deletedLines = 0;
                    deletedLines = DeleteCompleteLines();
                    fadeLineCounter[Gr] = 0;
                    lineToDelete[Gr] = false;

                    lines[Gr] += deletedLines;
                }
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            gameOver[Gr] = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(Color C1, Color C2, Color C3)
{
        if (!gameOver[Gr])
        {
            // Draw gameplay area
            Vector2 offset;
            offset.x = screenWidth/2 - (GRID_HORIZONTAL_SIZE*SQUARE_SIZE/2) - 50 + masterOffsetX;
            offset.y = screenHeight/2 - ((GRID_VERTICAL_SIZE - 1)*SQUARE_SIZE/2) + SQUARE_SIZE*2;

            offset.y -= 2*SQUARE_SIZE;

            int controller = offset.x;

            for (int j = 0; j < GRID_VERTICAL_SIZE; j++)
            {
                for (int i = 0; i < GRID_HORIZONTAL_SIZE; i++)
                {
                    // Draw each square of the grid
                    if (grid[Gr][i][j] == EMPTY)
                    {
                        DrawLine(offset.x, offset.y, offset.x + SQUARE_SIZE, offset.y, C1 );
                        DrawLine(offset.x, offset.y, offset.x, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x + SQUARE_SIZE, offset.y, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x, offset.y + SQUARE_SIZE, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == FULL)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C2);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == MOVING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C3);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == BLOCK)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C1);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == FADING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, fadingColor[Gr]);
                        offset.x += SQUARE_SIZE;
                    }
                }

                offset.x = controller;
                offset.y += SQUARE_SIZE;
            }

            // Draw incoming piece (semi hardcoded)
//            offset.x = screenWidth / 2 + 4 * SQUARE_SIZE;
            offset.x = screenWidth/2 + (GRID_HORIZONTAL_SIZE*SQUARE_SIZE/2) + masterOffsetX;
            offset.y = 4 * SQUARE_SIZE;

            int controler = offset.x;

            for (int j = 0; j < 4; j++)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (incomingPiece[Gr][i][j] == EMPTY)
                    {
                        DrawLine(offset.x, offset.y, offset.x + SQUARE_SIZE, offset.y, C1 );
                        DrawLine(offset.x, offset.y, offset.x, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x + SQUARE_SIZE, offset.y, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x, offset.y + SQUARE_SIZE, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        offset.x += SQUARE_SIZE;
                    }
                    else if (incomingPiece[Gr][i][j] == MOVING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C2);
                        offset.x += SQUARE_SIZE;
                    }
                }

                offset.x = controler;
                offset.y += SQUARE_SIZE;
            }

            DrawText("INCOMING:", offset.x, offset.y - 5*SQUARE_SIZE, SQUARE_SIZE/2, GRAY);
            DrawText(TextFormat("LINES:   %04i", lines[Gr]), offset.x, offset.y + 20, SQUARE_SIZE/2, GRAY);

            if (pause) DrawText("GAME PAUSED", screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, screenHeight/2 - 40, 40, GRAY);
        }
        else DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth()/2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20)/2, GetScreenHeight()/2 - 50, 20, GRAY);

}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    Gr = 0;
    UpdateGame();
    Gr++;
    UpdateGame();
    Gr = 0;

        BeginDrawing();

        ClearBackground(RAYWHITE);

        masterOffsetX = -580;
    DrawGame(SKYBLUE, BLUE, DARKBLUE);
    Gr++;
            masterOffsetX = 400;

    DrawGame(PURPLE, VIOLET, DARKPURPLE);


//     DrawGame(BEIGE, BROWN, DARKBROWN, 400);
//     DrawGame(GREEN, LIME, DARKGREEN, 400);
//       DrawGame(LIGHTGRAY, GRAY, DARKGRAY, 400);

        EndDrawing();

}

//--------------------------------------------------------------------------------------
// Additional module functions
//--------------------------------------------------------------------------------------
static bool Createpiece()
{
    piecePositionX[Gr] = (int)((GRID_HORIZONTAL_SIZE - 4)/2);
    piecePositionY[Gr] = 0;

    // If the game is starting and you are going to create the first piece, we create an extra one
    if (beginPlay)
    {
        GetRandompiece();
        beginPlay[Gr] = false;
    }

    // We assign the incoming piece to the actual piece
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j< 4; j++)
        {
            piece[Gr][i][j] = incomingPiece[Gr][i][j];
        }
    }

    // We assign a random piece to the incoming one
    GetRandompiece();

    // Assign the piece to the grid
    for (int i = piecePositionX[Gr]; i < piecePositionX[Gr] + 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (piece[Gr][i - (int)piecePositionX[Gr]][j] == MOVING) grid[Gr][i][j] = MOVING;
        }
    }

    return true;
}

static void GetRandompiece()
{
    int random = GetRandomValue(0, 6);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            incomingPiece[Gr][i][j] = EMPTY;
        }
    }

    switch (random)
    {
        case 0: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; } break;    //Cube
        case 1: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; } break;    //L
        case 2: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][0] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; } break;    //L inversa
        case 3: { incomingPiece[Gr][0][1] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][3][1] = MOVING; } break;    //Recta
        case 4: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][1] = MOVING; } break;    //Creu tallada
        case 5: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][2] = MOVING; } break;    //S
        case 6: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][3][1] = MOVING; } break;    //S inversa
    }
}

static void ResolveFallingMovement(bool *detection, bool *pieceActive, int Gr)
{
    // If we finished moving this piece, we stop it
    if (*(detection + Gr))
    {
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    grid[Gr][i][j] = FULL;
                    *(detection + Gr) = false;
                    *(pieceActive +Gr) = false;
                }
            }
        }
    }
    else    // We move down the piece
    {
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    grid[Gr][i][j+1] = MOVING;
                    grid[Gr][i][j] = EMPTY;
                }
            }
        }

        piecePositionY[Gr]++;
    }
}

static bool ResolveLateralMovement()
{
    bool collision = false;

    // Piece movement
    if ((IsKeyDown(KEY_LEFT) && Gr ==1)
        || (IsKeyDown(KEY_A) && Gr ==0)
    ) // Move left
    {
        // Check if is possible to move to left
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    // Check if we are touching the left wall or we have a full square at the left
                    if ((i-1 == 0) || (grid[Gr][i-1][j] == FULL)) collision = true;
                }
            }
        }

        // If able, move left
        if (!collision)
        {
            for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
            {
                for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)             // We check the matrix from left to right
                {
                    // Move everything to the left
                    if (grid[Gr][i][j] == MOVING)
                    {
                        grid[Gr][i-1][j] = MOVING;
                        grid[Gr][i][j] = EMPTY;
                    }
                }
            }

            piecePositionX[Gr]--;
        }
    }
    else if ((IsKeyDown(KEY_RIGHT) && Gr ==1)
            || (IsKeyDown(KEY_D) && Gr ==0)
    )  // Move right
    {
        // Check if is possible to move to right
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    // Check if we are touching the right wall or we have a full square at the right
                    if ((i+1 == GRID_HORIZONTAL_SIZE - 1) || (grid[Gr][i+1][j] == FULL))
                    {
                        collision = true;

                    }
                }
            }
        }

        // If able move right
        if (!collision)
        {
            for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
            {
                for (int i = GRID_HORIZONTAL_SIZE - 1; i >= 1; i--)             // We check the matrix from right to left
                {
                    // Move everything to the right
                    if (grid[Gr][i][j] == MOVING)
                    {
                        grid[Gr][i+1][j] = MOVING;
                        grid[Gr][i][j] = EMPTY;
                    }
                }
            }

            piecePositionX[Gr]++;
        }
    }

    return collision;
}

static bool ResolveTurnMovement()
{
    // Input for turning the piece
    if ( (IsKeyDown(KEY_UP) && Gr ==1)
       || (IsKeyDown(KEY_W) && Gr ==0)
    )
    {
        GridSquare aux;
        bool checker = false;

        // Check all turning possibilities
        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if (!checker)
        {
            aux = piece[Gr][0][0];
            piece[Gr][0][0] = piece[Gr][3][0];
            piece[Gr][3][0] = piece[Gr][3][3];
            piece[Gr][3][3] = piece[Gr][0][3];
            piece[Gr][0][3] = aux;

            aux = piece[Gr][1][0];
            piece[Gr][1][0] = piece[Gr][3][1];
            piece[Gr][3][1] = piece[Gr][2][3];
            piece[Gr][2][3] = piece[Gr][0][2];
            piece[Gr][0][2] = aux;

            aux = piece[Gr][2][0];
            piece[Gr][2][0] = piece[Gr][3][2];
            piece[Gr][3][2] = piece[Gr][1][3];
            piece[Gr][1][3] = piece[Gr][0][1];
            piece[Gr][0][1] = aux;

            aux = piece[Gr][1][1];
            piece[Gr][1][1] = piece[Gr][2][1];
            piece[Gr][2][1] = piece[Gr][2][2];
            piece[Gr][2][2] = piece[Gr][1][2];
            piece[Gr][1][2] = aux;
        }

        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    grid[Gr][i][j] = EMPTY;
                }
            }
        }

        for (int i = piecePositionX[Gr]; i < piecePositionX[Gr] + 4; i++)
        {
            for (int j = piecePositionY[Gr]; j < piecePositionY[Gr] + 4; j++)
            {
                if (piece[Gr][i - piecePositionX[Gr]][j - piecePositionY[Gr]] == MOVING)
                {
                    grid[Gr][i][j] = MOVING;
                }
            }
        }

        return true;
    }

    return false;
}

static void CheckDetection(bool *detection, int Gr)
{
    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
        {
            if ((grid[Gr][i][j] == MOVING) && ((grid[Gr][i][j+1] == FULL) || (grid[Gr][i][j+1] == BLOCK))) *(detection + Gr) = true;
        }
    }
}

static void CheckCompletion(bool *lineToDelete, int Gr)
{
    int calculator = 0;

    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        calculator = 0;
        for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
        {
            // Count each square of the line
            if (grid[Gr][i][j] == FULL)
            {
                calculator++;
            }

            // Check if we completed the whole line
            if (calculator == GRID_HORIZONTAL_SIZE - 2)
            {
                *(lineToDelete + Gr) = true;
                calculator = 0;
                // points++;

                // Mark the completed line
                for (int z = 1; z < GRID_HORIZONTAL_SIZE - 1; z++)
                {
                    grid[Gr][z][j] = FADING;
                }
            }
        }
    }
}

static int DeleteCompleteLines()
{
    int deletedLines = 0;

    // Erase the completed line
    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        while (grid[Gr][1][j] == FADING)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                grid[Gr][i][j] = EMPTY;
            }

            for (int j2 = j-1; j2 >= 0; j2--)
            {
                for (int i2 = 1; i2 < GRID_HORIZONTAL_SIZE - 1; i2++)
                {
                    if (grid[Gr][i2][j2] == FULL)
                    {
                        grid[Gr][i2][j2+1] = FULL;
                        grid[Gr][i2][j2] = EMPTY;
                    }
                    else if (grid[Gr][i2][j2] == FADING)
                    {
                        grid[Gr][i2][j2+1] = FADING;
                        grid[Gr][i2][j2] = EMPTY;
                    }
                }
            }

             deletedLines++;
        }
    }

    return deletedLines;
}
