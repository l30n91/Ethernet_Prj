#ifndef NTC_SENSOR_H
#define NTC_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * NTC library for Vishay NTCALUG01T103G501A.
 * Supports both:
 * - formula conversion using Vishay Steinhart-Hart coefficients
 * - lookup table conversion using Rnom values from Vishay CSV
 */

typedef enum
{
    /*
     * VREF -> RLOAD -> ADC -> NTC -> GND
     * Rntc = Rload * Vadc / (Vref - Vadc)
     */
    NTC_DIVIDER_RLOAD_TOP_NTC_BOTTOM = 0,

    /*
     * VREF -> NTC -> ADC -> RLOAD -> GND
     * Rntc = Rload * (Vref - Vadc) / Vadc
     */
    NTC_DIVIDER_NTC_TOP_RLOAD_BOTTOM = 1

} NTC_DividerMode_t;

typedef struct
{
    float vref;
    float rload_ohm;
    float adc_max;
    NTC_DividerMode_t mode;
} NTC_Config_t;

typedef struct
{
    int16_t temp_c;
    uint32_t resistance_ohm;
} NTC_LutPoint_t;

#define NTC_DEFAULT_VREF          (3.3f)
#define NTC_DEFAULT_RLOAD_OHM     (2200.0f)
#define NTC_DEFAULT_ADC_MAX       (4095.0f)

#define NTC_R25_OHM               (10000.0f)

#define NTC_SH_A                  (0.00335401643468053f)
#define NTC_SH_B                  (0.000256523550896126f)
#define NTC_SH_C                  (0.00000260597012072052f)
#define NTC_SH_D                  (0.000000063292612648746f)

#define NTC_ERROR_TEMP_C          (-999.0f)

NTC_Config_t NTC_DefaultConfig(void);

float NTC_ADCToVoltage(uint16_t adc_raw, const NTC_Config_t *cfg);
float NTC_VoltageToResistance(float vadc, const NTC_Config_t *cfg);
float NTC_ADCToResistance(uint16_t adc_raw, const NTC_Config_t *cfg);

float NTC_ResistanceToTemperatureFormula(float r_ntc_ohm);
float NTC_ADCToTemperatureFormula(uint16_t adc_raw, const NTC_Config_t *cfg);

float NTC_ResistanceToTemperatureLUT(float r_ntc_ohm);
float NTC_ADCToTemperatureLUT(uint16_t adc_raw, const NTC_Config_t *cfg);
void NTC_CreateTask(void);

const NTC_LutPoint_t *NTC_GetLUT(size_t *count);

#ifdef __cplusplus
}
#endif

#endif /* NTC_SENSOR_H */
