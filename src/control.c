#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "radar.h"

LOG_MODULE_REGISTER(radar_control, LOG_LEVEL_INF);

struct camera_msg {
    bool trigger;
    bool done;
    bool plate_valid;
    char plate[16];
};

extern const struct zbus_channel camera_chan;

void control_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    uint32_t limit_light = CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH;
    uint32_t limit_heavy = CONFIG_RADAR_SPEED_LIMIT_HEAVY_KMH;
    uint32_t warn_pct = CONFIG_RADAR_WARNING_THRESHOLD_PERCENT;

    while (1) {
        struct sensor_sample sample;

        if (k_msgq_get(&g_sensor_msgq, &sample, K_FOREVER) != 0) {
            continue;
        }

        vehicle_type_t type = radar_classify_vehicle(sample.axes);
        uint32_t limit = (type == VEHICLE_TYPE_LIGHT) ? limit_light : limit_heavy;

        radar_status_t status = RADAR_STATUS_NORMAL;

        if (sample.speed_kmh > limit) {
            status = RADAR_STATUS_INFRACTION;
        } else {
            uint32_t warn_threshold = (limit * warn_pct) / 100U;
            if (sample.speed_kmh >= warn_threshold) {
                status = RADAR_STATUS_WARNING;
            } else {
                status = RADAR_STATUS_NORMAL;
            }
        }

        struct display_msg dmsg = {0};
        dmsg.sample = sample;
        dmsg.status = status;
        dmsg.plate[0] = '\0';
        dmsg.plate_valid = false;

        if (status == RADAR_STATUS_INFRACTION) {
            /* Dispara câmera via ZBUS */
            struct camera_msg cmsg = {
                .trigger = true,
                .done = false,
                .plate_valid = false,
                .plate = {0},
            };

            int err = zbus_chan_pub(&camera_chan, &cmsg, K_MSEC(100));
            if (err != 0) {
                LOG_ERR("Erro ao publicar trigger da camera: %d", err);
            } else {
                /* Aguarda a câmera escrever o resultado de volta */
                err = zbus_chan_read(&camera_chan, &cmsg, K_MSEC(1000));
                if (err == 0 && cmsg.done) {
                    strncpy(dmsg.plate, cmsg.plate, sizeof(dmsg.plate) - 1);
                    dmsg.plate[sizeof(dmsg.plate) - 1] = '\0';
                    dmsg.plate_valid = cmsg.plate_valid;
                }
            }
        }

        int ret = k_msgq_put(&g_display_msgq, &dmsg, K_MSEC(100));
        if (ret != 0) {
            LOG_WRN("Fila de display cheia, descartando mensagem");
        }
    }
}
