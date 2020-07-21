
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>

// GLOBAL VARIABLES
// PROGRAM ARGUMENTS
// M = ROWS, N = COLUMNS, X = MAX NUMBER OF SQUARES TO BE CONSIDERED A VALID DEAD END
int M, N, X;

// MAINTAINS THE MAXIMUM NUMBER OF SQUARES COVERED BY SONNY
int max_squares;
// MAINTAINS A LIST OF THE DEAD END BOARD CONFIGURATIONS
struct KnightsTour **deadBoards;
// MAINTAINS A COUNT OF THE NUMBER OF DEAD BOARDS WE CURRENTLY HAVE
int deadBoardsCount;
// MAINTAINS A CURRENT SIZE OF DEADBOARDS ARRAY
int deadBoardsSize;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// KNIGHTS TOUR STRUCT
struct KnightsTour
{
    int **board;
    int numMoves;
    int knight_x;
    int knight_y;
};

// INITIALIZES OUR KNIGHTS TOUR GAME
struct KnightsTour *initGame(int **arr, int numMoves, int knight_x, int knight_y)
{
    struct KnightsTour *knightstour = (struct KnightsTour *)malloc(sizeof(struct KnightsTour));
    knightstour->board = arr;
    knightstour->numMoves = numMoves;
    knightstour->knight_x = knight_x;
    knightstour->knight_y = knight_y;
    return knightstour;
}

// PRINTS THE CONTENTS OF A 2D ARRAY
void print2dArray(int **arr)
{
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            printf("%d ", arr[i][j]);
        }
        printf("\n");
    }
}

// FUNCTION TO PRINT DEAD END BOARDS AT THE END OF ASSIGNMENT
void printDeadEndBoards()
{
    // LOOP THROUGH ALL THE DEAD BOARDS AND PRINT THEM
    for (int i = 0; i < deadBoardsCount; i++)
    {
        for (int j = 0; j < M; j++)
        {
            printf("THREAD %ld: ", pthread_self());
            if (j == 0)
            {
                printf("> ");
            }
            else
            {
                printf("  ");
            }
            for (int z = 0; z < N; z++)
            {
                if (deadBoards[i]->board[j][z] > 0)
                {
                    printf("S");
                }
                else
                {
                    printf(".");
                }
            }
            printf("\n");
        }
    }
}

// DEEP COPYS AN ARRAY AND RETURNS IT
int **copyArray(int **arr)
{
    int **newBoard = calloc(M, sizeof(int *));
    for (int i = 0; i < M; i++)
    {
        newBoard[i] = calloc(N, sizeof(int));
        for (int j = 0; j < N; j++)
        {
            newBoard[i][j] = arr[i][j];
        }
    }
    return newBoard;
}

// FREES KNIGHTS TOUR STRUCT
void freeKnightsTour(struct KnightsTour *game)
{
    for (int i = 0; i < M; i++)
    {
        free(game->board[i]);
    }
    free(game->board);
    free(game);
}

// LOOPS THROUGH DEADBOARDS AND CALLS FREEKNIGHTSTOUR
void freeDeadBoards()
{
    for (int i = 0; i < deadBoardsCount; i++)
    {
        freeKnightsTour(deadBoards[i]);
    }
    free(deadBoards);
}

