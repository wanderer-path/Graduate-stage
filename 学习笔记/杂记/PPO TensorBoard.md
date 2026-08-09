# PPO TensorBoard 训练指标整理总结

> 本文只整理 PPO 训练过程中 TensorBoard 中常见的训练指标含义、参考范围，以及指标变大/变小时通常代表什么。  
> **注意：下面的“正常值”是工程上的经验参考，不是严格标准。不同奖励尺度、动作空间、环境长度和实现方式都会改变数值范围。**

---

## 1. 总览

常见指标可分成 4 类：

| 类别 | 指标 | 主要作用 |
|---|---|---|
| 策略更新幅度 | `approx_kl`、`clip_fraction`、`clip_range` | 判断 Actor 每次更新是否过大 |
| 探索程度 | `entropy_loss`、`std` | 判断策略是继续探索还是逐渐确定 |
| Critic 学习质量 | `value_loss`、`explained_variance` | 判断价值函数是否能预测未来回报 |
| 优化过程 | `policy_gradient_loss`、`loss`、`learning_rate` | 观察优化强度和训练过程 |

实际判断 PPO 是否健康时，建议优先级：

1. `approx_kl`
2. `clip_fraction`
3. `explained_variance`
4. `std / entropy_loss`
5. `value_loss`
6. `policy_gradient_loss`
7. `loss`

**不要仅根据 `loss` 是否下降判断 PPO 好坏。**

---

# 2. approx_kl

## 含义

`approx_kl` 是旧策略与新策略之间 KL 散度的近似值，用来衡量：

> 一次 PPO 更新之后，新策略和旧策略变化了多少。

直观理解：

- 很小：策略几乎没变；
- 适中：策略正在正常学习；
- 很大：策略更新过猛。

PPO 的核心思想就是：

> 允许策略变好，但不要一次改得太多。

---

## 参考范围

经验上常见：

| approx_kl | 判断 |
|---:|---|
| `< 0.001` | 更新可能过小 |
| `0.001 ~ 0.01` | 通常较稳定 |
| `0.01 ~ 0.03` | 更新较明显，仍可能正常 |
| `> 0.03` | 需要警惕更新过大 |
| 持续快速增大 | 可能发生策略不稳定 |

如果设置：

```text
target_kl = 0.03
```

那么一般希望 `approx_kl` 大部分时间明显低于 `0.03`。

---

## 变大代表什么

`approx_kl ↑`

意味着：

- 新旧策略差异变大；
- Actor 更新更激进；
- 学习速度可能更快；
- 但过大容易造成训练不稳定。

常见原因：

- learning rate 太大；
- `n_epochs` 太多；
- advantage 波动过大；
- reward 方差太大。

---

## 变小代表什么

`approx_kl ↓`

意味着：

- 策略更新更保守；
- 训练更稳定；
- 但过小时可能基本没有学习。

如果长期：

```text
approx_kl ≈ 0
```

同时验证收益也不再改善，就可能是更新太弱。

---

# 3. clip_fraction

## 含义

`clip_fraction` 表示：

> PPO 更新中，有多少比例的样本触发了 clipping。

PPO 的核心比率为：

```text
ratio = π_new(a|s) / π_old(a|s)
```

如果 `clip_range = 0.2`，则希望 ratio 大致保持在：

```text
0.8 ~ 1.2
```

超出范围的部分会被裁剪。

---

## 参考范围

经验参考：

| clip_fraction | 判断 |
|---:|---|
| `0 ~ 0.01` | 更新很小 |
| `0.01 ~ 0.10` | 常见、稳定 |
| `0.10 ~ 0.20` | 更新较积极 |
| `0.20 ~ 0.30` | 偏高，需要观察 |
| `> 0.30` | 更新可能过猛 |

没有要求必须达到某个固定值。

---

## 变大代表什么

`clip_fraction ↑`

说明：

- 更多样本触碰 PPO 更新边界；
- Actor 的策略变化越来越大；
- clipping 开始频繁介入。

如果同时：

```text
approx_kl ↑
clip_fraction ↑
```

通常说明策略更新越来越激进。

---

## 变小代表什么

`clip_fraction ↓`

说明：

- 大部分策略更新没有碰到 clipping；
- Actor 更新比较温和。

如果：

```text
clip_fraction ≈ 0
approx_kl ≈ 0
```

而模型性能长期没有提升，可能说明学习过慢。

---

# 4. clip_range

## 含义

`clip_range` 是 PPO 本身的超参数，不是训练结果。

例如：

```text
clip_range = 0.2
```

表示 PPO 对概率比率的主要约束范围是：

```text
1 - 0.2 ≤ ratio ≤ 1 + 0.2
```

