/**********************************************************************************************************************/
/* header files */
/**********************************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bsec_integration.h"

/**********************************************************************************************************************/
/* local macro definitions */
/**********************************************************************************************************************/

#define NUM_USED_OUTPUTS 10

/**********************************************************************************************************************/
/* global variable declarations */
/**********************************************************************************************************************/

/* Global sensor APIs data structure */
static struct bme68x_dev bme68x;

/* Global temperature offset to be subtracted */
static float bme68x_temperature_offset_g = 0.0f;

/**********************************************************************************************************************/
/* functions */
/**********************************************************************************************************************/

/*!
 * @brief        Virtual sensor subscription
 *               Please call this function before processing of data using bsec_do_steps function
 *
 * @param[in]    sample_rate         mode to be used (either BSEC_SAMPLE_RATE_ULP or BSEC_SAMPLE_RATE_LP)
 *
 * @return       subscription result, zero when successful
 */
static bsec_library_return_t bme68x_bsec_update_subscription(float sample_rate) {
    bsec_sensor_configuration_t requested_virtual_sensors[NUM_USED_OUTPUTS];
    uint8_t n_requested_virtual_sensors = NUM_USED_OUTPUTS;

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    bsec_library_return_t status = BSEC_OK;

    /* note: Virtual sensors as desired to be added here */
    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_IAQ;
    requested_virtual_sensors[0].sample_rate = sample_rate;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_STATIC_IAQ;
    requested_virtual_sensors[1].sample_rate = sample_rate;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    requested_virtual_sensors[2].sample_rate = sample_rate;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    requested_virtual_sensors[3].sample_rate = sample_rate;
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[4].sample_rate = sample_rate;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_BREATH_VOC_EQUIVALENT;
    requested_virtual_sensors[5].sample_rate = sample_rate;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_CO2_EQUIVALENT;
    requested_virtual_sensors[6].sample_rate = sample_rate;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_COMPENSATED_GAS;
    requested_virtual_sensors[7].sample_rate = sample_rate;
    requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_RAW_GAS;
    requested_virtual_sensors[8].sample_rate = sample_rate;
    requested_virtual_sensors[9].sensor_id = BSEC_OUTPUT_GAS_PERCENTAGE;
    requested_virtual_sensors[9].sample_rate = sample_rate;


    /* Call bsec_update_subscription() to enable/disable the requested virtual sensors */
    status = bsec_update_subscription(requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings,
                                      &n_required_sensor_settings);

    return status;
}

/*!
 * @brief       Initialize the BME68X sensor and the BSEC library
 *
 * @param[in]   sample_rate         mode to be used (either BSEC_SAMPLE_RATE_ULP or BSEC_SAMPLE_RATE_LP)
 * @param[in]   temperature_offset  device-specific temperature offset (due to self-heating)
 * @param[in]   bus_write           pointer to the bus writing function
 * @param[in]   bus_read            pointer to the bus reading function
 * @param[in]   sleep               pointer to the system specific sleep function
 * @param[in]   state_load          pointer to the system-specific state load function
 * @param[in]   config_load         pointer to the system-specific config load function
 *
 * @return      zero if successful, negative otherwise
 */
return_values_init bsec_iot_init(float sample_rate, float temperature_offset, bme68x_write_fptr_t bus_write,
                                 bme68x_read_fptr_t bus_read, bme68x_delay_us_fptr_t sleep, state_load_fct state_load,
                                 config_load_fct config_load) {
    return_values_init ret = {BME68X_OK, BSEC_OK};
    bsec_library_return_t bsec_status = BSEC_OK;

    uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    uint8_t bsec_config[BSEC_MAX_PROPERTY_BLOB_SIZE] = {0};
    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    int bsec_state_len, bsec_config_len;

    /* Fixed I2C configuration */
    bme68x.variant_id = BME68X_I2C_ADDR_LOW;
    bme68x.intf = BME68X_I2C_INTF;
    /* User configurable I2C configuration */
    bme68x.write = bus_write;
    bme68x.read = bus_read;
    bme68x.delay_us = sleep;

    /* Initialize BME68X API */
    ret.bme68x_status = bme68x_init(&bme68x);
    if (ret.bme68x_status != BME68X_OK) {
        return ret;
    }

    /* Initialize BSEC library */
    ret.bsec_status = bsec_init();
    if (ret.bsec_status != BSEC_OK) {
        return ret;
    }

    /* Load library config, if available */
    bsec_config_len = config_load(bsec_config, sizeof(bsec_config));
    if (bsec_config_len != 0) {
        ret.bsec_status = bsec_set_configuration(bsec_config, bsec_config_len, work_buffer, sizeof(work_buffer));
        if (ret.bsec_status != BSEC_OK) {
            return ret;
        }
    }

    /* Load previous library state, if available */
    bsec_state_len = state_load(bsec_state, sizeof(bsec_state));
    if (bsec_state_len != 0) {
        ret.bsec_status = bsec_set_state(bsec_state, bsec_state_len, work_buffer, sizeof(work_buffer));
        if (ret.bsec_status != BSEC_OK) {
            return ret;
        }
    }

    /* Set temperature offset */
    bme68x_temperature_offset_g = temperature_offset;

    /* Call to the function which sets the library with subscription information */
    ret.bsec_status = bme68x_bsec_update_subscription(sample_rate);
    if (ret.bsec_status != BSEC_OK) {
        return ret;
    }

    return ret;
}

