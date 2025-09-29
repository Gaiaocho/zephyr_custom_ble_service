#include <zephyr/logging/log.h> 
#include <arm_math.h>

#include "dsp/transform_functions.h"
#include "inference_service.h"
#include "fsm_service.h"
#include "audio_service.h"

LOG_MODULE_REGISTER(inference_module);

static float32_t fir_state[SAMPLES_PER_BLOCK + NUM_TAPS - 1];
float32_t samples_f[SAMPLES_PER_BLOCK];
float32_t fft_samples[SAMPLES_PER_BLOCK];
float32_t filtered_samples[SAMPLES_PER_BLOCK];

const float32_t filter_taps[NUM_TAPS] = {
	0.000000008f,         -0.000000017f,        -0.000005158008031f,  0.000001306510464f,
	0.000133311671510f,   0.000441338085554f,   -0.000003558565317f,  -0.004678776861470f,
	-0.019949989253140f,  -0.050762976431733f,  -0.089992776291018f,  -0.106658945543195f,
	-0.047616537578776f,  0.128400969571106f,   0.268940032322269f,   0.462916716560925f,
	1.178737310340750f,   1.424457502062019f,   -3.207525353661072f,  -19.048378444758412f,
	-47.055221581646288f, -74.530801502545513f, -76.819606475710525f, -34.360189603714439f,
	45.911370731458956f,  126.083204431962914f, 159.765576060044795f, 126.083204431961582f,
	45.911370731459150f,  -34.360189603714272f, -76.819606475710567f, -74.530801502545402f,
	-47.055221581646260f, -19.048378444758388f, -3.207525353661006f,  1.424457502062048f,
	1.178737310340738f,   0.462916716560904f,   0.268940032322248f,   0.128400969571093f,
	-0.047616537578774f,  -0.106658945543187f,  -0.089992776290978f,  -0.050762976431705f,
	-0.019949989253143f,  -0.004678776861483f,  -0.000003558565316f,  0.000441338085556f,
	0.000133311671509f,   0.000001306510473f,   -0.000005158008016f,  0.000000001f,
	0.000000008f};
/*End Coefficients*/


void inference_init() {
	LOG_WRN("Inference initialized"); 
}

void inference_thread_run(void *arg1, void *arg2,  void *arg3) {
	arm_rfft_fast_instance_f32 FFT_INST;
	int fft_flag = 0;
	arm_status fft_s = arm_rfft_fast_init_4096_f32(&FFT_INST);
	LOG_WRN("FFT Init status  %d", fft_s);

	while(1) {
		k_sem_take(&inference_gate, K_FOREVER);
		LOG_WRN("Inference has been run");


		for (int b = 0; b < 3; b++) {
			void *rx_block = k_fifo_get(&infer_fifo, K_NO_WAIT);
			//1.normalize samples 
			int32_t *samples = rx_block;
			
			//Normalize sample
			
			for(int f = 0; f < SAMPLES_PER_BLOCK; f++)
			{
				// convert to float32
				int32_t  sam = samples[f] >> 8; 
				float32_t f_sample = (float32_t)sam / 8388608.0f;
				samples_f[f] = f_sample;
			}
			//2. Compute FFT of samples 
			arm_rfft_fast_f32(&FFT_INST, samples_f, fft_samples, fft_flag); 

			
			for(int f = 0; f < 100; f++)
			{
				LOG_ERR("FFT Sample %d: %f", f, 
						(double)fft_samples[f]);
			}
			
			// 4. Free each block
			k_mem_slab_free(&inference_slab, (void *)rx_block);

		}
		
		// 5 Use hysteresis  to check for clap


				// Apply Filter
				
			
				

		k_sem_give(&inference_gate);
		fsm_post_event(EVENT_INFERENCE_END);
		k_sleep(K_SECONDS(4)); 
	}
}