即：

```text
0.8 ≤ ratio ≤ 1.2
```

---

## 常用范围

| clip_range | 特征 |
|---:|---|
| `0.1` | 更保守 |
| `0.2` | 最常用默认值 |
| `0.3` | 更新更宽松 |
| `> 0.3` | 通常需要谨慎 |

---

## 变大

`clip_range ↑`

表示：

- 允许新旧策略变化更大；
- Actor 更新更加激进；
- 可能学习更快；
- 也可能降低稳定性。

---

## 变小

`clip_range ↓`

表示：

- PPO 更新更加保守；
- 稳定性提高；
- 但可能导致学习速度降低。

一般没有明显问题时，`0.2` 是合理起点。

---

# 5. entropy_loss

## 含义

在 Stable-Baselines3 PPO 中，TensorBoard 的：

```text
entropy_loss
```

通常记录的是**负熵**：

```text
entropy_loss = - entropy
```

因此它和普通“loss 越小越好”的理解不同。

对于连续动作高斯策略，entropy 与动作标准差 `std` 密切相关。

---

## 如何理解

策略熵越高：

- 动作越随机；
- 探索越强。

策略熵越低：

- 动作越确定；
- 探索越弱。

由于显示的是负熵，因此在某些连续动作任务中：

```text
entropy_loss 上升
```

可能恰好表示：

```text
entropy 下降
std 下降
策略越来越确定
```

---

## 是否存在正常值

**没有统一正常范围。**

它强烈依赖：

- 动作维度；
- 动作分布；
- `std`；
- `log_std_init`。

因此主要观察**趋势**，不要比较绝对值。

---

## 变大

在常见连续动作 PPO 中：

```text
entropy_loss ↑
```

通常对应：

```text
entropy ↓
std ↓
探索减少
```

这是训练过程中常见现象。

但如果上升过快，同时 `std` 很快趋近于 0：

> 可能发生探索过早消失。

---

## 变小

通常意味着：

```text
entropy ↑
探索 ↑
```

如果长期探索过强，动作可能过于随机，策略难以稳定。

---

# 6. std

## 含义

`std` 是连续动作 PPO 中高斯策略的标准差。

策略可近似理解为：

```text
action ~ Normal(mean, std)
```

其中：

- mean：Actor 认为最合适的动作；
- std：围绕 mean 随机探索的幅度。

---

## 参考范围

没有严格范围。

对于归一化动作空间 `[-1, 1]`，可粗略参考：

| std | 含义 |
|---:|---|
| `> 0.5` | 探索很强 |
| `0.1 ~ 0.5` | 常见范围 |
| `0.05 ~ 0.1` | 策略较确定 |
| `< 0.05` | 需要警惕探索塌缩 |
| 接近 `0` | 几乎完全确定性策略 |

以上只是经验值，不能作为硬阈值。

---

## 变大

`std ↑`

代表：

- 动作随机性提高；
- 探索增强；
- 同一个状态下动作变化更大。

如果过大：

> 策略可能一直很随机，难以稳定执行。

---

## 变小

`std ↓`

通常意味着：

- 模型逐渐找到较稳定的动作；
- 策略越来越确定；
- 探索减少。

训练过程中缓慢下降通常是正常的。

需要警惕：

```text
std 快速 → 0
```

同时验证集性能停止提升，这可能是探索过早消失。

---

# 7. explained_variance

## 含义

`explained_variance` 是判断 Critic 价值函数质量最重要的指标之一。

它衡量：

> Critic 预测的 Value 能解释真实 return 变化的程度。

通常可理解为：

```text
1：预测非常好
0：和直接预测平均值差不多
<0：甚至比预测平均值还差
```

---

## 参考范围

| explained_variance | 判断 |
|---:|---|
| `< 0` | 很差 |
| `0 ~ 0.3` | 较弱 |
| `0.3 ~ 0.5` | 一般 |
| `0.5 ~ 0.8` | 较好、可用 |
| `0.8 ~ 0.95` | 很好 |
| `0.95 ~ 1.0` | 非常好 |

对于复杂强化学习环境，不要求一定达到 `0.9+`。

---

## 变大

`explained_variance ↑`

通常是好现象：

- Critic 对未来回报预测更准确；
- Advantage 估计通常更稳定；
- Actor 得到的学习信号质量提高。

---

## 变小

`explained_variance ↓`

表示 Critic 预测能力下降。

如果短期波动：

> 很正常。

如果长期下降，尤其下降到 `0` 或负数：

可能存在：

- reward 方差过大；
- learning rate 不合适；
- gamma 太大导致长期回报难预测；
- Critic 容量不足；
- 环境非平稳；
- 奖励中存在很大的离散跳变。

