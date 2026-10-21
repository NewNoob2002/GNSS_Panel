#include "adc_config.h"

// configurable ADC resolution
#ifndef CORE_ADC_RESOLUTION
#define CORE_ADC_RESOLUTION 12 // fallback to 10 bit
#endif

#if CORE_ADC_RESOLUTION == 8
#define ADC_RESOLUTION ADC_RESOLUTION_8BIT
#elif CORE_ADC_RESOLUTION == 10
#define ADC_RESOLUTION ADC_RESOLUTION_10BIT
#elif CORE_ADC_RESOLUTION == 12
#define ADC_RESOLUTION ADC_RESOLUTION_12BIT
#else
#error "Invalid ADC resolution. only 8, 10, 12 bit are supported"
#endif

//
// ADC devices
//
adc_device_t ADC1_device = {
    .adc = {
        .register_base = CM_ADC1,
        .clock_id = PWC_FCG3_ADC1,
        .sequence = ADC_SEQ_A,
        .channel_count = 17u,
    },
    .init_params = {
        .resolution = ADC_RESOLUTION,
        .data_alignment = ADC_DATAALIGN_RIGHT,
        .scan_mode = ADC_MD_SEQA_SINGLESHOT, // only sequence A
    },
};

uint32_t ADC1_startCov_time;

adc_device_t ADC2_device = {
		.adc = {
        .register_base = CM_ADC2,
        .clock_id = PWC_FCG3_ADC2,
        .sequence = ADC_SEQ_A,
        .channel_count = 9u,
    },
    .init_params = {
        .resolution = ADC_RESOLUTION,
        .data_alignment = ADC_DATAALIGN_RIGHT,
        .scan_mode = ADC_MD_SEQA_SINGLESHOT, // only sequence A
    },
};
