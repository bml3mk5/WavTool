/// @file paw_param.cpp
///
/// @brief waveデータ <-> 搬送波データ <-> シリアルデータ　変換用パラメータ
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2011.7.1
///
#include "paw_param.h"


namespace PARSEWAV
{

const int c_sample_rate[] = {
	11025,
	22050,
	44100,
	48000,
	0
};

Parameter::Parameter()
{
	Clear();
}

Parameter::~Parameter()
{
}

void Parameter::Clear()
{
	// パラメータ初期化
	fsk_speed = 0;

	for(int i=0; i<3; i++) {
		freq[i] = (1200 << i);
	}
	range[0] = 25;
	range[1] = 50;
	reverse = false;

	half_wave = true;
	correct_type = 0;
	correct_amp[0] = 1000;
	correct_amp[1] = 1000;

	baud = 0;
	auto_baud = true;
	word_select = 0x04;

	debug_log = 0;

	sample_rate = 3;	// 48000
	sample_bits = 0;	// 8bit

	file_split = 0;
	del_mhead = 0;

	out_err_ser = false;
}

void Parameter::SetFrequency(int magnify)
{
	fsk_speed = (magnify - 1);
//	freq1200 = 1200 * magnify;
//	freq2400 = 2400 * magnify;
}

int Parameter::GetSampleRate(int val)
{
	if (val >= 0 && val < 4) {
		return c_sample_rate[val];
	} else {
		return c_sample_rate[3];
	}
}

int Parameter::GetSampleRate(void) const
{
	return GetSampleRate(sample_rate);
}

int Parameter::GetBaseFreq(void) const
{
	return 1200 * (fsk_speed + 1);
}

/// 1ワード全体のビット数
/// @note start bit + data bit + parity + stop bit
int Parameter::GetWordAllBitLen(void) const
{
	int len = 1;
	len += GetWordDataBitLen();
	len += GetWordParityBit() > 0 ? 1 : 0;
	len += GetWordStopBitLen();
	return len;
}

/// 1ワードのデータビット数
int Parameter::GetWordDataBitLen(void) const
{
	return (word_select & 0x04) ? 8 : 7;
}

/// 1ワードのパリティビットの有無
/// @return -1:パリティなし 0:偶数パリティ 1:奇数パリティ 
int Parameter::GetWordParityBit(void) const
{
	return ((word_select & 0x06) == 0x04) ? -1 : (word_select & 0x01);
}

/// ストップビット数
int Parameter::GetWordStopBitLen(void) const
{
	return ((word_select & 0x02) == 0 && (word_select & 0x07) != 0x05) ? 2 : 1;
}

//

TempParameter::TempParameter()
{
	Clear();
}

TempParameter::~TempParameter()
{
}

void TempParameter::Clear()
{
	// パラメータ初期化
	half_wave = true;
	auto_baud = true;
	reverse = false;
	correct_type = 0;
	view_progbox = true;
	debug_log = 0;
}

}; /* namespace PARSEWAV */
