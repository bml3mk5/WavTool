/// @file paw_parsewav.cpp
///
/// @author Sasaji
/// @date   2019.08.01
///

#include "paw_parsewav.h"
#include "paw_file.h"
#include "utils.h"


namespace PARSEWAV 
{

//

lamda_t::lamda_t()
{
	clear();
}

void lamda_t::clear()
{
	us_delta = 0.0;
	for(int n=0; n<3; n++) {
		samples[n] = 0.0;
		us_range[n] = 0.0;
		us[n] = 0.0;
		us_avg[n] = 0.0;
		us_min[n] = 0.0;
		us_max[n] = 0.0;
	}
	for(int n=0; n<2; n++) {
		us_mid[n] = 0.0;
		us_mid_avg[n] = 0.0;
		us_limit[n] = 0.0;
	}
}

//

parse_carrier_t::parse_carrier_t()
{
	clear();
}

void parse_carrier_t::clear()
{
	x0_prev = 0.0;
	sample_cnt = 0;
	wav_prev = 0;
	carr_prev = 0;
	odd = 0;
}

//

PrevCross::PrevCross()
{
	Clear();
}

void PrevCross::Clear()
{
	spos = 0;
//	ptn = 0;
//	cnt = 0;
}

//

#ifdef PARSEWAV_USE_REPORT
REPORT1::REPORT1()
{
	Clear();
}

void REPORT1::Clear()
{
	for(int i=0; i<6; i++) {
		sample_num[i] = 0;
//		sample_odd[i] = 0;
	}
}
#endif

//

WaveParser::WaveParser()
	: ParserBase()
{
#ifdef PARSEWAV_USE_REPORT
	rep1.Clear();
#endif
}

void WaveParser::Clear()
{
	st_pa_carr.clear();
	st_lamda.clear();
}

void WaveParser::ClearResult()
{
#ifdef PARSEWAV_USE_REPORT
	rep1.Clear();
#endif
}

/// @brief デコード時の初期処理
///
void WaveParser::InitForDecode(enum_process_mode process_mode_, WaveFormat &inwav_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);
	inwav = &inwav_;

	st_pa_carr.clear();
	prev_cross.Clear();

	st_lamda.us_delta = 1000000.0 / (double)inwav->GetSampleRate();
	for(int n=0; n<3; n++) {
		st_lamda.samples[n] = (double)inwav->GetSampleRate() / (double)(1200 << n);

		st_lamda.us_range[n] = 1000000.0 / (double)(1200 << n) / (double)(tmp_param->GetHalfWave() ? 2 : 1);
		st_lamda.us[n] = 1000000.0 / (double)param->GetFreq(n) / (double)(tmp_param->GetHalfWave() ? 2 : 1);
		st_lamda.us_avg[n] = st_lamda.us[n];
	}
	for(int n=0; n<2; n++) {
		st_lamda.us_mid[n] = 1000000.0 / (double)(1800 << n) / (double)(tmp_param->GetHalfWave() ? 2 : 1);
		st_lamda.us_mid_avg[n] = st_lamda.us_mid[n];

		st_lamda.us_limit[n] = 1000000.0 / (double)(1200 << (n + 3));
	}

	int ns = param->GetFskSpeed();
	if (process_mode == PROCESS_ANALYZING) {
		for(int n=0; n<2; n++, ns++) {
			st_lamda.us_min[ns] = st_lamda.us[ns] - st_lamda.us_range[ns] * 0.25;
			st_lamda.us_max[ns] = st_lamda.us[ns] + st_lamda.us_range[ns] * 0.50;
		}
	} else {
		for(int n=0; n<2; n++, ns++) {
			st_lamda.us_min[ns] = st_lamda.us[ns] - st_lamda.us_range[ns] * param->GetRange(n) / 100.0;
			st_lamda.us_max[ns] = st_lamda.us[ns] + st_lamda.us_range[ns] * param->GetRange(n) / 100.0;
		}
	}
}

/// @brief エンコード時の初期処理
///
void WaveParser::InitForEncode(enum_process_mode process_mode_, WaveFormat &inwav_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);
	inwav = &inwav_;
}

