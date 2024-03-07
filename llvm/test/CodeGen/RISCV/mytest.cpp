
int foo();

const int Numbers[7] = {5, 20, 70, 164, 320, 25, 555};


long long int  bar(short int x){
	long int y = x;
	int z = 5;
  for(int i = 0; i < 530+x; i++){
    if(x < 120){
      y += x + Numbers[i];
    }else{
      y += -x + 2 * foo() - Numbers[i];
    }
  }    
	return y+z;
}
