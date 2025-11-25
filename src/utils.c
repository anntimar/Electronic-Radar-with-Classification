#include <ctype.h>
#include <string.h>
#include <zephyr/random/random.h>
#include "radar.h"

/* Cálculo simples de velocidade em km/h
 * distance_mm: distância entre sensores em milímetros
 * time_ms: tempo entre detecção nos sensores em milissegundos
 *
 * Fórmula: v[km/h] = (distance_mm / time_ms) * 3.6
 */
uint32_t radar_calc_speed_kmh(uint32_t distance_mm, uint32_t time_ms)
{
    if (time_ms == 0U) {
        return 0U;
    }

    double v_mps = ((double)distance_mm / (double)time_ms);
    double v_kmh = v_mps * 3.6;

    if (v_kmh < 0.0) {
        return 0U;
    }

    return (uint32_t)(v_kmh + 0.5); /* arredonda */
}

vehicle_type_t radar_classify_vehicle(uint32_t axes)
{
    if (axes >= 3U) {
        return VEHICLE_TYPE_HEAVY;
    }
    return VEHICLE_TYPE_LIGHT;
}

/* Valida um formato simples de placa Mercosul: LLLNLNN
 * (3 letras, 1 dígito, 1 letra, 2 dígitos)
 */
bool radar_validate_plate(const char *plate)
{
    if (!plate) {
        return false;
    }

    if (strlen(plate) != 7U) {
        return false;
    }

    /* LLL */
    for (int i = 0; i < 3; i++) {
        if (!isalpha((unsigned char)plate[i])) {
            return false;
        }
    }

    /* N */
    if (!isdigit((unsigned char)plate[3])) {
        return false;
    }

    /* L */
    if (!isalpha((unsigned char)plate[4])) {
        return false;
    }

    /* NN */
    if (!isdigit((unsigned char)plate[5]) ||
        !isdigit((unsigned char)plate[6])) {
        return false;
    }

    return true;
}

/* Gera uma placa aleatória e marca se ela é válida ou não,
 * de acordo com a taxa de falha configurada.
 */
void radar_generate_plate(char *out, size_t len, bool *valid)
{
    if (!out || len < 8U || !valid) {
        return;
    }

    /* Decide se a placa será válida ou inválida */
    uint32_t fail_rate = CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT;
    uint32_t r = sys_rand32_get() % 100U;
    bool is_valid = (r >= fail_rate);

    if (is_valid) {
        /* AAA1A23 */
        for (int i = 0; i < 3; i++) {
            out[i] = 'A' + (sys_rand32_get() % 26U);
        }
        out[3] = '0' + (sys_rand32_get() % 10U);
        out[4] = 'A' + (sys_rand32_get() % 26U);
        out[5] = '0' + (sys_rand32_get() % 10U);
        out[6] = '0' + (sys_rand32_get() % 10U);
        out[7] = '\0';
    } else {
        /* Gera uma string aleatória de 7 chars que provavelmente é inválida */
        for (int i = 0; i < 7; i++) {
            uint32_t v = sys_rand32_get() % 36U; /* 26 letras + 10 dígitos */
            if (v < 26U) {
                out[i] = 'A' + v;
            } else {
                out[i] = '0' + (v - 26U);
            }
        }
        out[7] = '\0';
    }

    *valid = is_valid && radar_validate_plate(out);
}
