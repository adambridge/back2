#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define NAMELEN 63
#define ARR 26
#define WSTART 0
#define BSTART 25
#define PNUM 2
#define DNUM 2
#define MNUM 15
#define FNUM 6
#define HEIGHT 5
#define BMAN "X"
#define WMAN "O"
#define DBMAN "B"
#define DWMAN "W"
#define NP1 "/tmp/.backA"
#define NP2 "/tmp/.backB"

enum colour {WHT, BLK};
enum boolean {FALSE, TRUE};
enum toOrFrom {FR, TO};

int  men[PNUM][ARR], d[DNUM*2], me, him, myStart, hisStart, myDir,
     myBoundary, discreet, debug, activeColour, passiveColour,
     activeStart, passiveStart, to, fr, len;
char playerName[PNUM][NAMELEN+1], colourName[PNUM][NAMELEN+1],
     preMsg[NAMELEN+1], postMsg[NAMELEN+1];

FILE* fpin;
FILE* fpout;

int  parseArgs(int argc, char** argv);
int  option(char op1[], char op2[]);
void initGame();
void joinGame();
void setupPlayer(int colour);
void setupBoard();
void printBoard();
void printMsgs();
void rollDice();
void watchTurn();
void doTurn();
int  stuck();
int  inputMove(int toOrFrom);
int  validateMove();
int  diceMatch();
int  allHome();
int  overBearing();
void applyMove();
void writeMove();
void readMove();
int  done();

int main(int argc, char** argv)
{
   if (parseArgs(argc, argv)) {
      printf("usage: back2 -sd\n");
      return 1;
   }
   if (debug == FALSE)
      system("clear");
   if (option("Initiate game", "Join game") == 1)
      initGame();
   else
      joinGame();
   setupBoard();
   int turn, kept;
   srand(time(NULL));
   while (d[0] == d[1])
      rollDice();
   printBoard();
   if (me == WHT && d[WHT] > d[BLK] ||
       me == BLK && d[BLK] > d[WHT]) {
      printf( "You rolled %d, %s rolled %d. You go first.\n",
              d[me], playerName[him], d[him] );
      turn = me == WHT ? 1 : 0;
      if (option("Keep dice", "Reroll") == 1)
         kept = TRUE;
      else
         kept = FALSE;
      fprintf(fpout, "%d\n", kept);
      fflush(fpout);
   } else {
      printf("You rolled %d, %s rolled %d. %s goes first.\n",
              d[me], playerName[him], d[him], playerName[him]);
      turn = me == WHT ? 0 : 1;
      fscanf(fpin, "%d", &kept);
   }
   while (men[BLK][WSTART] < MNUM &&
          men[WHT][BSTART] < MNUM) {
      if (kept == FALSE)
         rollDice();
      kept = FALSE;
      activeColour = (turn + 1) % PNUM;
      passiveColour = turn % PNUM;
      activeStart = activeColour == WHT ? WSTART : BSTART;
      passiveStart = passiveColour == WHT ? WSTART : BSTART;
      if (activeColour == me)
         doTurn();
      else
         watchTurn();
      ++turn;
   }
   printBoard();
   if (debug == TRUE)
      printf("men[WHT][BSTART]:%d\n", men[WHT][BSTART]);
   printf("%s is the winner!\n", men[WHT][BSTART] == MNUM ?
          playerName[WHT] : playerName[BLK]);
   return 0;
}

int parseArgs(int argc, char** argv)
{
   int ret = 0;
   char c;
   discreet = FALSE;
   debug = FALSE;
   while ((c = getopt(argc, argv, "sd")) != -1)
      switch (c) {
         case 's':
            discreet = TRUE;
            break;
         case 'd':
            debug = TRUE;
            break;
         case '?':
            if (isprint(optopt))
               fprintf(stderr, "Unknown option '-%c.\n", optopt);
            else
               fprintf(stderr, "Unknown option character '\\x%x.\n",
                       optopt);
            ret = 1;
            break;
      }
   return ret;
}

void rollDice()
{
   if (debug == TRUE)
      printf("entering rollDice\n");
   int i, dbl;
   dbl = TRUE;
   for (i = 0; i < DNUM; ++i)
      d[i] = (rand() % FNUM) + 1;
   for (i = 0; i < DNUM - 1; ++i)
      if (dbl == TRUE && d[i] != d[i+1])
         dbl = FALSE;
   for (i = DNUM; i < DNUM * 2; ++i)
      if (dbl == TRUE)
         d[i] = d[0];
      else
         d[i] = 0;
   if (debug == TRUE)
      printf("leaving rollDice\n");
}

