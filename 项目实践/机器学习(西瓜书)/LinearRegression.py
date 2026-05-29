import numpy as np


# 一元线性回归
def simpleLinearRegression(X, y):
    """
    一元线性回归（最小二乘法闭式解）

    参数:
        X: 形状 (m,) 的特征向量，m 个样本
        y: 形状 (m,) 的标签向量

    返回:
        w: 权重标量
        b: 偏置标量
    """
    m = len(X)
    x_mean = np.mean(X)

    # 第 1 步：计算 w
    numerator = np.sum(y * (X - x_mean))
    denominator = np.sum(X**2) - (1 / m) * np.sum(X) ** 2
    w = numerator / denominator

    # 第 2 步：计算 b
    b = np.mean(y) - w * x_mean

    return w, b


def simplePredict(X, w, b):
    """用学到的 w, b 做预测"""
    return w * X + b


X = np.array([1, 2, 3, 4, 5], dtype=float)
y = np.array([3, 5, 7, 9, 11], dtype=float)  # 真实关系 y = 2x + 1

w, b = simpleLinearRegression(X, y)
print(f"w = {w}, b = {b}")  # 应该是 w=2, b=1
print(f"预测值: {simplePredict(X, w, b)}")
print(f"真实值: {y}")


# 多元线性回归
def multipleLinearRegression(X, y):
    """
    多元线性回归（最小二乘法闭式解）

    参数:
        X: 形状 (m, d) 的特征矩阵，m 个样本，d 个特征
        y: 形状 (m,) 的标签向量

    返回:
        w: 权重向量，形状 (d,)
        b: 偏置标量
    """
    m = X.shape[0]

    # 第 1 步：在 X 末尾拼一列 1，把 b 吸收进 w_hat
    X_hat = np.hstack([X, np.ones((m, 1))])

    # 第 2 步：闭式解 w_hat = (X^T X)^(-1) X^T y
    w_hat = np.linalg.solve(X_hat.T @ X_hat, X_hat.T @ y)

    # 第 3 步：拆出 w 和 b
    w = w_hat[:-1]
    b = w_hat[-1]

    return w, b


def multiplePredict(X, w, b):
    """用学到的 w, b 做预测"""
    return X @ w + b


# 简单测试
X = np.array([[1, 1], [2, 1], [3, 2], [4, 3], [5, 5]], dtype=float)
y = np.array([3, 4, 7, 10, 15], dtype=float)

w, b = multipleLinearRegression(X, y)
print(f"w = {w}, b = {b}")
print(f"预测值: {multiplePredict(X, w, b)}")
print(f"真实值: {y}")
