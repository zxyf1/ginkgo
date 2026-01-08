# CG 迭代日志增强说明

## 概述

在 `custom-logger.cpp` 的基础上添加了对 CG（共轭梯度法）迭代内部计算步骤的详细记录功能。

## 新增功能

### 1. 迭代内部操作追踪

增强的 `ResidualLogger` 现在可以追踪并记录 CG 算法每次迭代内的关键操作：

- **SpMV (A*p->q)**: 稀疏矩阵-向量乘法
  - 输入：搜索方向向量 p
  - 输出：结果向量 q = A*p

- **Precond (M*r->z)**: 预条件化操作
  - 输入：残差向量 r
  - 输出：预条件后的向量 z = M⁻¹r

每个操作都会记录输入和输出向量的 L2 范数。

### 2. 新增的类成员

```cpp
struct OperationLog {
    std::string operation_type;  // 操作类型
    RealValueType input_norm;    // 输入向量范数
    RealValueType output_norm;   // 输出向量范数
};
```

### 3. 新增的方法

- `on_linop_apply_completed()`: 拦截线性算子应用完成事件
- `set_system_matrix()`: 设置系统矩阵引用以识别 SpMV 操作
- `set_preconditioner()`: 设置预条件子引用以识别预条件化操作
- `needs_propagation()`: 重写此方法返回 true，允许接收从 executor 传播的事件

## CG 算法步骤解析

每次 CG 迭代包含以下步骤：

1. **z = M⁻¹r** - 预条件化（Precond 操作）
2. **ρ = r†z** - 计算内积
3. **p = z + (ρ/ρ_prev)p** - 更新搜索方向
4. **q = Ap** - 矩阵向量乘法（SpMV 操作）
5. **β = p†q** - 计算内积
6. **α = ρ/β** - 计算步长
7. **x = x + αp** - 更新解向量
8. **r = r - αq** - 更新残差

Logger 捕获其中的两个关键 LinOp 操作（步骤 1 和 4）。

## 输出示例

运行增强版的 custom-logger 将产生两部分输出：

### 迭代级别摘要（保留原有功能）

```
========================================
Iteration-level Summary:
========================================
| Iteration|  Recurrent Residual Norm|     True Residual Norm|  Implicit Residual Norm|
|----------|-------------------------|------------------------|------------------------|
|         0|         1.234567e-01|         1.234567e-01|         1.234567e-01|
|         1|         5.678901e-02|         5.678901e-02|         5.678901e-02|
...
```

### 详细操作日志（新增功能）

```
========================================
Detailed CG Iteration Operations:
========================================

--- Iteration 0 ---
           Operation |         Input Norm |        Output Norm
----------------------------------------------------------------------
   Precond (M*r->z) |   1.234567e-01 |   1.234567e-01
     SpMV (A*p->q) |   1.234567e-01 |   2.345678e-01

--- Iteration 1 ---
           Operation |         Input Norm |        Output Norm
----------------------------------------------------------------------
   Precond (M*r->z) |   5.678901e-02 |   5.678901e-02
     SpMV (A*p->q) |   6.789012e-02 |   1.234567e-01
...
```

## 使用方法

1. **编译**：
   ```bash
   cd ginkgo
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release -DGINKGO_BUILD_EXAMPLES=ON
   make custom-logger
   ```

2. **运行**：
   ```bash
   cd examples/custom-logger
   ./custom-logger [executor]
   ```

   可选的执行器：`reference`（默认）、`omp`、`cuda`、`hip`、`dpcpp`

## 代码修改总结

1. 在 `ResidualLogger` 类中添加了 `OperationLog` 结构体
2. 增强了 `write()` 方法以显示详细的操作日志
3. 添加了 `on_linop_apply_completed()` 方法来拦截 LinOp 应用事件
4. 添加了 `set_system_matrix()` 和 `set_preconditioner()` 方法
5. 重写了 `needs_propagation()` 方法返回 true
6. 更新了构造函数以包含 `linop_apply_completed_mask`
7. 在 `main()` 函数中：
   - 将 logger 添加到 executor（捕获所有 LinOp 操作）
   - 将 logger 添加到 solver factory（捕获迭代信息）
   - 设置系统矩阵和预条件子引用

## 关键实现细节

### Logger 事件传播机制

为了捕获 CG 迭代内部的 LinOp 操作（SpMV 和预条件化），需要：

1. **重写 `needs_propagation()`**：返回 true，使 logger 可以接收从 executor 传播的事件
2. **添加到 executor**：`exec->add_logger(logger)` - 这样可以捕获在该 executor 上执行的所有 LinOp 操作
3. **添加到 solver factory**：`solver_gen->add_logger(logger)` - 这样可以捕获迭代完成事件

### 操作过滤

由于 logger 会接收所有 LinOp apply 事件，`on_linop_apply_completed()` 方法实现了过滤机制：
- 只记录与 `system_matrix_` 或 `preconditioner_` 匹配的操作
- 忽略其他不相关的 LinOp 操作

## 与原版的区别

| 特性 | 原版 | 增强版 |
|------|------|--------|
| 迭代级别残差记录 | ✓ | ✓ |
| 操作级别详细日志 | ✗ | ✓ |
| LinOp apply 追踪 | ✗ | ✓ |
| 操作类型识别 | ✗ | ✓ |
| 向量范数计算 | ✓ | ✓（更详细） |

## 扩展建议

可以进一步扩展来：

1. **记录标量值**：追踪 ρ、β、α 等标量的值变化
2. **支持其他求解器**：适配 BiCGStab、GMRES 等迭代求解器
3. **性能计时**：添加每个操作的执行时间记录
4. **向量快照**：保存特定迭代的完整向量内容用于深度分析

## 参考

- Ginkgo Logger 文档: https://ginkgo-project.github.io/ginkgo/doc/develop/classgko_1_1log_1_1Logger.html
- CG 算法: https://en.wikipedia.org/wiki/Conjugate_gradient_method
