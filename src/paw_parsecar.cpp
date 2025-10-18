/// @file paw_parsecar.cpp
///
/// @brief 搬送波データ解析
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2019.08.01
///
#include "paw_parsecar.h"
#include "paw_defs.h"
#include "paw_file.h"
#include "utils.h"


namespace PARSEWAV 
{

// 2400ボーの時はエッジ判定を優先
static const struct st_pattern carrier_edge_pattern[]={
	{ (const uint8_t *)"0010", 4 },		// 01 (2400 baud)
	{ (const uint8_t *)"1101", 4 },		// 01 (2400 baud frip)
	{ (const uint8_t *)"0100", 4 },		// 10 (2400 baud)
	{ (const uint8_t *)"1011", 4 },		// 10 (2400 baud frip)
	{ NULL, 0 }
};

//#define MAX_CARRIER_PATTERN	1
static const struct st_pattern carrier_pattern[4][4]={
	{
		{ (const uint8_t *)"11001100", 8 },	// 0 (600 baud)
		{ (const uint8_t *)"00110011", 8 },	// 0 (600 baud frip)
		{ (const uint8_t *)"10101010", 8 },	// 1 (600 baud)
		{ (const uint8_t *)"01010101", 8 },	// 1 (600 baud frip)
	},{
		{ (const uint8_t *)"1100", 4 },		// 0 (1200 baud)
		{ (const uint8_t *)"0011", 4 },		// 0 (1200 baud frip)
		{ (const uint8_t *)"1010", 4 },		// 1 (1200 baud)
		{ (const uint8_t *)"0101", 4 },		// 1 (1200 baud frip)
	},{
		{ (const uint8_t *)"11", 2 },		// 0 (2400 baud)
		{ (const uint8_t *)"00", 2 },		// 0 (2400 baud frip)
		{ (const uint8_t *)"10", 2 },		// 1 (2400 baud)
		{ (const uint8_t *)"01", 2 },		// 1 (2400 baud frip)
	},{
		{ (const uint8_t *)"1100110011001100", 16 },	// 0 (300 baud)
		{ (const uint8_t *)"0011001100110011", 16 },	// 0 (300 baud frip)
		{ (const uint8_t *)"1010101010101010", 16 },	// 1 (300 baud)
		{ (const uint8_t *)"0101010101010101", 16 },	// 1 (300 baud frip)
	}
};

//

#ifdef PARSEWAV_USE_REPORT
REPORT2::REPORT2()
{
	Clear();
}

void REPORT2::Clear()
{
	error_num = 0;
}
#endif

//

CarrierParser::CarrierParser()
	: ParserBase()
{
	phase = 0;
	frip = 0;
	baud24_frip = 0;
}

void CarrierParser::ClearResult()
{
#ifdef PARSEWAV_USE_REPORT
	rep2.Clear();
#endif
}

/// @brief デコード時の初期処理
///
void CarrierParser::InitForDecode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);

	phase = 0;
	frip = 0;
	baud24_frip = 0;

	prev_data = 0;
	prev_width = 0;
	over_pos = 0;
}

/// @brief エンコード時の初期処理
///
void CarrierParser::InitForEncode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);

	phase = 0;
	frip = 0;
	baud24_frip = 0;

	prev_data = 0;
	prev_width = 0;
	over_pos = 0;
}

/// @brief l3cファイル(搬送波ビットデータ)からサイズを計算
int CarrierParser::CalcL3CSize(InputFile &file)
{
	int l;
	int sample_num = 0;
	int file_size = file.GetSize();
	file.Fseek(0, SEEK_SET);
	while(file_size > 0) {
		file_size--;

		l = file.Fgetc();
		l &= 0xff;

		if (l == '\r' || l == '\n') {
			continue;
		}
		sample_num++;
	}
	file.SampleNum(sample_num);
	file.Fseek(0, SEEK_SET);
	return sample_num;
}

