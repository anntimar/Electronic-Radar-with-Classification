#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "radar.h"

LOG_MODULE_REGISTER(radar_sensors, LOG_LEVEL_INF);

/* Esta thread apenas SIMULA os sensores.
 * Em uma versão mais completa, você pode substituir essa simulação pelo
 * uso real de GPIOs/interrupts conforme o enunciado do trabalho.
 */
void sensors_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        struct sensor_sample sample = {0};

        /* Simula número de eixos: 2 (leve) ou 3 (pesado) */
        uint32_t r = sys_rand32_get() % 2U; /* 0 ou 1 */
        sample.axes = (r == 0U) ? 2U : 3U;

        /* Simula velocidade entre 30 km/h e 140 km/h */
        uint32_t base = 30U;
        uint32_t span = 110U;
        sample.speed_kmh = base + (sys_rand32_get() % span);

        int ret = k_msgq_put(&g_sensor_msgq, &sample, K_MSEC(100));
        if (ret != 0) {
            LOG_WRN("Fila de sensores cheia, descartando amostra");
        }

        /* Intervalo entre leituras simuladas */
        k_sleep(K_SECONDS(3));
    }
}
