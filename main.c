#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_SIZE 8
#define MAX_MOVES 40
#define UPPER 32

#define ENPASSANTER -1
#define CAN_CASTLE 1

typedef enum Type{
    Pawn = 0,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
} Type;
typedef struct move{
    int rank;
    int file;
    int flag;
} move;
typedef struct piece{
    Type type;
    char rep;
    int owner;
    int flag;
    move* (*getPossibleMoves)(int rank, int file, struct piece*** board, int owner, int* cnt);
} piece;



// Function prototypes
//{
piece ***makeBoard();
void printBoard(piece ***board);
void freeBoard(piece ***board);
void printLine();
void readyBoard(piece ***board);
void addPiece(piece temp, piece ***board, int rank, int file, int owner);
void movePiece(piece ***board, int curRank, int curFile, int targetRank, int targetFile);
void addPossibleMove(move *moves, int *len, int rank, int file, int flag);
move *getPawnMoves(int rank, int file, piece ***board, int owner, int *cnt);
int isValidTile(int rank, int file);
int isPossibleMove(int rank, int file, move *moves, int cnt, int *flag);
move getMoveInput();
void promotePawn(piece ***board, int rank, int file);
void checkEnPassant(piece ***board, int rank, int file);
//}

// Pieces
const piece pieceTypes[] = {
        { Pawn, 'P', 0, 0, &getPawnMoves },
        { Knight, 'N', 0, 0, 0},
        { Bishop, 'B', 0, 0, 0},
        { Rook, 'R', 0, 0, 0},
        { Queen, 'Q', 0, 0, 0},
        { King, 'K', 0, 1, 0}
};
int turn = 1;

int main()
{

    piece ***board = makeBoard();
    readyBoard(board);


    while(1){
        printBoard(board);
        // Get and validate input
        printf("Select a piece to move\n");
        move cur = getMoveInput();


        // Check if piece is valid
        if(isValidTile(cur.rank, cur.file) && board[cur.rank][cur.file] != NULL && board[cur.rank][cur.file]->owner != turn % 2){
            printf("Piece selected: %c\n", board[cur.rank][cur.file]->rep);

            // Get input and validate
            printf("Where would you like to move this piece to?\n");
            move tar = getMoveInput();

            // Check if move is possible
            int cnt, flag;
            move *moves = board[cur.rank][cur.file]->getPossibleMoves(cur.rank, cur.file, board, board[cur.rank][cur.file]->owner, &cnt);
            if(isPossibleMove(tar.rank, tar.file, moves, cnt, &flag)){
                board[cur.rank][cur.file]->flag = flag;
                movePiece(board, cur.rank, cur.file, tar.rank, tar.file);
                turn++;
            } else {
                printf("Invalid move.\n");
            }
            free(moves);
        } else if(isValidTile(cur.rank, cur.file) && board[cur.rank][cur.file] != NULL && board[cur.rank][cur.file]->owner == turn % 2){
            printf("You don't own that piece!\n");
        } else {
            printf("Piece not found.\n");
        }
    }
    freeBoard(board);
    return 0;
}

//{ Piece movement
// Adds a piece to the board. Does not check for valid moves or piece overlapping
void addPiece(piece temp, piece ***board, int rank, int file, int owner){
    if(board[rank][file] != NULL){
        free(board[rank][file]);
    }
    piece *newPiece = malloc(sizeof(piece));
    memcpy(newPiece, &temp, sizeof(temp));
    newPiece->owner = owner;
    board[rank][file] = newPiece;
}
// Moves a piece on the board. Does not check for valid moves or piece overlapping.
void movePiece(piece ***board, int curRank, int curFile, int targetRank, int targetFile){
    if(board[targetRank][targetFile] != NULL){
        printf("Captured enemy %c.\n", board[targetRank][targetFile]->rep);
        free(board[targetRank][targetFile]);
    }
    board[targetRank][targetFile] = board[curRank][curFile];
    board[curRank][curFile] = NULL;

    checkEnPassant(board, targetRank, targetFile);
    promotePawn(board, targetRank, targetFile);
}
// Promotes to piece to a queen if it is an eligible pawn
void promotePawn(piece ***board, int rank, int file){
    int owner = board[rank][file]->owner;
    if(board[rank][file]->type == Pawn && ((owner == 0 && rank == 0) || (owner == 1 && rank == BOARD_SIZE - 1))){
        addPiece(pieceTypes[Queen], board, rank, file, owner);
        printf("Pawn promoted to Queen.\n");
    }
}
// Checks for an en passant
void checkEnPassant(piece ***board, int rank, int file){
    int owner = board[rank][file]->owner;
    int dir = (owner == 1) ? -1 : 1;
    if(board[rank][file]->type == Pawn && board[rank][file]->flag == ENPASSANTER){
        printf("Captured enemy %c by EN PASSANT!!\n", board[rank + dir][file]->rep);
        free(board[rank + dir][file]);
        board[rank + dir][file] = NULL;
    }
}
//}

