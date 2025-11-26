# Abstract SpMV 独立版本与原始版本对比

本文档详细列出 `abstract_spmv_minimal` 与原始 Ginkgo `csr_kernels.template.cpp` 的所有差异。

## 📋 对比概览

| 维度 | 原始版本 | 独立版本 | 差异原因 |
|------|----------|----------|----------|
| **文件位置** | `common/cuda_hip/matrix/csr_kernels.template.cpp` | `abstract_spmv_minimal/abstract_spmv_standalone.cu` | 独立组织 |
| **代码行数** | ~2200行（包含其他kernels） | ~500行（仅SpMV相关） | 仅提取必要代码 |
| **依赖项** | 依赖整个Ginkgo库 | 完全独立，无外部依赖 | 自包含所有依赖 |
| **模板特化** | 完全泛型 | 特化为int32/double | 简化使用 |

---

## 🔍 详细修改列表

### 1. 文件头部和包含项

#### ✅ 原始版本（第5-55行）
```cpp
#include "core/matrix/csr_kernels.hpp"
#include <algorithm>
#include <thrust/copy.h>
#include <thrust/count.h>
// ... 20+ 个 Ginkgo 内部头文件
#include "accessor/cuda_hip_helper.hpp"
#include "common/cuda_hip/base/config.hpp"
#include "common/cuda_hip/base/math.hpp"
// ... 等等
```

#### ✏️ 独立版本（第8-12行）
```cpp
#include <cuda_runtime.h>
#include <cooperative_groups.h>
#include <cstdint>
#include <cstdio>
#include <utility>
```

**修改原因：**
- 移除所有 Ginkgo 内部依赖
- 仅保留必要的 CUDA 和 C++ 标准库头文件
- 添加 `<utility>` 用于 `std::forward` 和 `std::declval`

---

### 2. 命名空间结构

#### ✅ 原始版本（第58-66行）
```cpp
namespace gko {
namespace kernels {
namespace GKO_DEVICE_NAMESPACE {  // 根据编译目标展开为 cuda 或 hip
namespace csr {
```

#### ✏️ 独立版本
```cpp
// 无命名空间，或仅使用必要的局部命名空间
namespace config {
    constexpr uint32 warp_size = 32;
}
namespace acc { ... }
namespace group { ... }
```

**修改原因：**
- 移除 Ginkgo 特定的命名空间结构
- 简化命名空间层次
- 直接定义必要的子命名空间

---

### 3. 基础类型定义

#### ✅ 原始版本
```cpp
// 依赖 ginkgo/core/base/types.hpp
// 使用 Ginkgo 的 size_type, int32, uint32 等
```

#### ✏️ 独立版本（第17-18行）
```cpp
using size_type = std::int64_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
```

**修改原因：**
- 显式定义类型别名
- 避免依赖 Ginkgo 类型系统

---

### 4. 数学工具函数

#### ✅ 原始版本
```cpp
// 依赖 ginkgo/core/base/math.hpp 中的 zero<T>(), one<T>() 等
```

#### ✏️ 独立版本（第36-66行）
```cpp
template <typename T>
__host__ __device__ __forceinline__ constexpr T zero()
{
    return T{};
}

template <typename T>
__host__ __device__ __forceinline__ constexpr T one()
{
    return T(1);
}

template <typename T>
__host__ __device__ __forceinline__ T ceildivT(T nom, T denom)
{
    return (nom + denom - 1ll) / denom;
}

template <typename T>
__host__ __device__ __forceinline__ T min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
__host__ __device__ __forceinline__ T max(T a, T b)
{
    return a > b ? a : b;
}
```

**修改原因：**
- 内联实现基础数学函数
- 提供 SpMV kernel 所需的所有数学工具

---

### 5. Atomic 操作

#### ✅ 原始版本
```cpp
// 依赖 common/cuda_hip/components/atomic.hpp
// 支持多种类型的 atomic_add
```

#### ✏️ 独立版本（第72-75行）
```cpp
__forceinline__ __device__ double atomic_add(double* __restrict__ addr, double val)
{
    return atomicAdd(addr, val);
}
```

**修改原因：**
- 仅保留 `double` 类型的 atomic_add
- 简化实现，特化为所需类型

---

### 6. Cooperative Groups

#### ✅ 原始版本
```cpp
// 依赖 common/cuda_hip/components/cooperative_groups.hpp
// 包含自定义的 grid_group 等扩展
```

