
>论文：《Attention Is All You Need》
>核心贡献：**Transformer**架构，自注意力，多头注意力，位置编码，并行计算

## 整体架构

### 编码器

```
输入 Embedding + Position
        │
        ▼
Multi-Head Attention
（和其他 Token 交换信息）
        │
Residual + LayerNorm
（保留原始信息，稳定数值）
        │
Feed Forward Network
（每个 Token 独立进行深度特征提取）
        │
Residual + LayerNorm
（再次保留信息并稳定数值）
        │
输出到下一层 Encoder
```

### 解码器

```
目标序列 Embedding + Position
        │
        ▼
Masked Multi-Head Attention
（目标序列内部交换信息，只能看当前位置及之前的 Token）
        │
Residual + LayerNorm
（保留原始信息，稳定数值）
        │
        ▼
Cross Multi-Head Attention
（查询 Encoder 输出，从源序列获取相关信息）
        │
Residual + LayerNorm
（融合源序列信息，同时保留 Decoder 原有信息）
        │
        ▼
Feed Forward Network
（每个 Token 独立进行非线性特征提取）
        │
Residual + LayerNorm
（再次保留信息并稳定数值）
        │
        ▼
输出到下一层 Decoder
```

## 输入

### 真实输入

- 编码器
	- 训练和推理阶段同为：`源序列的 Token Embedding + Positional Encoding`
	- 第一层 EncoderBlock 接收上述结果，后续 EncoderBlock 接收上一层输出
- 解码器
	- 训练：`右移后的目标序列.Embedding + Positional Encoding`
		- 真实目标：`I like machine learning <EOS>`
		- 右移后：`<BOS> I like machine learning`
	- 推理：当前最新生成的一个目标 Token

### Embedding

>把每个词映射成一个有语义的稠密向量（Dense Vector）

- Embedding 维度是超参数
- 一般来说，Embedding 维度=模型维度

#### 发展脉络

##### 1. 编号

```
king       0
queen      1
dog        2
```

问题：编号只是索引，没有任何语义
##### 2. One-Hot编码

```
king      [1 0 0]
queen     [0 1 0]
dog       [0 0 1]
```
问题：没有语义，king和queen与king和dog的距离一致，不知道king和queen的关系

##### 3. Embedding

```
king      [0.21 1.03 -0.88 ...]
queen     [0.18 0.97 -0.81 ...]
dog       [-1.2 0.5  3.7   ...]
```
可以学到king和queen之间的关系

### 位置编码

#### 问题

>Self-Attention 的计算只依赖 Token 向量之间的相似度，本身没有任何表示“第几个位置”的变量

例子：我爱你

RNN：顺序读取，天然知道位置信息
Transformer：三个词一起读取，不知道位置关系

#### 作用

>加入位置编码后，每个位置的输入表示不同，模型可以通过训练学习如何利用这些位置差异

$Q=(X+P)W_Q​ -> Q=XW_Q​+PW_Q​$

#### 为什么不用拼接

如果使用的是拼接，则`词（512维），位置（512维）-->X（1024维）`，计算量翻倍

#### 为什么可以直接相加

$$PE_{(pos, 2i)} = \sin\left(\frac{pos}{10000^{2i/d_{model}}}\right)$$
$$PE_{(pos, 2i+1)} = \cos\left(\frac{pos}{10000^{2i/d_{model}}}\right)$$
- 每个位置都有唯一的编码
- 距离可以计算，模型可以知道不同位置间的距离关系

## 编码器
### 自注意力机制

#### Q,K,V

- Q (Query / 查询)：代表“我要找什么”
- K (Key / 键)：代表“我有什么特征”
- V (Value / 值)：代表“我的实际内容”

#### 缩放点积注意力(Scaled Dot-Product Attention)

核心就是一个**注意力打分函数**
$$\text{Attention}(Q, K, V) = \text{softmax}\left(\frac{QK^T}{\sqrt{d_k}}\right)V$$
- $QK^T$：计算词与词之间的亲密度
- 除以 $\sqrt{d_k}$（缩放）：将数值拉回了平稳的方差区间，保证了训练的**稳定性**
- softmax（归一化）：统一权重
- 右乘V：提取核心信息

### 多头注意力

>让每个 Token 根据其他 Token 的信息更新自己的表示，建立 Token 之间的上下文依赖

单头注意力模型在计算时，往往只能把注意力集中在某一种关系上，而忽略了其他信息，多头注意力在不同维度计算信息，最后进行汇总
#### 核心流程

1. 降维：将输入的 $Q$、$K$、$V$ 分别映射（降维）到 多 个低维子空间中
2. 并行计算 $\text{head}_i = \text{Attention}(QW_i^Q, KW_i^K, VW_i^V)$
3. 拼接融合多个注意力头：$\text{MultiHead}(Q, K, V) = \text{Concat}(\text{head}_1, ..., \text{head}_h)W^O$

#### $W_i^Q, W_i^K, W_i^V$

核心思想：分别从自己的角度关注不同的特征
为什么会从不同角度关注：这样loss才会最大的下降

#### $W^O$

融合多个 Head 的信息，得到最终需要的信息

### 残差连接

>保留原始输入信息；帮助梯度传播；降低深层网络训练难度
>Y=X+Attention(X)

- 防止模型性能退化
- 防止梯度消失

### 层归一化

>稳定各层输入的数值分布；提高训练稳定性；帮助模型更快收敛

#### 为什么不适用BatchNorm

Transformer处理的是序列数据，每个样本长度不同，而且训练和推理时Batch大小可能变化，LayerNorm不会受到这些因素影响

### 逐位置前馈网络

- 逐位置：每一个 Token 单独计算，所有位置的表示进行变换时使用的是同一个多层感知机
- 前馈：数据只往前传播，不会回头

>Attention 负责"信息交流"，FFN 负责"深度加工"
>对每个 Token 已经聚合到的上下文信息进行非线性特征变换和深层加工

$$\text{FFN}(x) = \max(0, xW_1 + b_1)W_2 + b_2$$

- 第一层（升维）：剔除无用信息，激活有用的深层特征
- 第二层（降维）：方便下一个网络计算

## 解码器

### 掩码注意力

>只允许目标位置关注自己和前面的目标 Token，不能看到未来 Token。防止当前位置看到未来目标 Token，从而保证训练方式与自回归推理一致

第一次输入Q、K、V全部来自Docoder输入，即`<BOS>`，之后的输入是Decoder最后输出的内容

### 交叉注意力 (Encoder-Decoder Attention)

>让解码器根据当前目标侧状态，从编码器输出中寻找相关的源序列信息

输入Q来自Decoder中第一个Multi-Head Attention的输出。输入K、V来自Encoder中的输出
- Q：表示解码器当前需要什么信息
- K：表示源序列中有哪些信息可以被匹配
- V：表示匹配后真正取出的源序列内容


例子：
- 源序列：我 喜欢 机器 学习
1. 当解码器已经生成：当解码器已经生成：
2. 准备生成下一个词时，Cross-Attention 可以重点关注编码器中的：机器
3. 然后预测：machine

## 输出

### Linear 

>每个位置对vocab_size个候选Token分别给出一个分数

假设词表vocab_size = 30000，解码器最后输出为(batch_size, target_length, num_hiddens)，经过Linear输出(batch_size, target_length, vocab_size)

### Softmax

> 将linear给出的分数转为概率


### Cross Entropy Loss

解码器经过 Linear 得到的 logits 与 目标序列中正确的 Token ID