install(
    TARGETS parallel-noise-reduction_exe
    RUNTIME COMPONENT parallel-noise-reduction_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
