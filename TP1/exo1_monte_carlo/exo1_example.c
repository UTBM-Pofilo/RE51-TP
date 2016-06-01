 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>



int main(int argc, char ** argv) {
	if(argc < 2) {
		printf("Usage: %s <nb_iter>\n", argv[0]);
		return 1;
	}
	
	int niter = atoi(argv[1]);

	srand(time(NULL));
	int count = 0;
	int i;
	for(i = 0; i < niter; ++i) {
		double x = (double)rand()/RAND_MAX;
		double y = (double)rand()/RAND_MAX;
		double z = x*x+y*y;
		if (z<=1) ++count; /* Dans le cercle */
	}

	double pi = (double)count/niter*4;
	printf("nb essais= %d , estimation de Pi %g \n",niter,pi);

	return 0;
}
