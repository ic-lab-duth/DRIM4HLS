#include <stdlib.h>

int ag = 200, bg = 300, cg = 400;

int main()
{
	int a = 5, b = 10, c = 100;
	/* c = 15 */
	c = a + b;
	/* b = 20 */
	b = a + c;
	/* a= 35 */
	a = b + c;
	/* ag = 35 */
	ag = a;
	/* bg = 20 */
	bg = b;
	/* cg = 15 */
	cg = c;

	return 0;
}