// DRIVER FUNCTION THAT SOLVES THE KNIGHTS TOUR PROBLEM
void *SolveTour(void *game)
{
    struct KnightsTour *game_ = (struct KnightsTour *)game;
    int *retVal = malloc(sizeof(int));
    int *t_result;
    int rc;

    if (game_->numMoves == 1)
    {
        printf("THREAD %ld: Solving Sonny's knight's tour problem for a %dx%d board\n", pthread_self(), M, N);
    }

    // ALL THE POSSIBLE MOVES A KNIGHT CAN MAKE
    int x_possibilities[] = {-2, -1, 1, 2, 2, 1, -1, -2};
    int y_possibilities[] = {-1, -2, -2, -1, 1, 2, 2, 1};

    // STORE A LIST OF THE INDEX WHERE THE X/Y MOVE IS VALID
    int validMoves[8];
    // KEEP TRACK OF THE LAST LOCATION OF WHERE YOU ADDED A MOVE
    int validMovesCount = 0;
    for (int i = 0; i < 8; i++)
    {
        // POTENTIAL X/Y LOCATION OF THE KNIGHT WHEN THE MOVE IS EXECUTED
        int potential_x = game_->knight_x + x_possibilities[i];
        int potential_y = game_->knight_y + y_possibilities[i];

        // IF THE MOVE IS NOT OFF THE BOARD OR VISITING A ALREADY VISITED SQUARE ADD THE MOVE TO VALID MOVES
        if (potential_x >= 0 && potential_y >= 0 && potential_x < N && potential_y < M && game_->board[potential_y][potential_x] == 0)
        {
            validMoves[validMovesCount] = i;
            validMovesCount += 1;
        }
    }

    // WE HAVE ENCOUNTERED A FINISHED BOARD OR A DEADEND BOARD
    if (validMovesCount == 0)
    {
        pthread_mutex_lock(&mutex);

        if (game_->numMoves > max_squares)
        {
            max_squares = game_->numMoves;
        }
        *retVal = game_->numMoves;
        pthread_mutex_unlock(&mutex);

        if (game_->numMoves != M * N)
        {
            printf("THREAD %ld: Dead end after move #%d\n", pthread_self(), game_->numMoves);
            if (game_->numMoves >= X)
            {
                pthread_mutex_lock(&mutex);

                if (deadBoardsCount >= deadBoardsSize)
                {
                    deadBoardsSize = deadBoardsSize * 2;
                    deadBoards = realloc(deadBoards, deadBoardsSize * sizeof(struct KnightsTour *));
                }
                deadBoards[deadBoardsCount] = game_;
                deadBoardsCount = deadBoardsCount + 1;
                pthread_mutex_unlock(&mutex);
            }
        }
        else
        {
            printf("THREAD %ld: Sonny found a full knight's tour!\n", pthread_self());
            max_squares = game_->numMoves;
            freeKnightsTour(game_);
        }
        pthread_exit(retVal);
    }

    else // VALID MOVES CAN STILL BE MADE
    {
        // RECURSE WITHOUT CREATING AN ADDITIONAL PROCESS
        if (validMovesCount == 1)
        {
            game_->knight_x += x_possibilities[validMoves[0]];                // get the x position
            game_->knight_y += y_possibilities[validMoves[0]];                // get the y position
            game_->numMoves += 1;                                             // increase the number of moves made
            game_->board[game_->knight_y][game_->knight_x] = game_->numMoves; // update the knights postion to be the move number
            free(retVal);
            SolveTour((void *)game_); // recurse
        }

        // RECURSE AND CREATE MULTIPLE THREADS DEPENDING ON THE AMOUNT OF VALID MOVES
        else
        {
            printf("THREAD %ld: %d moves possible after move #%d; creating threads...\n", pthread_self(), validMovesCount, game_->numMoves);
            pthread_t tid[validMovesCount];
            // FOR ALL OF THE VALID MOVES MAKE A NEW STRUCT
            for (int i = 0; i < validMovesCount; i++)
            {
                // CREATE A NEW BOARD AND COPY OLD ONE
                int **newBoard = copyArray(game_->board);
                // INITIALIZE A NEW GAME WITH THE NEW GAME INFO DERIVED FROM THE OLD GAME
                struct KnightsTour *newGame = initGame(newBoard, game_->numMoves + 1, game_->knight_x + x_possibilities[validMoves[i]], game_->knight_y + y_possibilities[validMoves[i]]);
                // SET THE NEW KNIGHT POSITION EQUAL TO THE NUMBER OF MOVES
                newGame->board[newGame->knight_y][newGame->knight_x] = newGame->numMoves;
                rc = pthread_create(&tid[i], NULL, SolveTour, (void *)newGame);
                if (rc != 0)
                {
                    fprintf(stderr, "MAIN: Could not create thread\n");
                    fprintf(stderr, "MAIN: rc is %d\n", rc);
                }
#ifdef NO_PARALLEL
                rc = pthread_join(tid[i], (void **)&(t_result));
                printf("THREAD %ld: Thread [%ld] joined (returned %d)\n", pthread_self(), tid[i], *t_result);
                if (*t_result > *retVal)
                {
                    *retVal = *t_result;
                }
                free(t_result);
#endif
            }
#ifndef NO_PARALLEL
            for (int i = 0; i < validMovesCount; i++)
            {
                rc = pthread_join(tid[i], (void **)&(t_result));
                printf("THREAD %ld: Thread [%ld] joined (returned %d)\n", pthread_self(), tid[i], *t_result);

                if (*t_result > *retVal)
                {
                    *retVal = *t_result;
                }
                free(t_result);
            }
#endif
            freeKnightsTour(game_);
            pthread_exit(retVal);
        }
    }
    return retVal;
}

