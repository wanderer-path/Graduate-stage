# 常见损失函数解析指南 (Loss Functions Guide)

本指南系统整理了深度学习与机器学习中最常用的损失函数，涵盖通用基础损失、特定问题解决方案以及常见任务对应的选型建议。

---

## P0：最常用的损失函数

### 1. 均方误差 (MSE Loss)

**适用场景**：回归任务（如预测温度、功率、负荷、价格等连续值）。

**数学公式**：
$$L_{\text{MSE}} = \frac{1}{N}\sum_{i=1}^{N}(y_i - \hat{y}_i)^2$$

**主要特点**：
- 误差越大，惩罚呈二次方增长。
- 对异常值（Outliers）非常敏感。
- 适用于连续数值预测。

**PyTorch 实现**：
```python
loss_fn = torch.nn.MSELoss()
```

---

### 2. 平均绝对误差 (MAE Loss)

**适用场景**：含有较多异常值的回归任务。

**数学公式**：
$$L_{\text{MAE}} = \frac{1}{N}\sum_{i=1}^{N}|y_i - \hat{y}_i|$$

**主要特点**：
- 对异常值的敏感度低于 MSE。
- 梯度固定（处于常数级别），训练收敛速度可能比 MSE 慢。

**PyTorch 实现**：
```python
loss_fn = torch.nn.L1Loss()
```

---

### 3. Huber Loss

**适用场景**：结合 MSE 和 MAE 优势的稳健回归任务。

**数学公式**：
$$L_\delta = \begin{cases} \frac{1}{2}(y - \hat{y})^2, & |y - \hat{y}| \le \delta \\ \delta\left(|y - \hat{y}| - \frac{1}{2}\delta\right), & |y - \hat{y}| > \delta \end{cases}$$

**主要特点**：
- 小误差时使用平方惩罚（平滑可导）。
- 大误差时使用线性惩罚（对异常值不敏感）。
- 在实际回归任务中通常比单独使用 MSE 更稳健。

**PyTorch 实现**：
```python
loss_fn = torch.nn.HuberLoss(delta=1.0)
```

---

### 4. 交叉熵损失 (Cross Entropy Loss)

**适用场景**：多分类任务。

**数学公式**：
$$L_{\text{CE}} = -\sum_{c=1}^{C} y_c \log p_c$$

当真实类别为 $k$ 时：
$$L_{\text{CE}} = -\log p_k$$

**示例解析**：
> 例如猫、狗、鸟三分类，真实类别是狗：
> - 模型预测输出概率：`[0.1, 0.8, 0.1]`
> - 计算损失：$-\log(0.8)$
> - 模型赋给真实类别的预测概率越高，产生的损失就越小。

**PyTorch 实现**：
```python
loss_fn = torch.nn.CrossEntropyLoss()
loss = loss_fn(logits, labels)
```
> **注意**：PyTorch 的 `CrossEntropyLoss` 接收的输入必须是未经 Softmax 处理的 raw `logits`。

---

### 5. 二元交叉熵 (BCE Loss)

**适用场景**：二分类或多标签分类（例如是否故障、是否包含目标、一张图片包含多个独立标签）。

**数学公式**：
$$L_{\text{BCE}} = -[y \log p + (1 - y) \log(1 - p)]$$

**PyTorch 实现**：
```python
loss_fn = torch.nn.BCEWithLogitsLoss()
loss = loss_fn(logits, targets.float())
```
> **推荐**：推荐直接使用 `BCEWithLogitsLoss`，内部整合了 Sigmoid 层，数值稳定性优于手动计算 Sigmoid + BCE。

---

## P1：解决特定问题的损失函数

### 6. Focal Loss

**解决问题**：类别严重不平衡问题。

**数学公式**：
$$L_{\text{focal}} = -\alpha(1 - p_t)^\gamma \log p_t$$

**核心作用**：
- 降低简单样本（Easy Examples）的损失权重。
- 促使模型聚焦于困难样本（Hard Examples）的学习。
- 广泛应用于目标检测、故障诊断及少数类识别。
- 当 $p_t$ 较高时说明样本容易分类，$(1-p_t)^\gamma$ 因子会大幅降低其对总体 Loss 的贡献。

---

### 7. KL 散度 (Kullback-Leibler Divergence)

**解决问题**：衡量两个概率分布之间的差异。

**数学公式**：
$$D_{\text{KL}}(P \parallel Q) = \sum_i P(i) \log \frac{P(i)}{Q(i)}$$