/// @brief l3cファイルのフォーマットをチェック（チェックしていないが）
///
/// @param[in] file 入力ファイル
/// @return pwOK
///
PwErrType CarrierParser::CheckL3CFormat(InputFile &file)
{
	SetInputFile(file);

	CalcL3CSize(file);

	file.SampleRate(GetSampleRate());

	return pwOK;
}

double CarrierParser::GetSampleRate()
{
	int baud_mag = param->GetFskSpeed() + 1;
	return 4800.0 * baud_mag;
}

/// @brief l3cファイル(搬送波ビットデータ)から１データ読んでバッファに追記
///
/// @param[in,out] c_data 搬送波データ用のバッファ(追記していく)
/// @return 読み込んだデータの長さ
///
///
int CarrierParser::GetL3CSample(CarrierData *c_data)
{
	int l;

	while(c_data->IsFull() != true && infile->SamplePos() < infile->SampleNum()) {
		l = infile->Fgetc();
		l &= 0xff;

		if (l == '\r' || l == '\n') {
			continue;
		}
		c_data->Add(l, infile->SamplePos());

		mile_stone->MarkIfNeed(infile->SamplePos());

		infile->IncreaseSamplePos();
	}
	if (infile->SamplePos() >= infile->SampleNum()) {
		c_data->LastData(true);
	}
	return c_data->GetWritePos();
}

/// @brief l3cファイル(搬送波ビットデータ)から１サンプルスキップする
///
/// @param[in] dir
/// @return スキップ数
///
int CarrierParser::SkipL3CSample(int dir)
{
	int l;
	int pos = 0;
	if (dir > 0) {
		if (infile->SamplePos() + dir + 1 >= infile->SampleNum()) {
			dir = infile->SampleNum() - infile->SamplePos() - 1;
		}
		while(pos < dir) {
			l = infile->Fgetc();
			if (l == '\r' || l == '\n') {
				continue;
			}
			pos++;
		}
		infile->AddSamplePos(dir);
	} else if (dir < 0) {
		if (infile->SamplePos() < -dir) {
			dir = -infile->SamplePos();
		}
		 while (dir < 0) {
			infile->Fseek(dir, SEEK_CUR);
			int crlf = 0;
			pos = 0;
			while(pos < -dir) {
				l = infile->Fgetc();
				if (l == '\r' || l == '\n') {
					crlf++;
				}
				pos++;
			}
			infile->Fseek(dir, SEEK_CUR);
			infile->AddSamplePos(dir);
			dir = -crlf;
		}
	}

	return pos;
}

/// @brief 一致するパターンを探してデコード
int CarrierParser::Decode(CarrierData *c_data, SerialData *s_data, int8_t baud, int &step)
{
	int rc = 0;

	if (phase == 0) {
		rc = FindStartCarrierBit(c_data, s_data, baud, step);
		if (step >= 0) phase = 1;
	} else {
		rc = DecodeToSerial(c_data, s_data, baud, step);
		if (step < 0) phase = 0;
	}
	return rc;
}

