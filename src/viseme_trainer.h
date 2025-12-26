#ifndef VISEME_TRAINER_H
#define VISEME_TRAINER_H

#define VIS_SLOT_SILENCE 1
#define VIS_SLOT_CH      2
#define VIS_SLOT_OU      3
#define VIS_SLOT_AA      4

void VisemeInit(void);
void VisemeShutdown(void);
void VisemeProcess(const float* pcm, int frameCount);
void VisemeSetTraining(int slot);
void VisemeSave(const char* filename);
int VisemeLoad(const char* filename);
int VisemeGetBest(void);
float* VisemeGetConfidences(void);

#endif