---

# 8. value_loss

## 含义

`value_loss` 是 Critic 的价值函数误差。

简单理解：

```text
Value 网络预测值
        ↓
和目标 return 比较
        ↓
产生 value_loss
```

它通常类似均方误差。

---

## 正常值是多少

**没有通用正常值。**

原因是 value loss 的绝对大小直接受 reward 尺度影响。

例如：

```text
reward × 100
```

可能让 value loss 大幅增加，但策略质量不一定发生同等变化。

因此：

> `value_loss = 2` 不一定比 `value_loss = 10` 好。

不同实验只有在 reward 尺度完全一致时才适合比较。

---

## 变大

`value_loss ↑`

可能表示：

- Critic 当前更难预测 return；
- 策略改变后，数据分布发生变化；
- reward 波动增加；
- 产生新的高峰值/大惩罚；
- 长期回报估计变困难。

短期突然升高并不等于训练失败。

---

## 变小

`value_loss ↓`

通常表示：

- Critic 对当前采样数据的拟合更好。

但不能单独判断。

最推荐结合：

```text
value_loss ↓
explained_variance ↑
```

如果两者同时出现，说明 Critic 确实在改善。

---

## 真正危险的情况

需要重点警惕：

```text
value_loss 持续爆炸
+
explained_variance → 0 或负数
+
验证收益持续下降
```

这种情况才比较像 Critic 训练出现问题。

---

# 9. policy_gradient_loss

## 含义

`policy_gradient_loss` 是 Actor 的策略梯度损失。

它体现：

> 当前 PPO 更新正在多大程度上推动策略改变。

在 SB3 PPO 中它通常是一个较小的负数。

---

## 参考范围

没有严格标准。

很多任务中常见：

```text
-1e-4 ~ -1e-2
```

但 reward、batch、advantage normalization 等都会影响它。

---

## 绝对值变大

例如：

```text
-0.001 → -0.005
```

绝对值变大通常意味着：

- 当前策略更新信号更强；
- Actor 更新幅度可能更明显。

如果同时：

```text
approx_kl ↑
clip_fraction ↑
```

说明策略更新确实正在变激进。

---

## 接近 0

例如：

```text
-0.003 → -0.0001
```

通常意味着：

- 当前 batch 给 Actor 的更新信号变弱；
- 策略已经比较稳定；
- 或者优势值较小。

如果长期接近0，同时性能不再改善：

> 可能已经进入平台期。

---

# 10. loss

## 含义

TensorBoard 中的：

```text
train/loss
```

是 PPO 多个损失项组合后的总优化目标。

大致包含：

```text
policy loss
+
value function loss × vf_coef
+
entropy loss × ent_coef
```

因此它同时受到 Actor、Critic 和熵项影响。

---

## 正常值是多少

**没有统一正常值。**

尤其不能使用监督学习中的思维：

```text
loss 必须一直下降
```

PPO 中这是错误的判断方式。

原因是 PPO 每轮训练的数据都是当前策略重新采样得到的，因此：

```text
策略变化
→ 状态访问分布变化
→ reward 分布变化
→ return 变化
→ loss 的目标本身也变化
```

所以 `loss` 上下波动完全正常。

---

## 变大

`loss ↑`

不一定是坏事。

可能只是：

- value loss 上升；
- 当前 rollout 更难拟合；
- 策略进入新的状态区域。

应该继续看：

```text
approx_kl
clip_fraction
explained_variance
验证集收益
```

---

## 变小

`loss ↓`

也不一定意味着策略一定变好了。

如果：

```text
loss 很低
```

但验证收益不提升，则可能只是网络更好地拟合当前 rollout。

---

# 11. learning_rate

## 含义

`learning_rate` 控制神经网络每次梯度更新的步长。

---

## 常见范围

PPO 常见：

```text
1e-5 ~ 3e-4
```

常用起点：

```text
3e-4
1e-4
```

不同任务差异很大。

---

## 变大

`learning_rate ↑`

优点：

- 学习速度可能加快。

风险：

- `approx_kl` 增大；
- `clip_fraction` 增大；
- loss 波动更强；
- 策略可能不稳定。

---

## 变小

`learning_rate ↓`

优点：

- 更新更稳定。

风险：

- 学习速度下降；
- `approx_kl`、`clip_fraction` 可能非常低；
- 很长时间几乎不改善。

---

# 12. 最重要的指标组合

单个指标容易误判，最好成组分析。

---

## 12.1 Actor 是否稳定

重点看：

```text
approx_kl
clip_fraction
policy_gradient_loss
```

### 健康状态

