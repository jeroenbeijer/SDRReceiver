#include "audiobufferpoolaccess.h"

AudioSampleBufferPool& audioPool()
{
    static AudioSampleBufferPool pool;
    return pool;
}

ComplexAudioSampleBufferPool& iqPool()
{
    static ComplexAudioSampleBufferPool pool;
    return pool;
}
