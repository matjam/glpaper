install(
    TARGETS glpaper_exe
    RUNTIME COMPONENT glpaper_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
