#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"

int N = 640000;                                                                                                                                                                                         
int doPrint = 0; 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MPI CODE
//
// cpu holds my processor number, cpu=0 is master, rest are slaves
// numcpus is the total number of processors
int cpu, numcpus;
// Normal C function to square root values
void normal(float* a, int N)
{
  int i;
  for (i = 0; i < N; ++i)
    a[i] = sqrt(a[i]);
}

// MPI function to square root values
void mpi(float* a, int N)
{
   int i, slave;
   MPI_Status status;

   int numeach = N/numcpus;

   ////////////////////////////////////////////////////////////////////////////////////////
   // I AM THE MASTER
   if (cpu == 0) {
      for (slave = 1; slave < numcpus; slave++)
         MPI_Send(&a[numeach*slave], numeach, MPI_FLOAT, slave, 1, MPI_COMM_WORLD);

      for (i = 0; i < numeach; i++)
         a[i] = sqrt(a[i]);

      
      for (slave = 1; slave < numcpus; slave++) 
         MPI_Recv(&a[numeach*slave], numeach, MPI_FLOAT, slave, 2, MPI_COMM_WORLD, &status);
   }
   /////////////////////////////////////////////////////////////////////////////////////////
   // I AM A SLAVE
   else {
      float data[numeach];
      MPI_Recv(&data[0], numeach, MPI_FLOAT, 0, 1, MPI_COMM_WORLD, &status);
      for (i = 0; i < numeach; i++)
         data[i] = sqrt(data[i]);
      MPI_Send(&data[0], numeach, MPI_FLOAT, 0, 2, MPI_COMM_WORLD);
   }
   /////////////////////////////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPER CODE TO INITIALIZE, PRINT AND TIME
struct timeval start, end;
void initialize(float *a, int N) {
  int i;
  for (i = 0; i < N; ++i) { 
    a[i] = pow(rand() % 10, 2); 
  }                                                                                                                                                                                       
}

void print(float* a, int N) {
   if (doPrint) {
   int i;
   for (i = 0; i < N; ++i)
      printf("%f ", a[i]);
   printf("\n");
   }
}  

void starttime() {
  gettimeofday( &start, 0 );
}

void endtime(const char* c) {
   gettimeofday( &end, 0 );
   double elapsed = ( end.tv_sec - start.tv_sec ) * 1000.0 + ( end.tv_usec - start.tv_usec ) / 1000.0;
   printf("%s: %f ms\n", c, elapsed); 
}

void init(float* a, int N, const char* c) {
  printf("***************** %s **********************\n", c);
  initialize(a, N); 
  print(a, N);
  starttime();
}

void finish(float* a, int N, const char* c) {
  endtime(c);
  print(a, N);
  printf("***************************************************\n");
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv)                                                                                                                                                                                  
{                                                                                                                                                                                                                

   ///////////////////////////////////////////////////////////
   // MPI Starter Code
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &cpu);
   MPI_Comm_size(MPI_COMM_WORLD, &numcpus);
   //////////////////////////////////////////////////////////

   float a[N];
   
   //////////////////////////////////////////////////////////////////////////////////
   // ONLY THE MASTER: Initialize the array, run the normal for loop, initialize MPI
   if (cpu == 0) {
      // Master code
      initialize(a, N);
      // Test 1: Sequential For Loop
      init(a, N, "Normal");
      normal(a, N); 
      finish(a, N, "Normal");
      // Test 2: MPI
      init(a, N, "MPI");
   }
   /////////////////////////////////////////////////////////////////////////////////

   /////////////////////////////////////////////////////////////////////////////////
   // EVERYBODY: SQUARE ROOT THEIR VALUES
   mpi(a, N);
   ////////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////////
   // ONLY THE MASTER: PRINT TIME FOR MPI
   if (cpu == 0) 
      finish(a, N, "MPI");
   ////////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////////
   // MPI Finish Code
   MPI_Finalize();
   ////////////////////////////////////////////////////////////////////////////////

 return 0;
}


