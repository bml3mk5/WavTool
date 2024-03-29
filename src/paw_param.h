﻿/// @file paw_param.h
///
/// @brief waveデータ <-> 搬送波データ <-> シリアルデータ　変換用パラメータ
///
/// @author Sasaji
/// @date   2011.7.1
///


#ifndef _PARSEWAV_PARAM_H_
#define _PARSEWAV_PARAM_H_


namespace PARSEWAV
{

/// ダイアログ用パラメータ
class Parameter
{
private:
	int  fsk_speed;	///< 0:standard(1200 and 2400Hz) 1:double speed(2400 and 4800Hz)
	int  freq[3];	///< 0:1200 1:2400 2:4800 (Hz)
	int  range[2];	///< 0:long 1:short
	bool reverse;	///< reverse wave
	int  baud;		///< 0:600band 1:1200band 2:2400baud 3:300baud
	bool auto_baud;	///< auto judgement of baud rate
	int  word_select;	///< bit structure of one data
	bool half_wave;	///< parse half wave
	int  correct_type;	///< 0:none 1: cos wave 2:sin wave
	int  correct_amp[2];	///< amplitude of correct wave
	int  debug_log;
	int  sample_bits;	///< 0:8bits 1:16bits
	int  sample_rate;	///< 0:11025  1:22050  2:44100  3:48000
	int  file_split;
	int  del_mhead;
	int  chg_gap_size;
	bool out_err_ser;

public:
	Parameter();
	~Parameter();
	void Clear();

	void SetSampleRatePos(int value)	{ sample_rate = value; }
	void SetSampleBitsPos(int value)	{ sample_bits = value; }
	void SetBaud(int value)			{ baud = value; }
	void SetAutoBaud(bool value)	{ auto_baud = value; }
	void SetReverseWave(bool value)	{ reverse = value; }
	void SetHalfWave(bool value)		{ half_wave = value; }
	void SetCorrectType(int value)	{ correct_type = value; }
	void SetCorrectAmp(int num, int value) { correct_amp[num] = value; }
	void SetFskSpeed(int value)		{ fsk_speed = value; }
	void SetFreq(int num, int value) { freq[num] = value; }
	void SetFrequency(int magnify);
	void SetRange(int num, int value) { range[num] = value; }
	void SetWordSelect(int value)	{ word_select = value; }
	void SetFileSplit(int value)	{ file_split = value; }
	void SetDeleteMHead(int value)	{ del_mhead = value; }
	void SetChangeGapSize(int value) { chg_gap_size = value; }
	void SetOutErrSerial(bool val)	{ out_err_ser = val; }
	void SetDebugMode(int value)	{ debug_log = value; }

	static int GetSampleRate(int val);
	int GetSampleRate(void) const;
	int GetSampleRatePos(void) const	{ return sample_rate; }
	int GetSampleBitsPos(void) const	{ return sample_bits; }
	int GetBaud(void) const			{ return baud; }
	bool GetAutoBaud(void) const		{ return auto_baud; }
	bool GetReverseWave(void) const	{ return reverse; }
	bool GetHalfWave(void) const		{ return half_wave; }
	int GetCorrectType(void) const	{ return correct_type; }
	int GetCorrectAmp(int num) const { return correct_amp[num]; }
	int GetFskSpeed(void) const		{ return fsk_speed; }
	int GetFreq(int num) const		{ return freq[num]; }
	int	GetRange(int num) const		{ return range[num]; }
	int	GetWordSelect(void) const	{ return word_select; }
	int GetFileSplit(void) const	{ return file_split; }
	int GetDeleteMHead(void) const	{ return del_mhead; }
	int GetChangeGapSize(void) const { return chg_gap_size; }
	bool GetOutErrSerial() const	{ return out_err_ser; }
	int GetDebugMode(void) const	{ return debug_log; }
	int GetBaseFreq(void) const;

	int GetWordAllBitLen(void) const;
	int GetWordDataBitLen(void) const;
	int GetWordParityBit(void) const;
	int GetWordStopBitLen(void) const;
};

/// 解析時に使用する一時的なパラメータ
class TempParameter
{
private:
	bool half_wave;
	bool auto_baud;
	bool reverse;	///< reverse wave
	int  correct_type;	///< 0:none 1: cos wave 2:sin wave
	bool view_progbox;
	int debug_log;
public:
	TempParameter();
	~TempParameter();
	void Clear();

	void SetAutoBaud(bool value)	{ auto_baud = value; }
	void SetHalfWave(bool value)	{ half_wave = value; }
	void SetReverseWave(bool value)	{ reverse = value; }
	void SetCorrectType(int value)	{ correct_type = value; }
	void SetViewProgBox(bool value)	{ view_progbox = value; }
	void SetDebugMode(int value)	{ debug_log = value; }

	bool GetAutoBaud(void) const	{ return auto_baud; }
	bool GetHalfWave(void) const	{ return half_wave; }
	bool GetReverseWave(void) const	{ return reverse; }
	int GetCorrectType(void) const	{ return correct_type; }
	bool GetViewProgBox(void) const	{ return view_progbox; }
	int GetDebugMode(void) const	{ return debug_log; }
};


}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_PARAM_H_ */
