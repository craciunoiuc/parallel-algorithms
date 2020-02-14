// Copyright Craciunoiu Cezar - 334CA
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <pthread.h>

// Uncomment the define to run the sequential DFT with the output to "seq.txt".
// The sequential function is run before the parallel one.
// #define __SEQ__

// Structure used to give parameters to the thread function
typedef struct {
	double complex* input;
	double complex* output;
	int nrOfElems;
	int threadId;
	int numThreads;
} structuredParams;

// Function to display the result in the wanted format.
void display(FILE* fileOut, double complex* output, int nrOfElems) {
	fprintf(fileOut, "%d\n", nrOfElems);
	for (int i = 0; i < nrOfElems; ++i) {
		fprintf(fileOut, "%lf %lf\n", creal(output[i]), cimag(output[i]));
	}
}

#ifdef __SEQ__
// Sequential implementation for calculating the DFT based on the formula in
// the assignment.
void sequentialFT(double complex* input,
				  double complex* output, int nrOfElems) {
	for (int k = 0; k < nrOfElems; ++k) {
		double complex partialSum = 0.0;
		for (int iter = 0; iter < nrOfElems; ++iter) {
			partialSum += input[iter] * cexp(-2 * M_PI * iter * k / nrOfElems * I);
		}
		output[k] = partialSum;
	}
}
#endif

// Thread function to calculate the parallel version of DFT. Like in the labs
// the function uses a start and a stop to iterate and the areas that are
// modified are already mutually exclusive so there is no need for a mutex or
// barrier. The remaining work is given to the last thread in the cases where
// the work can't be devided equally between threads. The algorithm to
// calculate the DFT stays the same in the transition from sequential to
// parallel.
void* parallelFT(void* var) {
	structuredParams* params = (structuredParams *) var;
	const int start = params->nrOfElems/params->numThreads *
					  params->threadId;
	const int stop = params->nrOfElems/params->numThreads *
					 (params->threadId + 1) - 1;

	for (int k = start; k <= stop; ++k) {
		double complex partialSum = 0.0;
		for (int iter = 0; iter < params->nrOfElems; ++iter) {
			partialSum += params->input[iter] *
						  cexp(-2 * M_PI * iter * k / params->nrOfElems * I);
		}
		params->output[k] = partialSum;
	}

	if (params->threadId == params->numThreads - 1 &&
		params->nrOfElems % params->numThreads != 0) {
		for (int k = stop + 1; k < params->nrOfElems; ++k) {
			double complex partialSum = 0.0;
			for (int iter = 0; iter < params->nrOfElems; ++iter) {
				partialSum += params->input[iter] *
							cexp(-2 * M_PI * iter * k / params->nrOfElems * I);
			}
			params->output[k] = partialSum;
		}
	}
	return NULL;
}

// In main the files are opened, the input is read and the threads are started.
// Afterwards the result is written using the display function.
int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("Wrong Number of parameters, format:\n\
			   ./homeworkFT inputValues.txt outputValues.txt numThreads");
		return -1;
	}

	int numThreads = atoi(argv[3]);
	int nrOfElems = 0;
	FILE* fileIn = fopen(argv[1], "r");
	FILE* fileOut = fopen(argv[2], "w");
#ifdef __SEQ__
	FILE* fileOutSeq = fopen("seq.txt", "w");
#endif
	double complex* input = NULL;
#ifdef __SEQ__
	double complex* outputSeq = NULL;
#endif
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
#ifdef __SEQ__
	outputSeq = malloc(sizeof(double complex) * nrOfElems);
#endif
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
	}

#ifdef __SEQ__
	sequentialFT(input, outputSeq, nrOfElems);
	display(fileOutSeq, outputSeq, nrOfElems);
#endif

	// Before giving the parameters to the function they need to be formated
	// in a function.
	structuredParams params[numThreads];
	for(int i = 0; i < numThreads; ++i) {
		params[i].input = input;
		params[i].output = outputPar;
		params[i].nrOfElems = nrOfElems;
		params[i].numThreads = numThreads;
		params[i].threadId = i;
	}

	pthread_t tid[numThreads];
	for(int i = 0; i < numThreads; ++i) {
		pthread_create(&(tid[i]), NULL, parallelFT, &(params[i]));
	}

	for(int i = 0; i < numThreads; ++i) {
		pthread_join(tid[i], NULL);
	}

	display(fileOut, outputPar, nrOfElems);
	
	free(input);
	free(outputPar);
	fclose(fileIn);
	fclose(fileOut);
#ifdef __SEQ__
	free(outputSeq);
	fclose(fileOutSeq);
#endif
	return 0;
}