/*!
 * @brief       Trigger the measurement based on sensor settings
 *
 * @param[in]   sensor_settings     settings of the BME68X sensor adopted by sensor control function
 * @param[in]   sleep               pointer to the system specific sleep function
 *
 * @return      none
 */
static void bme68x_bsec_trigger_measurement(bsec_bme_settings_t* sensor_settings, bme68x_delay_us_fptr_t sleep) {
    uint16_t meas_period;
    uint8_t set_required_settings;
    int8_t bme68x_status = BME68X_OK;
    struct bme68x_conf bme68x_sensor_settings;
    struct bme68x_heatr_conf bme68x_heater_settings;

    /* Check if a forced-mode measurement should be triggered now */
    if (sensor_settings->trigger_measurement) {
        /* Set sensor configuration */

        bme68x_sensor_settings.os_hum = sensor_settings->humidity_oversampling;
        bme68x_sensor_settings.os_pres = sensor_settings->pressure_oversampling;
        bme68x_sensor_settings.os_temp = sensor_settings->temperature_oversampling;

        bme68x_heater_settings.enable = sensor_settings->run_gas;
        bme68x_heater_settings.heatr_temp  = sensor_settings->heater_temperature; /* degree Celsius */
        bme68x_heater_settings.heatr_dur = sensor_settings->heater_duration;    /* milliseconds */

        /* Set the desired sensor configuration */
        bme68x_status = bme68x_set_conf(&bme68x_sensor_settings, &bme68x);

        /* Set the desired heater configuration */
        bme68x_status = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &bme68x_heater_settings, &bme68x);

        /* Set power mode as forced mode and trigger forced mode measurement */
        bme68x_status = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme68x);

        /* Get the total measurement duration so as to sleep or wait till the measurement is complete */
        meas_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &bme68x_sensor_settings, &bme68x);

        /* Delay till the measurement is ready. Timestamp resolution in us */
        sleep((uint32_t)meas_period, bme68x.intf_ptr);
    }

    /* Call the API to get current operation mode of the sensor */
    uint8_t opmode;
    bme68x_status = bme68x_get_op_mode(&opmode, &bme68x);
    /* When the measurement is completed and data is ready for reading, the sensor must be in BME68X_SLEEP_MODE.
     * Read operation mode to check whether measurement is completely done and wait until the sensor is no more
     * in BME68X_FORCED_MODE. */
    while (opmode == BME68X_FORCED_MODE) {
        /* sleep for 5 ms */
        sleep(5000, bme68x.intf_ptr);
        bme68x_status = bme68x_get_op_mode(&opmode, &bme68x);
    }
}

/*!
 * @brief       Read the data from registers and populate the inputs structure to be passed to do_steps function
 *
 * @param[in]   time_stamp_trigger      settings of the sensor returned from sensor control function
 * @param[in]   inputs                  input structure containing the information on sensors to be passed to do_steps
 * @param[in]   num_bsec_inputs         number of inputs to be passed to do_steps
 * @param[in]   bsec_process_data       process data variable returned from sensor_control
 *
 * @return      none
 */