// MAIN PROGRAM
int main(int argc, char *argv[])
{
    // CHECK AND MAKE SURE THE CORRECT NUMBER OF ARGUMENTS ARE PROVIDED.
    if ((argc != 3 && argc != 4) || (atoi(argv[1]) < 3 || atoi(argv[2]) < 3))
    {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<x>]\n");
        return EXIT_FAILURE;
    }
    if (argc == 4)
    {
        if ((atoi(argv[3]) < 1) || (atoi(argv[2]) * atoi(argv[1]) < atoi(argv[3])))
        {
            fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<x>]\n");
            return EXIT_FAILURE;
        }
    }

    // ALL INPUT HAS BEEN VALIDATED CONTINUE WITH THE PROGRAM

    // SET GLOBAL VARIABLE M AND N, ALLOCATE MEMORY TO HOLD 1 DEAD END BOARD, SET THE COUNT OF DEAD END BOARDS TO ZERO & SIZE OF DEAD END BOARDS TO 1
    M = atoi(*(argv + 1)); // number of rows
    N = atoi(*(argv + 2)); // number of columns
    X = 0;
    deadBoards = calloc(1, sizeof(struct KnightsTour *));
    deadBoardsCount = 0;
    deadBoardsSize = 1;

    // ARGUMENT TO DISPLAY BOARDS THAT HAVE AT LEAST X NUMBER OF SQUARES COVERED
    if (argc == 4)
    {
        X = atoi(*(argv + 3));
    }

    // ALLOCATE A BOARD
    int **arr = (int **)calloc(atoi(*(argv + 1)), sizeof(int *));
    for (int i = 0; i < atoi(*(argv + 1)); i++)
    {
        arr[i] = calloc(atoi(*(argv + 2)), sizeof(int));
    }

    // SET THE INITIAL KNIGHT POSITION
    arr[0][0] = 1;

    // DECLARE A STRUCT CALLED GAME (board, numMoves, knight x, knight y)
    struct KnightsTour *game = initGame(arr, 1, 0, 0);

    int *t_result;
    pthread_t tid;

    int rc = pthread_create(&tid, NULL, SolveTour, (void *)game);

    // ERROR THREAD WAS NOT CREATED
    if (rc != 0)
    {
        fprintf(stderr, "MAIN: Could not creat thread (%d)\n", rc);
    }

    // BLOCKING CALL WAITS FOR THREAD TO RETURN
    rc = pthread_join(tid, (void **)&(t_result));

    // THREAD DID NOT RETURN SUCCESSFULLY
    if (rc != 0)
    {
        fprintf(stderr, "MAIN: Could not join thread (%d)\n", rc);
    }

    printf("THREAD %ld: Best solution(s) found visit %d squares (out of %d)\n", tid, max_squares, M * N);
    printf("THREAD %ld: Dead end boards:\n", pthread_self());

    //PRINT SOLUTIONS
    printDeadEndBoards();

    free(t_result);

    // FREE BOARDS
    freeDeadBoards();

    return EXIT_SUCCESS;
}
