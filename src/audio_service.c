#include <zephyr/logging/log.h>
#include <arm_math.h>

#include "audio_service.h"
#include "fsm_service.h"

LOG_MODULE_REGISTER(audio_module);

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static const struct device *i2s_dev;

static float32_t fir_state[SAMPLES_PER_BLOCK + NUM_TAPS - 1 ];
float32_t samples_f[SAMPLES_PER_BLOCK];
float32_t filtered_samples[SAMPLES_PER_BLOCK];

const float32_t filter_taps[53] = {
    0.000000008f,
    -0.000000017f,
    -0.000005158008031f,
    0.000001306510464f,
    0.000133311671510f,
    0.000441338085554f,
    -0.000003558565317f,
    -0.004678776861470f,
    -0.019949989253140f,
    -0.050762976431733f,
    -0.089992776291018f,
    -0.106658945543195f,
    -0.047616537578776f,
    0.128400969571106f,
    0.268940032322269f,
    0.462916716560925f,
    1.178737310340750f,
    1.424457502062019f,
    -3.207525353661072f,
    -19.048378444758412f,
    -47.055221581646288f,
    -74.530801502545513f,
    -76.819606475710525f,
    -34.360189603714439f,
    45.911370731458956f,
    126.083204431962914f,
    159.765576060044795f,
    126.083204431961582f,
    45.911370731459150f,
    -34.360189603714272f,
    -76.819606475710567f,
    -74.530801502545402f,
    -47.055221581646260f,
    -19.048378444758388f,
    -3.207525353661006f,
    1.424457502062048f,
    1.178737310340738f,
    0.462916716560904f,
    0.268940032322248f,
    0.128400969571093f,
    -0.047616537578774f,
    -0.106658945543187f,
    -0.089992776290978f,
    -0.050762976431705f,
    -0.019949989253143f,
    -0.004678776861483f,
    -0.000003558565316f,
    0.000441338085556f,
    0.000133311671509f,
    0.000001306510473f,
    -0.000005158008016f,
    0.000000001f,
    0.000000008f
};
/*End Coefficients*/
void audio_service_init()
{
	i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
	struct i2s_config config;
	int ret;

	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT_MS;

	ret = i2s_configure(i2s_dev, I2S_DIR_RX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S RX: %d\n", ret);
		return;
	}

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("I2S device not ready\n");
		return;
	}
}

void audio_sense_thread(void *arg1, void *arg2, void *arg3)
{
	int ret;

	arm_fir_instance_f32 FIR;
		
	arm_fir_init_f32(&FIR, NUM_TAPS, (float32_t *)&filter_taps[0], 
							&fir_state[0], SAMPLES_PER_BLOCK);


	while (1) {
		/*Wait on a semaphore*/
		k_sem_take(&mic_sense_gate, K_FOREVER);

		/*Check the audio sense gate before working*/
		void *mem_block;
		uint32_t size;

		ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
		if (ret < 0) {
			LOG_ERR("Failed to start I2S RX: %d \n", ret);
			return;
		}

		LOG_WRN("SENSING AUDIO");
		ret = i2s_read(i2s_dev, &mem_block, &size);
		if (ret == 0) {
			for (int b = 1; b <= 3; b++) {
				int32_t *samples = mem_block;
				/*Normalize samples*/
				
				for(int f = 0; f < SAMPLES_PER_BLOCK; f++)
				{
					// convert to float32
					int32_t  sam = samples[f] >> 8; 
					samples_f[f] = (float32_t)sam / 8388608.0f;
				}

				/* End Normalization*/

				/* Apply Filter*/
				
			
				arm_fir_f32(&FIR, samples_f, filtered_samples, SAMPLES_PER_BLOCK);


				for(int f = 0; f < 100; f++)
				{
					LOG_ERR("Filtered Sample %d: %f", f, 
							(double)filtered_samples[f]);
				}

				/*End Filter apply*/

				k_mem_slab_free(&mem_slab, (void *)mem_block);
			}

		} else {
			LOG_ERR("I2S Failed to read %d", ret);
		}

		// 3. Stop capture, then drop any packes
		i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
		i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);

		// 4. Sleep before next cycle
		k_sem_give(&mic_sense_gate);
		k_sleep(K_SECONDS(2));
	}
}
