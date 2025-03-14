#ifndef SOUND_H
#define SOUND_H

// Define frequency constant
#define LEDC_BASE_FREQ 5000  // Base frequency for the buzzer

void playTone(int frequency, int duration);
void playClassicPagerTone();
void playChirp();
void playVintageBeep();

#endif // SOUND_H