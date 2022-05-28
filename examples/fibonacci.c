//#include <stdio.h>

#define NUM 10

int fibbonacci(int n) {
   if(n == 0){
      return 0;
   } else if(n == 1) {
      return 1;
   } else {
      return (fibbonacci(n-1) + fibbonacci(n-2));
   }
}

//int main() {
void notmain() {

   //printf("Fibbonacci of %d: " , NUM);
   //printf("%d\n",fibbonacci(NUM));     
   //return 0;       


   int x = fibbonacci(NUM);
}