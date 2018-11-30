/* Space Invaders
   Timothy Pettit
   
   This program is modeled after the 80's arcade classic Space Invaders. A "tank" moves across
   the bottom of the screen and shoots at "aliens" that move across and down the screen while
   dropping bombs. The game is lost if the tank is hit by a bomb or an alien reaches
   the bottom of the screen. The game is won by shooting down all of the aliens. This game
   also features a menu which allows the user to adjust the overall speed of the game as well
   as the relative speeds of the individual elements.
*/

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <time.h>
#include "mpi.h"
#include "invaderstructs.h"

#define MAX_BOMBS 1000

#define ALIENS 5
#define ALIEN_ROWS 1
#define ALIEN_COLUMNS (ALIENS / ALIEN_ROWS)

void menu(struct options *settings);
void gameover(int win);
void mpi_aliens(struct *alien aliens, struct *player tank,
                  struct *shoot shot, struct *bomb bombs);

int cpu, numcpus;

/* The main function handles user input, the game visuals, and checks for win/loss conditions */
int main(int argc, char **argv) {
   struct player tank;
   struct alien aliens[ALIENS];
   struct shoot shot[3];
   struct bomb bomb[MAX_BOMBS];
   struct options settings;
   unsigned int input, loops=0, i=0, j=0, currentshots=0, currentbombs=0, currentaliens=ALIENS;
   int cageExists = 0;
   int random=0, score=0, win=-1;
   char tellscore[30];

   /// - MPI Starter - ////////////////////////////////////////////////////////// 
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &cpu);
   MPI_Comm_size(MPI_COMM_WORLD, &numcpus);
   //////////////////////////////////////////////////////////
   
   initscr();
   clear();
   noecho();
   cbreak();
   nodelay(stdscr,1);
   keypad(stdscr,1);
   srand(time(NULL)); 

   /* Set default settings */
   settings.overall = 15000;
   settings.alien = 12;
   settings.shots = 3;
   settings.bombs = 10;
   settings.bombchance = 5;

   /* Set tank settings */
   tank.r = LINES - 1;
   tank.c = COLS / 2;
   tank.ch = '^';

   /* Set aliens settings */
   unsigned int a = 0;
   for (i = 0; i < ALIEN_ROWS; i++) {
      for (j = 0; j < ALIEN_COLUMNS; j++) {
         a = i * ALIEN_COLUMNS + j;
         
         aliens[a].r = (i + 1) * 2;
         aliens[a].c = j * 5;
         aliens[a].pr = 0;
         aliens[a].pc = 0;
         aliens[a].ch = '#';
         aliens[a].alive = 1;
         random = rand()%4;
         aliens[a].behavior = random;
         aliens[a].direction = 'r';

      }
   }
   
   /* Set shot settings */
   for (i=0; i<3; ++i) {
      shot[i].active = 0;
      shot[i].ch = '*';
   }
   
   /* Set bomb settings */
   for (i=0; i<MAX_BOMBS; ++i) {
      bomb[i].active = 0;
      bomb[i].ch = 'o';
      bomb[i].loop = 0;
   }
   
   /* Display game title,score header,options */
   move(0,(COLS/2)-9);
   addstr("--SPACE INVADERS--");
   move (0,1);
   addstr("SCORE: ");
   move(0,COLS-19);
   addstr("m = menu  q = quit");
   srand(time(0)); //Sets the new seed for the rand() function.
   while(1) {
      /* Show score */
      sprintf(tellscore, "%d", score);
      move(0,8);
      addstr(tellscore);
      
      
      // show numer of aliens left
      sprintf(tellscore, "Aliens remaining: %d", currentaliens);
      move(0, 20);
      addstr(tellscore);
      
      /* Move tank */
      move(tank.r,tank.c);
      addch(tank.ch);
      
      /* Move bombs */
      if (loops % settings.bombs == 0)
      for (i=0; i<MAX_BOMBS; ++i) {
         if (bomb[i].active == 1) {
            if (bomb[i].r < LINES) {
               if (bomb[i].loop != 0) {
                  move(bomb[i].r-1,bomb[i].c);
                  addch(' ');
               }
               else
                  ++bomb[i].loop;
               
               move(bomb[i].r,bomb[i].c);
               addch(bomb[i].ch);
               
               ++bomb[i].r;
            }
            else {
               bomb[i].active = 0;
               bomb[i].loop = 0;
               --currentbombs;
               move(bomb[i].r - 1,bomb[i].c);
               addch(' ');
            }
         }
      }
      
      /* Move shots */
      if (loops % settings.shots == 0)
      for (i=0; i<3; ++i) {
         if (shot[i].active == 1) {
            if (shot[i].r > 0) {
               if (shot[i].r < LINES - 2) {
                  move(shot[i].r + 1,shot[i].c);
                  addch(' ');
               }
               
               for (j=0; j<ALIENS; ++j) {
                  if (aliens[j].alive == 1 && aliens[j].r == shot[i].r && aliens[j].pc == shot[i].c) {
                     score += 20;
                     aliens[j].alive = 0;
                     shot[i].active = 0;
                     --currentshots;
                     --currentaliens;
                     move(aliens[j].pr,aliens[j].pc);
                     addch(' ');
                     break;
                  }
               }
               
               if (shot[i].active == 1) {
                  move(shot[i].r,shot[i].c);
                  addch(shot[i].ch);
                  
                  --shot[i].r;
               }
            }
            else {
               shot[i].active = 0;
               --currentshots;
               move(shot[i].r + 1,shot[i].c);
               addch(' ');
            }
         }
      }     
      
      /* Move aliens */
      if (loops % settings.alien == 0)
      for (i=0; i<ALIENS; ++i) {
         if (aliens[i].alive == 1) {
            move(aliens[i].pr,aliens[i].pc);
            addch(' ');
            
            move(aliens[i].r,aliens[i].c);
            addch(aliens[i].ch);
            
            aliens[i].pr = aliens[i].r;
            aliens[i].pc = aliens[i].c;
            
            random = rand() % 200;
            
            if (random < aliens[i].time) {
                
                if (aliens[i].behavior == 3) {
                    random = rand() % 3;
                    cageExists = 0;
                } else {
                    
                    if (cageExists) {
                        random = rand() % 3;
                    } else {
                        random = rand() % 4;
                        
                        if (random == 3) {
                            cageExists = 1;
                        }
                    }
                }
                
                aliens[i].behavior = random;
            }

            if (aliens[i].behavior == 0){ //Wander around the screen.   
                aliens[i].ch = 'W';
                if (aliens[i].time % 15 == 0 && rand() % 4 == 0) {
                    
                    if (aliens[i].direction == 'l') {
                        aliens[i].direction = 'r';
                    } 
                    else {
                        aliens[i].direction = 'l';
                    }
                }

                /*set alien next position*/
                if (aliens[i].direction == 'l') {
                    if (aliens[i].c != 0) {
                        --aliens[i].c;
                    } 
                    else {
                        ++aliens[i].c;
                        aliens[i].direction = 'r';
                    }
                }
                else if (aliens[i].direction == 'r') {
                    if (aliens[i].c != COLS - 2) {
                        ++aliens[i].c;
                    } 
                    else {
                        --aliens[i].c;
                        aliens[i].direction = 'l';
                    }
                }
                
                if (settings.bombchance - (1+rand()%100) >= 0 && currentbombs < MAX_BOMBS){
                    for (j=0; j<MAX_BOMBS; ++j) {
                        if (bomb[j].active == 0) {
                                bomb[j].active = 1;
                                ++currentbombs;
                                bomb[j].r = aliens[i].r + 1;
                                bomb[j].c = aliens[i].c;
                                break;
                        }
                    }
                }
            }
            else if (aliens[i].behavior == 1) //Follow the player.
            {     
                  aliens[i].ch == 'F';
                  if (aliens[i].c < tank.c)
                        ++aliens[i].c;
                  else if (aliens[i].c > tank.c)
                        --aliens[i].c;
                  //Drops bombs when the player and the alien are in nearby columns
                  if (abs(aliens[i].c - tank.c) < 4)
                  {
                        for (j=0; j<MAX_BOMBS; ++j) {
                              if (bomb[j].active == 0) {
                                    bomb[j].active = 1;
                                    ++currentbombs;
                                    bomb[j].r = aliens[i].r + 1;
                                    bomb[j].c = aliens[i].c;
                                    break;
                              }
                        }
                  }
            } 
            else if (aliens[i].behavior == 2){      /*Dodge tank shots*/
                  aliens[i].ch = 'D';
                  int dodged = 0;
                  for (j=0; j<3; ++j) {  
                        if (shot[j].active == 1 && abs(shot[j].c - aliens[j].c) <= 1 && shot[j].r > aliens[i].r && shot[j].r - aliens[i].r <= 5 && dodged == 0) {     
                              random = rand() % 2;
                     
                              if (aliens[i].c >= COLS - 3) {
                                    aliens[i].c -= 2;
                                    aliens[i].direction = 'l';
                              } else if (aliens[i].c <= 1) {
                                    aliens[i].c += 2;
                                    aliens[i].direction = 'r';
                              } else { 
                                    if (random == 0) {
                                          aliens[i].c += 2;
                                          aliens[i].direction = 'r';
                                    } else {
                                          aliens[i].c -= 2;
                                          aliens[i].direction = 'l';
                                    }
                              }
                        dodged = 1;
                        }          
                  }
      }

                /* Check if alien should drop bomb */
                if ((settings.bombchance - random) >= 0 && currentbombs < MAX_BOMBS) {
                    for (j=0; j<MAX_BOMBS; ++j) {
                        if (bomb[j].active == 0) {
                            bomb[j].active = 1;
                            ++currentbombs;
                            bomb[j].r = aliens[i].r + 1;
                            bomb[j].c = aliens[i].c;
                            break;
                        }
                    }
                }
            }

            else if (aliens[i].behavior == 3) //Wall trap the player.
            {
                  aliens[i].ch = 'T';
                  /* Check if alien should drop bomb */
                  if ((settings.bombchance - random) >= 0 && currentbombs < MAX_BOMBS) {
                        for (j=0; j<MAX_BOMBS; ++j) {
                              if (bomb[j].active == 0) {
                              bomb[j].active = 1;
                              ++currentbombs;
                              bomb[j].r = aliens[i].r + 1;
                              bomb[j].c = aliens[i].c;
                              break;
                              }
                        }
                  }
            }
            
            //aliens[i].time++;
            if (loops % 250 == 0 && loops != 0) {
               ++aliens[i].r;
            }
         }
      
      
      
      /* See if game has been won or lost*/
      if (currentaliens == 0) {
         win = 1;
         break;
      }
      for (i=0; i<ALIENS; ++i) {
         if (aliens[i].r == LINES-1) {
            win = 0;
            break;
         }
      }
      for (i=0; i<MAX_BOMBS; ++i) {
         if (bomb[i].r == tank.r && bomb[i].c == tank.c) {
            win = 0;
            break;
         }
      }    
      
      move(0,COLS-1);
      refresh();
      usleep(settings.overall);
      ++loops;
      
      input = getch();
      move(tank.r,tank.c);
      addch(' ');
      
      /* Check input */
      if (input == 'q')
         win = 2;
      else if (input == KEY_LEFT)
         --tank.c;
      else if (input == KEY_RIGHT)
         ++tank.c;
      else if (input == ' ' && currentshots < 3) {
         for (i=0; i<3; ++i) {
            if (shot[i].active == 0) {
               shot[i].active = 1;
               ++currentshots;
               --score;
               shot[i].r = LINES - 2;
               shot[i].c = tank.c;
               break;
            }
         }
      }
      else if (input == 'm')
         menu(&settings); 
      
      if (win != -1)
         break;
      
      /* Check for valid tank position */
      if (tank.c > COLS-2)
         tank.c = COLS - 2;
      else if (tank.c < 0)
         tank.c = 0;     
   }
   
   gameover(win);
   endwin();
   return 0;
}