//{ Move validation
// Returns all possible moves for a pawn to make (and number of possible moves)
move *getPawnMoves(int rank, int file, piece ***board, int owner, int *cnt){
    move *possibleMoves = malloc(MAX_MOVES * sizeof(move));
    *cnt = 0;
    int dir = (owner == 1) ? 1 : -1;

    // Forward move
    if(board[rank + dir][file] == NULL){
        addPossibleMove(possibleMoves, cnt, rank + dir, file, 0);
        // 2-space first move
        if(((owner == 1 && rank == 1) || (owner == 0 && rank == BOARD_SIZE - 2)) && board[rank + (2 * dir)][file] == NULL){
            addPossibleMove(possibleMoves, cnt, rank + (2 * dir), file, turn);
        }
    }
    // Capture right
    if(board[rank + dir][file + 1] != NULL){
        addPossibleMove(possibleMoves, cnt, rank + dir, file + 1, 0);
    }
    // Capture left
    if(board[rank + dir][file - 1] != NULL){
        addPossibleMove(possibleMoves, cnt, rank + dir, file - 1, 0);
    }
    // EN PASSANT RIGHT
    if(isValidTile(rank, file + 1) && board[rank][file + 1] != NULL && board[rank][file + 1]->type == Pawn && turn - board[rank][file + 1]->flag == 1){
        addPossibleMove(possibleMoves, cnt, rank + dir, file + 1, ENPASSANTER);
    }
    // EN PASSANT LEFT
    if(isValidTile(rank, file - 1) && board[rank][file - 1] != NULL && board[rank][file - 1]->type == Pawn && turn - board[rank][file - 1]->flag == 1){
        addPossibleMove(possibleMoves, cnt, rank + dir, file - 1, ENPASSANTER);
    }
    return possibleMoves;
}
// Adds move to a list of possible moves if target tile is on the board
void addPossibleMove(move *moves, int *len, int rank, int file, int flag){
    if(isValidTile(rank, file)){
        moves[*len] = (move) { rank, file, flag };
        *len += 1;
    }
}
// Checks if a given tile is on the board
int isValidTile(int rank, int file){
    return ((rank < BOARD_SIZE && rank >= 0) && (file < BOARD_SIZE && file >= 0));
}
// Checks if a move is in a list of possible moves
int isPossibleMove(int rank, int file, move *moves, int cnt, int* flag){
    if(!isValidTile(rank, file)){
        return 0;
    }
    for(int i = 0; i < cnt; i++){
        if(moves[i].file == file && moves[i].rank == rank){
            *flag = moves[i].flag;
            return 1;
        }
    }
    return 0;
}
//}

//{ Initialization
// Generates an empty board
piece ***makeBoard(){
    piece ***board = malloc(BOARD_SIZE * sizeof(piece**));
    for(int i = 0; i < BOARD_SIZE; i++){
        board[i] = calloc(BOARD_SIZE, sizeof(piece*));
    }
    return board;
}
// Arranges pieces on board to the starting position of chess
void readyBoard( piece ***board){
    // Generate white pieces
    for(int i = 0; i < BOARD_SIZE; i++){
        addPiece(pieceTypes[Pawn], board, BOARD_SIZE - 2, i, 0);
    }
    addPiece(pieceTypes[Rook], board, BOARD_SIZE - 1, 0, 0);
    addPiece(pieceTypes[Knight], board, BOARD_SIZE - 1, 1, 0);
    addPiece(pieceTypes[Bishop], board, BOARD_SIZE - 1, 2, 0);
    addPiece(pieceTypes[Queen], board, BOARD_SIZE - 1, 3, 0);
    addPiece(pieceTypes[King], board, BOARD_SIZE - 1, 4, 0);
    addPiece(pieceTypes[Bishop], board, BOARD_SIZE - 1, 5, 0);
    addPiece(pieceTypes[Knight], board, BOARD_SIZE - 1, 6, 0);
    addPiece(pieceTypes[Rook], board, BOARD_SIZE - 1, 7, 0);

    // Generate black pieces
    for(int i = 0; i < BOARD_SIZE; i++){
        addPiece(pieceTypes[Pawn], board, 1, i, 1);
    }
    addPiece(pieceTypes[Rook], board, 0, 0, 1);
    addPiece(pieceTypes[Knight], board, 0, 1, 1);
    addPiece(pieceTypes[Bishop], board, 0, 2, 1);
    addPiece(pieceTypes[King], board, 0, 3, 1);
    addPiece(pieceTypes[Queen], board, 0, 4, 1);
    addPiece(pieceTypes[Bishop], board, 0, 5, 1);
    addPiece(pieceTypes[Knight], board, 0, 6, 1);
    addPiece(pieceTypes[Rook], board, 0, 7, 1);
}
//}

//{ Memory management

// Frees a board, including all pieces on the board
void freeBoard(piece ***board){
    for(int i = 0; i < BOARD_SIZE; i++){
        for(int j = 0; j < BOARD_SIZE; j++){
            free(board[i][j]);
        }
        free(board[i]);
    }
    free(board);
}
//}

//{ Input/Output

// Returns a move based on user input in chess notation
move getMoveInput(){
    fflush(stdin);

    int rawRank;
    char rawFile;
    scanf("%c%d", &rawFile, &rawRank);
    int file = rawFile - 'a', rank = BOARD_SIZE - rawRank;
    return (move) { rank, file, 0 };
}
// Prints a centered line of ---
void printLine(){
    printf(" ");
    for(int i = 0; i < BOARD_SIZE; i++){
        printf("---");
    }
}
// Prints contents of a board to the screen
void printBoard(piece ***board){
    printf("   ");
    for(int i = 0; i < BOARD_SIZE; i++){
        printf(" %c ", 'a' + i);
    }
    printf("\n  ");
    printLine();
    printf("\n");
    for(int i = 0; i < BOARD_SIZE; i++){
        printf("%d |", BOARD_SIZE - i);
        for(int j = 0; j < BOARD_SIZE; j++){
            if(board[i][j] == NULL){
                printf("   ");
            } else {
                printf(" %c ", board[i][j]->rep + (board[i][j]->owner * UPPER));
            }
        }
        printf("|\n");
    }
    printf("  ");
    printLine();
    printf("\n");
}
//}

