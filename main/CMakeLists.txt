idf_component_register(SRCS "main.c"
                            "state.c"
                            "bluetooth.c"
                            "encoding.c"
                            "haptics.c"
                            "sensors.c"
                            "iir_filter.c" "old_filter.c" "filter.c"
                            INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