#### ✏️ 独立版本（第80-85行）
```cpp
namespace group {
    using cooperative_groups::thread_block;
    using cooperative_groups::thread_block_tile;
    using cooperative_groups::this_thread_block;
    using cooperative_groups::tiled_partition;
}
```

**修改原因：**
- 直接使用 CUDA 标准 cooperative_groups
- 移除 Ginkgo 的扩展功能
- 仅保留 SpMV 所需的基本功能

---

### 7. Accessor 系统

#### ✅ 原始版本
```cpp
// 依赖 accessor/cuda_hip_helper.hpp
// 使用完整的 Ginkgo accessor 系统
// 包括 reduced_row_major, scaled_reduced_row_major 等多种 accessor
```

#### ✏️ 独立版本（第92-183行）
```cpp
namespace acc {

template <typename ValueType, typename IndexType>
class simple_row_major_2d {
    // 简化的 2D accessor 实现
    // 仅包含 operator(), get_storage_address(), length()
};

template <typename ValueType, typename IndexType>
class simple_1d {
    // 简化的 1D accessor 实现
    // 仅包含 operator()
};

template <typename Accessor>
class range {
    // 简化的 range 包装器
    // 仅包含必要的操作符重载
};

}  // namespace acc
```

**修改原因：**
- 实现简化版的 accessor 系统
- 仅支持 SpMV 所需的基本功能
- 移除复杂的类型转换和缩放功能
- 特化为 `int32` 索引和 `double` 值类型

**关键差异：**
1. 不支持 `reduced_row_major` 的压缩存储
2. 不支持 `scaled_reduced_row_major` 的缩放功能
3. 不支持复数类型
4. 不支持半精度类型（half, bfloat16）

---

### 8. Segment Scan

#### ✅ 原始版本
```cpp
// 依赖 common/cuda_hip/components/segment_scan.hpp
```

#### ✏️ 独立版本（第189-207行）
```cpp
template <unsigned subwarp_size, typename ValueType, typename IndexType,
          typename Operator>
__device__ __forceinline__ bool segment_scan(
    const group::thread_block_tile<subwarp_size>& group, const IndexType ind,
    ValueType& val, Operator op)
{
    // 完整的 segment_scan 实现
    // 与原始版本完全相同
}
```

**修改原因：**
- 内联实现，避免依赖外部文件
- **代码逻辑完全一致**

---

### 9. 核心 SpMV 辅助函数

#### 函数 `warp_atomic_add`

##### ✅ 原始版本（第148-164行）
```cpp
template <unsigned subwarp_size, typename ValueType, typename IndexType,
          typename output_accessor, typename Closure>
__device__ __forceinline__ void warp_atomic_add(
    const group::thread_block_tile<subwarp_size>& group, bool force_write,
    ValueType& val, const IndexType row, acc::range<output_accessor>& c,
    const IndexType column_id, Closure scale)
{
    // 实现代码相同
}
```

##### ✏️ 独立版本（第240-256行）
```cpp
template <unsigned subwarp_size, typename ValueType, typename IndexType,
          typename output_accessor, typename Closure>
__device__ __forceinline__ void warp_atomic_add(
    const group::thread_block_tile<subwarp_size>& group, bool force_write,
    ValueType& val, const IndexType row, acc::range<output_accessor>& c,
    const IndexType column_id, Closure&& scale)  // ⚠️ 修改：Closure -> Closure&&
{
    // 实现代码相同
}
```

**修改点：**
- `Closure scale` → `Closure&& scale`（添加右值引用）

**修改原因：**
- 支持完美转发
- 解决模板参数推导问题
- 避免不必要的 lambda 拷贝

---

#### 函数 `process_window`

##### ✅ 原始版本（第167-194行）
```cpp
template <bool last, unsigned subwarp_size, typename arithmetic_type,
          typename matrix_accessor, typename IndexType, typename input_accessor,
          typename output_accessor, typename Closure>
__device__ __forceinline__ void process_window(
    const group::thread_block_tile<subwarp_size>& group,
    const IndexType num_rows, const IndexType data_size, const IndexType ind,
    IndexType& row, IndexType& row_end, IndexType& nrow, IndexType& nrow_end,
    arithmetic_type& temp_val, acc::range<matrix_accessor> val,
    const IndexType* __restrict__ col_idxs,
    const IndexType* __restrict__ row_ptrs, acc::range<input_accessor> b,
    acc::range<output_accessor> c, const IndexType column_id, Closure scale)
{
    const auto curr_row = row;
    find_next_row<last>(...);
    if (group.any(curr_row != row)) {
        warp_atomic_add(group, curr_row != row, temp_val, curr_row, c,
                        column_id, scale);  // ⚠️ 无模板参数
        // ...
    }
    // ...
}
```

