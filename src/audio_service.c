#include <zephyr/logging/log.h>

#include "audio_service.h"

LOG_MODULE_REGISTER(audio_module);

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static const struct device *i2s_dev;

void audio_service_init() {
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

void audio_sense_thread(void *arg1, void *arg2, void *arg3) {
  int ret;
  while (1) {
    void *mem_block;
    uint32_t size;

    ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
    if (ret < 0) {
      LOG_ERR("Failed to start I2S RX: %d \n", ret);
      return;
    }

    ret = i2s_read(i2s_dev, &mem_block, &size);
    if (ret == 0){
      int32_t *samples = mem_block;
      LOG_WRN("Block first sample: %d \n", samples[0]);
    } else {
      LOG_ERR("I2S Failed to read %d", ret);
    }

    // 3. Stop capture
    i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
    LOG_WRN("Capture cycle complete");

    // 4. Sleep before next cycle
    k_sleep(K_SECONDS(2));
  }
}