void setupBoard()
{
   if (discreet == FALSE) {
      strncpy(colourName[BLK], BMAN, NAMELEN);
      strncpy(colourName[WHT], WMAN, NAMELEN);
   } else {
      strncpy(colourName[BLK], DBMAN, NAMELEN);
      strncpy(colourName[WHT], DWMAN, NAMELEN);
   }
   int whiteSetup[] = {
      0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
      0, 0, 0, 0, 3, 0, 5, 0, 0, 0, 0, 0, 0
   };
   int blackSetup[] = {
      0, 0, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 0,
      5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 ,0
   };
   int i;
   for (i = 0; i < ARR; ++i) {
      men[WHT][i] = whiteSetup[i];
      men[BLK][i] = blackSetup[i];
   }
   sprintf(preMsg, "");
   sprintf(postMsg, "");
}

void setupPlayer(int colour)
{
   me = colour;
   him = colour == WHT ? BLK : WHT;
   myStart = colour == WHT ? WSTART : BSTART;
   hisStart = colour == WHT ? BSTART : WSTART;
   myDir = colour == WHT ? 1 : -1;
   myBoundary = myStart + myDir * (ARR - 2) * 3 / 4;
   printf("Player %d enter name: ", colour + 1, colourName[colour]);
   scanf("%s", playerName[colour]);
   getchar();
   playerName[colour][NAMELEN] = '\0';
}

void printBoard()
{
   if (debug == FALSE)
      system("clear");
   int i, j;
   if (discreet == TRUE) {
      printf("bar:%d", men[WHT][WSTART]);
      for (i = 1; i < ARR - 1; ++i) {
         if (men[WHT][i] != 0)
            printf("%2dW", men[WHT][i]);
         else if (men[BLK][i] != 0)
            printf("%2dB", men[BLK][i]);
         else
            printf("   ");
      }
      printf("bar:%d", men[BLK][BSTART]);
      printf("\n      ");
      for (i = 1; i < ARR - 1; ++i)
         printf("%-3d", i);
   } else {
      printf("\n");
      for (i = ARR / 2; i < ARR - 1; ++i) {
         printf("%-3d", i);
         if (i == ARR / 2 + (ARR - 2) / 4 - 1)
            printf(" ");
      }
      printf("\n");
      for (j = 0; j < HEIGHT; ++j) {
         for (i = 0; i < (ARR - 2) / 2 ; ++i) {
            if (j == 0 && men[BLK][i+ARR/2] > HEIGHT)
               printf("%-3d", men[BLK][i+ARR/2]);
            else if (j == 0 && men[WHT][i+ARR/2] > HEIGHT)
               printf("%-3d", men[WHT][i+ARR/2]);
            else if (men[BLK][i+ARR/2] > j)
               printf("%-3s", BMAN);
            else if (men[WHT][i+ARR/2] > j)
               printf("%-3s", WMAN);
            else if (i % 2 == 0)
               printf("%-3s", "|");
            else
               printf("%-3s", "!");
            if (i == (ARR - 2) / 4 - 1)
               printf(" ");
         }
      printf("\n");
      }
      if (men[BLK][BSTART] > 0 && men[WHT][WSTART] > 0)
         printf("%18d%s %d%s\n", men[BLK][BSTART], BMAN,
                men[WHT][WSTART], WMAN);
      else if (men[BLK][BSTART] > 0)
         printf("%18d%s\n", men[BLK][BSTART], BMAN);
      else if (men[WHT][WSTART] > 0)
         printf("%18d%s\n", men[WHT][WSTART], WMAN);
      else
         printf("\n");
      for (j = 0; j < HEIGHT; ++j) {
         for (i = 0; i < (ARR - 2) / 2 ; ++i) {
            if (j == HEIGHT - 1 && men[BLK][ARR/2-1-i] > HEIGHT)
               printf("%-3d", men[BLK][ARR/2-1-i]);
            else if (j == HEIGHT - 1 && men[WHT][ARR/2-1-i] > HEIGHT)
               printf("%-3d", men[WHT][ARR/2-1-i]);
            else if (men[BLK][ARR/2-1-i] >= HEIGHT - j)
               printf("%-3s", BMAN);
            else if (men[WHT][ARR/2-1-i] >= HEIGHT - j)
               printf("%-3s", WMAN);
            else if (i % 2 == 0)
               printf("%-3s", "!");
            else
               printf("%-3s", "|");
            if (i == (ARR - 2) / 4 - 1)
               printf(" ");
         }
         printf("\n");
      }
      for (i = ARR / 2 - 1; i > 0 ; --i) {
         printf("%-3d", i);
         if (i == (ARR - 2) / 4 + 1)
            printf(" ");
      }
   }
   printf("\n");
}