/// @brief 一致するパターンを探す
///
/// @param[in]   c_data 搬送波
/// @param[out]  s_data シリアルデータ
/// @param[in]   baud   600baud:0 1200baud:1 2400baud:2 300baud:3
/// @param[out]  step   スタート位置 / -1 なし
///   
int CarrierParser::FindStartCarrierBit(CarrierData *c_data, SerialData *s_data, int8_t baud, int &step)
{
	int pos1[4];
	int pos = -1;
	int idx_ptn = (baud & 3);
	int best_num = -1;
	int best_pos = c_data->GetSize();
	int i, n;
	CSampleData samples[4];
	int samples_len;

	samples[0].Data('?');
	samples_len = 1;

	// 2400ボーの時はエッジを探す
	if (idx_ptn == IDX_PTN_2400) {
		for(i=0; i<4; i++) {
			pos1[i]=c_data->FindRead(0, carrier_edge_pattern[i].ptn, carrier_edge_pattern[i].len);
		}
		for(i=0; i<4; i++) {
			if (pos1[i] >= 0 && best_pos > pos1[i]) {
				best_pos = pos1[i];
				best_num = i;
			}
		}
		if (best_num >= 0) {
			// あり
			pos = best_pos + carrier_edge_pattern[best_num].len;

			samples_len = 2;
			for(n=0; n<samples_len; n++) {
				samples[n].Set(c_data->GetRead(best_pos + n * 2));
				samples[n].Baud(baud);
				samples[n].CPhase(phase);
				samples[n].CFrip(frip);
			}
			if ((best_num / 2) == 0) {
				samples[0].Data('0');
				samples[1].Data('1');
			} else {
				samples[0].Data('1');
				samples[1].Data('0');
			}

			mile_stone->ModifyMarkIfNeed(samples[0].SPos(), baud, phase, frip);

			if ((best_num % 2) == 0) {
				frip = 0;
			} else {
				frip = 1;
			}
		}
	}

	// 一致するパターンを探す
	if (best_num < 0) {
		for(i=0; i<4; i++) {
			pos1[i]=c_data->FindRead(0, carrier_pattern[idx_ptn][i].ptn, carrier_pattern[idx_ptn][i].len);
		}
		for(i=0; i<4; i++) {
			if (pos1[i] >= 0 && best_pos > pos1[i]) {
				best_pos = pos1[i];
				best_num = i;
			}
		}
		if (best_num >= 0) {
			// あり
			pos = best_pos + carrier_pattern[idx_ptn][best_num].len;

			samples_len = 1;
			samples[0].Set(c_data->GetRead(best_pos));
			samples[0].Baud(baud);
			samples[0].CPhase(phase);
			samples[0].CFrip(frip);
			if ((best_num / 2) == 0) {
				samples[0].Data('0');
			} else {
				samples[0].Data('1');
			}

			mile_stone->ModifyMarkIfNeed(samples[0].SPos(), baud, phase, frip);

			if ((best_num % 2) == 0) {
				frip = 0;
			} else {
				frip = 1;
			}
		}
	}

	if (pos >= 0) {
		// データ有り
		if (tmp_param->GetDebugMode() > 1) {
			// デバッグログ
			gLogFile.Fprintf("p2 fst c:%12d(%s) pos:%d frip:%d ("
				, samples[0].SPos()
				, UTILS::get_time_cstr(infile->CalcrateSampleUSec(samples[0].SPos()))
				, best_pos, frip);
			CSampleString str(*c_data, c_data->GetReadPos() + best_pos, pos - best_pos);
			gLogFile.Fputs(str.Get());
			gLogFile.Fputs(")\n");
		}

		for(int i=0; i<samples_len; i++) {
			s_data->Add(samples[i]);
		}
		c_data->AddReadPos(pos);

	} else {
		// データなし
		c_data->SkipReadPos();
	}

	step = pos;

	bool c_last = (c_data->IsLastData() && c_data->IsTail());
	s_data->LastData(c_last);

	int rc = 0;
	if (c_last) {
		// 最後のデータ
		rc = 1;
	} else if (pos >= 0) {
		// データあり
		if (rc & 0x10) {
			// sデータがfull
			rc = 1;
		} else if (!c_data->IsLastData() && c_data->IsTail(32)) {
#ifdef PARSEWAV_FILL_BUFFER
			if (process_mode == PROCESS_VIEWING) {
				rc = 1;
			} else {
				rc = 2;
			}
#else
			rc = 1;
#endif
		}
	} else {
		// データ見つからない phase1へ
		rc = 2;
	}

	return rc;
}

