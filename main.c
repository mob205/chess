#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_SIZE 8
#define MAX_MOVES 40
#define UPPER 32
#define MAX_INPUT 5

#define ENPASSANTER -1
#define CAN_CASTLE 1
#define PROMOTED -10

//{ Structs
typedef enum Type{
    Pawn = 0,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    None
} Type;
typedef struct move{
    int rank;
    int file;
    int flag; // Flag denotes the ability to do special moves, namely en passant and castling
} move;

typedef struct piece{
    Type type;
    char rep;
    int owner;
    int flag;
    move* (*getPossibleMoves)(int rank, int file, struct piece*** board, int owner, int* cnt);
} piece;
typedef struct moveRecord{
    int player;
    move start;
    move end;
    Type captured;
    move capturedPos;
    struct moveRecord *next;
} moveRecord;
//}


// Function prototypes
//{
// Piece movement
void addPiece(piece temp, piece ***board, int rank, int file, int owner);
void movePiece(piece ***board, int curRank, int curFile, int targetRank, int targetFile, int flag);
void promotePawn(piece ***board, int rank, int file);
move checkEnPassant(piece ***board, int rank, int file, int flag);

// Piece possible moves
move *getPawnMoves(int rank, int file, piece ***board, int owner, int *cnt);
move *getKnightMoves(int rank, int file, piece ***board, int owner, int *cnt);
move *getBishopMoves(int rank, int file, piece ***board, int owner, int *cnt);
move *getRookMoves(int rank, int file, piece ***board, int owner, int *cnt);
move *getQueenMoves(int rank, int file, piece ***board, int owner, int *cnt);
void addDiagonalMoves(int rank, int file, piece ***board, int owner, int *cnt, move *moves);
void addStraightMoves(int rank, int file, piece ***board, int owner, int *cnt, move *moves);

// Move validation
void addPossibleMove(move *moves, int *len, int rank, int file, int flag);
int isValidTile(int rank, int file);
int isEnemyPiece(int rank, int file, int owner, piece ***board);
int isValidEmpty(int rank, int file, piece ***board);
int isPossibleMove(int rank, int file, move *moves, int cnt, int *flag);

// Initialization
piece ***makeBoard();

// Input/Output
void readyBoard(piece ***board);
void printBoard(piece ***board);
void freeBoard(piece ***board);
void printLine();
move getMoveInput();
void clearstdin();

// Move history
moveRecord *storeMove(moveRecord *head, move start, move end, Type captured, move capturedPos, int owner);
moveRecord *undoMove(moveRecord *head, piece ***board);

//}

// Definitions for chess piece types
const piece pieceTypes[] = {
        { Pawn, 'P', 0, 0, &getPawnMoves },
        { Knight, 'N', 0, 0, &getKnightMoves },
        { Bishop, 'B', 0, 0, &getBishopMoves },
        { Rook, 'R', 0, 1, &getRookMoves },
        { Queen, 'Q', 0, 0, &getQueenMoves },
        { King, 'K', 0, 1, 0}
};
int turn = 0;
moveRecord *moveRecords = NULL;

int main()
{
    piece ***board = makeBoard();
    readyBoard(board);
    while(1){
        printBoard(board);

        // Get and validate input
        printf("Select a piece to move\n");
        move cur = getMoveInput();
        if(cur.file == -1){
            moveRecords = undoMove(moveRecords, board);
            continue;
        }

        piece *selected = board[cur.rank][cur.file];
        // Check if piece is valid
        if(isValidTile(cur.rank, cur.file) && selected != NULL && selected->owner == turn % 2){
            printf("Piece selected: %c\n", selected->rep);

            // Get input and validate
            printf("Where would you like to move this piece to?\n");
            move tar = getMoveInput();

            // Check if move is possible
            int cnt, flag;
            move *moves = selected->getPossibleMoves(cur.rank, cur.file, board, selected->owner, &cnt);
            if(isPossibleMove(tar.rank, tar.file, moves, cnt, &flag)){
                // Carry out move
                movePiece(board, cur.rank, cur.file, tar.rank, tar.file, flag);
                board[tar.rank][tar.file]->flag = flag;
                turn++;
            } else {
                printf("Invalid move.\n");
            }
            free(moves);
        } else if(isValidTile(cur.rank, cur.file) && selected != NULL && selected->owner != turn % 2){
            printf("You don't own that piece!\n");
        } else {
            printf("Piece not found.\n");
        }
    }
    freeBoard(board);
    return 0;
}