void printMsgs()
{
   printf("%s\n", preMsg);
   if(activeColour == me)
      printf("Your turn, %s to move: ", colourName[me]);
   else
      printf("%s's turn, %s to move: ", playerName[him],
             colourName[him]);
   int i;
   for (i = 0; i < DNUM * 2; ++i)
      if (d[i] != 0)
         printf("%d ", d[i]);
   printf("\n%s\n", postMsg);
   sprintf(preMsg, "");
   sprintf(postMsg, "");
}

void doTurn()
{
   if (debug == TRUE)
      printf("entering doTurn\n");
   int ok;
   to = fr = len = 0;
   while (done() == FALSE) {
      if (men[BLK][WSTART] == MNUM ||
          men[WHT][BSTART] == MNUM) {
         to = fr = len = -1;
         break;
      } else if(stuck() == TRUE) {
         strcpy(postMsg, "There are no legal moves, "
                         "press Enter to continue");
         printBoard();
         printMsgs();
         while(getchar() != '\n')
            ;
         to = fr = len = 0;
      } else {
         ok = FALSE;
         while (ok == FALSE) {
            printBoard();
            printMsgs();
            if (inputMove( FR ) == TRUE &&
                inputMove( TO ) == TRUE &&
                validateMove() == TRUE)
               ok = TRUE;
         }
      }
      writeMove();
      applyMove();
   }
   if (debug == TRUE)
      printf("leaving doTurn\n");
}

void writeMove()
{
   if (debug == TRUE)
      printf("entering writeMove\n");
   fprintf(fpout, "%d,%d,%d\n", fr, to, len);
   fflush(fpout);
   if (debug == TRUE)
      printf("leaving writeMove\n");
}

void readMove()
{
   if (debug == TRUE)
      printf("entering readMove\n");
   fscanf(fpin, "%d,%d,%d", &fr, &to, &len);
   if (debug == TRUE)
      printf("leaving readMove\n");
}

void watchTurn()
{
   if (debug == TRUE)
      printf("entering watchTurn\n");
   to = fr = len = 0;
   while (done() == FALSE) {
      printBoard();
      printMsgs();
      readMove();
      applyMove();
   }
   if (debug == TRUE)
      printf("leaving watchTurn\n");
}

int inputMove(int toOrFrom)
{
   int ret = TRUE;
   char buf[NAMELEN];
   if (toOrFrom == FR)
      printf("from: ");
   else
      printf("to: ");
   scanf("%s", buf);
   getchar();
   buf[2] = '\0';
   if ('0' <= buf[0] && buf[0] <= '9' &&
       '0' <= buf[1] && buf[1] <= '9' ||
       '0' <= buf[0] && buf[0] <= '9' && buf[1] == '\0') {
      if (toOrFrom == TO)
         to = atoi(buf);
      else if (toOrFrom == FR)
         fr = atoi(buf);
   } else {
      sprintf(preMsg, "Eh?");
      ret = FALSE;
   }
   len = activeColour == WHT ? to - fr : fr - to;
   return ret;
}

