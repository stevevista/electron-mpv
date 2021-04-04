#include "gemm.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <intrin.h>
#endif
#if defined(_OPENMP)
#include <omp.h>
#endif
//#define USE_EIGEN
//#define DISABLE_AVX
#ifdef USE_EIGEN
#include <Eigen/Core>
#endif

#define TILE_M 4 // 4 ops
#define TILE_N 16 // AVX2 = 2 ops * 8 floats
#define TILE_K 16 // loop

// Function uses casting from int to unsigned to compare if value of
// parameter a is greater or equal to zero and lower than value of
// parameter b. The b parameter is of type signed and is always positive,
// therefore its value is always lower than 0x800... where casting
// negative value of a parameter converts it to value higher than 0x800...
// The casting allows to use one condition instead of two.
inline static int is_a_ge_zero_and_a_lt_b(int a, int b) {
    return (unsigned)(a) < (unsigned)(b);
}

// https://github.com/BVLC/caffe/blob/master/src/caffe/util/im2col.cpp
void im2col_cpu_ext(const float* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w,
    const int stride_h, const int stride_w,
    float* data_col)
{
    const int output_h = (height + 2 * pad_h -
        ((kernel_h - 1) + 1)) / stride_h + 1;
    const int output_w = (width + 2 * pad_w -
        ((kernel_w - 1) + 1)) / stride_w + 1;
    const int channel_size = height * width;
    int channel, kernel_row, kernel_col, output_rows, output_col;
    for (channel = channels; channel--; data_im += channel_size) {
        for (kernel_row = 0; kernel_row < kernel_h; kernel_row++) {
            for (kernel_col = 0; kernel_col < kernel_w; kernel_col++) {
                int input_row = -pad_h + kernel_row;
                for (output_rows = output_h; output_rows; output_rows--) {
                    if (!is_a_ge_zero_and_a_lt_b(input_row, height)) {
                        for (output_col = output_w; output_col; output_col--) {
                            *(data_col++) = 0;
                        }
                    }
                    else {
                        int input_col = -pad_w + kernel_col;
                        for (output_col = output_w; output_col; output_col--) {
                            if (is_a_ge_zero_and_a_lt_b(input_col, width)) {
                                *(data_col++) = data_im[input_row * width + input_col];
                            }
                            else {
                                *(data_col++) = 0;
                            }
                            input_col += stride_w;
                        }
                    }
                    input_row += stride_h;
                }
            }
        }
    }
}

//----------------------------

//  Misc.
static int HW_MMX, HW_x64, HW_RDRAND, HW_BMI1, HW_BMI2, HW_ADX, HW_PREFETCHWT1;
static int HW_ABM;      // Advanced Bit Manipulation

//  SIMD: 128-bit
static int HW_SSE, HW_SSE2, HW_SSE3, HW_SSSE3, HW_SSE41, HW_SSE42, HW_SSE4a, HW_AES, HW_SHA;

//  SIMD: 256-bit
static int HW_AVX = 0, HW_XOP = 0, HW_FMA3 = 0, HW_FMA4 = 0, HW_AVX2 = 0;

#if ((defined(__AVX__) && defined(__x86_64__)) || (defined(_WIN64) && !defined(__MINGW32__))) && !defined(DISABLE_AVX)

#if (defined(_WIN64) && !defined(__MINGW64__))
#include <intrin.h>
#include <ammintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

#else    // Linux GCC/Clang
#include <x86intrin.h>
#include <ammintrin.h>
#include <immintrin.h>
#include <smmintrin.h>
#include <cpuid.h>

void asm_cpuid(uint32_t* abcd, uint32_t eax)
{
    uint32_t ebx = 0, edx = 0, ecx = 0;

    // EBX is saved to EDI and later restored
    __asm__("movl %%ebx, %%edi;"
        "cpuid;"
        "xchgl %%ebx, %%edi;"
        : "=D"(ebx),
        "+a"(eax), "+c"(ecx), "=d"(edx));

    abcd[0] = eax;
    abcd[1] = ebx;
    abcd[2] = ecx;
    abcd[3] = edx;
}
#endif

