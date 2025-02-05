# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(parallel-noise-reductionTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(parallel-noise-reduction_test source/parallel-noise-reduction_test.cpp)
target_link_libraries(
    parallel-noise-reduction_test PRIVATE
    parallel-noise-reduction_lib
    Catch2::Catch2WithMain
)
target_compile_features(parallel-noise-reduction_test PRIVATE cxx_std_20)

catch_discover_tests(parallel-noise-reduction_test)

# ---- End-of-file commands ----

add_folders(Test)
