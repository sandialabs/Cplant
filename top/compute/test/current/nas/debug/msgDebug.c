#include <stdio.h>

#define NRECS 60

static int debugData[NRECS][5];

int lastRec = -1;

void
printRec(int recNo)
{
    printf("%3d) %d %d %d %d %d\n", recNo,
      debugData[recNo][0],
      debugData[recNo][1],
      debugData[recNo][2],
      debugData[recNo][3],
      debugData[recNo][4]);
}
extern unsigned int _my_rank;
void
svRec(int a, int b, int c, int d, int e)
{
    lastRec++;
    if (lastRec == NRECS) lastRec = 0;
    debugData[lastRec][0] = (int)(_my_rank);
    debugData[lastRec][1] = (int)(b);
    debugData[lastRec][2] = (int)(c);
    debugData[lastRec][3] = (int)(d);
    debugData[lastRec][4] = (int)(e);
}
void
svrec_(int *a,int *b,int *c,int *d,int *e)
{
    svRec(*a,*b,*c,*d,*e);
}

void 
print_lastRecs(void)
{
int i;

   if (lastRec == -1){
       printf("no rec data\n");
       return;
   }

   for (i=lastRec+1; i<NRECS; i++){
      printRec(i);
   }
   for (i=0; i<=lastRec; i++){
      printRec(i);
   }
   lastRec = -1;
}
void 
print_lastrecs_(void)
{
    print_lastRecs();
}