#ifdef _WIN32
//  Windows
#define cpuid(info, x)    __cpuidex(info, x, 0)
#else
//  GCC Intrinsics
void cpuid(int info[4], int InfoType) {
    __cpuid_count(InfoType, 0, info[0], info[1], info[2], info[3]);
}
#endif

//  SIMD: 512-bit
static int HW_AVX512F;    //  AVX512 Foundation
static int HW_AVX512CD;   //  AVX512 Conflict Detection
static int HW_AVX512PF;   //  AVX512 Prefetch
static int HW_AVX512ER;   //  AVX512 Exponential + Reciprocal
static int HW_AVX512VL;   //  AVX512 Vector Length Extensions
static int HW_AVX512BW;   //  AVX512 Byte + Word
static int HW_AVX512DQ;   //  AVX512 Doubleword + Quadword
static int HW_AVX512IFMA; //  AVX512 Integer 52-bit Fused Multiply-Add
static int HW_AVX512VBMI; //  AVX512 Vector Byte Manipulation Instructions

// https://stackoverflow.com/questions/6121792/how-to-check-if-a-cpu-supports-the-sse3-instruction-set
static void check_cpu_features(void) {
    static int checked = 0;
    if (checked)
        return;

    checked = 1;

    int info[4];
    cpuid(info, 0);
    int nIds = info[0];

    cpuid(info, 0x80000000);
    unsigned nExIds = info[0];

    //  Detect Features
    if (nIds >= 0x00000001) {
        cpuid(info, 0x00000001);
        HW_MMX = (info[3] & ((uint32_t)1 << 23)) != 0;
        HW_SSE = (info[3] & ((uint32_t)1 << 25)) != 0;
        HW_SSE2 = (info[3] & ((uint32_t)1 << 26)) != 0;
        HW_SSE3 = (info[2] & ((uint32_t)1 << 0)) != 0;

        HW_SSSE3 = (info[2] & ((uint32_t)1 << 9)) != 0;
        HW_SSE41 = (info[2] & ((uint32_t)1 << 19)) != 0;
        HW_SSE42 = (info[2] & ((uint32_t)1 << 20)) != 0;
        HW_AES = (info[2] & ((uint32_t)1 << 25)) != 0;

        HW_AVX = (info[2] & ((uint32_t)1 << 28)) != 0;
        HW_FMA3 = (info[2] & ((uint32_t)1 << 12)) != 0;

        HW_RDRAND = (info[2] & ((uint32_t)1 << 30)) != 0;
    }
    if (nIds >= 0x00000007) {
        cpuid(info, 0x00000007);
        HW_AVX2 = (info[1] & ((uint32_t)1 << 5)) != 0;

        HW_BMI1 = (info[1] & ((uint32_t)1 << 3)) != 0;
        HW_BMI2 = (info[1] & ((uint32_t)1 << 8)) != 0;
        HW_ADX = (info[1] & ((uint32_t)1 << 19)) != 0;
        HW_SHA = (info[1] & ((uint32_t)1 << 29)) != 0;
        HW_PREFETCHWT1 = (info[2] & ((uint32_t)1 << 0)) != 0;

        HW_AVX512F = (info[1] & ((uint32_t)1 << 16)) != 0;
        HW_AVX512CD = (info[1] & ((uint32_t)1 << 28)) != 0;
        HW_AVX512PF = (info[1] & ((uint32_t)1 << 26)) != 0;
        HW_AVX512ER = (info[1] & ((uint32_t)1 << 27)) != 0;
        HW_AVX512VL = (info[1] & ((uint32_t)1 << 31)) != 0;
        HW_AVX512BW = (info[1] & ((uint32_t)1 << 30)) != 0;
        HW_AVX512DQ = (info[1] & ((uint32_t)1 << 17)) != 0;
        HW_AVX512IFMA = (info[1] & ((uint32_t)1 << 21)) != 0;
        HW_AVX512VBMI = (info[2] & ((uint32_t)1 << 1)) != 0;
    }
    if (nExIds >= 0x80000001) {
        cpuid(info, 0x80000001);
        HW_x64 = (info[3] & ((uint32_t)1 << 29)) != 0;
        HW_ABM = (info[2] & ((uint32_t)1 << 5)) != 0;
        HW_SSE4a = (info[2] & ((uint32_t)1 << 6)) != 0;
        HW_FMA4 = (info[2] & ((uint32_t)1 << 16)) != 0;
        HW_XOP = (info[2] & ((uint32_t)1 << 11)) != 0;
    }
}

