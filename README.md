📘 RMSNorm FPGA 가속기 (KV260 & Zynq-7020)
✅ Vitis HLS 및 XRT를 활용한 RMSNorm 하드웨어 가속 프로젝트

이 프로젝트는 AMD Xilinx KV260 또는 Zynq-7020 FPGA에서 RMSNorm 연산을 가속화하는 예제입니다.
Vitis HLS를 이용해 커널을 생성하고, XRT 기반의 호스트 프로그램을 작성하여 FPGA에서 실행할 수 있도록 구성되었습니다.

📂 프로젝트 구조
rmsnorm_fpga/
├── src/  
│   ├── rmsnorm.cpp          # Vitis HLS 커널 코드  
│   ├── host.cpp             # XRT 기반 호스트 프로그램  
│   ├── generate_bin.py      # 입력 및 가중치 데이터 생성 (BIN 파일)  
├── data/  
│   ├── x.bin                # 입력 데이터 (실행 전 생성됨)  
│   ├── weight.bin           # 가중치 데이터 (실행 전 생성됨)  
├── binary_container_1.xclbin  # FPGA 비트스트림 (.xclbin)  
├── host                     # 크로스 컴파일된 실행 파일 (호스트 프로그램)  
├── Makefile                 # 빌드 스크립트  
└── README.md                # 프로젝트 설명 파일  

🛠️ 1. 환경 설정 및 요구 사항
이 프로젝트는 AMD Xilinx KV260 또는 Zynq-7020을 지원하며, 다음 환경에서 실행됩니다.
✅ FPGA 하드웨어: AMD KV260 또는 Zynq-7020 기반 보드 (ZCU104, PYNQ-Z2 등)
✅ 운영체제: Ubuntu 20.04 (KV260 기본 OS) 또는 PetaLinux
✅ 소프트웨어 패키지:
Vitis 2022.2
Xilinx Runtime (XRT)
크로스 컴파일 툴체인 (aarch64-linux-gnu-g++)

⚡ XRT 및 Vitis 설치 방법:
아래 명령어를 사용하여 보드에 XRT를 설치합니다.

sudo apt-get update && sudo apt-get install -y xrt

이후, xbutil examine 명령어를 실행하여 FPGA 상태를 확인할 수 있습니다.

🔧 2. 빌드 및 실행 방법
📌 (1) 입력 데이터 및 가중치 생성
먼저 Python을 사용하여 입력 벡터(x.bin)와 가중치(weight.bin)를 생성합니다.

cd src
python3 generate_bin.py

✔ data/x.bin 및 data/weight.bin 파일이 생성됩니다.

📌 (2) HLS 커널 및 호스트 프로그램 빌드
Vitis 환경에서 FPGA 커널(.xclbin)을 생성하고 호스트 프로그램을 빌드합니다.

make
✔ binary_container_1.xclbin (FPGA 비트스트림) 및 host (호스트 실행 파일)가 생성됩니다.

📌 Zynq-7020을 사용하는 경우, Makefile의 PLATFORM 변수를 변경해야 합니다.

📌 (3) 보드로 파일 전송 및 실행
생성된 파일을 FPGA 보드로 복사합니다.

scp binary_container_1.xclbin host data/x.bin data/weight.bin ubuntu@<BOARD_IP>:/home/ubuntu/

📌 <BOARD_IP>를 보드의 IP 주소로 변경하세요.

cd /home/ubuntu/
export XILINX_XRT=/usr
./host binary_container_1.xclbin data/x.bin data/weight.bin
✔ RMSNorm 커널이 FPGA에서 실행되며, 실행 시간이 출력됩니다.

Kernel execution time: 0.042 ms

🏎️ 3. 성능 평가 및 테스트
✅ (1) 실행 시간(지연 시간, Latency) 측정
호스트 프로그램 실행 시, **커널 실행 시간(ms 단위)**이 출력됩니다.
XRT 프로파일링을 사용하여 실행 시간을 보다 정밀하게 측정할 수도 있습니다.

✅ (2) FPGA 자원 사용량 (Resource Utilization)
Vitis HLS에서 생성된 FPGA 리소스 사용량을 확인하려면 다음 명령을 실행합니다.


vitis_analyzer binary_container_1.xclbin
📌 LUT, FF, BRAM, DSP 사용량을 분석하여 FPGA에서 RMSNorm 커널이 차지하는 공간을 확인할 수 있습니다.

✅ (3) 전력 소비량 (Power Consumption)
xbutil 명령을 사용하여 FPGA의 전력 사용량을 모니터링할 수 있습니다.

xbutil examine -r power
✔ RMSNorm 실행 중 **FPGA의 전력 소비량(Watt 단위)**이 출력됩니다.

✅ (4) Throughput (처리량) 분석

벡터 크기(S)를 늘려가며 실행 시간을 측정하면 초당 처리 가능한 연산량을 계산할 수 있습니다.
예를 들어, S=256일 때 실행 시간이 0.04ms라면:

처리량 = 1 / 0.00004 = 25,000 vectors/sec
