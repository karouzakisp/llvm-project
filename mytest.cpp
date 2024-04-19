#include <stdio.h>
int foo();
int bar();
short foo_s();
long long int foo_l();

int main(void){
  short i, j, k;
  int sum = 0;
  for(k = 1; k < 50; k++){
    if(k == j + 1 || k == i + 5){
      continue;
    } 
    for(j = 0; j < 100; j++){
      for(i = 0; i < 1000; i++){
        if( i % 2 == 0){
          sum += i && k;
        }else{
          sum -= i && (k-j);
        } 
      }
    }
  }
  printf("Sum is %d\n", sum);
  return 0;
}
