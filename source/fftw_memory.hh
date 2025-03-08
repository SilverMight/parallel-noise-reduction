#include <memory>
#include <type_traits>
#include "fftw3.h"

// Utility functions & types for handling fftw aligned memory 
// w/ C++ conventions
namespace fftw_memory {
    // Custom deleter for FFTW allocations
    template<typename T>
    struct fftw_deleter {
        void operator()(T* ptr) const noexcept {
            fftw_free(ptr);
        }
    };

    // Specialization for FFTW plans 
    // note: fftw_plan is an opaque pointer
    using fftw_plan_deref = std::remove_pointer_t<fftw_plan>;
    template<>
    struct fftw_deleter<fftw_plan_deref> {
        void operator()(fftw_plan plan) const noexcept {
            fftw_destroy_plan(plan);
        }
    };
    

    template<class T>
    using fftw_unique_ptr = std::unique_ptr<T, fftw_deleter<T>>;

    // unique ptr for fftw plans
    // don't provide a separate make_* function, as plans have a lot of different
    // instantiation functions.
    using fftw_plan_unique_ptr = fftw_unique_ptr<fftw_plan_deref>;

    template<class T>
    fftw_unique_ptr<T> make_fftw_unique(size_t size) {
        return fftw_unique_ptr<T>{static_cast<T*>(fftw_malloc(size * sizeof(T)))};
    }

} // namespace fftw_memory
