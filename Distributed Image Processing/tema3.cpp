// Copyright - Craciunoiu Cezar 334CA

#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <vector>
#include "mpi.h"
#include "./transforms.h"
#include "./photo.h"

using namespace std;

int main(int argc, char *argv[]) {
    ifstream fileRead;
    ofstream fileWrite;
    int  numtasks, rank;
    vector<Filters> filters;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // Parse input and add filters into a list
    for (int i = 3; i < argc; ++i) {
        if (!strcmp(argv[i], "smooth")) {
            filters.push_back(SMOOTH);
            continue;
        }
        if (!strcmp(argv[i], "blur")) {
            filters.push_back(BLUR);
            continue;
        }
        if (!strcmp(argv[i], "sharpen")) {
            filters.push_back(SHARPEN);
            continue;
        }
        if (!strcmp(argv[i], "mean")) {
            filters.push_back(MEAN);
            continue;
        }
        if (!strcmp(argv[i], "emboss")) {
            filters.push_back(EMBOSS);
            continue;
        }
    }

    char* rawInput;
    int inputSize;

    // Master process opens both files and reads the image as raw input into
    // memory.
    if (rank == 0) {
        fileRead.open(argv[1], ios::in | ios::binary);
        fileWrite.open(argv[2], ios::out | ios::binary);
        if (fileRead.is_open()) {
            fileRead.seekg(0, ios::end);
            inputSize = fileRead.tellg();
            fileRead.seekg(0, std::ios_base::beg);
            rawInput = new char[inputSize];
            fileRead.read(rawInput, inputSize);
        }
    }

    // The master process sends the number of bytes it read and, afterwards,
    // the information it read from the file.
    MPI_Bcast(&inputSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        rawInput = new char[inputSize];
    }
    MPI_Bcast(rawInput, inputSize, MPI_CHAR, 0, MPI_COMM_WORLD);

    // The raw information is parsed and formatted into memory.
    Photo photo(rawInput, inputSize);
    Photo newPhoto;
    newPhoto.copyMeta(photo);
    Transforms processor;

    // Values for each process to start from and stop to.
    const int start = (photo.getNrLines())/numtasks * rank;
    const int stop  = fmin(((float) photo.getNrLines())/numtasks * 
                      (rank + 1), photo.getNrLines());

    int nrCrt = 0;

    // The outer for iterates through all filters.
    for (auto& filter : filters) {
        nrCrt++;
        auto pixelsMatrix = photo.getMatrix();
        auto pixelsNewMatrix = newPhoto.getMatrix();

        // The middle for iterates through lines between the limits given.
        // This for is split for paralelisation.
        for (int row = start + 1; row < stop + 1; ++row) {
            // The inner for iterates through cols for each line
            for (int col = 1; col < photo.getNrCols() + 1; ++col) {
                Pixel extractedPixel[3][3] = {{pixelsMatrix->at(row - 1)->at(col - 1), pixelsMatrix->at(row - 1)->at(col), pixelsMatrix->at(row - 1)->at(col + 1)},
                                              {pixelsMatrix->at(row)->at(col - 1), pixelsMatrix->at(row)->at(col), pixelsMatrix->at(row)->at(col + 1)},
                                              {pixelsMatrix->at(row + 1)->at(col - 1), pixelsMatrix->at(row + 1)->at(col), pixelsMatrix->at(row + 1)->at(col + 1)}};
                float extractedRed[3][3], extractedGreen[3][3], extractedBlue[3][3];
                // The extracted Pixel is split into 3 double-arrays to be
                // passed to the convolution product function.
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        extractedRed[i][j]   = extractedPixel[i][j].r;
                        if (photo.isColor()) {
                            extractedGreen[i][j] = extractedPixel[i][j].g;
                            extractedBlue[i][j]  = extractedPixel[i][j].b;
                        }
                    }
                }

                // Each result from the function is "clamped" to fit in the
                // [0,255] interval and then set in the resulting matrix.
                float resultR, resultG, resultB;
                resultR = processor.convolutionProduct(&extractedRed, filter);
                if (resultR > 255) {
                    resultR = 255;
                }
                if (resultR < 0) {
                    resultR = 0;
                }
                if (photo.isColor()) {
                    resultG = processor.convolutionProduct(&extractedGreen,
                                                            filter);
                    if (resultG > 255) {
                        resultG = 255;
                    }
                    if (resultG < 0) {
                        resultG = 0;
                    }
                    resultB = processor.convolutionProduct(&extractedBlue,
                                                            filter);
                    if (resultB > 255) {
                        resultB = 255;
                    }
                    if (resultB < 0) {
                        resultB = 0;
                    }
                    pixelsNewMatrix->at(row)->at(col).setCL(resultR, resultG,
                                                            resultB);
                } else {
                    pixelsNewMatrix->at(row)->at(col).setBW(resultR);
                }
            }
        }

        // The master process iterates through each other process and receives
        // one line at the time. Each element is then put in the new matrix.
        if (rank == 0) {
            Pixel buffer[pixelsNewMatrix->at(1)->size()];
            for (int task = 1; task < numtasks; ++task) {
                const int startTask = (photo.getNrLines())/numtasks * task;
                const int stopTask  = fmin(((float) photo.getNrLines()) /
                                            numtasks * (task + 1),
                                            photo.getNrLines());
                for (int row = startTask + 1; row < stopTask + 1; ++row) {
                    MPI_Recv((char *) buffer, sizeof(Pixel) *
                                pixelsNewMatrix->at(row)->size(), MPI_CHAR,
                                task, MPI_ANY_TAG, MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE);

                    for (int col = 1; col < pixelsNewMatrix->at(1)->size() - 1;
                                                                    ++col) {
                        pixelsNewMatrix->at(row)->at(col) = buffer[col];
                    }
                }
            }
        } else {
            // The other processes send each line they filtered to the
            // master process.
            for (int row = start + 1; row < stop + 1; ++row) {
                MPI_Send((char *) (&(pixelsNewMatrix->at(row)->at(0))),
                            sizeof(Pixel) * pixelsNewMatrix->at(row)->size(),
                            MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }

        // After the master process assembled the resulting photo it
        // sends it to the other processes to begin work on another filter.
        if (nrCrt < filters.size()) {
            for (int row = 1; row < photo.getNrLines() + 1; ++row) {
                MPI_Bcast((char *) (&(pixelsNewMatrix->at(row)->at(0))),
                            sizeof(Pixel) * pixelsNewMatrix->at(row)->size(),
                            MPI_CHAR, 0, MPI_COMM_WORLD);
            }
            photo.moveMatrixes(&newPhoto);
        }
    }

    // In the end, master writes the output to the wanted file.
    if (rank == 0) {
        newPhoto.writePhoto(&fileWrite);
        fileWrite.close();
        fileRead.close();
    }
    delete rawInput;
    MPI_Finalize();
    return 0;
}
