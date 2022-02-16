#define DATA_SIZE 5

int array[DATA_SIZE] =
{
    10 , 1000, 10000, 1000, 10000
};

/* EXPECTED RESULT = 10, 100, 100, 10, 1000*/

void notmain() {

	int i;
	
	for (i = 1; i < DATA_SIZE; i++) {
	
		array[i] = array[i] / array[i-1];
	}
}
