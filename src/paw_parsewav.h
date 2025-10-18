/// @file paw_parsewav.h
///
/// @brief waveデータ解析
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2019.08.01
///
#ifndef _PARSEWAV_PARSEWAV_H_
#define _PARSEWAV_PARSEWAV_H_

#include "common.h"
#include <vector>
#include <stdio.h>
#include <wx/string.h>
#include "paw_parse.h"
#include "errorinfo.h"
#include "paw_datas.h"
#include "paw_format.h"
#include "paw_file.h"
#include "paw_util.h"


namespace PARSEWAV 
{

/// wavファイル解析のパラメータ
class lamda_t
{
public:
	double samples[3];	// 1波のサンプル数 0:1200Hz 1:2400Hz 2:4800Hz
	double us_delta;	// 1サンプルの長さ(us)
	double us_range[3];	// 0:1200Hz 1:2400Hz 2:4800Hz
	double us[3];		// 0:1200Hz 1:2400Hz 2:4800Hz
	double us_min[3];
	double us_max[3];
	double us_avg[3];
	double us_mid[2];	// 0:1800Hz 1:3600Hz
	double us_mid_avg[2];
	double us_limit[2];	// 0:9600Hz 1:19200Hz
public:
	lamda_t();
	void clear();
};

/// 解析用のワーク
class parse_carrier_t
{
public:
	double x0_prev;
	int    wav_prev;
	int    sample_cnt;
	int    odd;
	int    carr_prev;
public:
	parse_carrier_t();
	void clear();
};

/// 交点を保存しておく
class PrevCross
{
public:
	int	spos;
//	int ptn;
//	int cnt;
public:
	PrevCross();
	void Clear();
	int SPos() const { return spos; }
	void SPos(int val) { spos = val; }
};

#ifdef PARSEWAV_USE_REPORT
/// レポート用
class REPORT1
{
private:
	int sample_num[6];
//	int sample_odd[6];
public:
	REPORT1();
	~REPORT1() {}

	void Clear();
	void IncSampleNum(int idx) { sample_num[idx]++; }
//	void IncSampleOdd(int idx) { sample_odd[idx]++; }
	int GetSampleNum(int idx) const { return sample_num[idx]; }
//	int GetSampleOdd(int idx) const { return sample_odd[idx]; }
};
#endif

/// WAVデータ解析用クラス
class WaveParser : public ParserBase
{
private:
	/// 解析用のワーク
	parse_carrier_t st_pa_carr;
	/// １つ前の交点
	PrevCross prev_cross;

	/// wavファイル解析のパラメータ
	lamda_t st_lamda;

#ifdef PARSEWAV_USE_REPORT
	/// レポート表示用
	REPORT1 rep1;
#endif

	/// Wavファイルのフォーマット情報
	WaveFormat *inwav;

//	void check_new_ptn(int ptn);

public:
	WaveParser();

	void Clear();
	void ClearResult();
	void InitForDecode(enum_process_mode process_mode_, WaveFormat &inwav_, TempParameter &tmp_param_, MileStoneList &mile_stone_);
	void InitForEncode(enum_process_mode process_mode_, WaveFormat &inwav_, TempParameter &tmp_param_, MileStoneList &mile_stone_);

	PwErrType CheckWaveFormat(InputFile &file, wav_header_t *head, wav_fmt_chank_t *fmt, wav_data_chank_t *data, Util& conv, PwErrCode &err_num, PwErrInfo &errinfo);

	int GetWaveSample(int blk_size, bool reverse);
	int GetWaveSample(WaveData *w_data, bool reverse);

	int SkipWaveSample(int dir);

	int DecodeToCarrier(int fsk_spd, WaveData *w_data, CarrierData *c_data);

	int EncodeToWave(CarrierData *c_data, uint8_t *w_data, int len);

#ifdef PARSEWAV_USE_REPORT
	const REPORT1 &GetReport() const { return rep1; }
#endif

	const lamda_t &GetLamda() const { return st_lamda; }

	void SetPrevCross(int spos_);

#ifdef PARSEWAV_USE_REPORT
	void DecordingReport(wxString &buff, wxString *logbuf);
	void EncordingReport(WaveFormat &outwav, wxString &buff, wxString *logbuf);
#endif
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_PARSEWAV_H_ */
