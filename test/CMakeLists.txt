# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(parallel-noise-reductionTests LANGUAGES CXX)

# ---- Dependencies ----


# ---- Tests ----



# ---- End-of-file commands ----

add_folders(Test)