/// @brief 搬送波(2400/1200Hz)のデータからシリアルデータを１ビット取り出す
///
/// @param[in]  c_data  搬送波
/// @param[out] s_data    シリアルデータ
/// @param[in]  baud    600baud:0 1200baud:1 2400baud:2 300baud:3
/// @param[out] step   スタート位置 / -1 なし
/// @return 次の位置
int CarrierParser::DecodeToSerial(CarrierData *c_data, SerialData *s_data, int8_t baud, int &step)
{
	int pos = -1;
	int idx_ptn = (baud & 3);
	int len;

	CSampleData sample;
	const uint8_t *ptn;


	len = carrier_pattern[idx_ptn][frip].len;
	ptn = carrier_pattern[idx_ptn][frip].ptn;
	sample.Set(c_data->GetRead());
	sample.Baud(baud);
	sample.CPhase(phase);
	sample.CFrip(frip);

	if (c_data->CompareRead(0, ptn, len) == 0) {	// 0
		// 0
		sample.Data('0');

		pos = len;
		if (idx_ptn == IDX_PTN_2400) {
			// 2400ボーで0の場合、常にfripする
			frip = (1 - frip);
		}
	}
	if (pos < 0 && idx_ptn == IDX_PTN_2400) {
		// 2400 ボーのときはfripして再度0を検索
		len = carrier_pattern[idx_ptn][1 - frip].len;
		ptn = carrier_pattern[idx_ptn][1 - frip].ptn;
		if (c_data->CompareRead(0, ptn, len) == 0) {	// 0
			sample.Data('0');

			pos = len;
			if (idx_ptn == IDX_PTN_2400) {
				// 2400ボーで0の場合、常にfripする
				frip = (1 - frip);
			}
		}
	}
	if (pos < 0) {
		len = carrier_pattern[idx_ptn][2 + frip].len;
		ptn = carrier_pattern[idx_ptn][2 + frip].ptn;
		if (c_data->CompareRead(0, ptn, len) == 0)	{ // 1
			// 1
			sample.Data('1');

			pos = len;
		}
	}

	if (pos >= 0) {
		// データ追加
		s_data->Add(sample);

		c_data->AddReadPos(pos);

		mile_stone->ModifyMarkIfNeed(sample.SPos(), sample.Baud(), sample.CPhase(), sample.CFrip());

	} else {
		// 一致するパターンがない
		sample.Data('?');
		sample.Err(0x8);

		s_data->Add(sample);


		if (c_data->RemainLength() > 0) {
			c_data->IncreaseReadPos();
		}

#ifdef PARSEWAV_USE_REPORT
		rep2.IncErrorNum();
#endif

		if (tmp_param->GetDebugMode() > 1) {
			// デバッグログ
			gLogFile.Fprintf("p2 c2s c:%12d(%s) error ("
				, sample.SPos()
				, UTILS::get_time_cstr(infile->CalcrateSampleUSec(sample.SPos()))
			);
			CSampleString str(*c_data, c_data->GetReadPos(), c_data->RemainLength() >= len ? len : c_data->RemainLength());
			gLogFile.Fputs(str.Get());
			gLogFile.Fputs(")\n");
		}
	}	

	step = pos;

	bool c_last = (c_data->IsLastData() && c_data->IsTail());
	s_data->LastData(c_last);

	int rc = 0;
	if (c_last || s_data->IsFull()) {
		// c_dataが最後
		// s_dataがいっぱい
		rc = 1;
	} else if (!c_data->IsLastData() && c_data->IsTail(32)) {
#ifdef PARSEWAV_FILL_BUFFER
		if (process_mode == PROCESS_VIEWING) {
			// s_data解析へ
			rc = 1;
		} else {
			// c_dataを埋める
			rc = 2;
		}
#else
		rc = 1;
#endif
	}

	return (rc | 0x10000);
}

