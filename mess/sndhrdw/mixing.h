extern udword splitBufferLen;
extern sbyte *signedPanMix8;
extern sword *signedPanMix16;

extern void MixerInit(bool threeVoiceAmplify, ubyte zero8, uword zero16);

void* fill8bitMono( void* buffer, udword numberOfSamples );
void* fill8bitMonoControl( void* buffer, udword numberOfSamples );
void* fill8bitStereo( void* buffer, udword numberOfSamples );
void* fill8bitStereoControl( void* buffer, udword numberOfSamples );
void* fill8bitStereoSurround( void* buffer, udword numberOfSamples );
void* fill8bitSplit( void* buffer, udword numberOfSamples );
void* fill16bitMono( void* buffer, udword numberOfSamples );
void* fill16bitMonoControl( void* buffer, udword numberOfSamples );
void* fill16bitStereo( void* buffer, udword numberOfSamples );
void* fill16bitStereoControl( void* buffer, udword numberOfSamples );
void* fill16bitStereoSurround( void* buffer, udword numberOfSamples );
void* fill16bitSplit( void* buffer, udword numberOfSamples );
