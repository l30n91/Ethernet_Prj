#include "ntc_sensor.h"
#include <math.h>

/*
 * LUT generated from Vishay NTC RT Calculation data.
 * Temperature range: -55 degC to 150 degC.
 * Resistance column: Rnom [ohms], rounded to nearest ohm.
 */
static const NTC_LutPoint_t s_ntc_lut[] =
{
    {  -55,  953774u },
    {  -54,  886261u },
    {  -53,  823959u },
    {  -52,  766433u },
    {  -51,  713290u },
    {  -50,  664169u },
    {  -49,  618743u },
    {  -48,  576711u },
    {  -47,  537801u },
    {  -46,  501761u },
    {  -45,  468363u },
    {  -44,  437399u },
    {  -43,  408676u },
    {  -42,  382019u },
    {  -41,  357268u },
    {  -40,  334274u },
    {  -39,  312904u },
    {  -38,  293034u },
    {  -37,  274548u },
    {  -36,  257343u },
    {  -35,  241323u },
    {  -34,  226399u },
    {  -33,  212490u },
    {  -32,  199521u },
    {  -31,  187423u },
    {  -30,  176133u },
    {  -29,  165591u },
    {  -28,  155746u },
    {  -27,  146545u },
    {  -26,  137944u },
    {  -25,  129900u },
    {  -24,  122374u },
    {  -23,  115329u },
    {  -22,  108732u },
    {  -21,  102553u },
    {  -20,   96761u },
    {  -19,   91332u },
    {  -18,   86239u },
    {  -17,   81461u },
    {  -16,   76976u },
    {  -15,   72765u },
    {  -14,   68809u },
    {  -13,   65091u },
    {  -12,   61596u },
    {  -11,   58310u },
    {  -10,   55218u },
    {   -9,   52308u },
    {   -8,   49569u },
    {   -7,   46989u },
    {   -6,   44559u },
    {   -5,   42268u },
    {   -4,   40108u },
    {   -3,   38071u },
    {   -2,   36150u },
    {   -1,   34336u },
    {    0,   32624u },
    {    1,   31007u },
    {    2,   29480u },
    {    3,   28036u },
    {    4,   26672u },
    {    5,   25381u },
    {    6,   24161u },
    {    7,   23006u },
    {    8,   21912u },
    {    9,   20877u },
    {   10,   19897u },
    {   11,   18968u },
    {   12,   18088u },
    {   13,   17253u },
    {   14,   16462u },
    {   15,   15711u },
    {   16,   14999u },
    {   17,   14323u },
    {   18,   13681u },
    {   19,   13072u },
    {   20,   12493u },
    {   21,   11943u },
    {   22,   11420u },
    {   23,   10923u },
    {   24,   10450u },
    {   25,   10000u },
    {   26,    9572u },
    {   27,    9165u },
    {   28,    8777u },
    {   29,    8408u },
    {   30,    8056u },
    {   31,    7721u },
    {   32,    7401u },
    {   33,    7097u },
    {   34,    6807u },
    {   35,    6530u },
    {   36,    6266u },
    {   37,    6014u },
    {   38,    5773u },
    {   39,    5543u },
    {   40,    5324u },
    {   41,    5114u },
    {   42,    4914u },
    {   43,    4723u },
    {   44,    4540u },
    {   45,    4365u },
    {   46,    4198u },
    {   47,    4038u },
    {   48,    3885u },
    {   49,    3739u },
    {   50,    3599u },
    {   51,    3465u },
    {   52,    3336u },
    {   53,    3213u },
    {   54,    3095u },
    {   55,    2982u },
    {   56,    2874u },
    {   57,    2770u },
    {   58,    2671u },
    {   59,    2575u },
    {   60,    2484u },
    {   61,    2396u },
    {   62,    2312u },
    {   63,    2231u },
    {   64,    2153u },
    {   65,    2079u },
    {   66,    2007u },
    {   67,    1938u },
    {   68,    1872u },
    {   69,    1809u },
    {   70,    1748u },
    {   71,    1689u },
    {   72,    1633u },
    {   73,    1578u },
    {   74,    1526u },
    {   75,    1476u },
    {   76,    1428u },
    {   77,    1381u },
    {   78,    1336u },
    {   79,    1293u },
    {   80,    1252u },
    {   81,    1212u },
    {   82,    1173u },
    {   83,    1136u },
    {   84,    1101u },
    {   85,    1066u },
    {   86,    1033u },
    {   87,    1001u },
    {   88,     970u },
    {   89,     940u },
    {   90,     912u },
    {   91,     884u },
    {   92,     857u },
    {   93,     831u },
    {   94,     806u },
    {   95,     782u },
    {   96,     759u },
    {   97,     737u },
    {   98,     715u },
    {   99,     694u },
    {  100,     674u },
    {  101,     655u },
    {  102,     636u },
    {  103,     618u },
    {  104,     600u },
    {  105,     583u },
    {  106,     566u },
    {  107,     550u },
    {  108,     535u },
    {  109,     520u },
    {  110,     506u },
    {  111,     492u },
    {  112,     478u },
    {  113,     465u },
    {  114,     452u },
    {  115,     440u },
    {  116,     428u },
    {  117,     417u },
    {  118,     406u },
    {  119,     395u },
    {  120,     384u },
    {  121,     374u },
    {  122,     364u },
    {  123,     355u },
    {  124,     346u },
    {  125,     337u },
    {  126,     328u },
    {  127,     320u },
    {  128,     311u },
    {  129,     304u },
    {  130,     296u },
    {  131,     288u },
    {  132,     281u },
    {  133,     274u },
    {  134,     267u },
    {  135,     261u },
    {  136,     254u },
    {  137,     248u },
    {  138,     242u },
    {  139,     236u },
    {  140,     230u },
    {  141,     225u },
    {  142,     219u },
    {  143,     214u },
    {  144,     209u },
    {  145,     204u },
    {  146,     199u },
    {  147,     195u },
    {  148,     190u },
    {  149,     186u },
    {  150,     181u },
};

