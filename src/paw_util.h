/// @file paw_util.h
///
/// @brief 関数群
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2011.12.01
///
#ifndef _PARSEWAV_UTIL_H_
#define _PARSEWAV_UTIL_H_

#include "errorinfo.h"
#include "paw_datas.h"
#include "paw_format.h"
#include "paw_file.h"


namespace PARSEWAV
{

#define OUTCONV_BUFFER_SIZE (DATA_ARRAY_SIZE + 4)

/// @brief WAVファイルを扱うクラス
class Util
{
private:
	int remain_buf[100];
	int remain_buf_len;
	int divide;
	int surplus;

	int16_t src_buf[OUTCONV_BUFFER_SIZE];
	int16_t new_buf[OUTCONV_BUFFER_SIZE];

	size_t out_sample_buffer(int16_t *in_buf, size_t in_len, int in_blk_size, int out_blk_size, OutputFile &file);

public:
	Util();
	~Util();

	void   InitConvSampleData();
	void   OutConvSampleData(WaveData &in_data, int in_rate
						, int outrate, int out_blk_size, OutputFile &file);
	void   OutConvSampleData(const void *in_buf, int in_rate, int in_blk_size, size_t in_len
						, int outrate, int out_blk_size, OutputFile &file);
	size_t ReadWavData(InputFile &file, wav_fmt_chank_t *in_fmt, size_t in_len, uint8_t *outbuf, uint32_t outrate, int outbits, size_t outlen);

	static PwErrCode CheckWavFormat(InputFile &file, wav_header_t *head, wav_fmt_chank_t *fmt, wav_data_chank_t *data, size_t *data_len = NULL);
	static PwErrCode CheckWavFormat(InputFile &file, WaveFormat &format, size_t *data_len = NULL);
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_UTIL_H_ */
