/*** host.cpp - XRT/OpenCL Host Code for RMSNorm ***/
#include <CL/cl2.hpp>    // Xilinx OpenCL C++ wrapper
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
using namespace std;

int main(int argc, char** argv) {
    // 1. FPGA 디바이스 및 컨텍스트 초기화
    string xclbinPath = "binary_container_1.xclbin";
    string inFile = "x.bin";
    string wtFile = "weight.bin";
    if (argc >= 2) xclbinPath = argv[1];
    if (argc >= 3) inFile    = argv[2];
    if (argc >= 4) wtFile    = argv[3];

    // OpenCL 플랫폼/디바이스 찾기 (Xilinx 플랫폼 선택)
    vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Platform platform;
    for (auto& plat : platforms) {
        string name = plat.getInfo<CL_PLATFORM_NAME>();
        if (name.find("Xilinx") != string::npos) {
            platform = plat;
            break;
        }
    }
    if (platform() == 0) {
        cerr << "Xilinx Platform not found!\n";
        return EXIT_FAILURE;
    }
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
    if (devices.empty()) {
        cerr << "No FPGA device found!\n";
        return EXIT_FAILURE;
    }
    cl::Device device = devices[0];
    // 컨텍스트 및 명령 큐 생성 (프로파일링 활성화)
    cl_int err;
    cl::Context context(device, nullptr, nullptr, nullptr, &err);
    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);

    // FPGA 비트스트림(xclbin) 파일 로드
    cout << "Loading FPGA bitstream: " << xclbinPath << endl;
    ifstream binFile(xclbinPath, ios::binary);
    if(!binFile) {
        cerr << "Unable to open xclbin file!\n";
        return EXIT_FAILURE;
    }
    binFile.seekg(0, binFile.end);
    size_t xclbinSize = binFile.tellg();
    binFile.seekg(0, binFile.beg);
    vector<char> binData(xclbinSize);
    binFile.read(binData.data(), xclbinSize);
    cl::Program::Binaries bins{{binData.data(), xclbinSize}};
    // 디바이스에 비트스트림 프로그래밍
    cl::Program program(context, {device}, bins, nullptr, &err);
    if (err != CL_SUCCESS) {
        cerr << "Failed to program device with xclbin file!\n";
        return EXIT_FAILURE;
    }
    // 커널 객체 생성
    cl::Kernel krnl_rmsnorm(program, "kernel_rmsnorm", &err);

    // 2. 입력/출력 버퍼 할당 (호스트-디바이스 공유 메모리)
    // 입력 벡터 x와 weight의 크기 (float 개수) 읽기
    ifstream inFileStream(inFile, ios::binary);
    ifstream wtFileStream(wtFile, ios::binary);
    if(!inFileStream || !wtFileStream) {
        cerr << "Unable to open input data files!\n";
        return EXIT_FAILURE;
    }
    // 파일 크기 계산
    inFileStream.seekg(0, inFileStream.end);
    size_t fileSize = inFileStream.tellg();
    inFileStream.seekg(0, inFileStream.beg);
    size_t numElements = fileSize / sizeof(float);  // float 개수
    // 입력 데이터를 메모리에 로드
    vector<float> host_x(numElements);
    vector<float> host_w(numElements);
    inFileStream.read(reinterpret_cast<char*>(host_x.data()), fileSize);
    wtFileStream.seekg(0, wtFileStream.beg);
    wtFileStream.read(reinterpret_cast<char*>(host_w.data()), fileSize);
    inFileStream.close();
    wtFileStream.close();
    size_t bytes = numElements * sizeof(float);

    // OpenCL 버퍼 생성 (입력: ReadOnly, 출력: WriteOnly)
    cl::Buffer buf_x(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bytes, nullptr, &err);
    cl::Buffer buf_w(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, bytes, nullptr, &err);
    cl::Buffer buf_out(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, bytes, nullptr, &err);
    // 버퍼를 호스트 메모리에 매핑하여 포인터 획득
    float* ptr_x   = (float*)queue.enqueueMapBuffer(buf_x, CL_TRUE, CL_MAP_WRITE, 0, bytes);
    float* ptr_w   = (float*)queue.enqueueMapBuffer(buf_w, CL_TRUE, CL_MAP_WRITE, 0, bytes);
    float* ptr_out = (float*)queue.enqueueMapBuffer(buf_out, CL_TRUE, CL_MAP_READ,  0, bytes);
    // 호스트 메모리 -> 버퍼로 데이터 복사
    memcpy(ptr_x, host_x.data(), bytes);
    memcpy(ptr_w, host_w.data(), bytes);
    // (출력 버퍼는 초기화 필요 없음)

    // 3. 커널 인자 설정 (버퍼와 크기 전달)
    krnl_rmsnorm.setArg(0, buf_x);
    krnl_rmsnorm.setArg(1, buf_w);
    krnl_rmsnorm.setArg(2, buf_out);
    krnl_rmsnorm.setArg(3, (int)numElements);

    // 4. 커널 실행 및 동기화
    cl::Event event;
    err = queue.enqueueTask(krnl_rmsnorm, nullptr, &event);
    if (err != CL_SUCCESS) {
        cerr << "Failed to launch kernel!\n";
        return EXIT_FAILURE;
    }
    queue.finish();  // 커널 완료 대기

    // 5. 결과 값 및 성능 확인
    // 호스트 메모리에 이미 결과가 맵핑되어 있으므로 바로 사용 가능
    // (원한다면 ptr_out를 이용해 결과를 검증하거나 출력 가능)
    // 실행 시간 측정 (프로파일링 이벤트 사용)
    cl_ulong start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong end   = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    double exec_ms = (end - start) * 1e-6;  // ns -> ms 변환
    cout << "Kernel execution time: " << exec_ms << " ms" << endl;

    // 자원 정리
    queue.enqueueUnmapMemObject(buf_x, ptr_x);
    queue.enqueueUnmapMemObject(buf_w, ptr_w);
    queue.enqueueUnmapMemObject(buf_out, ptr_out);
    queue.finish();
    return 0;
}
