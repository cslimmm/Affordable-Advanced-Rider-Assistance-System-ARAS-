#ifndef DFPLAYER_SERVICE_H
#define DFPLAYER_SERVICE_H

#include <DFRobotDFPlayerMini.h>

void dfplayerInit();
void playAlcoholWarning();
void playWarning(int warning);
void processAudioQueue();
void stopDFPlayer();
void setDfVolume(int volume);
void enqueue(int track);

#endif