NTC_Config_t NTC_DefaultConfig(void)
{
    NTC_Config_t cfg;

    cfg.vref = NTC_DEFAULT_VREF;
    cfg.rload_ohm = NTC_DEFAULT_RLOAD_OHM;
    cfg.adc_max = NTC_DEFAULT_ADC_MAX;
    cfg.mode = NTC_DIVIDER_RLOAD_TOP_NTC_BOTTOM;

    return cfg;
}

static int NTC_IsValidConfig(const NTC_Config_t *cfg)
{
    if (cfg == 0)
    {
        return 0;
    }

    if ((cfg->vref <= 0.0f) ||
        (cfg->rload_ohm <= 0.0f) ||
        (cfg->adc_max <= 0.0f))
    {
        return 0;
    }

    return 1;
}

float NTC_ADCToVoltage(uint16_t adc_raw, const NTC_Config_t *cfg)
{
    if (!NTC_IsValidConfig(cfg))
    {
        return -1.0f;
    }

    return ((float)adc_raw * cfg->vref) / cfg->adc_max;
}

float NTC_VoltageToResistance(float vadc, const NTC_Config_t *cfg)
{
    if (!NTC_IsValidConfig(cfg))
    {
        return -1.0f;
    }

    if ((vadc <= 0.0f) || (vadc >= cfg->vref))
    {
        return -1.0f;
    }

    if (cfg->mode == NTC_DIVIDER_RLOAD_TOP_NTC_BOTTOM)
    {
        return (cfg->rload_ohm * vadc) / (cfg->vref - vadc);
    }

    return (cfg->rload_ohm * (cfg->vref - vadc)) / vadc;
}

float NTC_ADCToResistance(uint16_t adc_raw, const NTC_Config_t *cfg)
{
    float vadc = NTC_ADCToVoltage(adc_raw, cfg);

    if (vadc < 0.0f)
    {
        return -1.0f;
    }

    return NTC_VoltageToResistance(vadc, cfg);
}

float NTC_ResistanceToTemperatureFormula(float r_ntc_ohm)
{
    float x;
    float inv_t;

    if (r_ntc_ohm <= 0.0f)
    {
        return NTC_ERROR_TEMP_C;
    }

    x = logf(r_ntc_ohm / NTC_R25_OHM);

    inv_t = NTC_SH_A +
            (NTC_SH_B * x) +
            (NTC_SH_C * x * x) +
            (NTC_SH_D * x * x * x);

    if (inv_t <= 0.0f)
    {
        return NTC_ERROR_TEMP_C;
    }

    return (1.0f / inv_t) - 273.15f;
}

float NTC_ADCToTemperatureFormula(uint16_t adc_raw, const NTC_Config_t *cfg)
{
    float r_ntc = NTC_ADCToResistance(adc_raw, cfg);

    if (r_ntc <= 0.0f)
    {
        return NTC_ERROR_TEMP_C;
    }

    return NTC_ResistanceToTemperatureFormula(r_ntc);
}

float NTC_ResistanceToTemperatureLUT(float r_ntc_ohm)
{
    size_t i;
    const size_t lut_count = sizeof(s_ntc_lut) / sizeof(s_ntc_lut[0]);

    if (r_ntc_ohm <= 0.0f)
    {
        return NTC_ERROR_TEMP_C;
    }

    if (r_ntc_ohm >= (float)s_ntc_lut[0].resistance_ohm)
    {
        return (float)s_ntc_lut[0].temp_c;
    }

    if (r_ntc_ohm <= (float)s_ntc_lut[lut_count - 1u].resistance_ohm)
    {
        return (float)s_ntc_lut[lut_count - 1u].temp_c;
    }

    for (i = 0u; i < (lut_count - 1u); i++)
    {
        float r1 = (float)s_ntc_lut[i].resistance_ohm;
        float r2 = (float)s_ntc_lut[i + 1u].resistance_ohm;

        if ((r_ntc_ohm <= r1) && (r_ntc_ohm >= r2))
        {
            float t1 = (float)s_ntc_lut[i].temp_c;
            float t2 = (float)s_ntc_lut[i + 1u].temp_c;

            return t1 + ((r_ntc_ohm - r1) * (t2 - t1)) / (r2 - r1);
        }
    }

    return NTC_ERROR_TEMP_C;
}

float NTC_ADCToTemperatureLUT(uint16_t adc_raw, const NTC_Config_t *cfg)
{
    float r_ntc = NTC_ADCToResistance(adc_raw, cfg);

    if (r_ntc <= 0.0f)
    {
        return NTC_ERROR_TEMP_C;
    }

    return NTC_ResistanceToTemperatureLUT(r_ntc);
}

const NTC_LutPoint_t *NTC_GetLUT(size_t *count)
{
    if (count != 0)
    {
        *count = sizeof(s_ntc_lut) / sizeof(s_ntc_lut[0]);
    }

    return s_ntc_lut;
}