**主要应用**：
- 知识蒸馏（Knowledge Distillation）
- 变分自编码器（VAE）
- 特征与预测分布对齐
- 联邦学习中的客户端表征对齐

> **注意**：$D_{\text{KL}}(P \parallel Q) \neq D_{\text{KL}}(Q \parallel P)$，KL 散度不满足对称性，因此并非严格意义上的距离度量。

---

### 8. 余弦相似度损失 (Cosine Similarity Loss)

**解决问题**：评估与优化两个向量的方向一致性。

**定义公式**：
$$\cos(\mathbf{x}, \mathbf{y}) = \frac{\mathbf{x}^T \mathbf{y}}{\|\mathbf{x}\| \|\mathbf{y}\|}$$

通常定义损失为：
$$L_{\cos} = 1 - \cos(\mathbf{x}, \mathbf{y})$$

**主要应用**：
- 文本表征对齐
- 图像表征对齐
- 图文跨模态匹配
- 特征层蒸馏

---

### 9. 对比损失 (Contrastive Loss)

**核心目标**：拉近正样本对之间的距离，推远负样本对之间的距离。

**常见形式**：
$$L = y D^2 + (1 - y) \max(0, m - D)^2$$

其中：
- $D$：两个样本特征间的距离。
- $m$：负样本的最小边距（Margin）。
- $y = 1$：正样本对。
- $y = 0$：负样本对。

**应用领域**：人脸识别、孪生网络（Siamese Network）、跨模态表征学习。

---

### 10. InfoNCE Loss

**核心作用**：视觉语言模型（VLM，如 CLIP）及对比学习中最关键的损失函数之一。

**数学公式**：
$$L_i = -\log \frac{\exp(s_{ii} / \tau)}{\sum_j \exp(s_{ij} / \tau)}$$

其中：
- $s_{ii}$：正确配对（正样本）的图文相似度。
- $s_{ij}$：图像 $i$ 与非匹配文本 $j$（负样本）的相似度。
- $\tau$：可学习或固定的温度参数（Temperature Parameter）。

**优化目标**：
- 提升正确的图像-文本对相似度。
- 降低错误的图像-文本对相似度。

> 在 CLIP 中，通常对称计算图像到文本与文本到图像的损失：
> $$L = \frac{L_{\text{image-to-text}} + L_{\text{text-to-image}}}{2}$$

---

### 11. 三元组损失 (Triplet Loss)

**输入结构**：三个样本一组
- **Anchor ($a$)**：基准样本
- **Positive ($p$)**：同类正样本
- **Negative ($n$)**：异类负样本

**数学公式**：
$$L = \max\left(d(a, p) - d(a, n) + m,\ 0\right)$$

**优化目标**：
$$d(a, p) + m < d(a, n)$$
保证正样本对与 Anchor 的距离，至少比负样本对与 Anchor 的距离近一个 Margin 间隔 $m$。

---

## P2：特定任务常见损失对应表

| 任务类型 | 常用损失函数 |
| :--- | :--- |
| **回归预测** | MSE、MAE、Huber Loss |
| **二分类** | BCEWithLogitsLoss |
| **多分类** | CrossEntropyLoss |
| **类别不平衡** | Focal Loss、加权交叉熵 (Weighted CE) |
| **语言模型 (LLM/LM)** | Cross Entropy / NLL Loss |
| **图文对齐 / 跨模态** | InfoNCE Loss、Cosine Similarity Loss |
| **目标检测** | 分类损失 (CE/Focal) + 边界框回归损失 (GIoU/DIoU/CIoU) |
| **图像分割** | Cross Entropy、Dice Loss、IoU Loss |
| **知识蒸馏** | KL 散度 + 交叉熵 |
| **表征学习** | Contrastive Loss、Triplet Loss、InfoNCE |
| **强化学习 (PPO等)** | Policy Loss + Value Loss + Entropy Loss |

---

## 实际选型指南

1. **连续数值回归**：优先考虑 **Huber Loss**（兼顾 MSE 收敛速度与 MAE 的抗噪能力）。
2. **普通多分类**：优先选择 **CrossEntropyLoss**。
3. **VLM 图文表征对齐**：首选 **InfoNCE Loss**。
4. **正负样本/类别极度不平衡**：使用 **加权交叉熵** 或 **Focal Loss**。
5. **强化学习（如 PPO）**：同时联合优化 **策略损失 (Policy Loss)**、**价值损失 (Value Loss)** 和 **熵损失 (Entropy Loss)**。