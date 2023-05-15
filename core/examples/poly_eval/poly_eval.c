//#include <stdio.h>

#define N 5
int coeffs[N] = {2, 1, 0, -1, -2};
int result;

int poly_eval(int poly[N], int x){ 
	int result;
	for (int i = 0; i < N; i++) {
    	if (i==0) result=poly[0];
		else result = result*x + poly[i]; 
	}
  return result;
}

//int main() {
void notmain() {

	result = poly_eval(coeffs, 2);
	//printf("Res = %d\n", poly_eval(coeffs, 2));
	//return 0;

}
