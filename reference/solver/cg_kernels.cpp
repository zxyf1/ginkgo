// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#include "core/solver/cg_kernels.hpp"

#include <iomanip>
#include <iostream>

#include <ginkgo/core/base/array.hpp>
#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/base/math.hpp>
#include <ginkgo/core/base/types.hpp>


namespace gko {
namespace kernels {
namespace reference {
/**
 * @brief The CG solver namespace.
 *
 * @ingroup cg
 */
namespace cg {


template <typename ValueType>
void initialize(std::shared_ptr<const ReferenceExecutor> exec,
                const matrix::Dense<ValueType>* b, matrix::Dense<ValueType>* r,
                matrix::Dense<ValueType>* z, matrix::Dense<ValueType>* p,
                matrix::Dense<ValueType>* q, matrix::Dense<ValueType>* prev_rho,
                matrix::Dense<ValueType>* rho,
                array<stopping_status>* stop_status)
{
    for (size_type j = 0; j < b->get_size()[1]; ++j) {
        rho->at(j) = zero<ValueType>();
        prev_rho->at(j) = one<ValueType>();
        stop_status->get_data()[j].reset();
    }
    for (size_type i = 0; i < b->get_size()[0]; ++i) {
        for (size_type j = 0; j < b->get_size()[1]; ++j) {
            r->at(i, j) = b->at(i, j);
            z->at(i, j) = p->at(i, j) = q->at(i, j) = zero<ValueType>();
        }
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_TYPE(GKO_DECLARE_CG_INITIALIZE_KERNEL);


template <typename ValueType>
void step_1(std::shared_ptr<const ReferenceExecutor> exec,
            matrix::Dense<ValueType>* p, const matrix::Dense<ValueType>* z,
            const matrix::Dense<ValueType>* rho,
            const matrix::Dense<ValueType>* prev_rho,
            const array<stopping_status>* stop_status)
{
    for (size_type i = 0; i < p->get_size()[0]; ++i) {
        for (size_type j = 0; j < p->get_size()[1]; ++j) {
            if (stop_status->get_const_data()[j].has_stopped()) {
                continue;
            }
            if (is_zero(prev_rho->at(j))) {
                p->at(i, j) = z->at(i, j);
            } else {
                auto tmp = rho->at(j) / prev_rho->at(j);
                p->at(i, j) = z->at(i, j) + tmp * p->at(i, j);
            }
        }
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_TYPE(GKO_DECLARE_CG_STEP_1_KERNEL);


template <typename ValueType>
void step_2(std::shared_ptr<const ReferenceExecutor> exec,
            matrix::Dense<ValueType>* x, matrix::Dense<ValueType>* r,
            const matrix::Dense<ValueType>* p,
            const matrix::Dense<ValueType>* q,
            const matrix::Dense<ValueType>* beta,
            const matrix::Dense<ValueType>* rho,
            const array<stopping_status>* stop_status)
{
    for (size_type i = 0; i < x->get_size()[0]; ++i) {
        for (size_type j = 0; j < x->get_size()[1]; ++j) {
            if (stop_status->get_const_data()[j].has_stopped()) {
                continue;
            }
            if (is_nonzero(beta->at(j))) {
                auto tmp = rho->at(j) / beta->at(j);

                // Print "before" state - only first row (i=0) of first column prints all rows
                if (j == 0 && i == 0) {
                    std::cout << "\n=== step_2 Debug (Reference) [col=" << j << "] ===" << std::endl;
                    std::cout << "  rho[col] = " << std::scientific << std::setprecision(10)
                              << real(rho->at(j)) << std::endl;
                    std::cout << "  beta[col] = " << std::scientific << std::setprecision(10)
                              << real(beta->at(j)) << std::endl;
                    std::cout << "  alpha = rho/beta = " << std::scientific << std::setprecision(10)
                              << real(tmp) << std::endl;
                    std::cout << "\n  Before update (first 3 rows):" << std::endl;
                    for (size_type k = 0; k < 3 && k < x->get_size()[0]; k++) {
                        std::cout << "    [row=" << k << "] x=" << std::scientific << std::setprecision(10)
                                  << real(x->at(k, j)) << ", p=" << real(p->at(k, j))
                                  << ", r=" << real(r->at(k, j)) << ", q=" << real(q->at(k, j))
                                  << std::endl;
                    }
                }

                x->at(i, j) += tmp * p->at(i, j);
                r->at(i, j) -= tmp * q->at(i, j);

                // Print "after" state - only third row (i=2) of first column prints all rows
                if (j == 0 && i == 2) {
                    std::cout << "\n  After update (first 3 rows):" << std::endl;
                    for (size_type k = 0; k < 3 && k < x->get_size()[0]; k++) {
                        std::cout << "    [row=" << k << "] x=" << std::scientific << std::setprecision(10)
                                  << real(x->at(k, j)) << " (new), r=" << real(r->at(k, j))
                                  << " (new)" << std::endl;
                    }
                    std::cout << "=== End step_2 ===\n" << std::endl;
                }
            }
        }
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_TYPE(GKO_DECLARE_CG_STEP_2_KERNEL);


}  // namespace cg
}  // namespace reference
}  // namespace kernels
}  // namespace gko