//{ Piece movement
// Adds a piece to the board
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
// Returns the position of the captured piece, if any
void movePiece(piece ***board, int curRank, int curFile, int targetRank, int targetFile, int flag){
    Type capturedType = None;
    move capturedPos = { -1, -1 };

    // Check for capture
    if(board[targetRank][targetFile] != NULL){
        capturedPos = (move) { targetRank, targetFile, board[targetRank][targetFile]->flag };
        capturedType = board[targetRank][targetFile]->type;
        free(board[targetRank][targetFile]);
    }
    // Move piece
    board[targetRank][targetFile] = board[curRank][curFile];
    board[curRank][curFile] = NULL;

    // Check for en passant
    move passantPos = checkEnPassant(board, targetRank, targetFile, flag);
    if(passantPos.rank > -1){
        capturedPos = passantPos;
        capturedType = Pawn;
    }
    // Check for promotion
    promotePawn(board, targetRank, targetFile);

    // Record move on stack
    move start =(move) { curRank, curFile, board[targetRank][targetFile]->flag };
    move end = (move) { targetRank, targetFile, flag };
    moveRecords = storeMove(moveRecords, start, end, capturedType, capturedPos, board[targetRank][targetFile]->owner);
}
// Promotes to piece to a queen if it is an eligible pawn
void promotePawn(piece ***board, int rank, int file){
    int owner = board[rank][file]->owner;
    if(board[rank][file]->type == Pawn && ((owner == 0 && rank == 0) || (owner == 1 && rank == BOARD_SIZE - 1))){
        // Get user's choice of promotion
        printf("What would you like to promote this pawn to? (Q - Queen | R - Rook | B - Bishop | N - Knight)\n");
        char input;
        int isValidInput = 0;
        piece choice;
        while(isValidInput == 0){
            scanf("%c", &input);
            clearstdin();
            isValidInput = 1;
            if(input == 'Q'){
                choice = pieceTypes[Queen];
                printf("Pawn promoted to Queen.\n");
            } else if(input == 'R'){
                choice = pieceTypes[Rook];
                printf("Pawn promoted to Rook.\n");
            } else if(input == 'B'){
                choice = pieceTypes[Bishop];
                printf("Pawn promoted to Bishop.\n");
            } else if(input == 'N'){
                choice = pieceTypes[Knight];
                printf("Pawn promoted to Knight.\n");
            } else {
                isValidInput = 0;
                printf("Invalid choice.\n");
            }
        }
        addPiece(choice, board, rank, file, owner);
    }
}
// Checks for an en passant. Returns position of en passanted pawn, if there is one
move checkEnPassant(piece ***board, int rank, int file, int flag){
    move res = { -1, -1 };
    int owner = board[rank][file]->owner;
    int dir = (owner == 1) ? -1 : 1;
    if(board[rank][file]->type == Pawn && flag == ENPASSANTER){
        printf("Captured enemy %c by EN PASSANT!!\n", board[rank + dir][file]->rep);
        res = (move) { rank + dir, file, board[rank + dir][file]->flag };
        free(board[rank + dir][file]);
        board[rank + dir][file] = NULL;

    }
    return res;
}
//}

