#ifndef INFERENCE_SERVICE_H
#define INFERENCE_SERVICE_H

#ifdef __cplusplus 
extern "C" {
#endif

void inference_init();
void inference_thread_run(void *arg1, void *arg2, void *arg3);
#endif
