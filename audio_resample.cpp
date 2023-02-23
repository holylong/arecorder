#include "audio_resample.h"
//#include "audio_log.h"
//#include "ffmpeg/include/libswresample/swresample.h"
//#include "ffmpeg/include/libavutil/samplefmt.h"
#include "tracer.h"
//using namespace au;

static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt)
{
	int i;
	struct sample_fmt_entry { enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le; }
	sample_fmt_entries[] = {
		{ AV_SAMPLE_FMT_U8,  "u8",    "u8" },
		{ AV_SAMPLE_FMT_S16, "s16be", "s16le" },
		{ AV_SAMPLE_FMT_S32, "s32be", "s32le" },
		{ AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
		{ AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
	};
	*fmt = NULL;

	for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
		struct sample_fmt_entry *entry = &sample_fmt_entries[i];
		if (sample_fmt == entry->sample_fmt) {
			*fmt = AV_NE(entry->fmt_be, entry->fmt_le);
			return 0;
		}
	}

	//fprintf(stderr, "Sample format %s not supported as output format\n", av_get_sample_fmt_name(sample_fmt));
	//LOGD("AudioResample Sample format %s not supported as output format", av_get_sample_fmt_name(sample_fmt));
	return AVERROR(EINVAL);
}

/**
* Fill dst buffer with nb_samples, generated starting from t.
*/
static void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t)
{
	int i, j;
	double tincr = 1.0 / sample_rate, *dstp = dst;
	const double c = 2 * M_PI * 440.0;

	/* generate sin tone with 440Hz frequency and duplicated channels */
	for (i = 0; i < nb_samples; i++) {
		*dstp = sin(c * *t);
		for (j = 1; j < nb_channels; j++)
			dstp[j] = dstp[0];
		dstp += nb_channels;
		*t += tincr;
	}
}

static int fill_samples(double *dst, int *bn, FILE *fp)
{
	int ret = fread(dst, 1, *bn, fp);
	//*bn += 1024;
	return ret;
}

AudioResample::AudioResample(int src_rate, int dst_rate, int64_t srcAuLayout, int64_t dstAuLayout,
	int src_nb_channels, int dst_nb_channels, int src_linesize, int dst_linesize,
	int src_nb_samples, int dst_nb_samples, int max_dst_nb_samples,
	AVSampleFormat src_sample_fmt, AVSampleFormat dst_sample_fmt):
	m_src_rate(src_rate),m_dst_rate(dst_rate),
	m_src_ch_layout(srcAuLayout),m_dst_ch_layout(dstAuLayout),
	m_src_nb_channels(src_nb_channels),m_dst_nb_channels(dst_nb_channels),
	m_src_linesize(src_linesize),m_dst_linesize(dst_linesize),
	m_src_nb_samples(src_nb_samples),m_dst_nb_samples(dst_nb_samples),
	m_max_dst_nb_samples(max_dst_nb_samples),
	m_src_sample_fmt(src_sample_fmt),m_dst_sample_fmt(dst_sample_fmt)
{
	//m_src_ch_layout = AV_CH_LAYOUT_STEREO;
	//m_dst_ch_layout = AV_CH_LAYOUT_STEREO;
	// android support
	//m_src_nb_samples = 480;
	InitARParam();
}

int AudioResample::InitARParam() {

	int ret = 0;

	/* create resampler context */
	m_swr_ctx = swr_alloc();
	if (!m_swr_ctx) {
		//fprintf(stderr, "Could not allocate resampler context\n");
		LOGE("AudioResample Could not allocate resampler context");
		ret = AVERROR(ENOMEM);
		swr_free(&m_swr_ctx);
		return ret;
	}

	/* set options */
	av_opt_set_int(m_swr_ctx, "in_channel_layout", m_src_ch_layout, 0);
	av_opt_set_int(m_swr_ctx, "in_sample_rate", m_src_rate, 0);
	av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt", m_src_sample_fmt, 0);

	av_opt_set_int(m_swr_ctx, "out_channel_layout", m_dst_ch_layout, 0);
	av_opt_set_int(m_swr_ctx, "out_sample_rate", m_dst_rate, 0);
	av_opt_set_sample_fmt(m_swr_ctx, "out_sample_fmt", m_dst_sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(m_swr_ctx)) < 0) {
		//fprintf(stderr, "Failed to initialize the resampling context\n");
		LOGE("AudioResample Failed to initialize the resampling context");
		swr_free(&m_swr_ctx);
		return ret;
	}

	/* allocate source and destination samples buffers */
	m_src_nb_channels = av_get_channel_layout_nb_channels(m_src_ch_layout);
	ret = av_samples_alloc_array_and_samples(&m_src_data, &m_src_linesize, m_src_nb_channels, m_src_nb_samples, m_src_sample_fmt, 0);
	if (ret < 0) {
		//fprintf(stderr, "Could not allocate source samples\n");
		LOGE("AudioResample Could not allocate source samples");
		swr_free(&m_swr_ctx);
		if (m_src_data)
			av_freep(&m_src_data[0]);
		av_freep(&m_src_data);
		return ret;
	}

	/* compute the number of converted samples: buffering is avoided
	* ensuring that the output buffer will contain at least all the
	* converted input samples */
	m_max_dst_nb_samples = m_dst_nb_samples = av_rescale_rnd(m_src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);

	/* buffer is going to be directly written to a rawaudio file, no alignment */
	m_dst_nb_channels = av_get_channel_layout_nb_channels(m_dst_ch_layout);
	ret = av_samples_alloc_array_and_samples(&m_dst_data, &m_dst_linesize, m_dst_nb_channels, m_dst_nb_samples, m_dst_sample_fmt, 0);
	if (ret < 0) {
		//fprintf(stderr, "Could not allocate destination samples\n");
		LOGE("AudioResample Could not allocate destination samples");
		if (m_src_data)
			av_freep(&m_src_data[0]);
		av_freep(&m_src_data);
		if (m_dst_data)
			av_freep(&m_dst_data[0]);
		av_freep(&m_dst_data);
		swr_free(&m_swr_ctx);
		return ret;
	}
	return ret;
}

