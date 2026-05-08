"""
案例:
    演示张量 和 numpy之间如何相互转换, 以及 如何从标量张量中 提取其内容.

涉及到的API:
    场景1: 张量 -> numpy  nd数组对象
        张量对象.numpy()            共享内存
        张量对象.numpy().copy()     不共享内存,  链式编程写法.
    场景2: numpy nd数组 -> 张量.
        from_numpy()              共享内存
        torch.tensor(nd数组)       不共享内存
    场景3: 从标量张量中 提取其内容.
        标量张量.item()

掌握:
    张量 -> numpy:   张量对象.numpy()
    numpy -> 张量:   torch.tensor(nd数组)
    从标量张量中 提取其内容:  标量张量.item()
"""
import torch
import numpy as np
from torch.onnx.symbolic_opset9 import tensor


# 1.张量 -> numpy
def demo1():
    # 1.创建张量
    t1 = torch,tensor([1, 2, 3, 4, 5])
    print(f't1: {t1}, type: {type(t1)}')

    # 2.张量 -> numpy
    # n1 = t1.numpy()     # 共享内存
    n1 = t1.numpy().copy()      # 不共享内存
    print(f'n1: {n1}, type: {type(n1)}')

    # 测试是否共享内存
    n1[0] = 100
    print(f't1: {t1}, type: {type(t1)}')
    print(f'n1: {n1}, type: {type(n1)}')

# 2.numpy -> 张量
def demo2():
    # 1.创建 numpy 数组
    n1 = np.array([1, 2, 3, 4, 5])
    print(f'n1: {n1}, type: {type(n1)}')

    # 2.把上述的numpy数组 转换成张量
    # t1 = torch.from_numpy(n1).type(torch.float)
    t1 = torch.from_numpy(n1)       # 共享内存
    print(f't1: {t1}, type: {type(t1)}')

    t2 = torch.tensor(n1)       # 不共享内存
    print(f't2: {t2}, type: {type(t2)}')

    # 3. 演示上述方式 是否共享内存.
    n1[0] = 100
    print(f'n1: {n1}')
    print(f't1: {t1}')
    print(f't2: {t2}')

if __name__ == '__main__':
    demo2()
