"""
案例:
    演示如何创建全0, 全1, 指定值的张量.

涉及到的函数如下:
    torch.ones 和 torch.ones_like 创建全1张量
    torch.zeros 和 torch.zeros_like 创建全0张量
    torch.full 和 torch.full_like 创建全为指定值张量

需要你掌握的函数:
    zeros(), full()
"""
import torch

# 1.torch.ones 和 torch.ones_like 创建全1张量
t1 = torch.ones(2, 3)       # 创建2行3列全1张量
print(f't1: {t1}, type: {type(t1)}')
t2 = torch.tensor([[1, 2], [3, 4], [5, 6]])
t3 = torch.ones_like(t2)
print(f't3: {t3}, type: {type(t3)}')
print('-' * 30)

# 2.torch.zeros 和 torch.zeros_like 创建全0张量
t4 = torch.zeros(2, 3)
print(f't4: {t4}, type: {type(t4)}')
t5 = torch.zeros_like(t2)
print(f't5: {t5}, type: {type(t5)}')
print('-' * 30)

# 3.torch.full 和 torch.full_like 创建全为指定值张量
t6 = torch.full(size=(2, 3), fill_value=10)
print(f't6: {t6}, type: {type(t6)}')
t7 = torch.full_like(t2, fill_value=10)
print(f't7: {t7}, type: {type(t7)}')
