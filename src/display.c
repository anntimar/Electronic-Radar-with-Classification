#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "radar.h"

LOG_MODULE_REGISTER(radar_display, LOG_LEVEL_INF);

/* Códigos ANSI simples */
#define ANSI_RESET   "\x1b[0m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RED     "\x1b[31m"

void display_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        struct display_msg msg;

        if (k_msgq_get(&g_display_msgq, &msg, K_FOREVER) == 0) {
            const char *status_str = "NORMAL";
            const char *color = ANSI_GREEN;

            switch (msg.status) {
            case RADAR_STATUS_NORMAL:
                status_str = "NORMAL";
                color = ANSI_GREEN;
                break;
            case RADAR_STATUS_WARNING:
                status_str = "ALERTA";
                color = ANSI_YELLOW;
                break;
            case RADAR_STATUS_INFRACTION:
                status_str = "INFRACAO";
                color = ANSI_RED;
                break;
            default:
                break;
            }

            const char *type_str =
                (radar_classify_vehicle(msg.sample.axes) == VEHICLE_TYPE_LIGHT)
                    ? "LEVE" : "PESADO";

            if (msg.plate[0] != '\0') {
                printk("%s[RADAR] %s | %u km/h | %s | placa=%s (%s)%s\n",
                       color,
                       status_str,
                       msg.sample.speed_kmh,
                       type_str,
                       msg.plate,
                       msg.plate_valid ? "valida" : "invalida",
                       ANSI_RESET);
            } else {
                printk("%s[RADAR] %s | %u km/h | %s%s\n",
                       color,
                       status_str,
                       msg.sample.speed_kmh,
                       type_str,
                       ANSI_RESET);
            }
        }
    }
}
