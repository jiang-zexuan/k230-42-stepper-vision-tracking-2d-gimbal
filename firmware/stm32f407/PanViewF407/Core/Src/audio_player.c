#include "audio_player.h"

#include <stdint.h>

#include "audio_assets.h"
#include "i2s.h"

/*
 * I2S 的 Size 单位是 halfword，且 HAL 参数本身是 uint16_t。
 * 每个 WAV 样本复制到左右声道后，单次 DMA 固定发送 1024 个采样帧。
 */
enum
{
  AUDIO_DMA_FRAMES = 1024U,
  AUDIO_DMA_HALFWWORDS = AUDIO_DMA_FRAMES * 2U,
  /* 每次启动 I2S 先发送约 170 ms 静音，让 ES8388 输出稳定但不产生提示音。 */
  AUDIO_PREROLL_CHUNKS = 8U
};

typedef struct
{
  const uint16_t *samples;
  uint32_t sample_count;
} AudioClipData;

static const AudioClipData audio_clips[] = {
    {searching_audio, 77760U},
    {tracking_audio, 66240U},
    {locked_audio, 69120U},
    {limit_audio, 103680U},
};

static uint16_t audio_dma_buffer[AUDIO_DMA_HALFWWORDS];
static const uint16_t *audio_samples;
static uint32_t audio_sample_count;
static uint32_t audio_sample_offset;
static uint32_t audio_preroll_chunks;
static volatile bool audio_busy;

static void AudioPlayer_FillSilence(void)
{
  uint32_t index;

  for (index = 0U; index < AUDIO_DMA_HALFWWORDS; ++index)
  {
    audio_dma_buffer[index] = 0U;
  }
}

static bool AudioPlayer_FillBuffer(void)
{
  uint32_t frame;
  uint32_t frames_to_copy;

  if ((audio_samples == NULL) || (audio_sample_offset >= audio_sample_count))
  {
    return false;
  }

  frames_to_copy = audio_sample_count - audio_sample_offset;
  if (frames_to_copy > AUDIO_DMA_FRAMES)
  {
    frames_to_copy = AUDIO_DMA_FRAMES;
  }

  for (frame = 0U; frame < frames_to_copy; ++frame)
  {
    uint16_t sample = audio_samples[audio_sample_offset + frame];
    audio_dma_buffer[(frame * 2U) + 0U] = sample;
    audio_dma_buffer[(frame * 2U) + 1U] = sample;
  }

  for (; frame < AUDIO_DMA_FRAMES; ++frame)
  {
    audio_dma_buffer[(frame * 2U) + 0U] = 0U;
    audio_dma_buffer[(frame * 2U) + 1U] = 0U;
  }

  audio_sample_offset += frames_to_copy;
  return true;
}

void AudioPlayer_Init(void)
{
  audio_samples = NULL;
  audio_sample_count = 0U;
  audio_sample_offset = 0U;
  audio_preroll_chunks = 0U;
  audio_busy = false;
}

bool AudioPlayer_Play(PanViewAudioClip clip)
{
  const AudioClipData *clip_data;

  if (((uint32_t)clip >=
       (sizeof(audio_clips) / sizeof(audio_clips[0]))) || audio_busy)
  {
    return false;
  }

  clip_data = &audio_clips[(uint32_t)clip];
  audio_samples = clip_data->samples;
  audio_sample_count = clip_data->sample_count;
  audio_sample_offset = 0U;
  audio_preroll_chunks = AUDIO_PREROLL_CHUNKS;
  AudioPlayer_FillSilence();

  audio_busy = true;
  if (HAL_I2S_Transmit_DMA(&hi2s2, audio_dma_buffer,
                           AUDIO_DMA_HALFWWORDS) != HAL_OK)
  {
    audio_busy = false;
    return false;
  }
  return true;
}

bool AudioPlayer_IsBusy(void)
{
  return audio_busy;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if ((hi2s != &hi2s2) || !audio_busy)
  {
    return;
  }

  if (audio_preroll_chunks > 0U)
  {
    --audio_preroll_chunks;
    AudioPlayer_FillSilence();
    if (HAL_I2S_Transmit_DMA(hi2s, audio_dma_buffer,
                             AUDIO_DMA_HALFWWORDS) != HAL_OK)
    {
      audio_busy = false;
      (void)HAL_I2S_DMAStop(hi2s);
    }
    return;
  }

  if (!AudioPlayer_FillBuffer())
  {
    audio_busy = false;
    (void)HAL_I2S_DMAStop(hi2s);
    return;
  }

  if (HAL_I2S_Transmit_DMA(hi2s, audio_dma_buffer,
                           AUDIO_DMA_HALFWWORDS) != HAL_OK)
  {
    audio_busy = false;
    (void)HAL_I2S_DMAStop(hi2s);
  }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s == &hi2s2)
  {
    audio_busy = false;
  }
}
