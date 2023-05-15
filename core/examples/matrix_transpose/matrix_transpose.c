//#include <stdio.h>

#define N 3
#define M 4
int A[3][4] = {{1, 0, 2, 3}, {2, 1, 1, 0}, {-1, -2, -3, -1}};
int B[M][N];

/*
void print_matrix(int *arr, int m, int n)
{
    int i, j;
    for (i = 0; i < m; i++) {
    	for (j = 0; j < n; j++) {
        	printf("%d\t", *((arr+i*n) + j));
    	}
    	printf("\n");
    }
}*/

void matrix_transpose(int A[N][M], int B[M][N]) {
	for (int i = 0; i < M; i++) 
		for (int j=0; j < N; j++)
			B[i][j] = A[j][i];
}

//int main() {
void notmain() {

//	print_matrix((int *)A, 3, 4);
	matrix_transpose(A, B);
//	print_matrix((int *)B, 4, 3);
//	return 0;
}