/// @brief 搬送波からボーレートを判定する
///
/// @param[in] fsk_spd   1:倍速FSK
/// @param[in] c_data    搬送波
/// @param[in] st_chkwav 結果集計データ
/// @param[in] step      処理した位置
/// @return 解析した位置 / -1:該当なし
int CarrierParser::ParseBaudRate(int fsk_spd, CarrierData *c_data, ChkWave *st_chkwav, int &step)
{
	int len = 0;
	int pos = -1;
	int pos0 = 0;
	int pos1 = 0;
	int idx_ptn;
	int idx;
	char buf0[16 * 11];
	char buf1[16 * 11];

	for(idx = 0; idx < 4; idx++) {
		idx_ptn = c_baud_min_to_s1[idx];
		// 0になる位置を集計
		strcpy(buf0, (const char *)carrier_pattern[idx_ptn][2].ptn);	// 1
		strcpy(buf1, (const char *)carrier_pattern[idx_ptn][3].ptn);
		strcat(buf0, (const char *)carrier_pattern[idx_ptn][0].ptn);	// 0
		strcat(buf1, (const char *)carrier_pattern[idx_ptn][1].ptn);
		int idx_frip = (idx_ptn == 2 ? 3 : 2);	// 2400ボーの時は逆
		for(int i=0; i<8; i++) {
			strcat(buf0, (const char *)carrier_pattern[idx_ptn][idx_frip].ptn);	// 11111111
			strcat(buf1, (const char *)carrier_pattern[idx_ptn][5 - idx_frip].ptn);
		}
		pos0=c_data->FindRead(0, (const uint8_t *)buf0, (int)strlen(buf0));
		pos1=c_data->FindRead(0, (const uint8_t *)buf1, (int)strlen(buf1));
		if (pos0 >= 0) {
			// 順波
			st_chkwav[fsk_spd].rev_num[st_chkwav[fsk_spd].num][0]++;
		}
		if (pos1 >= 0) {
			// 逆波
			st_chkwav[fsk_spd].rev_num[st_chkwav[fsk_spd].num][1]++;
		}
		if (pos0 >= 0 || pos1 >= 0) {
			len = (int)strlen(buf0);
			st_chkwav[fsk_spd].baud_num[st_chkwav[fsk_spd].num][idx_ptn]++;
			break;
		}
	}

	if (pos0 >= 0 || pos1 >= 0) {
		if (pos0 >= pos1) {
			pos = pos0 + len;
		} else if (pos1 >= pos0) {
			pos = pos1 + len;
		}
	}
	step = pos;

	int rc = 0;
	if (pos >= 0) {
		c_data->AddReadPos(step);
		if (!c_data->IsLastData() && c_data->IsTail(32)) {
			rc = 1;
		} else if (c_data->IsLastData() && c_data->IsTail()) {
			rc = 1;
		}
	} else {
		rc = 1;
	}
	return rc;
}

/// @brief シリアルデータを搬送波ビットデータに変換する
///
/// @param[in] s_data シリアルデータ
/// @param[out] c_data 搬送波データ
/// @return 搬送波データに変換した長さ
int CarrierParser::EncodeToCarrier(SerialData *s_data, CarrierData *c_data)
{
	int idx_ptn = (param->GetBaud() & 3);
	int len = 0;
	CSampleData sample;
	const st_pattern *pattern;

	sample = s_data->GetRead();
	s_data->IncreaseReadPos();
	if (sample.Data() & 0x01) {
		// 1
		pattern = &carrier_pattern[idx_ptn][2 + baud24_frip];
	} else {
		// 0
		pattern = &carrier_pattern[idx_ptn][baud24_frip];
		if (idx_ptn == IDX_PTN_2400) {
			// 2400ボーの時 fripする
			baud24_frip = (1 - baud24_frip);
		}
	}
	len = c_data->AddString(pattern->ptn, pattern->len, sample.SPos());

	return len;
}

/// @brief L3Cデータをファイルに出力
///
/// @param[in,out] outfile ファイル
/// @param[in]     c_data  データ
/// @return        処理したデータ長さ
int CarrierParser::WriteL3CData(OutputFile &outfile, CarrierData *c_data)
{
	return WriteL3CData(outfile, c_data, 80, prev_data, prev_width);
}

