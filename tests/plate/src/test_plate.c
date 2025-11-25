#include <zephyr/ztest.h>
#include "radar.h"

ZTEST(plate, test_valid_plate_pattern)
{
    zassert_true(radar_validate_plate("ABC1D23"), "placa valida foi rejeitada");
    zassert_true(radar_validate_plate("BRA2E45"), "placa valida foi rejeitada");
}

ZTEST(plate, test_invalid_plate_pattern)
{
    zassert_false(radar_validate_plate("AB12D23"), "placa invalida aceita (pos 2 nao letra)");
    zassert_false(radar_validate_plate("ABC1234"), "placa invalida aceita (formato antigo)");
    zassert_false(radar_validate_plate("AAAAAAA"), "placa invalida aceita (sem digitos)");
}

ZTEST_SUITE(plate, NULL, NULL, NULL, NULL, NULL);