/// @brief wavファイルのフォーマットをチェック
///
/// @param[in] file 入力ファイル
/// @param[in] head WAVファイル ヘッダチャンク
/// @param[in] fmt  WAVファイル FMTチャンク
/// @param[in] data WAVファイル DATチャンク
/// @param[in] conv WAVファイルチェッククラス
/// @param[out] err_num エラー番号
/// @param[out] errinfo エラー情報
/// @return pwOK / pwError
///
PwErrType WaveParser::CheckWaveFormat(InputFile &file, wav_header_t *head, wav_fmt_chank_t *fmt, wav_data_chank_t *data, Util& conv, PwErrCode &err_num, PwErrInfo &errinfo)
{
	SetInputFile(file);

	err_num = conv.CheckWavFormat(file, head, fmt, data);
	if (err_num != pwErrNone) {
		errinfo.SetInfo(__LINE__, pwError, err_num);
		errinfo.ShowMsgBox();
		return pwError;
	}

	int sample_num = (data->data_len / fmt->channels);
	if(fmt->sample_bits == 16) {
		sample_num /= 2;
	}
	file.SampleNum(sample_num);

	file.SampleRate(fmt->sample_rate);

	return pwOK;
}



/// @brief wavファイルから１サンプル読む
///
/// @param[in] blk_size 1: uint8_tで返す  2: int16_tで返す
/// @param[in] reverse  波形を反転
/// @return 変換した１サンプルデータ
///
int WaveParser::GetWaveSample(int blk_size, bool reverse)
{
	int l = 0;
	int h = 0;
	int in_bits = inwav->GetSampleBits();

	// 1サンプルを得る
	l = infile->Fgetc();
	if(in_bits == 16) {
		// int16_tの場合
		h = infile->Fgetc();
		if (h >= 128) {
			h = h - 256;
		}
		if (reverse) {
			h *= -1;
			if (h >= 128) h = 127;
			l *= -1;
		}
	} else {
		if (reverse) {
			l = 256 - l;
			if (l >= 256) l = 255;
		}
	}
	// ステレオの場合、左側を無視する
	for(int i = 2; i <= inwav->GetChannels(); i++) {
		infile->Fgetc();
		if(in_bits == 16) {
			infile->Fgetc();
		}
	}

	if (blk_size == 1) {
		if(in_bits == 16) {
			// int16_t -> uint8_t
			return ((h + 128) & 0xff);
		} else {
			return (l & 0xff);
		}
	} else {
		if(in_bits == 16) {
			return h * 256 + l;
		} else {
			// uint8_t -> int16_t
			return (l - 128) * 256;
		}
	}
}

/// @brief wavファイルからサンプルを読んでバッファに追記
///
/// @param[in,out] w_data サンプルデータ用のバッファ(追記していく)
/// @param[in] reverse    波形を反転
/// @return 読み込んだデータの長さ
///
int WaveParser::GetWaveSample(WaveData *w_data, bool reverse)
{
	int l;

	// バッファがいっぱいになるまで読み込む
	while(!w_data->IsFull() && infile->SamplePos() < infile->SampleNum()) {
		l = GetWaveSample(1, reverse);

		w_data->Add((uint8_t)l, infile->SamplePos());

		mile_stone->MarkIfNeed(infile->SamplePos());

		infile->IncreaseSamplePos();
//		infile->CalcrateSampleUSec(inwav->GetSampleRate());
	}
	if (infile->SamplePos() >= infile->SampleNum()) {
		w_data->LastData(true);
	}
	return w_data->GetWritePos();
}

/// @brief wavファイルから１サンプルスキップする
///
/// @param[in] dir
/// @return スキップ数
///
int WaveParser::SkipWaveSample(int dir)
{
	int in_bits = inwav->GetSampleBits();
	int offset = 0;

	if (infile->IsFirstPos(-dir)) {
		dir = infile->SamplePos() * -1;
	} else if (infile->IsEndPos(dir)) {
		dir = infile->SampleNum() - infile->SamplePos();
	}
	offset = dir * inwav->GetChannels() * in_bits / 8;

	infile->Fseek(offset, SEEK_CUR);

	infile->SamplePos(infile->SamplePos()+dir);

	return dir;
}

