


#include <iostream>


#include "soapH.h"
#include "stdsoap2.h"
#include "soapStub.h"
#include "base64.h"

using namespace std;

/*opencv库*/
#include <opencv2\opencv.hpp>
#include"opencv2/face.hpp"
#include"opencv2/face/facerec.hpp"
#include"opencv2/objdetect.hpp"
#include"opencv2/core/base.hpp"
#include"opencv2/xfeatures2d.hpp"

extern "C"   /*这里必须要使用C方式导入*/
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "postproc.lib")

using namespace std;
using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;
using namespace cv::ml;
using namespace face;


IplImage* MyRisezeImage(IplImage* pRgbImg, double dScale)
{
	IplImage* pDesImage;
	int nWidth = pRgbImg->width*dScale;
	int nHeight = pRgbImg->height*dScale;
	pDesImage = cvCreateImage(CvSize(nWidth, nHeight), pRgbImg->depth, pRgbImg->nChannels);
	cvResize(pRgbImg, pDesImage, CV_INTER_AREA);
	cvReleaseImage(&pRgbImg);
	return pDesImage;
}

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 4699

int64 seek(AVFormatContext* inputContext, int64_t seekTime, int streamIndex)
{
	int defaultStreamIndex = av_find_default_stream_index(inputContext);
	auto time_base = inputContext->streams[defaultStreamIndex]->time_base;
	auto newDts = inputContext->streams[defaultStreamIndex]->start_time + av_rescale(seekTime, time_base.den, time_base.num);
	if (newDts > inputContext->streams[streamIndex]->cur_dts)
	{
		av_seek_frame(inputContext, defaultStreamIndex, newDts, AVSEEK_FLAG_ANY);
	}
	else
	{
		av_seek_frame(inputContext, defaultStreamIndex, newDts, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
	}
	return newDts;
}

bool IsNearDts(int64_t sdts, int64_t edts)
{
	if (edts - sdts > 0 && edts - sdts < 5000)
		return true;
	return false;
}

//视频倒播方法,更具tdts请求视频帧
AVPacket QueryFrameByDts(AVFormatContext* inputContext, int64_t tdts)
{
	AVPacket        pkt;
	av_init_packet(&pkt);                                                       // initialize packet.
	pkt.data = NULL;
	pkt.size = 0;
	while (1)
	{
		av_read_frame(inputContext, &pkt);                                // read frames
		if (IsNearDts(pkt.dts, tdts))
			return pkt;
	}
	return pkt;
}

//根据视频帧获取dts
AVPacket QueryFrameByIndex(AVFormatContext* inputContext, int64 nFrame, int streamIndex)
{
	//找到距离这一帧，最近的前一关键帧数据seek到那里,获取一个时间节点
	int defaultStreamIndex = av_find_default_stream_index(inputContext);
	auto time_base = inputContext->streams[defaultStreamIndex]->time_base;
	auto newDts = inputContext->streams[defaultStreamIndex]->start_time + av_rescale(nFrame, time_base.den, time_base.num);

	//seek(inputContext, nFrame, streamIndex);

	return QueryFrameByDts(inputContext, newDts);
}

double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

double get_duration_sec(AVFormatContext* inputContext, int video_stream)
{
	double sec = (double)inputContext->duration / (double)AV_TIME_BASE;
	double eps_zero = 0.000025;
	if (sec < eps_zero)
	{
		sec = (double)inputContext->streams[video_stream]->duration * r2d(inputContext->streams[video_stream]->time_base);
	}

	return sec;
}

int64_t GetTotalIFrame(AVFormatContext* inputContext, int video_stream)
{
	int64_t nTotalI = 0;
	int64_t nSlit = 0;
	AVPacket        pkt;
	av_init_packet(&pkt);                                                       // initialize packet.
	pkt.data = NULL;
	pkt.size = 0;
	while (1)
	{
		if (av_read_frame(inputContext, &pkt) < 0)
		{
			nSlit++;
			if(nSlit == 4)
				break;
		}
		else
		{
			if(pkt.stream_index == video_stream && pkt.flags == 1)
				nTotalI++;
		}
	}
	return nTotalI;
}

double get_fps(AVFormatContext* inputContext, int video_stream)
{
	double eps_zero = 0.000025;
	double fps = r2d(inputContext->streams[video_stream]->r_frame_rate);
	if (fps < eps_zero)
	{
		fps = 1.0 / r2d(inputContext->streams[video_stream]->codec->time_base);
	}
	return fps;
}

int64_t get_total_frames(AVFormatContext* inputContext, int video_stream)
{
	int64_t nbf = inputContext->streams[video_stream]->nb_frames;

	if (nbf == 0)
	{
		nbf = (int64_t)floor(get_duration_sec(inputContext, video_stream) * get_fps(inputContext, video_stream) + 0.5);
	}
	return nbf;
}

std::string RtspGetUrl(char* code)
{
	struct soap soap1;
	char* str = "";
	wchar_t* str_1 = NULL;
	int b = 0;
	ns1__startGuiResponse resultStr;
	ns1__startGui a;
	soap_init(&soap1);
	soap_set_mode(&soap1, SOAP_C_MBSTRING);
	a.arg0 = code;//ConvAnsiToUtf8("18600960941");
	soap_call___ns1__startGui(&soap1, "http://13.40.31.73:8080/GetRTSPURL/StartGuiImplPort?wsdl", "", &a, resultStr);
	std::string strOut = "";
	if (soap1.error)
	{
		soap_print_fault(&soap1, stderr);
	}
	else
	{
		str = resultStr.return_;
		if (str == NULL)
			return strOut;
		char *bindata;
		size_t inlen = 0;
		unsigned char *out = (unsigned char*)malloc(strlen(str) + 1);
		memset(out, '\0', strlen(str) + 1);
		unsigned long outlen = 0;
		//过滤掉换行符和空格

		int i = CBase64::DecodeBase64((const unsigned char*)str, out, strlen(str) + 1);
		//std::cout << str << std::endl;
		//std::cout << "-----------------------------------------------------------" << std::endl;

		strOut = (const char*)out;
		free(out);
		//std::cout << strOut.c_str() << std::endl;
	}
	soap_destroy(&soap1);
	soap_end(&soap1);
	soap_done(&soap1);
	return strOut;
}

string trimSpace(string s)
{
	if (s.empty())
	{
		return s;
	}

	//cout << s << endl;
	string::size_type i = 0, j = 0;
	while (i < s.size())
	{
		j = s.find_first_of(" ", i);
		if (j > s.size())
			break;
		s.erase(j, 1);
		i++;
	}
	return s;
}

IplImage* MyResizeImage(IplImage* pSrc, double dScale)
{
	CvSize size;
	size.width = pSrc->width*dScale;
	size.height = pSrc->height*dScale;
	IplImage *pDes = cvCreateImage(size, pSrc->depth, pSrc->nChannels);
	cvResize(pSrc, pDes, CV_INTER_CUBIC);
	return pDes;
}


void open_rtsp(const char *rtsp)
{
	unsigned int    i;
	int             ret;
	int             video_st_index = -1;
	int             audio_st_index = -1;
	AVFormatContext *ifmt_ctx = NULL;
	AVPacket        pkt;
	AVStream        *st = NULL;
	char            errbuf[64];
	AVDictionary *optionsDict = NULL;
	av_register_all();                                                          // Register all codecs and formats so that they can be used.
	avformat_network_init();                                                    // Initialization of network components
	av_dict_set(&optionsDict, "rtsp_transport", "tcp", 0);                //采用tcp传输	,,如果不设置这个有些rtsp流就会卡着
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);                  //如果没有设置stimeout
	av_dict_set(&optionsDict, "profile", "baseline", 0);					 //不要B帧
	if ((ret = avformat_open_input(&ifmt_ctx, rtsp, 0, &optionsDict)) < 0) {            // Open the input file for reading.
		printf("Could not open input file '%s' (error '%s')\n", rtsp, av_make_error_string(errbuf, sizeof(errbuf), ret));
		goto EXIT;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {                // Get information on the input file (number of streams etc.).
		printf("Could not open find stream info (error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
		goto EXIT;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // dump information
		av_dump_format(ifmt_ctx, i, rtsp, 0);
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // find video stream index
		st = ifmt_ctx->streams[i];
		switch (st->codec->codec_type) {
		case AVMEDIA_TYPE_AUDIO: audio_st_index = i; break;
		case AVMEDIA_TYPE_VIDEO: video_st_index = i; break;
		default: break;
		}
	}
	if (-1 == video_st_index) {
		printf("No H.264 video stream in the input file\n");
		goto EXIT;
	}

	av_init_packet(&pkt);                                                       // initialize packet.
	pkt.data = NULL;
	pkt.size = 0;
	bool nRestart = false;
	int videoindex = -1;
	int audioindex = -1;
	AVStream *pVst;

	//视频处理
	uint8_t* buffer_rgb = NULL;

	//音频处理
	uint8_t *pktdata;
	int pktsize;
	AVCodecContext *pVideoCodecCtx = NULL;
	AVCodecContext *pAudioCodecCtx = NULL;
	AVFrame         *pFrame = av_frame_alloc();
	AVFrame         *pFrameRGB = av_frame_alloc();
	int got_picture;
	SwsContext      *img_convert_ctx = NULL;
	AVCodec *pVideoCodec = NULL;
	AVCodec *pAudioCodec = NULL;
	int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
	uint8_t * inbuf = (uint8_t *)malloc(out_size);
	FILE* pcm = fopen("result.pcm", "wb");
	int len1, data_size = 0;
	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	auto lastDts = 0;
	int nCountFrame = 20;

	if (!nRestart)
	{
		for (int i = 0; i < ifmt_ctx->nb_streams; i++)
		{
			if ((ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) && (videoindex < 0))
			{
				videoindex = i;
				pVst = ifmt_ctx->streams[videoindex];
				pVideoCodecCtx = pVst->codec;
				pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
				if (pVideoCodec == NULL)
					return;
				//pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);

				if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0)
					return;
			}
			if ((ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) && (audioindex < 0))
			{
				audioindex = i;
				pVst = ifmt_ctx->streams[audioindex];
				pAudioCodecCtx = pVst->codec;
				pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
				if (pAudioCodec == NULL)
					return;
				//pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);

				if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0)
					return;
			}
		}
	}

	int nTotalI = GetTotalIFrame(ifmt_ctx, videoindex);

	seek(ifmt_ctx, 0, 0);

	QueryFrameByIndex(ifmt_ctx, 1000, videoindex);

	while (1)
	{
		do {
			
			ret = av_read_frame(ifmt_ctx, &pkt);                                // read frames

			if (pkt.stream_index == videoindex)
			{
				fprintf(stdout, "pkt.size=%d,pkt.pts=%lld, pkt.data=0x%x.", pkt.size, pkt.pts, (unsigned int)pkt.data);
				int av_result = avcodec_decode_video2(pVideoCodecCtx, pFrame, &got_picture, &pkt);
				if (av_result < 0)
				{
					fprintf(stderr, "decode failed: inputbuf = 0x%x , input_framesize = %d\n", pkt.data, pkt.size);
					return;
				}
				if (got_picture)
				{
					int bytes = avpicture_get_size(AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height);
					buffer_rgb = (uint8_t *)av_malloc(bytes);
					avpicture_fill((AVPicture *)pFrameRGB, buffer_rgb, AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height);

					img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
						pVideoCodecCtx->width, pVideoCodecCtx->height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
					if (img_convert_ctx == NULL)
					{

						printf("can't init convert context!\n");
						return;
					}
					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
					IplImage *pRgbImg = cvCreateImage(cvSize(pVideoCodecCtx->width, pVideoCodecCtx->height), 8, 3);
					memcpy(pRgbImg->imageData, buffer_rgb, pVideoCodecCtx->width * 3 * pVideoCodecCtx->height);
					//Mat Img = cvarrToMat(pRgbImg, true);

					//保存视频文件
					//VideoWriter *pWriter = new VideoWriter("1.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 25, Size(pVideoCodecCtx->width, pVideoCodecCtx->height), 1);
					//pWriter.
					IplImage* pImg = MyResizeImage(pRgbImg, 0.8);
					cvShowImage("ffmpeg", pImg);

					//DetectFace(Img);
					cvWaitKey(1000/36);
					cvReleaseImage(&pRgbImg);
					sws_freeContext(img_convert_ctx);
					av_free(buffer_rgb);
				}	
			}
		} while (ret == AVERROR(EAGAIN));

		if (ret < 0) {
			printf("Could not read frame ---(error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
			goto EXIT;
		}

		if (pkt.stream_index == video_st_index) {                               // video frame
			printf("Video Packet size = %d\n", pkt.size);
		}
		else if (pkt.stream_index == audio_st_index) {                         // audio frame
			printf("Audio Packet size = %d\n", pkt.size);
		}
		else {
			printf("Unknow Packet size = %d\n", pkt.size);
		}

		av_packet_unref(&pkt);
	}

EXIT:

	if (NULL != ifmt_ctx) {
		avformat_close_input(&ifmt_ctx);
		ifmt_ctx = NULL;
	}

	return;
}

//int main(int argc, char* argv[])
//{
//	//MYONVIF_DetectDevice(cb_discovery);
//
//	if (argc < 3)
//	{
//		//while (1)
//		{
//			//通过urlcode获取rtsp地址
//			//string strUrl = RtspGetUrl("00029240000000000101#f8f5858576b74f1bb31cc1406a729986");
//			//open_rtsp("rtsp://192.168.17.1/Titanic.mkv");
//			//open_rtsp("rtsp://admin:admin12345@192.168.4.66/1");
//			//open_rtsp(argv[1]);
//			open_rtsp("E:\\mycode\\LiveCompile\\x64\\Release\\Titanic.mkv");
//			//open_rtsp(strUrl.c_str());
//		}
//	}
//	else
//	{
//		switch (argc)
//		{
//		case 3:
//			if (argv[1] == "-c")
//			{
//				string strUrl = RtspGetUrl(argv[2]);
//				//open_rtsp("rtsp://192.168.17.1/Titanic.mkv");
//				open_rtsp(strUrl.c_str()); 
//			}
//			else if (argv[1] == "-u")
//			{
//				open_rtsp(argv[2]);
//			}
//			break;
//		case 4:
//			break;
//		case 5:
//			break;
//		default:
//			printf("error parameter\n");
//			break;
//		}
//	}
//
//	//PushStream("D:\\BaiduNetdiskDownload\\test.mp4", "rtmp://10.125.190.15:1935/live/room");
//
//	cout << "hello world" << endl;
//	getchar();
//	return 0;
//}

#include "ffmpegvideoseek.hpp"

int main(int argc, char* argv[])
{
	CvCapture_FFMPEG peg;
	peg.init();
	peg.open("E:\\mycode\\LiveCompile\\x64\\Release\\Titanic.mkv");
	//peg.open("D:\\迅雷下载\\ymz.mkv");
	unsigned char* rgb;
	int nStep = 0;
	int nWidth = 0;
	int nHeight = 0;
	int nCm = 0;
	int i = 0;
	int64_t nCount = peg.get_total_frames();
	while (1)
	{
		peg.seek(nCount--);
		peg.grabFrame();
		peg.retrieveFrame(0, &rgb, &nStep, &nWidth, &nHeight, &nCm);
		if (nStep == 0)
		{
			cout << "is null " << i++ << endl;
			continue;
		}
		IplImage *pRgbImg = cvCreateImage(cvSize(nWidth, nHeight), 8, nCm);
		memcpy(pRgbImg->imageData, rgb, nWidth * nCm * nHeight);
		IplImage *pImg = MyRisezeImage(pRgbImg, 0.4);
		cvShowImage("hello", pImg);
		//Mat Img = cvarrToMat(pImg, true);
		//DetectFace(Img);
		waitKey(1000/40);
		cvReleaseImage(&pImg);
	}
	return 0;
}