AudioResample::~AudioResample()
{
	if (m_src_data)
		av_freep(&m_src_data[0]);
	av_freep(&m_src_data);
	if (m_dst_data)
		av_freep(&m_dst_data[0]);
	av_freep(&m_dst_data);
	swr_free(&m_swr_ctx);
}

int AudioResample::auresample(uint8_t *src_t_data,int src_len, uint8_t** dst_t_data, int* dst_len)
{
	//LOGD("AudioResample auresample audio data enter srclen:%d", src_len);
	int ret = 0;
	const char *fmt;
	memcpy((double*)m_src_data[0], src_t_data, src_len);
	//LOGD("memcpy ok");

	/* compute destination number of samples */
	m_dst_nb_samples = av_rescale_rnd(swr_get_delay(m_swr_ctx, m_src_rate) + m_src_nb_samples, m_dst_rate, m_src_rate, AV_ROUND_UP);
	if (m_dst_nb_samples > m_max_dst_nb_samples) {
		av_freep(&m_dst_data[0]);
		ret = av_samples_alloc(m_dst_data, &m_dst_linesize, m_dst_nb_channels,
			m_dst_nb_samples, m_dst_sample_fmt, 1);
		if (ret < 0)
		{
			LOGE("AudioResample av_samples_alloc error");
			return ret;
		}
		m_max_dst_nb_samples = m_dst_nb_samples;
	}

	/* convert to destination format */
	ret = swr_convert(m_swr_ctx, m_dst_data, m_dst_nb_samples, (const uint8_t **)m_src_data, m_src_nb_samples);
	if (ret < 0) {
		//fprintf(stderr, "AudioResample Error while converting\n");
		LOGE("AudioResample Error while converting");
		return ret;
	}
	int dst_bufsize = av_samples_get_buffer_size(&m_dst_linesize, m_dst_nb_channels, ret, m_dst_sample_fmt, 1);
	if (dst_bufsize < 0) {
		//fprintf(stderr, "AudioResample Could not get sample buffer size\n");
		LOGE("AudioResample Could not get sample buffer size");
		return ret;
	}
	if (dst_bufsize > 0)
	{
		//dst_t_data = (uint8_t*)malloc(dst_bufsize);
		//memcpy(*dst_t_data, m_dst_data[0], dst_bufsize);
		*dst_t_data = m_dst_data[0];
		*dst_len = dst_bufsize;
		//LOGD("get resample buffer success");
	}
	//LOGD("AudioResample src_len:%d dst_len:%d nb_samples:%d ret:%d\n", src_len, *dst_len, m_src_nb_samples, ret);
	
	if ((ret = get_format_from_sample_fmt(&fmt, m_dst_sample_fmt)) < 0)
		return ret;
	//fprintf(stderr, "AudioResample Resampling succeeded. Play the output file with the command:\n" 
	//	"ffplay -f %s -channel_layout %ld -channels %d -ar %d\n",
        //		fmt, m_dst_ch_layout, m_dst_nb_channels, m_dst_rate);
//	LOGD("AudioResample Resampling succeeded. Play the output file with the command:\n"
//		"ffplay -f %s -channel_layout %ld -channels %d -ar %d\n",
//        		fmt, m_dst_ch_layout, m_dst_nb_channels, m_dst_rate);
	return ret;
}
