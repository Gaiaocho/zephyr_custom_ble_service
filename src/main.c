/*
 * Copyright (c) gaiaochos.com
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app.h"
#include "audio_service.h"
#include "ble_service.h"
#include "temperature.h"

// Enable logs
LOG_MODULE_REGISTER(custom_service_log);

#define THREAD_STACK_SIZE 1024

K_MSGQ_DEFINE(tempmsgq, TEMP_MSG_SIZE, TEMPQSIZE, 2);
K_THREAD_DEFINE(temp_thread_id, THREAD_STACK_SIZE, temp_sensor_thread, NULL, NULL, NULL, 5, 0, 0); 
K_THREAD_DEFINE(ble_temp_thread_id, THREAD_STACK_SIZE, ble_temp_read_thread, NULL, NULL, NULL, 6, 0,0);
K_THREAD_DEFINE(audio_thread_id, THREAD_STACK_SIZE, audio_sense_thread, NULL, NULL, NULL, 4, 0, 0);


int main(void) {
  /*Blocking bluetooth init*/
  ble_service_init();

  /*Bluetooth setup was a success do sensor now*/
  sensor_temp_sensor_init();

  /*Audop Service init*/
  audio_service_init();


  LOG_WRN("Updating temp every 2000 MSEC");

  /*This becomes our idle loop*/
  while (1) {
    k_sleep(K_SECONDS(2));
  }
}
