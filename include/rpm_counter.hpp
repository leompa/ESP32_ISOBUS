#pragma once

#include <stdint.h>

void rpm_init();

int rpm_get(int index);

bool rpm_get_rotacion ();

bool rpm_get_eje_cero ();

void rpm_set_limit (int index);

void rpm_set_all_limits();
