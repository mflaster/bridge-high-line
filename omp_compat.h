#ifndef BRIDGE_OMP_COMPAT_H_
#define BRIDGE_OMP_COMPAT_H_

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_thread_num() { return 0; }
inline int omp_get_max_threads() { return 1; }
inline void omp_set_num_threads(int) {}
inline int omp_in_parallel() { return 0; }
#endif

#endif  // BRIDGE_OMP_COMPAT_H_
