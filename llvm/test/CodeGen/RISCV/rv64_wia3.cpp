#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

int main(void){
  int a, b, c;
  int16_t bb;

  srand(time(NULL));
  a = rand() % 1000;
  b = rand() % 1000;

  c = a ^ b;
  bb = c & 24;
  return bb;
}

