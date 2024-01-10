/// @file paw_format.h
///
/// wave format header definition
///


#ifndef _PARSEWAV_FORMAT_H_
#define _PARSEWAV_FORMAT_H_

#include "common.h"
#include <stdio.h>
#include "paw_file.h"


namespace PARSEWAV 
{

#define T9X_IDENTIFIER "eMB-689X CassetteTapeImageFile  "

#pragma pack(1)
/// wavファイルのヘッダ
typedef struct wav_header_st {
	char RIFF[4];
	uint32_t file_len;
	char WAVE[4];
} wav_header_t;
#pragma pack()

#pragma pack(1)
/// wavファイルのFMTチャンク
typedef struct wav_fmt_chank_st {
	char fmt[4];
	uint32_t fmt_size;
	uint16_t format_id;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t data_speed;
	uint16_t block_size;
	uint16_t sample_bits;
} wav_fmt_chank_t;
#pragma pack()

#pragma pack(1)
/// wavファイルのDATAチャンク
typedef struct wav_data_chank_st {
	char data[4];
	uint32_t data_len;
} wav_data_chank_t;
#pragma pack()

#pragma pack(1)
/// wavファイルのその他のチャンク
typedef struct wav_unknown_chank_st {
	char data[4];
	uint32_t len;
} wav_unknown_chank_t;
#pragma pack()

#pragma pack(1)
/// t9xファイルヘッダ
typedef struct t9x_header_st {
	char ident[32];
	int  data1;
	int  data2;
	char reserved[24];
} t9x_header_t;
#pragma pack()

/// WAVファイルフォーマット解析用クラス
class WaveFormat
{
private:
	wav_header_t	 head;
	wav_fmt_chank_t  fmt;
	wav_data_chank_t data;

public:
	WaveFormat();
	WaveFormat(int rate, int bits, int channels);
	~WaveFormat();
	void Init(int rate, int bits, int channels);
	int Out(uint8_t *buffer);
	int Out(OutputFile &file);
	static void OutSize(OutputFile &file);

	wav_header_t	 *GetHead() { return &head; }
	wav_fmt_chank_t  *GetFmtChank() { return &fmt; }
	wav_data_chank_t *GetDataChank() { return &data; }

	int GetSampleRate(void) { return (int)fmt.sample_rate; }
	int GetSampleBits(void) { return (int)fmt.sample_bits; }
	int GetChannels(void)   { return (int)fmt.channels; }
	int GetBlockSize(void)  { return (int)fmt.block_size; }
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_FORMAT_H_ */