void gemm_nn_fast(int M, int N, int K,
    float *A,
    float *B,
    float *C)
{
    check_cpu_features();
    if (!HW_FMA3 || !HW_AVX2) {
        #pragma omp parallel for
        for (int t = 0; t < M; ++t) {
            if (HW_AVX) {
                for (int k = 0; k < K; ++k) {
                    float A_PART = A[k + t*K];
                    __m256 a256, b256, c256, result256;    // AVX
                    a256 = _mm256_set1_ps(A_PART);
                    for (int j = 0; j < N - 8; j += 8) {
                        b256 = _mm256_loadu_ps(&B[k*N + j]);
                        c256 = _mm256_loadu_ps(&C[j + t*N]);
                        // FMA - Intel Haswell (2013), AMD Piledriver (2012)
                        //result256 = _mm256_fmadd_ps(a256, b256, c256);
                        result256 = _mm256_mul_ps(a256, b256);
                        result256 = _mm256_add_ps(result256, c256);
                        _mm256_storeu_ps(&C[j + t*N], result256);
                    }

                    int prev_end = (N % 8 == 0) ? (N - 8) : (N / 8) * 8;
                    for (int j = prev_end; j < N; ++j)
                        C[j + t*N] += A_PART*B[k*N + j];
                }
            } else {
                for (int k = 0; k < K; ++k) {
                    float A_PART = A[k + t*K];
                    for (int j = 0; j < N; ++j) {
                        C[j + t*N] += A_PART*B[k*N + j];
                    }
                }
            }
            
        }
        return;
    }

    #pragma omp parallel for
    for (int i = 0; i < (M / TILE_M)*TILE_M; i += TILE_M)
    {
        int j, k;
        int i_d, k_d;

        for (k = 0; k < (K / TILE_K)*TILE_K; k += TILE_K)
        {
            for (j = 0; j < (N / TILE_N)*TILE_N; j += TILE_N)
            {
                // L1 - 6 bits tag [11:6] - cache size 32 KB, conflict for each 4 KB
                // L2 - 9 bits tag [14:6] - cache size 256 KB, conflict for each 32 KB
                // L3 - 13 bits tag [18:6] - cache size 8 MB, conflict for each 512 KB

                __m256 result256;
                __m256 a256_0, b256_0;    // AVX
                __m256 a256_1, b256_1;    // AVX
                __m256 a256_2;// , b256_2;    // AVX
                __m256 a256_3;// , b256_3;    // AVX
                __m256 c256_0, c256_1, c256_2, c256_3;
                __m256 c256_4, c256_5, c256_6, c256_7;

                c256_0 = _mm256_loadu_ps(&C[(0 + i)*N + (0 + j)]);
                c256_1 = _mm256_loadu_ps(&C[(1 + i)*N + (0 + j)]);
                c256_2 = _mm256_loadu_ps(&C[(0 + i)*N + (8 + j)]);
                c256_3 = _mm256_loadu_ps(&C[(1 + i)*N + (8 + j)]);

                c256_4 = _mm256_loadu_ps(&C[(2 + i)*N + (0 + j)]);
                c256_5 = _mm256_loadu_ps(&C[(3 + i)*N + (0 + j)]);
                c256_6 = _mm256_loadu_ps(&C[(2 + i)*N + (8 + j)]);
                c256_7 = _mm256_loadu_ps(&C[(3 + i)*N + (8 + j)]);


                for (k_d = 0; k_d < (TILE_K); ++k_d)
                {
                    a256_0 = _mm256_set1_ps(A[(0 + i)*K + (k_d + k)]);
                    a256_1 = _mm256_set1_ps(A[(1 + i)*K + (k_d + k)]);

                    a256_2 = _mm256_set1_ps(A[(2 + i)*K + (k_d + k)]);
                    a256_3 = _mm256_set1_ps(A[(3 + i)*K + (k_d + k)]);


                    b256_0 = _mm256_loadu_ps(&B[(k_d + k)*N + (0 + j)]);
                    b256_1 = _mm256_loadu_ps(&B[(k_d + k)*N + (8 + j)]);

                    // FMA - Intel Haswell (2013), AMD Piledriver (2012)
                    //c256_0 = _mm256_fmadd_ps(a256_0, b256_0, c256_0);
                    //c256_1 = _mm256_fmadd_ps(a256_1, b256_0, c256_1);
                    //c256_2 = _mm256_fmadd_ps(a256_0, b256_1, c256_2);
                    //c256_3 = _mm256_fmadd_ps(a256_1, b256_1, c256_3);

                    //c256_4 = _mm256_fmadd_ps(a256_2, b256_0, c256_4);
                    //c256_5 = _mm256_fmadd_ps(a256_3, b256_0, c256_5);
                    //c256_6 = _mm256_fmadd_ps(a256_2, b256_1, c256_6);
                    //c256_7 = _mm256_fmadd_ps(a256_3, b256_1, c256_7);

                    result256 = _mm256_mul_ps(a256_0, b256_0);
                    c256_0 = _mm256_add_ps(result256, c256_0);

                    result256 = _mm256_mul_ps(a256_1, b256_0);
                    c256_1 = _mm256_add_ps(result256, c256_1);

                    result256 = _mm256_mul_ps(a256_0, b256_1);
                    c256_2 = _mm256_add_ps(result256, c256_2);

                    result256 = _mm256_mul_ps(a256_1, b256_1);
                    c256_3 = _mm256_add_ps(result256, c256_3);


                    result256 = _mm256_mul_ps(a256_2, b256_0);
                    c256_4 = _mm256_add_ps(result256, c256_4);

                    result256 = _mm256_mul_ps(a256_3, b256_0);
                    c256_5 = _mm256_add_ps(result256, c256_5);

                    result256 = _mm256_mul_ps(a256_2, b256_1);
                    c256_6 = _mm256_add_ps(result256, c256_6);

                    result256 = _mm256_mul_ps(a256_3, b256_1);
                    c256_7 = _mm256_add_ps(result256, c256_7);
                }
                _mm256_storeu_ps(&C[(0 + i)*N + (0 + j)], c256_0);
                _mm256_storeu_ps(&C[(1 + i)*N + (0 + j)], c256_1);
                _mm256_storeu_ps(&C[(0 + i)*N + (8 + j)], c256_2);
                _mm256_storeu_ps(&C[(1 + i)*N + (8 + j)], c256_3);

                _mm256_storeu_ps(&C[(2 + i)*N + (0 + j)], c256_4);
                _mm256_storeu_ps(&C[(3 + i)*N + (0 + j)], c256_5);
                _mm256_storeu_ps(&C[(2 + i)*N + (8 + j)], c256_6);
                _mm256_storeu_ps(&C[(3 + i)*N + (8 + j)], c256_7);
            }

            for (j = (N / TILE_N)*TILE_N; j < N; ++j) {
                for (i_d = i; i_d < (i + TILE_M); ++i_d)
                {
                    for (k_d = k; k_d < (k + TILE_K); ++k_d)
                    {
                        float A_PART = A[i_d*K + k_d];
                        C[i_d*N + j] += A_PART*B[k_d*N + j];
                    }
                }
            }
        }

        for (k = (K / TILE_K)*TILE_K; k < K; ++k)
        {
            for (i_d = i; i_d < (i + TILE_M); ++i_d)
            {
                float A_PART = A[i_d*K + k];
                for (j = 0; j < N; ++j) {
                    C[i_d*N + j] += A_PART*B[k*N + j];
                }
            }
        }
    }

    for (int i = (M / TILE_M)*TILE_M; i < M; ++i) {
        int j, k;
        for (k = 0; k < K; ++k) {
            float A_PART = A[i*K + k];
            for (j = 0; j < N; ++j) {
                C[i*N + j] += A_PART*B[k*N + j];
            }
        }
    }
}

