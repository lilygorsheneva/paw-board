idf_component_register(SRCS "bluetooth.c" "encoding.c"
                            "haptics.c"
                            "sensors.c"
                            "main.c"
                            INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
