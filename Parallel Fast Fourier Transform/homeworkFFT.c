// Copyright Craciunoiu Cezar - 334CA
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <complex.h>

// Structure that contains the parameters from the Rosetta code.
typedef struct {
	double complex* input;
	double complex* output;
	int nrOfElems;
	int threadId;
	int numThreads;
	int step;
} structuredParams;

// The thread function transforms the parameters before every function call and
// calls itself. The threads use the same input and output memory but the zones
// that are accessed are independent from one another so there is no need for
// a mutex.
void* threadFFT(void* var) {
	structuredParams* params = (structuredParams *)var;
	if (params->step < params->nrOfElems) {
		structuredParams newParams;
		newParams.input = params->output;
		newParams.output = params->input;
		newParams.nrOfElems = params->nrOfElems;
		newParams.numThreads = params->numThreads;
		newParams.threadId = params->threadId;
		newParams.step = params->step * 2;
		threadFFT(&newParams);

		newParams.input = params->output + params->step;
		newParams.output = params->input + params->step;
		threadFFT(&newParams);
 
		for (int i = 0; i < params->nrOfElems; i += 2 * params->step) {
			double complex t = cexp(-I * M_PI * i / params->nrOfElems) *
							   params->output[i + params->step];
			params->input[i / 2] = params->output[i] + t;
			params->input[(i + params->nrOfElems)/2] = params->output[i] - t;
		}
	}
	return NULL;
}

// The C function implementation from the Rosetta website.
void sequentialFFT(double complex* input, double complex* output, 
				   int nrOfElems, int step) {
	if (step < nrOfElems) {
		sequentialFFT(output, input, nrOfElems, step * 2);
		sequentialFFT(output + step, input + step, nrOfElems, step * 2);

		for (int i = 0; i < nrOfElems; i += 2 * step) {
			double complex t = cexp(-I * M_PI * i / nrOfElems) * output[i + step];
			input[i / 2] = output[i] + t;
			input[(i + nrOfElems)/2] = output[i] - t;
		}
	}
}

// Function to display the result in the wanted format.
void display(FILE* fileOut, double complex* output, int nrOfElems) {
	fprintf(fileOut, "%d\n", nrOfElems);
	for (int i = 0; i < nrOfElems; ++i) {
		fprintf(fileOut, "%lf %lf\n", creal(output[i]), cimag(output[i]));
	}
}