/// @brief 2400Hzと1200Hzの波を見つけてビットデータに変換
///
/// @param[in]   fsk_spd   1:倍速FSK
/// @param[in]   w_data    WAVサンプルデータ
/// @param[out]  c_data    "10": 2400Hz "1100":1200Hz "?GGLL":Too long
/// @return bit0:wavバッファのデータが足りない bit1:wavバッファ最後のブロック bit4::carrierバッファに空きがない
int WaveParser::DecodeToCarrier(int fsk_spd, WaveData *w_data, CarrierData *c_data)
{
	uint8_t bit_data[8];
	int bit_len = 0;

	int rc = 0;
	int data = 0;
	CSampleData sample_data;
	bool hi = false;
	bool found = false;

	double x0 = 0.0;
	double lamda = 0.0;

	// find next trigger point
	while(!w_data->IsTail()) {
		sample_data = w_data->GetRead();
		w_data->IncreaseReadPos();
		data = (int)sample_data.Data() - 128;

		if (st_pa_carr.wav_prev < 0 && data >= 0) {
			// l -> h になる位置から波長(us)を求める
			hi = true;
			x0 = - (st_pa_carr.wav_prev * st_lamda.us_delta / (data - st_pa_carr.wav_prev));
			lamda = st_pa_carr.x0_prev + (st_pa_carr.sample_cnt * st_lamda.us_delta) + x0;
			// 波長が短すぎる場合はノイズと判断して処理継続
			if (lamda >= st_lamda.us_limit[fsk_spd]) {
				st_pa_carr.x0_prev = st_lamda.us_delta - x0;
				st_pa_carr.odd = 1 - st_pa_carr.odd;
				found = true;
				break;
			}
		} else if (st_pa_carr.wav_prev > 0 && data <= 0 && tmp_param->GetHalfWave()) {
			// h -> l になる位置から波長(us)を求める
			hi = false;
			x0 = - (st_pa_carr.wav_prev * st_lamda.us_delta / (data - st_pa_carr.wav_prev));
			lamda = st_pa_carr.x0_prev + (st_pa_carr.sample_cnt * st_lamda.us_delta) + x0;
			// 波長が短すぎる場合はノイズと判断して処理継続
			if (lamda >= st_lamda.us_limit[fsk_spd]) {
				st_pa_carr.x0_prev = st_lamda.us_delta - x0;
				st_pa_carr.odd = 1 - st_pa_carr.odd;
				found = true;
				break;
			}
		}
		st_pa_carr.sample_cnt++;
		st_pa_carr.wav_prev = data;
	}
	if (found != true) {
		rc = w_data->IsLastData() ? (w_data->IsTail() ? 0x03 : 0x02) : (w_data->IsTail((int)st_lamda.samples[1] + 2) ? 0x01 : 0);
		return rc;
	}

	if (!tmp_param->GetHalfWave()) {
		// 全波
		if (st_lamda.us_min[fsk_spd] <= lamda && lamda <= st_lamda.us_max[fsk_spd]) {
			// long (half wave)
			bit_len = 4;
			memcpy(bit_data, "1100", bit_len);
			st_pa_carr.carr_prev = 4;
			st_lamda.us_avg[fsk_spd] = (st_lamda.us_avg[fsk_spd] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(0);
#endif
		}
		else if (st_lamda.us_min[fsk_spd+1] <= lamda && lamda <= st_lamda.us_max[fsk_spd+1]) {
			// short (half wave)
			bit_len = 2;
			memcpy(bit_data, "10", bit_len);
			st_pa_carr.carr_prev = 2;
			st_lamda.us_avg[fsk_spd+1] = (st_lamda.us_avg[fsk_spd+1] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(1);
#endif
		}
		else if (st_lamda.us_mid[fsk_spd] <= lamda && lamda < st_lamda.us_min[fsk_spd]) {
			// middle long
			bit_len = 5;
			memcpy(bit_data, "?1100", bit_len);
			st_pa_carr.carr_prev = 4;
			st_lamda.us_mid_avg[fsk_spd] = (st_lamda.us_mid_avg[fsk_spd] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(2);
#endif
		}
		else if (st_lamda.us_max[fsk_spd+1] < lamda && lamda < st_lamda.us_mid[fsk_spd]) {
			// middle short
			bit_len = 3;
			memcpy(bit_data, "?10", bit_len);
			st_pa_carr.carr_prev = 2;
			st_lamda.us_mid_avg[fsk_spd] = (st_lamda.us_mid_avg[fsk_spd] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(2);
#endif
		}
		else if (st_lamda.us_max[fsk_spd] < lamda) {
			// too long
			bit_len = 5;
			memcpy(bit_data, "?GGLL", bit_len);
			st_pa_carr.carr_prev = 5;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(3);
#endif
		}
		else if (lamda < st_lamda.us_min[fsk_spd+1]) {
			// too short
			bit_len = 3;
			memcpy(bit_data, "?GL", bit_len);
			st_pa_carr.carr_prev = 1;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(4);
#endif
		}
		else {
			// error
			bit_len = 2;
			memcpy(bit_data, "??", bit_len);
			st_pa_carr.carr_prev = 0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(5);
#endif
		}
	} else {
		// 半波
		if (st_lamda.us_min[fsk_spd] <= lamda && lamda <= st_lamda.us_max[fsk_spd]) {
			// long (half wave)
			if (hi) {
				bit_len = 2;
				memcpy(bit_data, "00", bit_len);
				if (st_pa_carr.carr_prev != 4) st_pa_carr.odd = 1;
				st_pa_carr.carr_prev = -4;
			} else {
				bit_len = 2;
				memcpy(bit_data, "11", bit_len);
				if (st_pa_carr.carr_prev != -4) st_pa_carr.odd = 1;
				st_pa_carr.carr_prev = 4;
			}
			st_lamda.us_avg[fsk_spd] = (st_lamda.us_avg[fsk_spd] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(0);
//			check_new_ptn(0);
#endif
		}
		else if (st_lamda.us_min[fsk_spd+1] <= lamda && lamda <= st_lamda.us_max[fsk_spd+1]) {
			// short (half wave)
			if (hi) {
				bit_len = 1;
				memcpy(bit_data, "0", bit_len);
				if (st_pa_carr.carr_prev != 2) st_pa_carr.odd = 1;
				st_pa_carr.carr_prev = -2;
			} else {
				bit_len = 1;
				memcpy(bit_data, "1", bit_len);
				if (st_pa_carr.carr_prev != -2) st_pa_carr.odd = 1;
				st_pa_carr.carr_prev = 2;
			}
			st_lamda.us_avg[fsk_spd+1] = (st_lamda.us_avg[fsk_spd+1] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(1);
//			check_new_ptn(1);
#endif
		}
		else if (st_lamda.us_max[fsk_spd+1] < lamda && lamda < st_lamda.us_min[fsk_spd]) {
			// middle
			if (hi) {
				if ((st_pa_carr.odd == 0 && st_pa_carr.carr_prev == 4)
				 || (st_pa_carr.odd == 1 && st_pa_carr.carr_prev == 2)) {
					bit_len = 3;
					memcpy(bit_data, "?00", bit_len);
					st_pa_carr.carr_prev = -4;
				} else {
					bit_len = 2;
					memcpy(bit_data, "?0", bit_len);
					st_pa_carr.carr_prev = -2;
				}
			} else {
				if ((st_pa_carr.odd == 0 && st_pa_carr.carr_prev == -4)
				 || (st_pa_carr.odd == 1 && st_pa_carr.carr_prev == -2)) {
					bit_len = 3;
					memcpy(bit_data, "?11", bit_len);
					st_pa_carr.carr_prev = 4;
				} else {
					bit_len = 2;
					memcpy(bit_data, "?1", bit_len);
					st_pa_carr.carr_prev = 2;
				}
			}
			st_lamda.us_mid_avg[fsk_spd] = (st_lamda.us_mid_avg[fsk_spd] + lamda) / 2.0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(2);
//			check_new_ptn(2);
#endif
		}
		else if (st_lamda.us_max[fsk_spd] < lamda) {
			// too long
			if (hi) {
				bit_len = 3;
				memcpy(bit_data, "?LL", bit_len);
				st_pa_carr.carr_prev = -5;
			} else {
				bit_len = 3;
				memcpy(bit_data, "?GG", bit_len);
				st_pa_carr.carr_prev = 5;
			}

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(3);
//			check_new_ptn(3);
#endif
		}
		else if (lamda < st_lamda.us_min[fsk_spd+1]) {
			// too short
			if (hi) {
				bit_len = 2;
				memcpy(bit_data, "?L", bit_len);
				st_pa_carr.carr_prev = -1;
			} else {
				bit_len = 2;
				memcpy(bit_data, "?G", bit_len);
				st_pa_carr.carr_prev = 1;
			}

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(4);
//			check_new_ptn(4);
#endif
		}
		else {
			// error
			bit_len = 2;
			memcpy(bit_data, "??", bit_len);
			st_pa_carr.carr_prev = 0;

#ifdef PARSEWAV_USE_REPORT
			rep1.IncSampleNum(5);
//			check_new_ptn(5);
#endif
		}
	}

	// 一定間隔を覚えておく
	mile_stone->ModifyMarkIfNeed(prev_cross.SPos());

	// データ追加
	if (bit_data[0] != '?') {
		// OK data
		c_data->AddString(bit_data, bit_len, prev_cross.SPos());

		if (tmp_param->GetDebugMode() > 1) {
			// デバッグログ
			gLogFile.Fprintf("p1 w:%10d(%s) %2d %3.6f "
				, prev_cross.SPos()
				, UTILS::get_time_cstr(infile->CalcrateSampleUSec(prev_cross.SPos()))
				, st_pa_carr.sample_cnt
				, lamda);
			gLogFile.Fwrite(bit_data, sizeof(uint8_t), bit_len);
			gLogFile.Fputc('\n');
		}
	} else {
		// NG data
		c_data->AddString(&bit_data[1], bit_len - 1, prev_cross.SPos(), 0, 0x8);

		if (tmp_param->GetDebugMode() > 1) {
			// デバッグログ
			gLogFile.Fprintf("p1 w:%10d(%s) %2d %3.6f "
				, prev_cross.SPos()
				, UTILS::get_time_cstr(infile->CalcrateSampleUSec(prev_cross.SPos()))
				, st_pa_carr.sample_cnt
				, lamda);
			gLogFile.Fwrite(bit_data, sizeof(uint8_t), bit_len);
			gLogFile.Fputc('\n');
		}
	}

	st_pa_carr.wav_prev = data;
	st_pa_carr.sample_cnt = 0;

	prev_cross.SPos(sample_data.SPos());

	rc = w_data->IsLastData() ? (w_data->IsTail() ? 0x03 : 0x02) : (w_data->IsTail((int)st_lamda.samples[1] + 2) ? 0x01 : 0);
	rc |= (c_data->IsFull(4) ? 0x10 : 0);
	return rc;
}

#if 0
void WaveParser::check_new_ptn(int ptn)
{
	if (prev_cross.ptn != ptn) {
		if (prev_cross.cnt & 1) {
			rep1.IncSampleOdd(prev_cross.ptn);
		}
		prev_cross.ptn = ptn;
		prev_cross.cnt = 0;
	}
	prev_cross.cnt++;
}
#endif

/// @brief 搬送波ビットデータをwaveサンプルデータに変換する
/// @brief (48000Hz 16bit)
///
/// @param[in]  c_data 搬送波データ
/// @param[out] w_data waveサンプルデータ
/// @param[in]  len    waveサンプルデータバッファサイズ
/// @return waveサンプルデータに変換した長さ
int WaveParser::EncodeToWave(CarrierData *c_data, uint8_t *w_data, int len)
{
	int pos = 0;
	int8_t wav_samples[10];
	int last = param->GetFskSpeed() ? 5 : 10;

	if (last > len) {
		last = len;
	}

	memset(wav_samples, 0x70, 10);

	if (c_data->GetReadPos() <= 0
	|| (c_data->GetRead(-1).Data() & 1) != (c_data->GetRead().Data() & 1)) {
		wav_samples[0] = 0x6c;
	}
	if (last > 0 && (c_data->GetReadPos() >= c_data->GetWritePos() - 1
	|| (c_data->GetRead().Data() & 1) != (c_data->GetRead(1).Data() & 1))) {
		wav_samples[last-1] = 0x6c;
	}

	// convert to wave sample data
	for(int i=0; i<last; i++) {
		w_data[pos++]=0x80 + ((c_data->GetRead().Data() & 1) ? wav_samples[i] : - wav_samples[i]);
	}
	c_data->IncreaseReadPos();

	return pos;
}

/// @brief 事前のサンプリング位置をセット
void WaveParser::SetPrevCross(int spos_)
{
	prev_cross.SPos(spos_);
}

#ifdef PARSEWAV_USE_REPORT
/// @brief デコード時のレポート
void WaveParser::DecordingReport(wxString &buff, wxString *logbuf)
{
	int spd = param->GetFskSpeed();

	gLogFile.SetLogBuf(logbuf);

	buff = _T(" [ wav -> l3c ]");
	gLogFile.Write(buff, 1);
	buff.Printf(_T("  Wave Type: %s"), (spd ? _T("Double Speed FSK") : _T("Standard FSK")));
	gLogFile.Write(buff, 1);
	buff.Printf(_T("  Detection: %s"),(param->GetHalfWave() ? _T("Half wave") : _T("Full wave")));
	gLogFile.Write(buff, 1);
	buff.Printf(_T("  Range: Long(0) : %2d%% (%3.3fus(%6.1fHz) - %3.3fus(%6.1fHz) - %3.3fus(%6.1fHz))")
		,param->GetRange(0)
		,st_lamda.us_max[spd],	(1000000.0 / st_lamda.us_max[spd])
		,st_lamda.us[spd],		(1000000.0 / st_lamda.us[spd])
		,st_lamda.us_min[spd],  (1000000.0 / st_lamda.us_min[spd])
	);
	gLogFile.Write(buff, 1);
	buff.Printf(_T("  Range: Short(1): %2d%% (%3.3fus(%6.1fHz) - %3.3fus(%6.1fHz) - %3.3fus(%6.1fHz))")
		,param->GetRange(1)
		,st_lamda.us_max[spd+1],(1000000.0 / st_lamda.us_max[spd+1])
		,st_lamda.us[spd+1],	(1000000.0 / st_lamda.us[spd+1])
		,st_lamda.us_min[spd+1],(1000000.0 / st_lamda.us_min[spd+1])
	);
	gLogFile.Write(buff, 1);
	buff.Printf(_T("  Wave Reverse: %s"),(param->GetReverseWave() ? _T("on") : _T("off")));
	gLogFile.Write(buff, 1);
	buff.Printf(_T("  Correct: %s"),(param->GetCorrectType() > 0 ? _T("on") : _T("off")));
	gLogFile.Write(buff, 1);
	if (param->GetCorrectType() > 0) {
		buff.Printf(_T("  Correct Type: %s"),(param->GetCorrectType() == 2 ? _T("sin wave") : _T("cos wave")));
		gLogFile.Write(buff, 1);
	}
	buff = _T("  Wave Sum:");
	gLogFile.Write(buff, 1);

	buff.Printf(_T("    Long(0) : %8d Cent:%6.1fHz Avg:%6.1fHz")
		, rep1.GetSampleNum(0)
		, 1000000.0 / st_lamda.us[spd]
		, 1000000.0 / st_lamda.us_avg[spd]);
	gLogFile.Write(buff, 1);
	buff.Printf(_T("    Short(1): %8d Cent:%6.1fHz Avg:%6.1fHz")
		, rep1.GetSampleNum(1)
		, 1000000.0 / st_lamda.us[spd+1]
		, 1000000.0 / st_lamda.us_avg[spd+1]);
	gLogFile.Write(buff, 1);
	buff.Printf(_T("    Middle  : %8d Cent:%6.1fHz Avg:%6.1fHz")
		, rep1.GetSampleNum(2)
		, 1000000.0 / st_lamda.us_mid[spd]
		, 1000000.0 / st_lamda.us_mid_avg[spd]);
	gLogFile.Write(buff, 1);
	buff.Printf(_T("    Too Long : %8d"),rep1.GetSampleNum(3));
	gLogFile.Write(buff, 1);
	buff.Printf(_T("    Too Short: %8d"),rep1.GetSampleNum(4));
	gLogFile.Write(buff, 1);
	buff.Printf(_T("    Error    : %8d"),rep1.GetSampleNum(5));
	gLogFile.Write(buff, 1);

	gLogFile.Write(_T(""), 1);
}

/// @brief エンコード時のレポート
void WaveParser::EncordingReport(WaveFormat &outwav, wxString &buff, wxString *logbuf)
{
	buff = _T(" [ l3c -> wav ]");
	gLogFile.Write(buff, 1);
	buff.Printf(_T(" %dHz %dbit %dch"),outwav.GetSampleRate(), outwav.GetSampleBits(), outwav.GetChannels());
	gLogFile.Write(buff, 1);

	gLogFile.Write(_T(""), 1);
}
#endif

}; /* namespace PARSEWAV */
