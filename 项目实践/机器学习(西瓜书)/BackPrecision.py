import numpy as np

def sigmoid(z):
    return 1 / (1 + np.exp(-z))


def neural_network(X, y, hidden_size=4, lr=0.5, n_iter=10000):
    """
    三层神经网络（BP 算法）
    
    参数:
        X: (m, d) 输入
        y: (m, l) 标签（若是一维会自动 reshape）
        hidden_size: 隐层神经元数 q
        lr: 学习率 η
        n_iter: 迭代次数
    
    返回:
        W1, b1, W2, b2: 训练好的参数
    """
    m, d = X.shape
    if y.ndim == 1:
        y = y.reshape(-1, 1)
    l = y.shape[1]
    
    # 初始化（小随机数，避免对称性问题）
    np.random.seed(42)
    W1 = np.random.randn(d, hidden_size) * 0.5
    b1 = np.zeros(hidden_size)
    W2 = np.random.randn(hidden_size, l) * 0.5
    b2 = np.zeros(l)
    
    for _ in range(n_iter):
        # ===== 前向传播 =====
        y1_hat = sigmoid(X @ W1 + b1)     # 隐层输出
        y2_hat = sigmoid(y1_hat @ W2 + b2)    # 输出层输出
        
        # ===== 反向传播：算梯度项 =====
        g = y2_hat * (1 - y2_hat) * (y - y2_hat)       
        e = y1_hat * (1 - y1_hat) * (g @ W2.T)       
        
        # ===== 更新参数 =====
        W2 += lr * y1_hat.T @ g
        b2 += lr * np.sum(g, axis=0)
        W1 += lr * X.T @ e
        b1 += lr * np.sum(e, axis=0)
    
    return W1, b1, W2, b2


def predict(X, W1, b1, W2, b2):
    """前向传播得到预测"""
    y1_hat = sigmoid(X @ W1 + b1)
    y2_hat = sigmoid(y1_hat @ W2 + b2)
    return y2_hat

X = np.array([[0,0], [0,1], [1,0], [1,1]], dtype=float)
y = np.array([[0], [1], [1], [0]], dtype=float)

W1, b1, W2, b2 = neural_network(X, y, hidden_size=4, lr=0.5, n_iter=10000)
pred = predict(X, W1, b1, W2, b2)
print(f"预测概率: {pred.flatten()}")
print(f"四舍五入: {(pred >= 0.5).astype(int).flatten()}")
print(f"真实标签: {y.flatten().astype(int)}")