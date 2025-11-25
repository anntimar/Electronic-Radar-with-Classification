#pragma once
#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VEHICLE_TYPE_LIGHT = 0,
    VEHICLE_TYPE_HEAVY = 1,
} vehicle_type_t;

typedef enum {
    RADAR_STATUS_NORMAL = 0,
    RADAR_STATUS_WARNING,
    RADAR_STATUS_INFRACTION,
} radar_status_t;

struct sensor_sample {
    uint32_t axes;        /* número de eixos detectados */
    uint32_t speed_kmh;   /* velocidade simulada */
};

struct display_msg {
    struct sensor_sample sample;
    radar_status_t status;
    char plate[16];
    bool plate_valid;
};

/* Fila de comunicação sensor -> controle */
extern struct k_msgq g_sensor_msgq;

/* Fila de comunicação controle -> display */
extern struct k_msgq g_display_msgq;

/* Pilhas de threads */
#define RADAR_STACK_SIZE 2048
#define RADAR_PRIORITY   5

/* Funções utilitárias */
uint32_t radar_calc_speed_kmh(uint32_t distance_mm, uint32_t time_ms);
vehicle_type_t radar_classify_vehicle(uint32_t axes);
bool radar_validate_plate(const char *plate);
void radar_generate_plate(char *out, size_t len, bool *valid);

/* Entrypoints de threads */
void sensors_thread(void *p1, void *p2, void *p3);
void control_thread(void *p1, void *p2, void *p3);
void display_thread(void *p1, void *p2, void *p3);
void camera_thread(void *p1, void *p2, void *p3);
