# generate_bin.py - Create binary files for input (x) and weight
import numpy as np

S = 256  # 벡터 길이 (kernel_rmsnorm의 vec_size와 동일하게 설정)
# 예제용 무작위 데이터 생성 (0~1 사이 실수)
x = np.random.rand(S).astype(np.float32)
w = np.random.rand(S).astype(np.float32)
# 이진 파일로 저장
x.tofile("x.bin")
w.tofile("weight.bin")
print(f"Generated x.bin and weight.bin with {S} float entries each.")