// In the main function files are opened, memory is allocated and input is
// read. Afterwards, the cases with 1/2/4 threads are treated separately
// using a switch. In the case of 1 thread, the main thread is used instead.
int main(int argc, char * argv[]) {
	if (argc != 4) {
		printf("Wrong Number of parameters, format:\n\
			   ./homeworkFT inputValues.txt outputValues.txt numThreads");
		return -1;
	}

	int numThreads = atoi(argv[3]);
	int nrOfElems = 0;
	FILE* fileIn = fopen(argv[1], "r");
	FILE* fileOut = fopen(argv[2], "w");
	double complex* input = NULL;
	double complex* outputPar = NULL;


	if (fileIn == NULL) {
		printf("Could not open input file...");
		exit(-1);
	}

	if (fileOut == NULL) {
		printf("Could not open output file...");
		exit(-1);
	}

	if(fscanf(fileIn, "%d\n", &nrOfElems) == 0) {
		printf("Could not read number of  elements...");
		exit(-1);
	}

	input = malloc(sizeof(double complex) * nrOfElems);
	outputPar = malloc(sizeof(double complex) * nrOfElems);
	if (input == NULL || outputPar == NULL) {
		printf("Could not allocate space...");
		exit(-1);
	}

	double readingAux;
	for (int i = 0; i < nrOfElems; ++i) {
		if(fscanf(fileIn, "%lf\n", &readingAux) == 0) {
			printf("File reading error...");
			exit(-1);
		}
		input[i] = CMPLX(readingAux, 0);
		outputPar[i] = input[i];
	}

	pthread_t tid[numThreads];
	int step = 1;

	// The paralellisation idea is to do recursion unrolling. More threads mean
	// a start from a deeper point in the recursion. For 1 thread the start is
	// at 0, for 2 - 1 and so on (log2(numThreads) is the depth). The cases
	// are treated and explained separately.
	switch(numThreads) {

		// For 1 thread the Rosetta function is simply run. There is no need
		// to create one more thread for running the code.
		case 1: {
			sequentialFFT(input, outputPar, nrOfElems, step);
			break;
		}

		// For 2 threads the depth will be 1 so the 2 threads will be started
		// with different parameters. The first one will have the parameters
		// for the first left node in the recursion and the second one the
		// forst right node. The threads run and after they finish the last
		// step must be done, meaning running the "for" from the function one
		// last time.
		case 2: {
			structuredParams params[numThreads];
			params[0].input = outputPar;
			params[0].output = input;
			params[0].nrOfElems = nrOfElems;
			params[0].numThreads = numThreads;
			params[0].threadId = 0;
			params[0].step = step * 2;

			params[1].input = outputPar + step;
			params[1].output = input + step;
			params[1].nrOfElems = nrOfElems;
			params[1].numThreads = numThreads;
			params[1].threadId = 1;
			params[1].step = step * 2;

			for(int i = 0; i < numThreads; ++i) {
				pthread_create(&(tid[i]), NULL, threadFFT, &(params[i]));
			}

			for(int i = 0; i < numThreads; ++i) {
				pthread_join(tid[i], NULL);
			}

			for (int i = 0; i < nrOfElems; i += 2 * step) {
				double complex t = cexp(-I * M_PI * i / nrOfElems) *
								   outputPar[i + step];
				input[i / 2]     = outputPar[i] + t;
				input[(i + nrOfElems)/2] = outputPar[i] - t;
			}
			break;
		}

		// The idea with 4 threads stays the same meaning that the depth will
		// be 2 now. The first parameters are obtained by applying the first
		// recursion call twice. The second by applying the first and then the
		// second one. The third by calling the second and then the first and,
		// lastly, the fourth by calling the right one 2 times.
		// The only difference between the threads will be the start in the
		// input and output array.
		// After the threads are done, before the last "for" is run, first,
		// 2 more must execute as the program returns iteratively from
		// recursion. Before every "for" the transformations on input, output
		// and step are reverted.
		case 4: {
			structuredParams params[numThreads];
			params[0].input = input;
			params[0].output = outputPar;
			params[0].nrOfElems = nrOfElems;
			params[0].numThreads = numThreads;
			params[0].threadId = 0;
			params[0].step = step * 2 * 2;

			params[1].input = input + step * 2;
			params[1].output = outputPar + step * 2;
			params[1].nrOfElems = nrOfElems;
			params[1].numThreads = numThreads;
			params[1].threadId = 1;
			params[1].step = step * 2 * 2;

			params[2].input = input + step;
			params[2].output = outputPar + step;
			params[2].nrOfElems = nrOfElems;
			params[2].numThreads = numThreads;
			params[2].threadId = 2;
			params[2].step = step * 2 * 2;

			params[3].input = input + step + step * 2;
			params[3].output = outputPar + step + step * 2;
			params[3].nrOfElems = nrOfElems;
			params[3].numThreads = numThreads;
			params[3].threadId = 3;
			params[3].step = step * 2 * 2;

			for(int i = 0; i < numThreads; ++i) {
				pthread_create(&(tid[i]), NULL, threadFFT, &(params[i]));
			}

			for(int i = 0; i < numThreads; ++i) {
				pthread_join(tid[i], NULL);
			}

			step *= 2;
			for (int i = 0; i < nrOfElems; i += 2 * step) {
				double complex t = cexp(-I * M_PI * i / nrOfElems) *
								   input[i + step];
				outputPar[i / 2]     = input[i] + t;
				outputPar[(i + nrOfElems)/2] = input[i] - t;
			}			

			outputPar += step / 2;
			input += step / 2;
			for (int i = 0; i < nrOfElems; i += 2 * step) {
				double complex t = cexp(-I * M_PI * i / nrOfElems) *
								   input[i + step];
				outputPar[i / 2]     = input[i] + t;
				outputPar[(i + nrOfElems)/2] = input[i] - t;
			}

			step /= 2;
			outputPar -= step;
			input -= step;
			for (int i = 0; i < nrOfElems; i += 2 * step) {
				double complex t = cexp(-I * M_PI * i / nrOfElems) *
								   outputPar[i + step];
				input[i / 2]     = outputPar[i] + t;
				input[(i + nrOfElems)/2] = outputPar[i] - t;
			}
			break;
		}

		default: {
			printf("Incorrect number of threads, only 1/2/4 are available, exiting...");
			exit(-1);
			break;
		}
	}
	display(fileOut, input, nrOfElems);

	free(input);
	free(outputPar);
	fclose(fileIn);
	fclose(fileOut);
	return 0;
}