##### ✏️ 独立版本（第258-285行）
```cpp
template <bool last, unsigned subwarp_size, typename ArithmeticType,  // ⚠️ 重命名
          typename MatrixAccessor, typename IndexType, typename InputAccessor,  // ⚠️ 重命名
          typename OutputAccessor, typename Closure>                             // ⚠️ 重命名
__device__ __forceinline__ void process_window(
    const group::thread_block_tile<subwarp_size>& group,
    const IndexType num_rows, const IndexType data_size, const IndexType ind,
    IndexType& row, IndexType& row_end, IndexType& nrow, IndexType& nrow_end,
    ArithmeticType& temp_val, acc::range<MatrixAccessor> val,
    const IndexType* __restrict__ col_idxs,
    const IndexType* __restrict__ row_ptrs, acc::range<InputAccessor> b,
    acc::range<OutputAccessor> c, const IndexType column_id, Closure&& scale)  // ⚠️ 修改
{
    const auto curr_row = row;
    find_next_row<last>(...);
    if (group.any(curr_row != row)) {
        warp_atomic_add<subwarp_size>(group, curr_row != row, temp_val, curr_row, c,
                        column_id, std::forward<Closure>(scale));  // ⚠️ 显式模板参数 + forward
        // ...
    }
    // ...
}
```

**修改点：**
1. 模板参数重命名：
   - `arithmetic_type` → `ArithmeticType`
   - `matrix_accessor` → `MatrixAccessor`
   - `input_accessor` → `InputAccessor`
   - `output_accessor` → `OutputAccessor`
2. `Closure scale` → `Closure&& scale`
3. `warp_atomic_add(...)` → `warp_atomic_add<subwarp_size>(..., std::forward<Closure>(scale))`

**修改原因：**
- PascalCase 命名提高可读性
- 完美转发避免拷贝
- 显式模板参数解决推导失败

---

#### 函数 `spmv_kernel`

##### ✅ 原始版本（第237-246行）
```cpp
for (; ind < ind_end; ind += wsize) {
    process_window<false>(tile_block, num_rows, data_size, ind, row,
                          row_end, nrow, nrow_end, temp_val, val, col_idxs,
                          row_ptrs, b, c, column_id, scale);  // ⚠️ 无 wsize
}
process_window<true>(tile_block, num_rows, data_size, ind, row, row_end,
                     nrow, nrow_end, temp_val, val, col_idxs, row_ptrs, b,
                     c, column_id, scale);  // ⚠️ 无 wsize
warp_atomic_add(tile_block, true, temp_val, row, c, column_id, scale);  // ⚠️ 无模板参数
```

##### ✏️ 独立版本（第326-334行）
```cpp
for (; ind < ind_end; ind += wsize) {
    process_window<false, wsize>(tile_block, num_rows, data_size, ind, row,
                          row_end, nrow, nrow_end, temp_val, val, col_idxs,
                          row_ptrs, b, c, column_id, scale);  // ⚠️ 添加 wsize
}
process_window<true, wsize>(tile_block, num_rows, data_size, ind, row, row_end,
                     nrow, nrow_end, temp_val, val, col_idxs, row_ptrs, b,
                     c, column_id, scale);  // ⚠️ 添加 wsize
warp_atomic_add<wsize>(tile_block, true, temp_val, row, c, column_id, scale);  // ⚠️ 添加 wsize
```

**修改点：**
- `process_window<false>` → `process_window<false, wsize>`
- `process_window<true>` → `process_window<true, wsize>`
- `warp_atomic_add` → `warp_atomic_add<wsize>`

**修改原因：**
- 显式指定 `subwarp_size` 模板参数
- 解决 NVCC 模板参数推导失败
- 确保编译通过

---

### 10. 主 Kernel 函数 `abstract_spmv`

