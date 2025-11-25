#include <zephyr/ztest.h>
#include "radar.h"

ZTEST(speed, test_calc_speed_basic)
{
    uint32_t v = radar_calc_speed_kmh(2000U, 100U); /* 2 m em 100 ms */
    /* Esperado: ~72 km/h */
    zassert_true(v >= 70U && v <= 75U, "velocidade inesperada: %u", v);
}

ZTEST(speed, test_classify_vehicle)
{
    zassert_equal(radar_classify_vehicle(2U), VEHICLE_TYPE_LIGHT, "2 eixos deve ser leve");
    zassert_equal(radar_classify_vehicle(3U), VEHICLE_TYPE_HEAVY, "3 eixos deve ser pesado");
    zassert_equal(radar_classify_vehicle(4U), VEHICLE_TYPE_HEAVY, "4 eixos deve ser pesado");
}

ZTEST_SUITE(speed, NULL, NULL, NULL, NULL, NULL);