int validateMove()
{
   if (debug == TRUE)
      printf("entering validateMove\n");
   int isValid = TRUE;
   int frMin = me, toMin = him;
   int frMax = ARR - 1 - him, toMax = ARR - 1 - me;
   if (fr < frMin || fr > frMax) {
      sprintf(preMsg, "There is no space %d", fr);
      isValid = FALSE;
   } else if (to < toMin || to > toMax) {
      sprintf(preMsg, "There is no space %d", to);
      isValid = FALSE;
   } else if (men[me][myStart] > 0 && fr != myStart) {
      sprintf(preMsg, "You have to move in from bar (%d)", myStart);
      isValid = FALSE;
   } else if (men[me][fr] == 0) {
      sprintf(preMsg, "You don't have any pieces in %d", fr);
      isValid = FALSE;
   } else if (diceMatch() == FALSE) {
      sprintf(preMsg, "That move requires %d", len);
      isValid = FALSE;
   } else if (men[him][to] > 1 && to != hisStart) {
      sprintf(preMsg, "%s is blocking %d",
              colourName[him], to);
      isValid = FALSE;
   } else if (to == hisStart && allHome() == FALSE) {
      sprintf(preMsg, "Not all of your pieces are "
                      "in your home quarter");
      isValid = FALSE;
   } else if (to == hisStart && overBearing() == TRUE) {
      sprintf(preMsg, "There are pieces behind "
                       "which must move first");
      isValid = FALSE;
   }
   if (debug == TRUE)
      printf("leaving validateMove\n");
   return isValid;
}

/* check if move matches dice rolled */
int diceMatch()
{
   int i, ret = FALSE;
   for (i = 0; i < DNUM * 2; ++i)
      if (d[i] == len || to == hisStart && d[i] >= len) {
         ret = TRUE;
         break;
      }
   return ret;
}

/* check whether all pieces are in the home quarter */
int allHome() 
{
   if(debug == TRUE) {
      printf("entering allHome\n");
      printf("myStart:%d, myBoundary:%d\n", myStart, myBoundary);
   }
   int i, ret = TRUE;
   for (i = myStart;
        me == WHT ? i <= myBoundary : i >= myBoundary;
        i += myDir) {
      if (debug == TRUE)
         printf("myMen[%d]:%d\n", i, men[me][i]);
      if (men[me][i] != 0) {
         ret = FALSE;
         break;
      }
   }
   if (debug == TRUE)
      printf("leaving allHome\n");
   return ret;
}

/* if bearing off with a roll greater than the exact amount required,
 * check whether there are pieces behind which must move first */
int overBearing()
{
   if (debug == TRUE)
      printf("entering overBearing\n");
   int i, j, ret = FALSE, exact = FALSE;
   for (i = 0; i < DNUM * 2; ++i)
      if (d[i] == len)
         exact = TRUE;
   if (exact == FALSE)
      for (i = 0; i < DNUM * 2; ++i) {
         if (d[i] == len)
            exact = TRUE;
         if (d[i] > len)
            for (j = fr - myDir;
                 me == WHT ? j <= myBoundary : j >= myBoundary;
                 j -= myDir) {
               if (debug == TRUE)
                  printf("len:%d, d[%d]:%d, men[%d]:%d\n",
                         len, i, d[i], j, men[me][j]);
               if (men[me][j] > 0) {
                  ret = TRUE;
                  goto breakOverBearingNest;
               }
            }
      }
   breakOverBearingNest:
   if (debug == TRUE)
      if (ret == TRUE)
         printf("leaving overBearing, returning TRUE\n");
      else
         printf("leaving overBearing, returning FALSE\n");
   return ret;
}

void applyMove()
{
   if (debug == TRUE)
      printf("entering applyMove\n");
   int i;
   if( to == -1 && fr == -1 ) {
      for (i = 0; i < DNUM * 2; ++i)
         d[i] = 0;
   } else if (to == 0 && fr == 0) {
      for (i = 0; i < DNUM * 2; ++i)
         d[i] = 0;
      if (activeColour == him)
         sprintf(preMsg, "%s had no legal moves",
                 playerName[him]);
   } else {
      men[activeColour][fr] -= 1;
      men[activeColour][to] += 1;
      if (men[passiveColour][to] == 1 &&
          to != myStart &&
          to != hisStart) {
         if (me == passiveColour) {
            sprintf(preMsg, "%s zonked you!",
                    playerName[him] );
         } else {
            sprintf(preMsg, "You zonked %s!",
                    playerName[him]);
         }
         men[passiveColour][to] -= 1;
         men[passiveColour][passiveStart] += 1;
      }
      int sufficient, found = FALSE;
      for (i = 0; i < DNUM * 2; ++i) {
         if (len == d[i]) {
            d[i] = 0;
            found = TRUE;
            if (debug == TRUE)
               printf("d[%d] set to 0\n", i, d[i]);
            break;
         }
         if ((to == hisStart || to == myStart) && d[i] >= len)
            sufficient = i;
      }
      if (found == FALSE)
         d[sufficient] = 0;
   }
   if (debug == TRUE)
      printf("leaving applyMove\n");
}