/* This function handles the menu options available to the user */
void menu(struct options *settings) {
   char option, buf[30];
   int new;

   clear();
   echo();
   nocbreak();
   nodelay(stdscr,0);

   move(0,0);
   addstr("1. Change overall game speed");
   move(1,0);
   addstr("2. Change alien motion speed");
   move(2,0);
   addstr("3. Change tank shot speed");
   move(3,0);
   addstr("4. Change alien bomb speed");
   move(4,0);
   addstr("5. Change alien bomb dropping frequency");
   move(5,0);
   addstr("6. Return to the game");
   move(6,0);
   addstr("7. Exit the game");
   move(8,0);
   addstr("Enter your option: ");
   refresh();
   
   while(1) {
      move(8,19);
      option = getch();
      move(9,0);
      deleteln();
      move(10,0);
      deleteln();
      move(11,0);
      deleteln();
      
      if (option == '1') {
         sprintf(buf,"Current value: %d\n", settings->overall);
         
         move(9,0);
         addstr(buf);
         move(10,0);
         addstr("Enter new value: ");
         move(10,17);
         refresh();
         getch();
         getstr(buf);
         
         new = atoi(buf);
         
         /* Check for valid new value */
         if (new < 0) {
            move(11,0);
            addstr("ERROR: Inalid value");
         }
         else {
            settings->overall = new;
         }  
      }
      else if (option == '2') {
         sprintf(buf,"Current value: %d\n", settings->alien);
         
         move(9,0);
         addstr(buf);
         move(10,0);
         addstr("Enter new value: ");
         move(10,17);
         refresh();
         getch();
         getstr(buf);
         
         new = atoi(buf);
         
         /* Check for valid new value */
         if (new <= 0) {
            move(11,0);
            addstr("ERROR: Inalid value");
         }
         else {
            settings->alien = new;
         }  
      }
      else if (option == '3') {
         sprintf(buf,"Current value: %d\n", settings->shots);
         
         move(9,0);
         addstr(buf);
         move(10,0);
         addstr("Enter new value: ");
         move(10,17);
         refresh();
         getch();
         getstr(buf);
         
         new = atoi(buf);
         
         /* Check for valid new value */
         if (new <= 0) {
            move(11,0);
            addstr("ERROR: Inalid value");
         }
         else {
            settings->shots = new;
         }  
      }
      else if (option == '4') {
         sprintf(buf,"Current value: %d\n", settings->bombs);
         
         move(9,0);
         addstr(buf);
         move(10,0);
         addstr("Enter new value: ");
         move(10,17);
         refresh();
         getch();
         getstr(buf);
         
         new = atoi(buf);
         
         /* Check for valid new value */
         if (new <= 0) {
            move(11,0);
            addstr("ERROR: Inalid value");
         }
         else {
            settings->bombs = new;
         }  
      }
      else if (option == '5') {
         sprintf(buf,"Current value: %d\n", settings->bombchance);
         
         move(9,0);
         addstr(buf);
         move(10,0);
         addstr("Enter new value: ");
         move(10,17);
         refresh();
         getch();
         getstr(buf);
         
         new = atoi(buf);
         
         /* Check for valid new value */
         if (new > 100 || new < 0) {
            move(11,0);
            addstr("ERROR: Inalid value");
         }
         else {
            settings->bombchance = new;
         }  
      }
      else if (option == '6') {
         break;
      }
      else if (option == '7') {
         endwin();
         exit(0);
      }
      else {
         move(9,0);
         addstr("ERROR: Invalid selection");
         move(8,19);
         addstr(" ");
         refresh();        
      }
   }
   
   clear();
   noecho();
   cbreak();
   nodelay(stdscr,1);
   
   move(0,(COLS/2)-9);
   addstr("--SPACE INVADERS--");
   move (0,1);
   addstr("SCORE: ");
   move(0,COLS-19);
   addstr("m = menu  q = quit");
}

/* This function handles displaying the win/lose screen */
void gameover(int win) {

   nodelay(stdscr, 0);
   
   if (win == 0) {
      clear();
      move((LINES/2)-1,(COLS/2)-5);
      addstr("YOU LOSE!");
      move((LINES/2),(COLS/2)-11);
      addstr("PRESS ANY KEY TO EXIT");
      move(0,COLS-1);
      refresh();
      getch();
   }
   
   else if (win == 1) {
      clear();
      move((LINES/2)-1,(COLS/2)-5);
      addstr("YOU WIN!");
      move((LINES/2),(COLS/2)-11);
      addstr("PRESS ANY KEY TO EXIT");
      move(0,COLS-1);
      refresh();
      getch();
   }
}

void mpi_aliens(struct *alien aliens, struct *player tank,
                  struct *shoot shot, struct *bomb bombs)
{

}