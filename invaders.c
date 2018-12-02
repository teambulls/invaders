/* Space Invaders
   Timothy Pettit
   
   This program is modeled clock2 the 80's arcade classic Space Invaders. A "tank" moves across
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
void mpi_aliens(struct options *settings,struct alien* aliens, struct player *tank,
                  struct shoot* shot, struct bomb* bombs, int *current_bombs,
                  int *cageExists, int loops);
void init(struct options* settings, struct player* tank, struct alien* aliens, 
            struct shoot* shot, struct bomb* bomb);
void move_bombs(struct bomb* bomb, int loops, int bomb_speed, int *current_bombs);
void move_shots(struct shoot* shot, struct alien* aliens, int loops, struct options* settings, 
                    int *current_shots, int *current_aliens, int *score);
int win_or_lose(struct alien* aliens, struct bomb* bomb, struct player* tank, int current_aliens);
MPI_Datatype struct_datatype(struct alien *my_alien);

int cpu, numcpus;

/* The main function handles user input, the game visuals, and checks for win/loss conditions */
int main(int argc, char **argv) {

    /// - MPI Starter - ////////////////////////////////////////////////////////// 
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &cpu);
    MPI_Comm_size(MPI_COMM_WORLD, &numcpus);
    //////////////////////////////////////////////////////////

    struct player tank;
    struct alien aliens[ALIENS];
    struct shoot shot[3];
    struct bomb bomb[MAX_BOMBS];
    struct options settings;
    unsigned int input, loops=0, i=0, j=0, current_shots=0, current_bombs=0, current_aliens=ALIENS;
    int cageExists = 0;
    int random=0, score=0, win=-1;
    char tellscore[30];

    //Created by us, improves code readability by putting all 
    //the initialization procedures separately. 
    
    if (cpu == 0) {
        init(&settings, &tank, &aliens, &shot, &bomb);
    }
    
    while(1) 
    {
        if (cpu == 0) {
            // Show score 
            sprintf(tellscore, "%d", score);
            move(0,8);
            addstr(tellscore);
            
            // Show number of aliens left
            sprintf(tellscore, "Aliens remaining: %d", current_aliens);
            move(0, 20);
            addstr(tellscore);
            
            // Move tank
            move(tank.r,tank.c);
            addch(tank.ch);
            
            //Moves the bombs down the column they're currently in,
            //at the speed set in the settings, and deactivates them once they
            //go under the player row.
            move_bombs(&bomb, loops, settings.bombs, &current_bombs);
            
            //Moves shots upwards and deletes them when they hit an alien or they go out of bounds.
            move_shots(&shot, &aliens, loops, &settings, &current_shots, &current_aliens, &score);
        }

        //Call mpi_aliens which takes care of parallelization
        mpi_aliens(&settings, &aliens, &tank, &shot, &bomb, &current_bombs, &cageExists, loops);

        if (cpu == 0) {
            /* See if game has been won or lost*/
            if (current_aliens == 0) {
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
            else if (input == ' ' && current_shots < 3) {
                for (i=0; i<3; ++i) {
                    if (shot[i].active == 0) {
                    shot[i].active = 1;
                    ++current_shots;
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
   }

    MPI_Finalize(); 
    if (cpu == 0) {
        gameover(win);
        endwin();
    }
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

//We should probably check all the variables we're passing through
void mpi_aliens(struct options *settings, struct alien *aliens, struct player *tank,
                  struct shoot *shot, struct bomb *bombs, int *current_bombs, 
                  int *cageExists, int loops) 
{
    unsigned int i = 0, j = 0;
    MPI_Status status;
    //This should be done once in the main, then passed to this method
    //through a pointer.
    //Creates a new MPI datatype based on the alien struct.
    MPI_Datatype MPI_ALIENS_TYPE = struct_datatype(&aliens[0]);

    if (cpu == 0)   //Master
    {
        for (i = 0; i < ALIENS; i++)
        {
            if (aliens[i].alive == 1)
            {
                move(aliens[i].pr,aliens[i].pc);
                addch(' ');
                
                move(aliens[i].r,aliens[i].c);
                addch(aliens[i].ch);
                
                aliens[i].pr = aliens[i].r;
                aliens[i].pc = aliens[i].c;
            }
        }

        aliens[0].time++;
        if (loops % 250 == 0 && loops != 0) 
        {
            ++aliens[0].r;
        }

        for (int slave = 1; slave < ALIENS; slave++)
        {
            MPI_Send(&aliens[slave], 1, MPI_ALIENS_TYPE, slave, 1, MPI_COMM_WORLD);
        }

        //Alien logic
        if (aliens[0].alive == 1) 
        {
            int random = rand() % 200;
            
            if (random < aliens[0].time) 
            {                
                if (aliens[0].behavior == 3) 
                {
                    random = rand() % 3;
                    cageExists = 0;
                } 
                else 
                {  
                    if (cageExists) 
                    {
                        random = rand() % 3;
                    } 
                    else 
                    {
                        random = rand() % 4;
                        if (random == 3) 
                        {
                            cageExists = 1;
                        }
                    }
                }
                aliens[0].behavior = random;
            }

            if (aliens[0].behavior == 0)//Wander around the screen.
            {    
                aliens[0].ch = 'W';
                if (aliens[0].time % 15 == 0 && rand() % 4 == 0) 
                {
                    if (aliens[0].direction == 'l') 
                    {
                        aliens[0].direction = 'r';
                    } 
                    else 
                    {
                        aliens[0].direction = 'l';
                    }
                }

                /*set alien next position*/
                if (aliens[0].direction == 'l') 
                {
                    if (aliens[0].c != 0) 
                    {
                        --aliens[0].c;
                    } 
                    else 
                    {
                        ++aliens[0].c;
                        aliens[0].direction = 'r';
                    }
                }
                else if (aliens[0].direction == 'r') 
                {
                    if (aliens[0].c != COLS - 2) 
                    {
                        ++aliens[0].c;
                    } 
                    else 
                    {
                        --aliens[0].c;
                        aliens[0].direction = 'l';
                    }
                }
                //Randomly decides to drop a bomb.
                if ((settings->bombchance - random) >= 0 && current_bombs < MAX_BOMBS) 
                {
                    for (j=0; j<MAX_BOMBS; ++j) 
                    {
                        if (bombs[j].active == 0) 
                        {
                            bombs[j].active = 1;
                            ++current_bombs;
                            bombs[j].r = aliens[0].r + 1;
                            bombs[j].c = aliens[0].c;
                            break;
                        }
                    }
                }
            }
            else if (aliens[0].behavior == 1) //Follow the player.
            {     
                aliens[0].ch == 'F';
                if (aliens[0].c < tank->c)
                        ++aliens[0].c;
                else if (aliens[0].c > tank->c)
                        --aliens[0].c;
                //Drops bombs when the player and the alien are in nearby columns
                if (abs(aliens[0].c - tank->c) < 4)
                {
                        for (j=0; j<MAX_BOMBS; ++j) {
                            if (bombs[j].active == 0) {
                                    bombs[j].active = 1;
                                    ++current_bombs;
                                    bombs[j].r = aliens[0].r + 1;
                                    bombs[j].c = aliens[0].c;
                                    break;
                            }
                        }
                }
            } 
            else if (aliens[0].behavior == 2) //Dodge tank shots
            {      
                aliens[0].ch = 'D';
                int dodged = 0;
                for (j=0; j<3; ++j) 
                {  
                    if (shot[j].active == 1 && abs(shot[j].c - aliens[0].c) <= 1 && shot[j].r > aliens[0].r && shot[j].r - aliens[0].r <= 5 && dodged == 0) 
                    {     
                        random = rand() % 2;
                        if (aliens[0].c >= COLS - 3) 
                        {
                                aliens[0].c -= 2;
                                aliens[0].direction = 'l';
                        } 
                        else if (aliens[0].c <= 1) 
                        {
                                aliens[0].c += 2;
                                aliens[0].direction = 'r';
                        } 
                        else 
                        { 
                            if (random == 0) 
                            {
                                aliens[0].c += 2;
                                aliens[0].direction = 'r';
                            } 
                            else 
                            {
                                aliens[0].c -= 2;
                                aliens[0].direction = 'l';
                            }
                        }
                        dodged = 1;
                    }          
                }
            } 
            else if (aliens[0].behavior == 3) //Wall trap the player.
            {
                aliens[0].ch = 'T';
                /* Check if alien should drop bomb */
                if ((settings->bombchance - (random/2)) >= 0 && current_bombs < MAX_BOMBS) {
                        for (j=0; j<MAX_BOMBS; ++j) 
                        {
                            if (bombs[j].active == 0) 
                            {
                                bombs[j].active = 1;
                                ++current_bombs;
                                bombs[j].r = aliens[0].r + 1;
                                bombs[j].c = aliens[0].c;
                                break;
                            }
                        }
                }
            }      
        }
        aliens[0].time++;
        if (loops % 250 == 0 && loops != 0) 
        {
            ++aliens[0].r;
        }

        for (int slave = 1; slave < ALIENS; slave++)   
        {
            MPI_Recv(&aliens[slave], 1, MPI_ALIENS_TYPE, slave, 2, MPI_COMM_WORLD, &status);
        }
    }
    else    //Slave
    {
        struct alien my_alien;
        MPI_Recv(&my_alien, 1, MPI_ALIENS_TYPE, 0, 1, MPI_COMM_WORLD, &status);

        //Alien logic
        if (my_alien.alive == 1) 
        {
            int random = rand() % 200;
            
            if (random < my_alien.time) 
            {                
                if (my_alien.behavior == 3) 
                {
                    random = rand() % 3;
                    cageExists = 0;
                } 
                else 
                {  
                    if (cageExists) 
                    {
                        random = rand() % 3;
                    } 
                    else 
                    {
                        random = rand() % 4;
                        if (random == 3) 
                        {
                            cageExists = 1;
                        }
                    }
                }
                my_alien.behavior = random;
            }

            if (my_alien.behavior == 0)//Wander around the screen.
            {    
                my_alien.ch = 'W';
                if (my_alien.time % 15 == 0 && rand() % 4 == 0) 
                {
                    if (my_alien.direction == 'l') 
                    {
                        my_alien.direction = 'r';
                    } 
                    else 
                    {
                        my_alien.direction = 'l';
                    }
                }

                /*set alien next position*/
                if (my_alien.direction == 'l') 
                {
                    if (my_alien.c != 0) 
                    {
                        --my_alien.c;
                    } 
                    else 
                    {
                        ++my_alien.c;
                        my_alien.direction = 'r';
                    }
                }
                else if (my_alien.direction == 'r') 
                {
                    if (my_alien.c != COLS - 2) 
                    {
                        ++my_alien.c;
                    } 
                    else 
                    {
                        --my_alien.c;
                        my_alien.direction = 'l';
                    }
                }
                //Randomly decides to drop a bomb.
                if ((settings->bombchance - random) >= 0 && current_bombs < MAX_BOMBS) 
                {
                    for (j=0; j<MAX_BOMBS; ++j) 
                    {
                        if (bombs[j].active == 0) 
                        {
                            bombs[j].active = 1;
                            ++current_bombs;
                            bombs[j].r = my_alien.r + 1;
                            bombs[j].c = my_alien.c;
                            break;
                        }
                    }
                }
            }
            else if (my_alien.behavior == 1) //Follow the player.
            {     
                my_alien.ch == 'F';
                if (my_alien.c < tank->c)
                        ++my_alien.c;
                else if (my_alien.c > tank->c)
                        --my_alien.c;
                //Drops bombs when the player and the alien are in nearby columns
                if (abs(my_alien.c - tank->c) < 4)
                {
                        for (j=0; j<MAX_BOMBS; ++j) {
                            if (bombs[j].active == 0) {
                                    bombs[j].active = 1;
                                    ++current_bombs;
                                    bombs[j].r = my_alien.r + 1;
                                    bombs[j].c = my_alien.c;
                                    break;
                            }
                        }
                }
            } 
            else if (my_alien.behavior == 2) //Dodge tank shots
            {      
                my_alien.ch = 'D';
                int dodged = 0;
                for (j=0; j<3; ++j) 
                {  
                    if (shot[j].active == 1 && abs(shot[j].c - my_alien.c) <= 1 && shot[j].r > my_alien.r && shot[j].r - my_alien.r <= 5 && dodged == 0) 
                    {     
                        random = rand() % 2;
                        if (my_alien.c >= COLS - 3) 
                        {
                                my_alien.c -= 2;
                                my_alien.direction = 'l';
                        } 
                        else if (my_alien.c <= 1) 
                        {
                                my_alien.c += 2;
                                my_alien.direction = 'r';
                        } 
                        else 
                        { 
                            if (random == 0) 
                            {
                                my_alien.c += 2;
                                my_alien.direction = 'r';
                            } 
                            else 
                            {
                                my_alien.c -= 2;
                                my_alien.direction = 'l';
                            }
                        }
                        dodged = 1;
                    }          
                }
            } 
            else if (my_alien.behavior == 3) //Wall trap the player.
            {
                my_alien.ch = 'T';
                /* Check if alien should drop bomb */
                if ((settings->bombchance - (random/2)) >= 0 && current_bombs < MAX_BOMBS) {
                        for (j=0; j<MAX_BOMBS; ++j) 
                        {
                            if (bombs[j].active == 0) 
                            {
                                bombs[j].active = 1;
                                ++current_bombs;
                                bombs[j].r = my_alien.r + 1;
                                bombs[j].c = my_alien.c;
                                break;
                            }
                        }
                }
            }      
        } 
        my_alien.time++;
        if (loops % 250 == 0 && loops != 0) 
        {
            ++my_alien.r;
        }
        
        MPI_Send(&my_alien, 1, MPI_ALIENS_TYPE, 0, 2, MPI_COMM_WORLD);
    }   
    
}

//Creates a new MPI_Datatype based around our alien struct to be able
//to cleanly send it to each slave.
MPI_Datatype struct_datatype(struct alien *my_alien)
{
    MPI_Datatype new_struct_datatype;
    int struct_length = 2; //One for the integers and one for the chars
    int block_lengths[struct_length];
    MPI_Datatype types[struct_length];
    MPI_Aint displacements[struct_length];

    //We set how far each component is from the struct
    //int block -> r, c, pr, pc, alive, etc....
    block_lengths[0] = 7;
    types[0] = MPI_INT;
    displacements[0] = (size_t)&(my_alien->r) - (size_t)&my_alien;

    //char block -> direction, ch
    block_lengths[1] = 2;
    types[1] = MPI_CHAR;
    displacements[1] = (size_t)&(my_alien->direction) - (size_t)&my_alien;

    //We actually create the structure
    MPI_Type_create_struct(struct_length, block_lengths, displacements, types, &new_struct_datatype);
    MPI_Type_commit(&new_struct_datatype);
    return new_struct_datatype;
}

//Cleans up the beginning of the main method, making it easier to read.
void init(struct options* settings, struct player* tank, struct alien* aliens, 
            struct shoot* shot, struct bomb* bomb) {

   unsigned int i = 0, j = 0;
   
    //Ncurses initialization procedures
    initscr();
    clear();
    noecho();
    cbreak();
    nodelay(stdscr,1);
    keypad(stdscr,1);
    srand(time(NULL)); 

    /* Set default settings */
    settings->overall = 15000;
    settings->alien = 12;
    settings->shots = 3;
    settings->bombs = 10;
    settings->bombchance = 5;

    /* Set tank settings */
    tank->r = LINES - 1;
    tank->c = COLS / 2;
    tank->ch = '^';

    /* Set aliens settings */
    unsigned int a = 0, random = 0;
    for ( i = 0; i < ALIEN_ROWS; i++) {
        for ( j = 0; j < ALIEN_COLUMNS; j++) {
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
    for ( i=0; i<3; ++i) {
        shot[i].active = 0;
        shot[i].ch = '*';
    }

    /* Set bomb settings */
    for ( i=0; i<MAX_BOMBS; ++i) {
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
}

void move_bombs(struct bomb *bomb, int loops, int bomb_speed, int *current_bombs)
{
   
   unsigned int i = 0;
    /* Move bombs */
      if (loops % bomb_speed == 0)
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
               --current_bombs;
               move(bomb[i].r - 1,bomb[i].c);
               addch(' ');
            }
         }
      }
}

void move_shots(struct shoot* shot, struct alien *aliens, int loops, struct options* settings, 
                    int *current_shots, int *current_aliens, int *score)
{
   
   unsigned int i = 0, j = 0;
    /* Move shots */
      if (loops % settings->shots == 0)
      for (i=0; i<3; ++i) 
      {
         if (shot[i].active == 1) 
         {
            if (shot[i].r > 0) 
            {
               if (shot[i].r < LINES - 2) 
               {
                  move(shot[i].r + 1,shot[i].c);
                  addch(' ');
               }
               
               for (j=0; j<ALIENS; ++j) 
               {
                  if (aliens[j].alive == 1 && aliens[j].r == shot[i].r && aliens[j].pc == shot[i].c) 
                  {
                     score += 20;
                     aliens[j].alive = 0;
                     shot[i].active = 0;
                     --*current_shots;
                     --*current_aliens;
                     move(aliens[j].pr,aliens[j].pc);
                     addch(' ');
                     break;
                  }
               }
               
               if (shot[i].active == 1) 
               {
                  move(shot[i].r,shot[i].c);
                  addch(shot[i].ch);
                  
                  --shot[i].r;
               }
            } 
            else 
            {
               shot[i].active = 0;
               --*current_shots;
               move(shot[i].r + 1,shot[i].c);
               addch(' ');
            }
         }
      }  
}
