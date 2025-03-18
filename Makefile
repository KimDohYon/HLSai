# Makefile - Build for Vitis HLS kernel and host program
# 플랫폼 경로: KV260 기본 플랫폼 (사용자 환경에 따라 수정)
PLATFORM := ${XILINX_VITIS}/base_platforms/xilinx_k26_kv260_202220_1/xilinx_k26_kv260_202220_1.xpfm
KERNEL_SRC := src/rmsnorm.cpp
HOST_SRC := src/host.cpp
HOST_EXE := host

# 크로스 컴파일 툴체인 (aarch64 for ARM Cortex-A)
CXX := aarch64-linux-gnu-g++
CXXFLAGS := -Wall -O2 -std=c++17

# XRT/OpenCL 라이브러리 경로 (SYSROOT 환경변수 혹은 경로 지정)
XRT_INC := ${XILINX_XRT}/include
XRT_LIB := ${XILINX_XRT}/lib
LDFLAGS := -L$(XRT_LIB) -lOpenCL -lpthread -lrt -lstdc++

# 기본 타깃: hardware 빌드
all: binary_container_1.xclbin $(HOST_EXE)

# 1. HLS 커널 컴파일(.xo) 및 링크(.xclbin)
kernel.rmsnorm.xo: $(KERNEL_SRC)
	${XILINX_VITIS}/bin/v++ -c -t hw --platform $(PLATFORM) -k kernel_rmsnorm -o $@ $<
binary_container_1.xclbin: kernel.rmsnorm.xo
	${XILINX_VITIS}/bin/v++ -l -t hw --platform $(PLATFORM) -o $@ $^

# 2. 호스트 프로그램 컴파일
$(HOST_EXE): $(HOST_SRC)
	$(CXX) $(CXXFLAGS) -I$(XRT_INC) -o $@ $< $(LDFLAGS)

# 정리(target)
clean:
	rm -f *.xo *.xclbin $(HOST_EXE)
