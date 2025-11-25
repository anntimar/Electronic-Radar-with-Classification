#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "radar.h"

LOG_MODULE_REGISTER(radar_main, LOG_LEVEL_INF);

/* Mensagem usada no canal ZBUS da câmera */
struct camera_msg {
    bool trigger;      /* true para disparar captura */
    bool done;         /* true quando placa foi gerada */
    bool plate_valid;  /* se a placa é válida */
    char plate[16];    /* placa gerada */
};

/* Canal de comunicação via ZBUS (controle <-> câmera) */
ZBUS_CHAN_DEFINE(camera_chan,
                 struct camera_msg,
                 NULL, /* validator */
                 NULL, /* user data */
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.trigger = false, .done = false));

/* Filas globais declaradas em radar.h */
K_MSGQ_DEFINE(g_sensor_msgq, sizeof(struct sensor_sample), 8, 4);
K_MSGQ_DEFINE(g_display_msgq, sizeof(struct display_msg), 8, 4);

/* Pilhas de threads */
K_THREAD_STACK_DEFINE(sensor_stack,  RADAR_STACK_SIZE);
K_THREAD_STACK_DEFINE(control_stack, RADAR_STACK_SIZE);
K_THREAD_STACK_DEFINE(display_stack, RADAR_STACK_SIZE);
K_THREAD_STACK_DEFINE(camera_stack,  RADAR_STACK_SIZE);

static struct k_thread sensor_thread_data;
static struct k_thread control_thread_data;
static struct k_thread display_thread_data;
static struct k_thread camera_thread_data;

void main(void)
{
    LOG_INF("Radar eletrônico iniciado");

    k_thread_create(&sensor_thread_data, sensor_stack, K_THREAD_STACK_SIZEOF(sensor_stack),
                    sensors_thread, NULL, NULL, NULL,
                    RADAR_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&control_thread_data, control_stack, K_THREAD_STACK_SIZEOF(control_stack),
                    control_thread, NULL, NULL, NULL,
                    RADAR_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&display_thread_data, display_stack, K_THREAD_STACK_SIZEOF(display_stack),
                    display_thread, NULL, NULL, NULL,
                    RADAR_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&camera_thread_data, camera_stack, K_THREAD_STACK_SIZEOF(camera_stack),
                    camera_thread, NULL, NULL, NULL,
                    RADAR_PRIORITY, 0, K_NO_WAIT);

    /* Opcional: dar nomes às threads para debug */
    k_thread_name_set(&sensor_thread_data, "sensors");
    k_thread_name_set(&control_thread_data, "control");
    k_thread_name_set(&display_thread_data, "display");
    k_thread_name_set(&camera_thread_data, "camera");
}
