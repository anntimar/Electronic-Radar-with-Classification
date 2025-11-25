#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "radar.h"

LOG_MODULE_REGISTER(radar_camera, LOG_LEVEL_INF);

/* Estrutura e canal definidos em main.c */
struct camera_msg {
    bool trigger;
    bool done;
    bool plate_valid;
    char plate[16];
};

extern const struct zbus_channel camera_chan;

void camera_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    struct camera_msg msg;

    while (1) {
        /* Bloqueia até conseguir ler o último estado do canal */
        int err = zbus_chan_read(&camera_chan, &msg, K_FOREVER);
        if (err != 0) {
            LOG_ERR("Falha ao ler canal da camera: %d", err);
            continue;
        }

        if (!msg.trigger) {
            /* Nada a fazer, apenas aguarda novo trigger */
            continue;
        }

        LOG_INF("Camera acionada, processando placa...");
        k_sleep(K_MSEC(500)); /* simula tempo de processamento */

        bool valid = false;
        radar_generate_plate(msg.plate, sizeof(msg.plate), &valid);
        msg.plate_valid = valid;
        msg.done = true;
        msg.trigger = false;

        err = zbus_chan_pub(&camera_chan, &msg, K_MSEC(100));
        if (err != 0) {
            LOG_ERR("Falha ao publicar resultado da camera: %d", err);
        } else {
            LOG_INF("Camera publicou placa: %s (%s)",
                    msg.plate,
                    msg.plate_valid ? "valida" : "invalida");
        }
    }
}
