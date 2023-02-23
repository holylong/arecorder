#ifndef _AUDIO_MANAGER_H__
#define _AUDIO_MANAGER_H__

#include <stdio.h>
#include <stdint.h>
#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
    #include <libavutil/opt.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/samplefmt.h>
    #include <libswresample/swresample.h>
}
#endif

//namespace au{

class AudioResample;
	
class AudioManager{
	public:
		AudioManager():_auResample(NULL){};
        int InitManager(int src_rate, int dst_rate,
                        int64_t srcAuLayout = AV_CH_LAYOUT_STEREO, int64_t dstAuLayout = AV_CH_LAYOUT_STEREO,
                        int src_nb_channels = 0, int dst_nb_channels = 0, int src_linesize = 0, int dst_linesize = 0,
                        int src_nb_samples = 1024, int dst_nb_samples = 0, int max_dst_nb_samples = 0,
                        AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16);
		int UninitManager();
		int AuResample(uint8_t *src_t_data,int src_len, uint8_t** dst_t_data, int* dst_len);

		virtual~AudioManager();

	private:
		AudioResample *_auResample;

	};
//}



#endif