void leaky_activate_array_cpu(float *x, const int n)
{
    check_cpu_features();
  int i;
  if (HW_FMA3 && HW_AVX2) {
    __m256i all256_sing1 = _mm256_set_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000);
    __m256 all256_01 = _mm256_set1_ps(0.1F);

    for (i = 0; i < n - 8; i += 8) {
                //x[i] = (x[i]>0) ? x[i] : .1*x[i];

                __m256 src256 = _mm256_loadu_ps(&x[i]);
                __m256 mult256 = _mm256_mul_ps((src256), all256_01); // mult * 0.1

                __m256i sign256 = _mm256_and_si256(_mm256_castps_si256(src256), all256_sing1); // check sign in 8 x 32-bit floats

                __m256 result256 = _mm256_blendv_ps(src256, mult256, _mm256_castsi256_ps(sign256)); // (sign>0) ? src : mult;
                _mm256_storeu_ps(&x[i], result256);
    }
  } else
      for (i = 0; i < n; ++i) {
            x[i] = (x[i]>0) ? x[i] : .1*x[i];
      }
}

void forward_maxpool_op(float *src, float *dst, int size, int w, int h, int out_w, int out_h, int c,
    int pad, int stride)
{
    check_cpu_features();

    const int w_offset = -pad / 2;
    const int h_offset = -pad / 2;
    int k;

        #pragma omp parallel for
        for (k = 0; k < c; ++k) {
            int i, j, m, n;
            for (i = 0; i < out_h; ++i) {
                //for (j = 0; j < out_w; ++j) {
                j = 0;

                if(stride == 1 && HW_AVX) {
                    for (j = 0; j < out_w - 8 - (size - 1); j += 8) {
                        int out_index = j + out_w*(i + out_h*k);
                        __m256 max256 = _mm256_set1_ps(-FLT_MAX);
                        for (n = 0; n < size; ++n) {
                            for (m = 0; m < size; ++m) {
                                int cur_h = h_offset + i*stride + n;
                                int cur_w = w_offset + j*stride + m;
                                int index = cur_w + w*(cur_h + h*k);
                                int valid = (cur_h >= 0 && cur_h < h &&
                                    cur_w >= 0 && cur_w < w);
                                if (!valid) continue;

                                __m256 src256 = _mm256_loadu_ps(&src[index]);
                                max256 = _mm256_max_ps(src256, max256);
                            }
                        }
                        _mm256_storeu_ps(&dst[out_index], max256);

                    }
                }
                else if (size == 2 && stride == 2 && HW_AVX) {
                    for (j = 0; j < out_w - 4; j += 4) {
                        int out_index = j + out_w*(i + out_h*k);
                        //float max = -FLT_MAX;
                        //int max_i = -1;
                        __m128 max128 = _mm_set1_ps(-FLT_MAX);

                        for (n = 0; n < size; ++n) {
                            //for (m = 0; m < size; ++m)
                            m = 0;
                            {
                                int cur_h = h_offset + i*stride + n;
                                int cur_w = w_offset + j*stride + m;
                                int index = cur_w + w*(cur_h + h*k);
                                int valid = (cur_h >= 0 && cur_h < h &&
                                    cur_w >= 0 && cur_w < w);
                                if (!valid) continue;

                                __m256 src256 = _mm256_loadu_ps(&src[index]);
                                __m256 src256_2 = _mm256_permute_ps(src256, (1 << 0) | (3 << 4));
                                __m256 max256 = _mm256_max_ps(src256, src256_2);

                                __m128 src128_0 = _mm256_extractf128_ps(max256, 0);
                                __m128 src128_1 = _mm256_extractf128_ps(max256, 1);
                                __m128 src128 = _mm_shuffle_ps(src128_0, src128_1, (2 << 2) | (2 << 6));

                                max128 = _mm_max_ps(src128, max128);
                            }
                        }
                        _mm_storeu_ps(&dst[out_index], max128);
                    }
                }

                for (; j < out_w; ++j) {
                    int out_index = j + out_w*(i + out_h*k);
                    float max = -FLT_MAX;
                    int max_i = -1;
                    for (n = 0; n < size; ++n) {
                        for (m = 0; m < size; ++m) {
                            int cur_h = h_offset + i*stride + n;
                            int cur_w = w_offset + j*stride + m;
                            int index = cur_w + w*(cur_h + h*k);
                            int valid = (cur_h >= 0 && cur_h < h &&
                                cur_w >= 0 && cur_w < w);
                            float val = (valid != 0) ? src[index] : -FLT_MAX;
                            max_i = (val > max) ? index : max_i;
                            max = (val > max) ? val : max;
                        }
                    }
                    dst[out_index] = max;
                }
            }
        }
}

