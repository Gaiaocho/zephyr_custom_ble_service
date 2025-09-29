#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVIC_H
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>

#define SAMPLE_FREQUENCY 	44100
/*Mic data sheet*/
#define SAMPLE_BIT_WIDTH 	24
/*24 Bits packed into a 32 bit block*/
#define BYTES_PER_SAMPLE 	sizeof(int32_t)
#define NUMBER_OF_CHANNELS 	1
/*4410 samples so 0.1s
 * Swapped to 4096 because our FFT implementation demands that.
 * */
#define SAMPLES_PER_BLOCK 	4096 //((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCK_COUNT 	1
#define TIMEOUT_MS 		2000

/*Because i2s_read expects number of bytes*/
#define BLOCK_SIZE 		(BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK) 
/*Enough for 400ms of sound*/
#define BLOCK_COUNT		(INITIAL_BLOCK_COUNT + 3)


/*Filter  Definitions*/
#define NUM_TAPS			53

extern struct k_mem_slab inference_slab;
extern struct k_fifo infer_fifo; 

void audio_service_init();
void audio_sense_thread(void *arg1, void *arg2, void *arg3);
#endif
