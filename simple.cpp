#include <iostream>


long long int foo(short int z, int y){
	short int ss = z;
	int yy = ss + y;
 	long long int r = yy * 20;
	return r;
}

int main(){
	long long int res = foo(10, 200);
	std::cout << "Result is " << res << "\n";
	return 0;
}