#else   // AVX

void gemm_nn_fast(int M, int N, int K,
    float *A,
    float *B,
    float *C)
{
    #pragma omp parallel for
    for (int t = 0; t < M; ++t) {
        for (int k = 0; k < K; ++k) {
            float A_PART = A[k + t*K];
            for (int j = 0; j < N; ++j) {
                C[j + t*N] += A_PART*B[k*N + j];
            }
        }
    }
}

void leaky_activate_array_cpu(float *x, const int n)
{
    for (int i = 0; i < n; ++i) {
        x[i] = (x[i]>0) ? x[i] : .1*x[i];
    }
}

void forward_maxpool_op(float *src, float *dst, int size, int w, int h, int out_w, int out_h, int c,
    int pad, int stride)
{
    const int w_offset = -pad / 2;
    const int h_offset = -pad / 2;

    #pragma omp parallel for
    for (int k = 0; k < c; ++k) {
        for (int i = 0; i < out_h; ++i) {
                for (int j = 0; j < out_w; ++j) {
                    int out_index = j + out_w*(i + out_h*k);
                    float max = -FLT_MAX;
                    int max_i = -1;
                    for (int n = 0; n < size; ++n) {
                        for (int m = 0; m < size; ++m) {
                            int cur_h = h_offset + i*stride + n;
                            int cur_w = w_offset + j*stride + m;
                            int index = cur_w + w*(cur_h + h*k);
                            int valid = (cur_h >= 0 && cur_h < h &&
                                cur_w >= 0 && cur_w < w);
                            float val = (valid != 0) ? src[index] : -FLT_MAX;
                            max_i = (val > max) ? index : max_i;
                            max = (val > max) ? val : max;
                        }
                    }
                    dst[out_index] = max;
                }
        }
    }
}

#endif    // AVX

#ifdef USE_EIGEN

void matrixmul(int M, int N, int K,
                    float *A,
                    float *B,
                    float *C) {
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> mcol(A, M, K);
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> mk(B, K, N);
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> my(C, M, N);
  my = mcol * mk;
}

#else

void matrixmul(int M, int N, int K,
        float *A,
        float *B,
        float *C)
{
    gemm_nn_fast(M, N, K, A, B, C);
}

#endif