static void bme68x_bsec_read_data(int64_t time_stamp_trigger, bsec_input_t* inputs, uint8_t* num_bsec_inputs,
                                  int32_t bsec_process_data) {
    static struct bme68x_data data;
    int8_t bme68x_status = BME68X_OK;
    uint8_t n_data;

    /* We only have to read data if the previous call the bsec_sensor_control() actually asked for it */
    if (bsec_process_data) {
        bme68x_status = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_data, &bme68x);

        if (data.status & BME68X_NEW_DATA_MSK) {
            /* Pressure to be processed by BSEC */
            if (bsec_process_data & BSEC_PROCESS_PRESSURE) {
                /* Place presssure sample into input struct */
                inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_PRESSURE;
                inputs[*num_bsec_inputs].signal = data.pressure;
                inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                (*num_bsec_inputs)++;
            }
            /* Temperature to be processed by BSEC */
            if (bsec_process_data & BSEC_PROCESS_TEMPERATURE) {
                /* Place temperature sample into input struct */
                inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
#ifdef BME68X_USE_FPU
                inputs[*num_bsec_inputs].signal = data.temperature;
#else
                inputs[*num_bsec_inputs].signal = data.temperature / 100.0f;
#endif
                inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                (*num_bsec_inputs)++;

                /* Also add optional heatsource input which will be subtracted from the temperature reading to
                 * compensate for device-specific self-heating (supported in BSEC IAQ solution)*/
                inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
                inputs[*num_bsec_inputs].signal = bme68x_temperature_offset_g;
                inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                (*num_bsec_inputs)++;
            }
            /* Humidity to be processed by BSEC */
            if (bsec_process_data & BSEC_PROCESS_HUMIDITY) {
                /* Place humidity sample into input struct */
                inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
#ifdef BME68X_USE_FPU
                inputs[*num_bsec_inputs].signal = data.humidity;
#else
                inputs[*num_bsec_inputs].signal = data.humidity / 1000.0f;
#endif
                inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                (*num_bsec_inputs)++;
            }
            /* Gas to be processed by BSEC */
            if (bsec_process_data & BSEC_PROCESS_GAS) {
                /* Check whether gas_valid flag is set */
                if (data.status & BME68X_GASM_VALID_MSK) {
                    /* Place sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
                    inputs[*num_bsec_inputs].signal = data.gas_resistance;
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                }
            }
        }
    }
}

/*!
 * @brief       This function is written to process the sensor data for the requested virtual sensors
 *
 * @param[in]   bsec_inputs         input structure containing the information on sensors to be passed to do_steps
 * @param[in]   num_bsec_inputs     number of inputs to be passed to do_steps
 * @param[in]   output_ready        pointer to the function processing obtained BSEC outputs
 *
 * @return      none
 */
static void bme68x_bsec_process_data(bsec_input_t* bsec_inputs, uint8_t num_bsec_inputs,
                                     output_ready_fct output_ready) {
    /* Output buffer set to the maximum virtual sensor outputs supported */
    bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
    uint8_t num_bsec_outputs = 0;
    uint8_t index = 0;

    bsec_library_return_t bsec_status = BSEC_OK;

    int64_t timestamp = 0;
    float iaq = 0.0f;
    uint8_t iaq_accuracy = 0;
    float temp = 0.0f;
    float raw_temp = 0.0f;
    float raw_pressure = 0.0f;
    float humidity = 0.0f;
    float raw_humidity = 0.0f;
    float raw_gas = 0.0f;
    float static_iaq = 0.0f;
    uint8_t static_iaq_accuracy = 0;
    float co2_equivalent = 0.0f;
    uint8_t co2_accuracy = 0;
    float breath_voc_equivalent = 0.0f;
    uint8_t breath_voc_accuracy = 0;
    float comp_gas_value = 0.0f;
    uint8_t comp_gas_accuracy = 0;
    float gas_percentage = 0.0f;
    uint8_t gas_percentage_acccuracy = 0;

    /* Check if something should be processed by BSEC */
    if (num_bsec_inputs > 0) {
        /* Set number of outputs to the size of the allocated buffer */
        /* BSEC_NUMBER_OUTPUTS to be defined */
        num_bsec_outputs = BSEC_NUMBER_OUTPUTS;

        /* Perform processing of the data by BSEC
           Note:
           * The number of outputs you get depends on what you asked for during bsec_update_subscription(). This is
             handled under bme68x_bsec_update_subscription() function in this example file.
           * The number of actual outputs that are returned is written to num_bsec_outputs. */
        bsec_status = bsec_do_steps(bsec_inputs, num_bsec_inputs, bsec_outputs, &num_bsec_outputs);

        /* Iterate through the outputs and extract the relevant ones. */
        for (index = 0; index < num_bsec_outputs; index++) {
            switch (bsec_outputs[index].sensor_id) {
            case BSEC_OUTPUT_IAQ:
                iaq = bsec_outputs[index].signal;
                iaq_accuracy = bsec_outputs[index].accuracy;
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                static_iaq = bsec_outputs[index].signal;
                static_iaq_accuracy = bsec_outputs[index].accuracy;
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                co2_equivalent = bsec_outputs[index].signal;
                co2_accuracy = bsec_outputs[index].accuracy;
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                breath_voc_equivalent = bsec_outputs[index].signal;
                breath_voc_accuracy = bsec_outputs[index].accuracy;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                temp = bsec_outputs[index].signal;
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                raw_pressure = bsec_outputs[index].signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                humidity = bsec_outputs[index].signal;
                break;
            case BSEC_OUTPUT_RAW_GAS:
                raw_gas = bsec_outputs[index].signal;
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                raw_temp = bsec_outputs[index].signal;
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                raw_humidity = bsec_outputs[index].signal;
                break;
            case BSEC_OUTPUT_COMPENSATED_GAS:
                comp_gas_value = bsec_outputs[index].signal;
                comp_gas_accuracy = bsec_outputs[index].accuracy;
                break;
            case BSEC_OUTPUT_GAS_PERCENTAGE:
                gas_percentage = bsec_outputs[index].signal;
                gas_percentage_acccuracy = bsec_outputs[index].accuracy;
                break;
            default:
                continue;
            }

            /* Assume that all the returned timestamps are the same */
            timestamp = bsec_outputs[index].time_stamp;
        }

        /* Pass the extracted outputs to the user provided output_ready() function. */
        output_ready(timestamp,
                     iaq,
                     iaq_accuracy,
                     temp,
                     raw_temp,
                     raw_pressure,
                     humidity,
                     raw_humidity,
                     raw_gas,
                     static_iaq,
                     static_iaq_accuracy,
                     co2_equivalent,
                     co2_accuracy,
                     breath_voc_equivalent,
                     breath_voc_accuracy,
                     comp_gas_value,
                     comp_gas_accuracy,
                     gas_percentage,
                     gas_percentage_acccuracy,
                     bsec_status);
    }
}

/*!
 * @brief       Runs the main (endless) loop that queries sensor settings, applies them, and processes the measured data
 *
 * @param[in]   sleep               pointer to the system specific sleep function
 * @param[in]   get_timestamp_us    pointer to the system specific timestamp derivation function
 * @param[in]   output_ready        pointer to the function processing obtained BSEC outputs
 * @param[in]   state_save          pointer to the system-specific state save function
 * @param[in]   save_intvl          interval at which BSEC state should be saved (in samples)
 *
 * @return      none
 */
_Noreturn void bsec_iot_loop(bme68x_delay_us_fptr_t sleep, get_timestamp_us_fct get_timestamp_us, output_ready_fct output_ready,
                             state_save_fct state_save, uint32_t save_intvl) {
    /* Timestamp variables */
    int64_t time_stamp = 0;
    int64_t time_stamp_interval_us = 0;

    /* Allocate enough memory for up to BSEC_MAX_PHYSICAL_SENSOR physical inputs*/
    bsec_input_t bsec_inputs[BSEC_MAX_PHYSICAL_SENSOR];

    /* Number of inputs to BSEC */
    uint8_t num_bsec_inputs = 0;

    /* BSEC sensor settings struct */
    bsec_bme_settings_t sensor_settings;

    /* Save state variables */
    uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
    uint32_t bsec_state_len = 0;
    uint32_t n_samples = 0;

    bsec_library_return_t bsec_status = BSEC_OK;

    while (1) {
        /* get the timestamp in nanoseconds before calling bsec_sensor_control() */
        time_stamp = get_timestamp_us() * 1000;

        /* Retrieve sensor settings to be used in this time instant by calling bsec_sensor_control */
        bsec_sensor_control(time_stamp, &sensor_settings);

        /* Trigger a measurement if necessary */
        bme68x_bsec_trigger_measurement(&sensor_settings, sleep);

        /* Read data from last measurement */
        num_bsec_inputs = 0;
        bme68x_bsec_read_data(time_stamp, bsec_inputs, &num_bsec_inputs, sensor_settings.process_data);

        /* Time to invoke BSEC to perform the actual processing */
        bme68x_bsec_process_data(bsec_inputs, num_bsec_inputs, output_ready);

        /* Increment sample counter */
        n_samples++;

        /* Retrieve and store state if the passed save_intvl */
        if (n_samples >= save_intvl) {
            bsec_status = bsec_get_state(0, bsec_state, sizeof(bsec_state), work_buffer, sizeof(work_buffer), &bsec_state_len);
            if (bsec_status == BSEC_OK) {
                state_save(bsec_state, bsec_state_len);
            }
            n_samples = 0;
        }

        /* Compute how long we can sleep until we need to call bsec_sensor_control() next */
        /* Time_stamp is converted from microseconds to nanoseconds first and then the difference to microseconds */
        time_stamp_interval_us = (sensor_settings.next_call - get_timestamp_us() * 1000) / 1000;
        if (time_stamp_interval_us > 0) {
            sleep((uint32_t)time_stamp_interval_us, bme68x.intf_ptr);
        }
    }
}

/*! @}*/