int stuck()
{
   if (debug == TRUE)
      printf("entering stuck\n");
   int i, j, k, isStuck = TRUE;
   if (men[me][myStart] != 0) {
      /*check if any men unable to get out of bar*/
      if (debug == TRUE)
         printf("men unable to get out of bar?\n");
      for (i = 0; i < DNUM * 2; ++i) {
         j = myStart + myDir * d[i];
         if (debug == TRUE)
            printf("d[%d]:%d hisMen[%d]:%d\n",
                   i, d[i], j, men[him][j]);
         if (men[him][j] < 2 && d[i] != 0) {
            isStuck = FALSE;
            break;
         }
      }
   } else {
      /*check if any men in play can move*/
      if (debug == TRUE)
         printf("men in play unable to move?\n");
      int allHomeRet = allHome();
      for (i = 1; i <= ARR - 2; ++i)
         if (men[me][i] > 0)
            for (j = 0; j < DNUM * 2; ++j) {
               if (d[j] == 0)
                  continue;
               k = i + myDir * d[j];
               if (debug == TRUE)
                  printf("myMen[%d]:%d d[%d]:%d hisMen[%d]:%d\n",
                         i, men[me][i], j, d[j], k, men[him][k]);
               if (me == WHT ? k >= hisStart : k <= hisStart) {
                  if (allHomeRet == TRUE) {
                     isStuck = FALSE;
                     goto breakStuckNest;
                  }
               } else {
                  if (men[him][k] < 2) {
                     isStuck = FALSE;
                     goto breakStuckNest;
                  }
               }
            }
   }
   breakStuckNest:
   if (debug == TRUE)
      printf("leaving stuck\n");
   return isStuck;
}

int done()
{
   if (debug == TRUE)
      printf("entering done\n");
   int i, isDone = TRUE;
   for (i = 0; i < DNUM * 2; ++i) {
      if (men[WHT][BSTART] == MNUM || men[BLK][WSTART] == MNUM)
         break;
      if (d[i]!= 0) {
         isDone = FALSE;
         break;
      }
   }
   if (debug == TRUE) {
      printf("leaving done\n");
   if (isDone == TRUE)
      printf("isDone: TRUE\n");
   else if (isDone == FALSE)
      printf("isDone: FALSE\n");
   else
      printf("isDone: %d\n");
   }
   return isDone;
}

void initGame()
{
   int ret = mkfifo(NP1, 0777);
   if ((ret == -1) && (errno != EEXIST)) {
      printf("failed to create named pipe\n");
      exit(1);
   }
   ret = mkfifo(NP2, 0777);
   if ((ret == -1) && (errno != EEXIST)) {
      printf("failed to create named pipe\n");
      exit(1);
   }
   setupPlayer(WHT);
   printf("Waiting for opponent to join...\n");
   fpout = fopen(NP2, "w");
   fprintf(fpout, "%s\n", playerName[WHT]);
   fflush(fpout);
   fpin = fopen(NP1, "r");
   fscanf(fpin, "%s", playerName[BLK]);
   printf("Playing against %s\n", playerName[BLK]);
}

void joinGame()
{
   setupPlayer(BLK);
   fpin = fopen(NP2, "r");
   fscanf(fpin, "%s", playerName[WHT]);
   printf("Playing against %s\n", playerName[WHT]);
   fpout = fopen(NP1, "w");
   fprintf(fpout, "%s\n", playerName[BLK]);
   fflush(fpout);
}

int option(char op1[], char op2[])
{
   int ret;
   char buf[NAMELEN] = {'\0'};
   while (strncmp(buf, "1", 1 ) != 0 &&
          strncmp(buf, "2", 1 ) != 0) {
      printf("1: %s\n", op1);
      printf("2: %s\n", op2);
      scanf("%s", buf);
      getchar();
      if (debug == FALSE)
         system("clear");
      if (strncmp(buf, "1", 1) == 0)
         ret = 1;
      else if (strncmp(buf, "2", 1) == 0)
         ret = 2;
   }
   return ret;
}
