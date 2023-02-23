#ifndef _AUDIO_RESAMPLE_H_
#define _AUDIO_RESAMPLE_H_


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#else
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#endif


//namespace au{

//template<class AManager>
class AudioResample {
public:
    AudioResample(int src_rate, int dst_rate,
                  int64_t srcAuLayout = AV_CH_LAYOUT_STEREO, int64_t dstAuLayout = AV_CH_LAYOUT_STEREO,
                  int src_nb_channels = 0, int dst_nb_channels = 0, int src_linesize = 0, int dst_linesize = 0,
                  int src_nb_samples = 1024, int dst_nb_samples = 0, int max_dst_nb_samples = 0,
                  AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16);

	int InitARParam();
	int auresample(uint8_t *src_t_data, int src_len, uint8_t** dst_t_data, int* dst_len);

	virtual~AudioResample();

private:
	struct SwrContext *m_swr_ctx;
	int64_t m_src_ch_layout;
	int64_t m_dst_ch_layout;
	int m_dst_rate;
	int m_src_rate;
	int m_src_nb_channels;
	int m_dst_nb_channels;
	int m_src_linesize; 
	int m_dst_linesize;
	AVSampleFormat m_src_sample_fmt;
	AVSampleFormat m_dst_sample_fmt;
	int m_src_nb_samples;
	int m_dst_nb_samples;
	int m_max_dst_nb_samples;
	uint8_t **m_src_data;
	uint8_t **m_dst_data;
};

//}
#endif //AUDIO_RESAMPLE_H_