```text
approx_kl：小且有波动
clip_fraction：低到中等
policy_gradient_loss：小幅震荡
```

### 更新过猛

```text
approx_kl ↑↑
clip_fraction ↑↑
policy_gradient_loss 绝对值明显增大
```

此时可以考虑：

- 降低 learning rate；
- 减少 n_epochs；
- 减小 clip_range。

---

# 12.2 Critic 是否学好

重点看：

```text
value_loss
explained_variance
```

### 健康状态

```text
value_loss：总体稳定或下降
explained_variance：逐渐上升
```

### Critic 较差

```text
value_loss：高且持续剧烈震荡
explained_variance：长期 < 0.3
```

### Critic 发散风险

```text
value_loss：持续越来越大
explained_variance：下降到 0 或负数
```

---

# 12.3 探索是否正常

重点看：

```text
std
entropy_loss
```

对于连续动作 PPO，通常：

```text
std ↓
entropy ↓
entropy_loss ↑
```

说明策略逐渐确定。

这是正常的学习过程。

真正需要警惕的是：

```text
std 很快接近 0
+
验证收益停止提升
```

表示探索可能过早消失。

---

# 13. PPO 训练健康状态速查表

| 指标 | 理想趋势 | 太大可能意味着 | 太小可能意味着 |
|---|---|---|---|
| `approx_kl` | 小幅稳定震荡 | 策略更新过猛 | 几乎不更新 |
| `clip_fraction` | 低~中等 | 大量更新被裁剪 | 更新过弱 |
| `clip_range` | 通常固定 | 更新限制较松 | 更新限制较严 |
| `entropy_loss` | 随探索下降而变化 | 可能探索减少 | 可能探索较强 |
| `std` | 缓慢下降 | 探索过强 | 探索不足 |
| `explained_variance` | 越来越接近1 | 一般不是问题 | Critic拟合差 |
| `value_loss` | 有波动但受控 | Critic预测困难 | 不一定代表更好 |
| `policy_gradient_loss` | 小幅震荡 | Actor更新强 | Actor几乎不更新 |
| `loss` | 可以震荡 | 不一定有问题 | 不一定更好 |
| `learning_rate` | 通常固定或缓慢下降 | 更新过快 | 更新过慢 |

---

# 14. 推荐的实际诊断顺序

看到一组 PPO TensorBoard 图时，不要先看 `loss`。

推荐按下面顺序判断。

## 第一步：看 Actor 有没有发散

检查：

```text
approx_kl
clip_fraction
```

如果两者都正常：

> PPO 的策略更新机制基本正常。

---

## 第二步：看 Critic

检查：

```text
explained_variance
value_loss
```

重点是：

```text
explained_variance 是否总体上升
```

而不是要求：

```text
value_loss 必须单调下降
```

---

## 第三步：看探索

检查：

```text
std
entropy_loss
```

正常情况通常是：

```text
std 缓慢下降
策略越来越确定
```

---

## 第四步：最后看真实任务指标

PPO 内部训练指标只能说明：

> “算法训练过程是否健康。”

它们不能直接回答：

> “策略实际效果是否最好。”

最终模型应根据独立验证集上的任务指标选择，例如：

```text
验证集总回报
实际经济收益
约束违规次数
关键业务指标
```

而不是根据：

```text
loss 最低
value_loss 最低
explained_variance 最高
```

单独选择模型。

---

# 15. 一句话记忆版

```text
approx_kl
→ 新旧策略变化有多大

clip_fraction
→ 有多少策略更新撞到了 PPO clipping

clip_range
→ PPO允许策略变化多大的超参数

entropy_loss
→ 策略随机程度对应的熵损失

std
→ 连续动作的探索幅度

explained_variance
→ Critic 对未来回报解释得有多好

value_loss
→ Critic 预测未来回报的误差

policy_gradient_loss
→ Actor 当前策略更新信号大小

loss
→ PPO多个损失项的组合，不能要求单调下降

learning_rate
→ 每次梯度更新走多大一步
```

---

# 16. 最终结论

判断 PPO 训练是否正常，最重要的不是：

```text
loss 有没有一直下降
```

而是：

```text
1. approx_kl 是否处于稳定的小范围
2. clip_fraction 是否没有长期过高
3. explained_variance 是否总体提高
4. value_loss 是否有界而非持续爆炸
5. std 是否缓慢变化而不是快速塌缩
6. 独立验证集真实任务性能是否提高
```

因此：

> **PPO 的 loss 和 value_loss 出现震荡甚至阶段性升高，本身并不能说明训练失败。最终判断必须结合 Actor 更新稳定性、Critic 预测能力以及独立验证集效果。**
