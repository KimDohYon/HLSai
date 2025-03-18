/*** rmsnorm.cpp - HLS Kernel for RMSNorm ***/
#include <hls_stream.h>
#include <cmath>
#include <stdint.h>

const int MAX_SIZE = 1024;  // 최대 벡터 크기 (필요에 따라 조정 가능)

static void load_vec(const float* in, hls::stream<float>& outStream, int size) {
#pragma HLS INLINE
mem_rd:
    for (int i = 0; i < size; i++) {
        outStream << in[i];       // 입력 배열을 스트림으로 출력
    }
}

static void store_vec(float* out, hls::stream<float>& inStream, int size) {
#pragma HLS INLINE
mem_wr:
    for (int i = 0; i < size; i++) {
        out[i] = inStream.read(); // 스트림에서 값을 읽어 출력 배열에 저장
    }
}

static void compute_rmsnorm(hls::stream<float>& x_stream,
                             hls::stream<float>& w_stream,
                             hls::stream<float>& out_stream, int size) {
    // 로컬 버퍼 (FPGA BRAM/URAM에 저장)
    float x_local[MAX_SIZE];
    float w_local[MAX_SIZE];
#pragma HLS ARRAY_PARTITION variable=x_local dim=1 complete
#pragma HLS ARRAY_PARTITION variable=w_local dim=1 complete

    // 입력 스트림에서 데이터 읽기
    float sum_sq = 0.0f;
    for (int i = 0; i < size; i++) {
        x_local[i] = x_stream.read();
    }
    for (int i = 0; i < size; i++) {
        w_local[i] = w_stream.read();
    }
    // RMSNorm 계수 계산: norm = 1 / sqrt((sum(x[i]^2)/size) + eps)
    for (int i = 0; i < size; i++) {
        sum_sq += x_local[i] * x_local[i];
    }
    const float eps = 1e-5f;
    float mean_sq = sum_sq / size;
    float norm = 1.0f / std::sqrt(mean_sq + eps);
    // 정규화 및 가중치 곱 적용
    for (int i = 0; i < size; i++) {
        float y = x_local[i] * norm * w_local[i];
        out_stream << y;
    }
}

// Top-level HLS kernel function (AXI master interfaces for DDR access)
extern "C" {
void kernel_rmsnorm(const float* x, const float* weight, float* out, int vec_size) {
    #pragma HLS INTERFACE m_axi port=x      offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=weight offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi port=out    offset=slave bundle=gmem0
    #pragma HLS INTERFACE s_axilite port=x        bundle=control
    #pragma HLS INTERFACE s_axilite port=weight   bundle=control
    #pragma HLS INTERFACE s_axilite port=out      bundle=control
    #pragma HLS INTERFACE s_axilite port=vec_size bundle=control
    #pragma HLS INTERFACE s_axilite port=return   bundle=control

    // HLS 데이터플로우 영역: 메모리 로드 -> 계산 -> 저장 파이프라인
    #pragma HLS DATAFLOW
    hls::stream<float> x_stream("x_stream");
    hls::stream<float> w_stream("w_stream");
    hls::stream<float> out_stream("out_stream");
    // 각 함수 실행
    load_vec(x, x_stream, vec_size);
    load_vec(weight, w_stream, vec_size);
    compute_rmsnorm(x_stream, w_stream, out_stream, vec_size);
    store_vec(out, out_stream, vec_size);
}
}
