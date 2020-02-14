// Copyright - Craciunoiu Cezar 334CA
#ifndef __PHOTO__
#define __PHOTO__

#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

// A pixel is the same for Black&White and Color, but in B&W all colors are
// the same.
struct Pixel {
    float r = 0, g = 0, b = 0;

    Pixel& operator=(const Pixel& other) {
        this->r = other.r;
        this->g = other.g;
        this->b = other.b;
        return *this;
    }

    void setBW(unsigned char newVal) {
        r = g = b = newVal;
    }

    void setCL(unsigned char nR, unsigned char nG, unsigned char nB) {
        r = nR;
        g = nG;
        b = nB;
    }
};

class Photo {
 private:
    bool color;
    std::vector<char> garbageMessage;
    int width, height;
    int maxval;
    std::vector<std::vector<Pixel>*>* matrix;
    int outputSize;

 public:
    Photo() {}

    // The constructor parses the raw information received looking for '\n' and
    // ' ' for the header and, afterwards, iterating through the rest 1 by 1 or
    // 3 by 3 bytes.
    Photo(char* rawInput, int size) {
        outputSize = size;
        color = (rawInput[1] == '6');
        for (int i = 3; rawInput[i] != '\n'; ++i) {
            garbageMessage.push_back(rawInput[i]);
        }

        std::vector<char> widthCif;
        for (int i = 3 + garbageMessage.size() + 1; rawInput[i] != ' '; ++i) {
            widthCif.push_back(rawInput[i]);
        }
        width = atoi(&widthCif[0]);

        std::vector<char> heightCif;
        for (int i = 3 + garbageMessage.size() + 1 + widthCif.size() + 1;
                                                rawInput[i] != '\n'; ++i) {
            heightCif.push_back(rawInput[i]);
        }
        height = atoi(&heightCif[0]);

        std::vector<char> maxvalCif;
        for (int i = 3 + garbageMessage.size() + 1 + widthCif.size() + 1
                       + heightCif.size() + 1; rawInput[i] != '\n'; ++i) {
            maxvalCif.push_back(rawInput[i]);
        }
        maxval = atoi(&maxvalCif[0]);

        int rawMatrixIt = 3 + garbageMessage.size() + 1 + widthCif.size() + 1
                            + heightCif.size() + 1 + maxvalCif.size() + 1;

        Pixel emptyPixel;
        matrix = new std::vector<std::vector<Pixel>*>(height + 2);
        for (int row = 0; row < matrix->size(); ++row) {
            matrix->at(row) = new std::vector<Pixel>(width + 2, emptyPixel);
        }

        const int advance = color ? 3 : 1;
        for (int row = 1; row < height + 1; ++row) {
            for (int col = 1; col < width + 1; ++col) {
                if (color) {
                    matrix->at(row)->at(col).setCL(rawInput[rawMatrixIt + 0],
                        rawInput[rawMatrixIt + 1], rawInput[rawMatrixIt + 2]);
                } else {
                    matrix->at(row)->at(col).setBW(rawInput[rawMatrixIt]);
                }
                rawMatrixIt += advance;
            }
        }
    }

    // Copy assignment for ease of use.
    Photo& operator=(const Photo& other) {
        this->color = other.color;
        this->garbageMessage = other.garbageMessage;
        this->width = other.width;
        this->height = other.height;
        this->maxval = other.maxval;
        this->outputSize = other.outputSize;
        this->matrix = new std::vector<std::vector<Pixel>*>(this->height + 2);
        for (int row = 0; row < this->matrix->size(); ++row) {
            this->matrix->at(row) = new std::vector<Pixel>(this->width + 2);
            for (int col = 1; col < width + 1; ++col) {
                this->matrix->at(row)->at(col) = other.matrix->at(row)->at(col);
            }
        }
        return *this;
    }

    // Copy constructor for ease of use.
    Photo(const Photo& other) {
        this->color = other.color;
        this->garbageMessage = other.garbageMessage;
        this->width = other.width;
        this->height = other.height;
        this->maxval = other.maxval;
        this->outputSize = other.outputSize;
        this->matrix = new std::vector<std::vector<Pixel>*>(this->height + 2);
        for (int row = 0; row < this->matrix->size(); ++row) {
            this->matrix->at(row) = new std::vector<Pixel>(this->width + 2);
            for (int col = 1; col < width + 1; ++col) {
                this->matrix->at(row)->at(col) = other.matrix->at(row)->at(col);
            }
        }
    }

    // Destructor that cleans the allocated pixel matrix.
    ~Photo() {
        for (int row = 0; row < height + 2; ++row) {
            delete matrix->at(row);
        }
        delete matrix;
    }

    // Writes the image in the same structure as the input,
    // skipping the comment.
    void writePhoto(std::ofstream* output) {
        if (output->is_open()) {
            if (color) {
                output->write("P6\n", 3);
            } else {
                output->write("P5\n", 3);
            }
            output->write(std::to_string(width).c_str(),
                            std::to_string(width).length());
            output->write(" ", 1);
            output->write(std::to_string(height).c_str(),
                            std::to_string(height).length());
            output->write("\n", 1);
            output->write(std::to_string(maxval).c_str(),
                            std::to_string(maxval).length());
            output->write("\n", 1);
            for (int row = 1; row < height + 1; ++row) {
                for (int col = 1; col < width + 1; ++col) {
                    Pixel aux = matrix->at(row)->at(col);
                    char transform = aux.r;
                        output->write(&transform, 1);
                    if (color) {
                        transform = aux.g;
                        output->write(&transform, 1);
                        transform = aux.b;
                        output->write(&transform, 1);
                    }
                }
            }
        }
    }

    // Assigns the pointer to the matrix without deleting and allocating
    // new space.
    void moveMatrixes(Photo* other) {
        for (int i = 0; i < matrix->size(); ++i) {
            delete matrix->at(i);
        }
        delete matrix;
        matrix = other->matrix;
        other->matrix = new std::vector<std::vector<Pixel>*>(other->height + 2);
        for (int row = 0; row < other->matrix->size(); ++row) {
            other->matrix->at(row) = new std::vector<Pixel>(other->width + 2);
        }
    }

    // Copies information and creates matrix but ignores matrix information.
    void copyMeta(const Photo& other) {
        this->color = other.color;
        this->garbageMessage = other.garbageMessage;
        this->width = other.width;
        this->height = other.height;
        this->maxval = other.maxval;
        this->outputSize = other.outputSize;
        this->matrix = new std::vector<std::vector<Pixel>*>(this->height + 2);
        for (int row = 0; row < this->matrix->size(); ++row) {
            this->matrix->at(row) = new std::vector<Pixel>(this->width + 2);
        }
    }

    // Return number of pixels.
    int getNrPixels() {
        return width * height;
    }

    // Returns number of lines.
    int getNrLines() {
        return height;
    }

    // Returns number of columns.
    int getNrCols() {
        return width;
    }

    // Returns true if the image is color.
    bool isColor() {
        return color;
    }

    // Returns a pointer to the matrix of pixels.
    std::vector<std::vector<Pixel>*>* getMatrix() {
        return matrix;
    }
};
#endif  // __PHOTO__
