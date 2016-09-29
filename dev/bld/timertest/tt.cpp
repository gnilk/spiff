
#include "Timer.h"
using namespace Utils;
void doVertical(int *buffer, int width, int height) {
	for(int x=0;x<width;x++) {
		for(int y=0;y<height;y++) {
			int value = buffer[x+y*height];
			value += (x+y) & 255;
			buffer[x+y*height]=value;
		}
	}
}
void doHorizontal(int *buffer, int width, int height) {
	for(int y=0;y<height;y++) {
		for(int x=0;x<width;x++) {
			int value = buffer[x+y*height];
			value += (x+y) & 255;
			buffer[x+y*height]=value;
		}
	}
}

void bubblesort(int a, int b, int c) {
	#define swap(_a,_b) {int tmp; tmp=_a;_a=_b;_b=tmp; }
	if (a>b) swap(a,b);
	if (a>c) swap(a,c);
	if (b>c) swap(b,c);

	printf("%d,%d,%d\n",a,b,c);
}

void testCache() {
	Timer t;
	int *buffer = (int *)malloc(sizeof(int)*1024*1024);

	double tStart, tEnd;
	tStart = t.Sample();
	for(int i=0;i<256;i++) {
		doVertical(buffer, 1024, 1024);
	}
	tEnd = t.Sample();
	printf("Vertical: %f\n", tEnd - tStart);

	tStart = t.Sample();
	for(int i=0;i<256;i++) {
		doHorizontal(buffer, 1024, 1024);		
	}
	tEnd = t.Sample();
	printf("Horizontal: %f\n", tEnd - tStart);

	free(buffer);	
}
int main (int argc, char **argv) {
	testCache();

	// bubblesort(1,2,3);
	// bubblesort(3,2,1);
	// bubblesort(3,1,2);
	// bubblesort(2,3,1);	


}