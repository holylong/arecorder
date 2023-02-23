#include "audio_manager.h"
#include "audio_resample.h"
#include "tracer.h"

//using namespace au;

int AudioManager::InitManager(int src_rate, int dst_rate,
                              int64_t srcAuLayout, int64_t dstAuLayout,
                              int src_nb_channels, int dst_nb_channels, int src_linesize, int dst_linesize,
                              int src_nb_samples, int dst_nb_samples, int max_dst_nb_samples,
                              AVSampleFormat src_sample_fmt, AVSampleFormat dst_sample_fmt)
{
	_auResample = NULL;
 //       if(src_rate == 48000 && dst_rate == 16000)
 //       _auResample = new AudioResample(src_rate, dst_rate,
 //                                       AV_CH_LAYOUT_STEREO, AV_CH_LAYOUT_STEREO);
	//else if(src_rate == 22050 && dst_rate == 16000)
	//	_auResample = new AudioResample(src_rate, dst_rate, AV_CH_LAYOUT_STEREO, AV_CH_LAYOUT_MONO);	
 //   else if(src_rate == 44100 && dst_rate == 16000)
    _auResample = new AudioResample(src_rate, dst_rate,
		srcAuLayout, dstAuLayout,
										src_nb_channels, dst_nb_channels,
										src_linesize, dst_linesize,
										src_nb_samples, dst_nb_samples,
										max_dst_nb_samples, src_sample_fmt, dst_sample_fmt);
    //else if(src_rate == 16000 && dst_rate == 44100)
        //_auResample = new AudioResample(src_rate, dst_rate, AV_CH_LAYOUT_STEREO, AV_CH_LAYOUT_STEREO);
        
	return dst_rate - src_rate;
}

int AudioManager::UninitManager()
{
	if(!_auResample){
		delete _auResample;
		_auResample = NULL;
	}
	return 0;
}

int AudioManager::AuResample(uint8_t *src_t_data,int src_len, uint8_t** dst_t_data, int* dst_len)
{
	if(!_auResample)
	{
		LOGE("AudioManager not ready!i\n");
		return -1;
	}
	return _auResample->auresample(src_t_data, src_len, dst_t_data, dst_len);
}

AudioManager::~AudioManager(){
	if(!_auResample){
		UninitManager();
	}
}




