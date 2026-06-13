#include <stdio.h>

#define N 100

void _ZZN4Foam13distributions12unintegrable24interpolateIntegrateXPowERKNS_5FieldIfEEiS5_S5_ENK3$_0clEf__daisy_unintegrable(float* ptr) {
    for (int i = 0; i < N; i++) {
        ptr[i] = ptr[i] * 2.0f;
    }
}

int main() {
    float data[N];
    for (int i = 0; i < N; i++) {
        data[i] = (float) i;
    }

    _ZZN4Foam13distributions12unintegrable24interpolateIntegrateXPowERKNS_5FieldIfEEiS5_S5_ENK3$_0clEf__daisy_unintegrable(data);

    if (data[10] != 20.0f) {
        printf("Error: data[10] = %f, expected 20.0\n", data[10]);
        return 1;
    } else {
        printf("Success: data[10] = %f\n", data[10]);
    }

    return 0;
}