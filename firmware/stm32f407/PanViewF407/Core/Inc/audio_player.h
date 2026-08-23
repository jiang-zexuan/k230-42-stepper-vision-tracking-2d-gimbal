#ifndef PANVIEW_AUDIO_PLAYER_H
#define PANVIEW_AUDIO_PLAYER_H

#include <stdbool.h>

typedef enum
{
  PANVIEW_AUDIO_SEARCHING = 0,
  PANVIEW_AUDIO_TRACKING,
  PANVIEW_AUDIO_LOCKED,
  PANVIEW_AUDIO_LIMIT
} PanViewAudioClip;

void AudioPlayer_Init(void);
bool AudioPlayer_Play(PanViewAudioClip clip);
bool AudioPlayer_IsBusy(void);

#endif /* PANVIEW_AUDIO_PLAYER_H */
