// Copyright - Craciunoiu Cezar 334CA
#ifndef __TRANSFORMS__
#define __TRANSFORMS__

// Enum with filters for ease of use
enum Filters {SMOOTH = 0, BLUR = 1, SHARPEN = 2, MEAN = 3, EMBOSS = 4};

class Transforms {
 private:
    // Smooth, Gaussian, Sharpen, Mean, Emboss kernels
    float operations[5][3][3] = {{{1.0/9.0, 1.0/9.0, 1.0/9.0}, {1.0/9.0, 1.0/9.0, 1.0/9.0}, {1.0/9.0, 1.0/9.0, 1.0/9.0}},
                                   {{1.0/16.0, 2.0/16.0, 1.0/16.0}, {2.0/16.0, 4.0/16.0, 2.0/16.0}, {1.0/16.0, 2.0/16.0, 1.0/16.0}},
                                   {{0.0/3.0, -2.0/3.0, 0.0/3.0}, {-2.0/3.0, 11.0/3.0, -2.0/3.0}, {0.0/3.0, -2.0/3.0, 0.0/3.0}},
                                   {{-1.0, -1.0, -1.0}, {-1.0, 9.0, -1.0}, {-1.0, -1.0, -1.0}},
                                   {{0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, -1.0, 0.0}}};



 public:
    Transforms() {}
    ~Transforms() {}

    // The convolution product is applied on each matrix piece received
    float convolutionProduct(float matrixPiece[][3][3], Filters op) {
        float result = 0.0f;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result += (operations[op][2 - i][2 - j] * (*matrixPiece)[i][j]);
            }
        }
        return result;
    }
};
#endif  // __TRANSFORMS__