//{ Piece possible moves
// Returns all possible moves for a pawn to make (and number of possible moves)
move *getPawnMoves(int rank, int file, piece ***board, int owner, int *cnt){
    move *possibleMoves = malloc(MAX_MOVES * sizeof(move));
    *cnt = 0;
    int dir = (owner == 1) ? 1 : -1;

    // Forward move
    if(board[rank + dir][file] == NULL){
        // Check for promotion
        if(rank + dir == 0 || rank + dir == BOARD_SIZE - 1){
            addPossibleMove(possibleMoves, cnt, rank + dir, file, PROMOTED);
        } else {
            addPossibleMove(possibleMoves, cnt, rank + dir, file, 0);
        }
        // 2-space first move
        if(((owner == 1 && rank == 1) || (owner == 0 && rank == BOARD_SIZE - 2)) && board[rank + (2 * dir)][file] == NULL){
            addPossibleMove(possibleMoves, cnt, rank + (2 * dir), file, turn);
        }
    }
    // Capture right
    if(isEnemyPiece(rank + dir, file + 1, owner, board)){
        addPossibleMove(possibleMoves, cnt, rank + dir, file + 1, 0);
    }
    // Capture left
    if(isEnemyPiece(rank + dir, file - 1, owner, board)){
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
// Returns all possible moves for a knight to make (and number of possible moves)
move *getKnightMoves(int rank, int file, piece ***board, int owner, int *cnt){
    move *possibleMoves = malloc(MAX_MOVES * sizeof(move));
    *cnt = 0;
    int tarRank, tarFile;

    // Loop through all 8 possible knight L shapes
    for(int k = 0; k < 2; k++){ // number of L configurations, either 3 vertical/2 horizontal or 2 vertical/3 horizontal
        for(int i = 0; i < 2; i++){ // first positive rankOffset, then negative
            for(int j = 0; j < 2; j++){ // first positive fileOffset, then negative
                tarRank = rank + (2 - k) * (1 - (2 * i));
                tarFile = file + (1 + k) * (1 - (2 * j));
                if(isEnemyPiece(tarRank, tarFile, owner, board) || isValidEmpty(tarRank, tarFile, board)){
                    addPossibleMove(possibleMoves, cnt, tarRank, tarFile, 0);
                }
            }
        }
    }
    return possibleMoves;
}
// Returns all possible moves for a bishop to make
move *getBishopMoves(int rank, int file, piece ***board, int owner, int *cnt){
    move *possibleMoves = malloc(MAX_MOVES * sizeof(move));
    *cnt = 0;
    addDiagonalMoves(rank, file, board, owner, cnt, possibleMoves);
    return possibleMoves;
}
// Returns all possible moves for a rook to make
move *getRookMoves(int rank, int file, piece ***board, int owner, int *cnt){
    move *possibleMoves = malloc(MAX_MOVES * sizeof(move));
    *cnt = 0;
    addStraightMoves(rank, file, board, owner, cnt, possibleMoves);
    return possibleMoves;
}
// Returns all possible moves for a queen to make
move *getQueenMoves(int rank, int file, piece ***board, int owner, int *cnt){
    move *possibleMoves = malloc(MAX_MOVES * sizeof(move));
    *cnt = 0;
    addDiagonalMoves(rank, file, board, owner, cnt, possibleMoves);
    addStraightMoves(rank, file, board, owner, cnt, possibleMoves);
    return possibleMoves;
}
// Adds all clear moves to diagonal tiles up to (and including) the first opponent piece to an array of possible moves
void addDiagonalMoves(int rank, int file, piece ***board, int owner, int *cnt, move *moves){
    int rankDir, fileDir;
    for(int i = 0; i < 2; i++){ // i = 0: diagonals going up | i = 1: diagonals going down
        for(int j = 0; j < 2; j++){ // j = 0: diagonal going right | j = 1: diagonal going left
            rankDir = 1 - (2 * i), fileDir = 1 - (2 * j);
            int tarRank = rank + rankDir, tarFile = file + fileDir;

            // Add move if tile is empty or an enemy, but only if last tile wasn't an enemy (i.e. don't go past first enemy)
            while((isValidEmpty(tarRank, tarFile, board) || isEnemyPiece(tarRank, tarFile, owner, board))
                   && !isEnemyPiece(tarRank - rankDir, tarFile - fileDir, owner, board)){
                addPossibleMove(moves, cnt, tarRank, tarFile, 0);
                tarRank += rankDir;
                tarFile += fileDir;
            }
        }
    }
}
// Adds all clear moves to tiles in a straight line up to (and including) the first opponent piece to an array of possible moves
void addStraightMoves(int rank, int file, piece ***board, int owner, int *cnt, move *moves){
    int rankDir, fileDir;
    // Iterate through all 4 directions
    for(int i = 0; i < 4; i++){
        // Get direction to search in
        rankDir = 0, fileDir = 0;
        if(i == 0){
            rankDir = 1;
        } else if(i == 1){
            rankDir = -1;
        } else if(i == 2){
            fileDir = 1;
        } else if(i == 3){
            fileDir = -1;
        }
        int tarRank = rank + rankDir, tarFile = file + fileDir;
        // Add move if tile is empty or an enemy, but only if the last tile wasn't an enemy (i.e. don't go past first enemy)
        while((isValidEmpty(tarRank, tarFile, board) || isEnemyPiece(tarRank, tarFile, owner, board))
                   && !isEnemyPiece(tarRank - rankDir, tarFile - fileDir, owner, board)){
                addPossibleMove(moves, cnt, tarRank, tarFile, 0);
                tarRank += rankDir;
                tarFile += fileDir;
        }
    }
}

//}

//{ Move validation
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
// Checks if a tile is occupied by an enemy's piece
int isEnemyPiece(int rank, int file, int owner, piece ***board){
    return(isValidTile(rank, file) && board[rank][file] != NULL && board[rank][file]->owner == (owner + 1) % 2);
}
// Checks if a given tile is on the board and unoccupied
int isValidEmpty(int rank, int file, piece ***board){
    return(isValidTile(rank, file) && board[rank][file] == NULL);
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
// Clears input buffer
void clearstdin(){
    fseek(stdin, 0, SEEK_END);
    if(ftell(stdin) > 0){
        char c;
        while((c = getchar()) != '\n' && c != EOF);
    }
}
// Returns a move based on user input in chess notation
move getMoveInput(){
    // Check for command (currently just UNDO)
    clearstdin();
    char input[MAX_INPUT] = { 0 };
    char cmd[MAX_INPUT] = { 0 };
    fgets(input, MAX_INPUT * sizeof(char), stdin);
    sscanf(input, "%s", cmd);
    if(strcmp(cmd, "UNDO") == 0){
        return (move) { -1, -1 };
    }

    int rawRank;
    char rawFile;
    sscanf(input, "%c%d", &rawFile, &rawRank);
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

//{ Move history
// Adds a move record to the stack of all moves played in the game
moveRecord *storeMove(moveRecord *head, move start, move end, Type captured, move capturedPos, int owner){
    moveRecord *rec = malloc(sizeof(moveRecord));
    rec->start = start;
    rec->end = end;
    rec->captured = captured;
    rec->capturedPos = capturedPos;
    printf("Storing move's capture position at: %d %d\n", capturedPos.rank, capturedPos.file);
    rec->player = owner;
    rec->next = head;
    return rec;
}
// Undos previous move
moveRecord *undoMove(moveRecord *head, piece ***board){
    if(head == NULL){
        return NULL;
    }
    moveRecord *rec = head;
    // Undo movement
    board[rec->start.rank][rec->start.file] = board[rec->end.rank][rec->end.file];
    board[rec->end.rank][rec->end.file] = NULL;

    // Undo pawn promotes
    if(rec->end.flag == PROMOTED){
        addPiece(pieceTypes[Pawn], board, rec->start.rank, rec->start.file, rec->player);
    }
    // Reset flags
    board[rec->start.rank][rec->start.file]->flag = rec->start.flag;

    // Undo captures

    if(rec->captured != None){
        addPiece(pieceTypes[rec->captured], board, rec->capturedPos.rank, rec->capturedPos.file, (rec->player + 1) % 2);
        board[rec->capturedPos.rank][rec->capturedPos.file]->flag = rec->capturedPos.flag;
    }
    turn--;
    printf("Turn %d: flag of moved piece is %d, flag of captured piece is %d", turn, board[rec->start.rank][rec->start.file]->flag, board[rec->capturedPos.rank][rec->capturedPos.file]->flag);

    // Remove record from stack
    head = rec->next;
    free(rec);
    return head;
}
//}
