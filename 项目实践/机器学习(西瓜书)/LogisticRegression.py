import numpy as np


def sigmoid(z):
    return 1 / (1 + np.exp(-z))


def logistic_regression(X, y, lr=0.1, n_iter=1000):
    """
    逻辑回归（梯度下降求解）

    参数:
        X: 形状 (m, d) 的特征矩阵
        y: 形状 (m,) 的标签向量（取值 0 或 1）
        lr: 学习率
        n_iter: 迭代次数

    返回:
        w: 权重向量，形状 (d,)
        b: 偏置标量
    """
    m, d = X.shape

    # 第 1 步：初始化参数
    w = np.zeros(d)
    b = 0.0

    # 第 2 步：梯度下降迭代
    for _ in range(n_iter):
        # 前向：算预测概率
        y_hat = sigmoid(X @ w + b)

        # 算梯度
        dw = (1 / m) * X.T @ (y_hat - y)
        db = (1 / m) * np.sum(y_hat - y)

        # 更新参数
        w -= lr * dw
        b -= lr * db

    return w, b


def predict(X, w, b):
    """预测分类（概率 ≥ 0.5 归为 1，否则归为 0）"""
    return (sigmoid(X @ w + b) >= 0.5).astype(int)


# 简单的线性可分数据：左下角是类别 0，右上角是类别 1
X = np.array([[1, 1], [2, 1], [1, 2], [4, 5], [5, 4], [5, 5]], dtype=float)
y = np.array([0, 0, 0, 1, 1, 1])

w, b = logistic_regression(X, y)
print(f"w = {w}, b = {b}")
print(f"预测: {predict(X, w, b)}")
print(f"真实: {y}")
print(f"概率: {sigmoid(X @ w + b)}")