#### ✅ 原始版本（第249-288行）
```cpp
// 两个重载版本
template <typename matrix_accessor, typename input_accessor,
          typename output_accessor, typename IndexType>
__global__ __launch_bounds__(spmv_block_size) void abstract_spmv(
    const IndexType nwarps, const IndexType num_rows,
    acc::range<matrix_accessor> val, const IndexType* __restrict__ col_idxs,
    const IndexType* __restrict__ row_ptrs, const IndexType* __restrict__ srow,
    acc::range<input_accessor> b, acc::range<output_accessor> c)
{
    // 无 alpha 版本
}

template <typename matrix_accessor, typename input_accessor,
          typename output_accessor, typename IndexType>
__global__ __launch_bounds__(spmv_block_size) void abstract_spmv(
    const IndexType nwarps, const IndexType num_rows,
    const typename matrix_accessor::storage_type* __restrict__ alpha,
    acc::range<matrix_accessor> val, const IndexType* __restrict__ col_idxs,
    const IndexType* __restrict__ row_ptrs, const IndexType* __restrict__ srow,
    acc::range<input_accessor> b, acc::range<output_accessor> c)
{
    // 带 alpha 缩放版本
}
```

#### ✏️ 独立版本（第342-395行）
```cpp
// 完全相同的两个重载版本
// 代码逻辑 100% 一致
```

**修改点：** **无修改，完全一致**

---

### 11. 模板显式实例化

#### ✅ 原始版本
```cpp
// 无显式实例化
// 通过 Ginkgo 的模板系统自动实例化
```

#### ✏️ 独立版本（第377-394行）
```cpp
// Explicit instantiation for the common case: IndexType=int32, ValueType=double
using DoubleAccessor1D = acc::simple_1d<double, int32>;
using DoubleAccessor2D = acc::simple_row_major_2d<double, int32>;

// Version without alpha
template __global__ void abstract_spmv<DoubleAccessor1D, DoubleAccessor2D,
                                        DoubleAccessor2D, int32>(
    const int32 nwarps, const int32 num_rows,
    acc::range<DoubleAccessor1D> val, const int32* __restrict__ col_idxs,
    const int32* __restrict__ row_ptrs, const int32* __restrict__ srow,
    acc::range<DoubleAccessor2D> b, acc::range<DoubleAccessor2D> c);

// Version with alpha
template __global__ void abstract_spmv<DoubleAccessor1D, DoubleAccessor2D,
                                        DoubleAccessor2D, int32>(
    const int32 nwarps, const int32 num_rows,
    const double* __restrict__ alpha,
    acc::range<DoubleAccessor1D> val, const int32* __restrict__ col_idxs,
    const int32* __restrict__ row_ptrs, const int32* __restrict__ srow,
    acc::range<DoubleAccessor2D> b, acc::range<DoubleAccessor2D> c);
```

**修改原因：**
- 显式实例化 `int32/double` 特化版本
- 简化使用，避免模板实例化问题
- 减小编译后代码体积

---

### 12. 辅助函数：compute_srow

#### ✅ 原始版本
```cpp
// 不存在于 kernel 文件中
// 可能在其他文件或调用代码中实现
```

#### ✏️ 独立版本（第397-413行）
```cpp
__global__ void compute_srow_kernel(const int32 num_rows, const int32 nwarps,
                                     const int32* __restrict__ row_ptrs,
                                     int32* __restrict__ srow)
{
    const int32 warp_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (warp_idx >= nwarps) {
        return;
    }

    const int32 nnz = row_ptrs[num_rows];
    const int32 start_idx = get_warp_start_idx(nwarps, nnz, warp_idx);

    // Binary search to find the row that contains start_idx
    int32 left = 0;
    int32 right = num_rows;
    while (left < right) {
        int32 mid = (left + right) / 2;
        if (row_ptrs[mid] <= start_idx) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    srow[warp_idx] = left > 0 ? left - 1 : 0;
}
```

**修改原因：**
- 提供完整的 `srow` 数组计算实现
- 方便用户独立使用
- 原始版本可能分散在其他文件中

---

## 📊 完整修改汇总表

