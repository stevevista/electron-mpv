#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


void im2col_cpu_ext(const float* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w,
    const int stride_h, const int stride_w,
    float* data_col);

void leaky_activate_array_cpu(float *x, const int n);

void forward_maxpool_op(float *src, float *dst, int size, int w, int h, int out_w, int out_h, int c,
    int pad, int stride);

void matrixmul(int M, int N, int K,
        float *A,
        float *B,
        float *C);


#ifdef __cplusplus
}
#endif
