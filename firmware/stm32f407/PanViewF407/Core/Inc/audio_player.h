#ifndef PANVIEW_AUDIO_PLAYER_H
#define PANVIEW_AUDIO_PLAYER_H

#include <stdbool.h>

typedef enum
{
  PANVIEW_AUDIO_HIT = 0
} PanViewAudioClip;

void AudioPlayer_Init(void);
bool AudioPlayer_Play(PanViewAudioClip clip);
bool AudioPlayer_IsBusy(void);

#endif /* PANVIEW_AUDIO_PLAYER_H */