| 类别 | 修改项 | 位置 | 原始 | 独立版本 | 原因 |
|------|--------|------|------|----------|------|
| **依赖** | 头文件 | 顶部 | 20+ Ginkgo头文件 | 5个标准头文件 | 移除外部依赖 |
| **命名空间** | 结构 | 全局 | `gko::kernels::cuda::csr` | 简化/移除 | 独立组织 |
| **类型** | 定义 | 类型 | 依赖Ginkgo | 显式定义 | 自包含 |
| **数学** | 工具函数 | 全局 | 依赖math.hpp | 内联实现 | 独立实现 |
| **Atomic** | 操作 | 全局 | 多类型支持 | 仅double | 特化简化 |
| **Groups** | cooperative | 全局 | 自定义扩展 | 标准CUDA | 简化实现 |
| **Accessor** | 系统 | 全局 | 完整系统 | 简化版本 | 最小功能 |
| **Segment** | scan | 函数 | 外部依赖 | 内联实现 | 自包含 |
| **warp_atomic_add** | 签名 | 函数 | `Closure` | `Closure&&` | 完美转发 |
| **process_window** | 模板参数 | 函数 | snake_case | PascalCase | 规范命名 |
| **process_window** | 签名 | 函数 | `Closure` | `Closure&&` | 完美转发 |
| **process_window** | 调用 | 内部 | 无模板参数 | `<subwarp_size>` | 显式指定 |
| **spmv_kernel** | 调用 | 循环 | `<false>` | `<false, wsize>` | 显式指定 |
| **spmv_kernel** | 调用 | 循环后 | `<true>` | `<true, wsize>` | 显式指定 |
| **spmv_kernel** | 调用 | 最后 | 无参数 | `<wsize>` | 显式指定 |
| **abstract_spmv** | 主kernel | kernel | 泛型 | **完全一致** | 保留原样 |
| **实例化** | 显式 | 底部 | 无 | int32/double | 简化使用 |
| **compute_srow** | 辅助kernel | 新增 | 无 | 完整实现 | 便利功能 |

---

## 🎯 核心代码逻辑对比

### ✅ **完全保持一致的部分**

以下核心算法**完全没有修改**，与原始 Ginkgo 100% 一致：

1. ✅ **find_next_row** - 行定位逻辑
2. ✅ **segment_scan** - 分段扫描算法
3. ✅ **get_warp_start_idx** - Warp 起始索引计算
4. ✅ **spmv_kernel** - 主计算逻辑（除了调用点显式模板参数）
5. ✅ **abstract_spmv** - Kernel 入口函数
6. ✅ **warp_atomic_add** - 原子操作逻辑（仅签名改为 `Closure&&`）
7. ✅ **process_window** - 窗口处理逻辑（仅签名改为 `Closure&&` 和模板参数名）

### ⚠️ **修改的部分**

仅涉及以下几个方面：

1. **移除依赖** - 自实现基础工具函数
2. **简化 Accessor** - 仅支持必要的访问模式
3. **模板推导修复** - 添加显式模板参数和完美转发
4. **特化** - 针对 `int32/double` 特化

---

## 🔬 性能影响分析

| 修改 | 性能影响 | 说明 |
|------|----------|------|
| 显式模板参数 | **无影响** | 编译时确定，运行时零开销 |
| `Closure&&` 完美转发 | **无影响或略微提升** | 避免不必要的 lambda 拷贝 |
| 简化 Accessor | **无影响** | 同样的内存访问模式 |
| int32/double 特化 | **可能略微提升** | 避免泛型实例化开销 |

**结论：** 性能应该与原始版本**完全一致**或略有提升。

---

## 📝 总结

### 修改的核心原则

1. **保持算法一致** - 核心计算逻辑不变
2. **移除依赖** - 自包含所有必要代码
3. **修复编译** - 解决模板推导问题
4. **简化特化** - 针对常用类型优化

### 关键修改位置

| 位置 | 修改 |
|------|------|
| **全局** | 移除 Ginkgo 依赖，自实现基础功能 |
| **Accessor** | 简化实现，仅支持 int32/double |
| **warp_atomic_add** | `Closure` → `Closure&&` |
| **process_window** | 模板参数 PascalCase + `Closure&&` + 内部显式 `<subwarp_size>` |
| **spmv_kernel** | 调用时显式 `<wsize>` |
| **abstract_spmv** | **完全一致，无修改** |

### 可用性对比

| 特性 | 原始版本 | 独立版本 |
|------|----------|----------|
| 依赖 Ginkgo | ✅ 必须 | ❌ 不需要 |
| 支持多种类型 | ✅ 完整支持 | ⚠️ 仅 int32/double |
| 支持复数 | ✅ | ❌ |
| 支持半精度 | ✅ | ❌ |
| 独立编译 | ❌ | ✅ |
| 易于集成 | ⚠️ 需要整个Ginkgo | ✅ 单文件 |
| 学习曲线 | ⚠️ 需了解Ginkgo | ✅ 简单直接 |

---

**最重要的结论：**

✅ **核心 SpMV 算法逻辑与原始 Ginkgo 完全一致**
✅ **所有修改都是为了独立性和编译修复**
✅ **性能应该与原始版本相同**
✅ **更易于理解和集成到外部项目**
