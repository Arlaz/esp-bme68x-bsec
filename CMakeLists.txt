set(bme680_driver_dir "bme680_driver")
set(bsec_dir "bsec")


# Set exact profile name per the directory under config. This is one of the many
set (BME_PROFILE "generic_33v_3s_28d")

idf_component_register(
        SRCS
            ${bme680_driver_dir}/bme680.c

            ${bsec_dir}/bsec_integration.c
            ${bsec_dir}/config/${BME_PROFILE}/bsec_serialized_configurations_iaq.c

        INCLUDE_DIRS
            ${bme680_driver_dir}

            ${bsec_dir}
            ${bsec_dir}/algo/normal_version/inc
            ${bsec_dir}/config/${BME_PROFILE}

)

# The integration code gives plenty of these warnings so they are muted down in this component
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-but-set-variable -Wno-unused-variable)

# Declaring the precompiled BSEC lib as imported global so that it doesn't need to be recompiled
add_library(bseclib STATIC IMPORTED GLOBAL)
set_target_properties(bseclib PROPERTIES IMPORTED_LOCATION "${COMPONENT_DIR}/${bsec_dir}/algo/normal_version/bin/esp/esp32/libalgobsec.a")

target_link_libraries(${COMPONENT_LIB} PUBLIC bseclib)