/// @brief L3Cデータをファイルに出力
///
/// @param[in,out] outfile ファイル
/// @param[in]     c_data  データ
/// @param[in]     width   1行の長さ
/// @param[in,out] pdata   前のデータ
/// @param[in,out] pwidth  最終行の出力した長さ
/// @return        処理したデータ長さ
int CarrierParser::WriteL3CData(OutputFile &outfile, CarrierData *c_data, int width, uint32_t &pdata, int &pwidth)
{
	int w = pwidth;
	uint32_t prev = (pdata & 0xffffff);

	for(int l=c_data->GetStartPos(); l<c_data->GetWritePos(); l++) {
		uint8_t data = c_data->At(l).Data();
		if (w >= width
			&& (((prev & 0x010101) == 0x010001 && (data & 0x01) == 0x01)
			|| ((prev & 0x010101) == 0x000100 && (data & 0x01) == 0x00))
		) {
			// 改行
			if (over_pos > 0) {
				outfile.Fwrite(over_buf, sizeof(uint8_t), over_pos);
			}
			outfile.Fseek(-1, SEEK_CUR);
			outfile.Fwrite("\r\n", sizeof(char), 2);
			outfile.Fputc(prev & 0xff);
			w = 0;
			over_pos = 0;
		}
		// 改行が入らないまま、幅が２倍になったら強引に改行を入れる
		if ((w >= (width * 2 + 8))
		 || (w >= (width * 2) && (((prev ^ data) & 0x01) == 0x01))
		) {
			outfile.Fwrite("\r\n", sizeof(uint8_t), 2);
			outfile.Fwrite(over_buf, sizeof(uint8_t), over_pos);
			w -= width;
			over_pos = 0;
		}
		// １行の幅を超える分はいったんバッファに格納
		if (w >= width) {
			over_buf[over_pos] = data;
			over_pos++;
		} else {
			outfile.Fputc(data);
		}
		w++;

		prev <<= 8;
		prev |= data;
	}
	bool last = c_data->IsLastData();
	if (last && over_pos > 0) {
		// 最終データの場合、中途半端のデータも出力
		outfile.Fwrite(over_buf, sizeof(uint8_t), over_pos);
		over_pos = 0;
	}

	int len = c_data->Length();
	c_data->SetStartPos(c_data->GetWritePos());

	pdata = prev & 0xffffff;
	pwidth = w;

	return len;
}

#ifdef PARSEWAV_USE_REPORT
/// @brief デコード時のレポート
void CarrierParser::DecordingReport(CarrierData *c_data, wxString &buff, wxString *logbuf)
{
//	int spd = param->GetFskSpeed();

	gLogFile.SetLogBuf(logbuf);

	buff = _T(" [ l3c -> l3b, t9x ]");
	gLogFile.Write(buff, 1);

	if (c_data->GetTotalReadPos() > 0) {
		buff.Printf(_T(" %d / %d errors. (%.2f%%)"),rep2.GetErrorNum(), c_data->GetTotalReadPos(), (rep2.GetErrorNum() * 100.0 / c_data->GetTotalReadPos()));
		gLogFile.Write(buff, 1);
	}

	gLogFile.Write(_T(""), 1);
}

/// @brief エンコード時のレポート
void CarrierParser::EncordingReport(CarrierData *c_data, wxString &buff, wxString *logbuf)
{
	buff = _T(" [ l3b, t9x -> l3c ]");
	gLogFile.Write(buff, 1);
	buff.Printf(_T(" %4d Baud"),(int)c_baud_rate[param->GetBaud()] * (param->GetFskSpeed() + 1));
	gLogFile.Write(buff, 1);

	gLogFile.Write(_T(""), 1);
}
#endif

}; /* namespace PARSEWAV */
