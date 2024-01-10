/// @file paw_parseser.cpp
///
/// @author Sasaji
/// @date   2019.08.01
///

#include "paw_parseser.h"
#include "paw_file.h"
#include "paw_format.h"
#include "utils.h"


namespace PARSEWAV 
{

/// ボーレート自動解析用
/// start_bit + data_bit($ff) + 2stop_bit
static const struct st_conv_baud_tbl_ff {
	int            baud;
	int            len;
	int            mag;
	const uint8_t *ptn;
} conv_baud_tbl_ff[]={
	{  300, 72, 8, (const uint8_t *)"000000001111111111111111111111111111111111111111111111111111111111111111" },
	{  600, 36, 4, (const uint8_t *)"000011111111111111111111111111111111" },
	{ 1200, 18, 2, (const uint8_t *)"001111111111111111" },
	{ 2400,  9, 1, (const uint8_t *)"011111111" },
};

//

REPORT3::REPORT3()
{
	Clear();
}

void REPORT3::Clear()
{
	err_num = 0;
	over_err = false;
	err_pos.clear();
}

void REPORT3::AddError(int pos)
{
	err_num++;

	if (err_num > REPORT3_MAX_ERROR) {
		over_err = true;
		return;
	}

	err_pos.push_back(pos);
}

int REPORT3::GetError(int idx) const
{
	return err_pos[idx];
}

//

Phase2nBaudCount::Phase2nBaudCount()
{
	Clear();
}
void Phase2nBaudCount::Clear()
{
	Idx(-1);
	Cnt(0);
}

//

SerialParser::SerialParser()
	: ParserBase()
{
	param = NULL;
	tmp_param = NULL;
}

void SerialParser::ClearResult()
{
	rep3.Clear();
}

/// @brief デコード時の初期処理
///
void SerialParser::InitForDecode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_) 
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);

	phase2n_baud_count.Clear();

	phase3_baud = 0;

	data_pos = -1;
	parity_count = 0;

	write_pos = 0;
	over_pos = 0;

	prev_err.Clear();
}

/// @brief エンコード時の初期処理
///
void SerialParser::InitForEncode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);

	write_pos = 0;
	over_pos = 0;

	prev_err.Clear();
}

/// @brief l3bファイルのフォーマットをチェック（チェックしていないが）
///
/// @param[in] file 入力ファイル
/// @return pwOK
///
PwErrType SerialParser::CheckL3BFormat(InputFile &file)
{
	SetInputFile(file);

	CalcL3BSize(file);

	// サンプルレートはダイアログのボーレートを基準にする
	int baud_mag = param->GetFskSpeed() + 1;
	file.SampleRate(c_baud_rate[param->GetBaud()] * baud_mag);

	return pwOK;
}

/// @brief t9xファイルのフォーマットをチェック
///
/// @param[in] file 入力ファイル
/// @param[out] err_num エラー番号
/// @param[out] errinfo エラー情報
/// @return pwOK
///
PwErrType SerialParser::CheckT9XFormat(InputFile &file, PwErrCode &err_num, PwErrInfo &errinfo)
{
	SetInputFile(file);

	t9x_header_t head;

	file.Fread(&head, sizeof(t9x_header_t), 1);
	if(memcmp(head.ident,T9X_IDENTIFIER,sizeof(head.ident)) != 0) {
		// this is not t9x format
		err_num = pwErrNotT9XFormat;
		errinfo.SetInfo(__LINE__, pwError, err_num);
		errinfo.ShowMsgBox();
		return pwError;
	}

	// 実際のサンプルデータがはじまる位置の先頭
	CalcT9XSize(file);

	// サンプルレートはダイアログのボーレートを基準にする
	int baud_mag = param->GetFskSpeed() + 1;
	file.SampleRate(c_baud_rate[param->GetBaud()] * baud_mag);

	return pwOK;
}

/// @brief l3bファイルファイルのビットサイズを計算
int SerialParser::CalcL3BSize(InputFile &file)
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

/// @brief l3bファイルから１データ読んでバッファに追記
///
/// @param[in,out] s_data シリアルデータ用のバッファ(追記していく)
/// @return 読み込んだデータの長さ
///
///
int SerialParser::GetL3BSample(SerialData *s_data)
{
	int l;

	while(s_data->IsFull() != true && infile->SamplePos() < infile->SampleNum()) {
		l = infile->Fgetc();
		l &= 0xff;

		if (l == '\r' || l == '\n') {
			continue;
		}
		s_data->Add(l, infile->SamplePos());

		mile_stone->MarkIfNeed(infile->SamplePos());

		infile->IncreaseSamplePos();
	}
	if (infile->SamplePos() >= infile->SampleNum()) {
		s_data->LastData(true);
	}
	return s_data->GetWritePos();
}

/// @brief l3bファイルをスキップする
///
/// @param[in] dir
/// @return スキップ数
///
int SerialParser::SkipL3BSample(int dir)
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

/// @brief t9xファイルのビットサイズを計算
int SerialParser::CalcT9XSize(InputFile &file)
{
	int l;
	int sample_num = 0;
	int file_size = file.GetSize();
	file.Fseek(sizeof(t9x_header_t), SEEK_SET);
	file_size -= (int)sizeof(t9x_header_t);
	while(file_size > 0) {
		file_size--;

		l = file.Fgetc();
		l &= 0xff;

//		for(int i=0; i<8; i++) {
//			char c = (l & (1 << i)) ? '1' : '0';
		sample_num += 8;
//		}
	}
	file.SampleNum(sample_num);
	file.Fseek(sizeof(t9x_header_t), SEEK_SET);
	return sample_num;
}

/// @brief t9xファイルから１データ読んでバッファに追記
///
/// @param[in,out] s_data シリアルデータ用のバッファ(追記していく)
/// @return 読み込んだデータの長さ
///
///
int SerialParser::GetT9XSample(SerialData *s_data)
{
	int l;
//	int baud_mag = param->GetFskSpeed() + 1;
	long pos = (infile->SamplePos() >> 3);
	int sta = (infile->SamplePos() & 7);
	infile->Fseek(sizeof(t9x_header_t) + pos, SEEK_SET);

	while(s_data->IsFull() != true && infile->SamplePos() < infile->SampleNum()) {
		l = infile->Fgetc();
		l &= 0xff;

		mile_stone->MarkIfNeed(infile->SamplePos());

		for(int i=sta; i<8; i++) {
			char c = (l & (1 << i)) ? '1' : '0';
			s_data->Add(c, infile->SamplePos());
			infile->IncreaseSamplePos();
		}
	}
	if (infile->SamplePos() >= infile->SampleNum()) {
		s_data->LastData(true);
	}
	return s_data->GetWritePos();
}

/// @brief t9xファイルをスキップする
///
/// @param[in] dir
/// @return スキップ数
///
int SerialParser::SkipT9XSample(int dir)
{
	int dst_pos = infile->SamplePos() + dir;

	if (dst_pos >= infile->SampleNum()) {
		dir = infile->SampleNum() - 1 - infile->SamplePos();
		dst_pos = infile->SampleNum() - 1;
	} else if (dst_pos < 0) {
		dir = - infile->SamplePos();
		dst_pos = 0;
	}

	long pos = (dst_pos >> 3);
//	int sta = (dst_pos & 7);
	infile->Fseek(sizeof(t9x_header_t) + pos, SEEK_SET);

	infile->SamplePos(dst_pos);

	return dir;
}

/// @brief ボーレートを判定してシリアルデータを変換
/// 元のシリアルデータが2400ボーで解析しているのでヘッダ($FF)のデータ長をもとに
/// ボーレートを切り替える
///
/// @param[in] s_data  シリアルデータ
/// @param[out] sn_data 変換後シリアルデータ
/// @return 意味を持たない
int SerialParser::ConvertBaudRate(SerialData *s_data, SerialData *sn_data)
{
	int pos = 0;
//	uint8_t baud;
	int sn_w_pos;
	int s_r_pos;
	int idx;
	int o_idx;
	int cnt;
	Phase2nBaudCount prev_bcnt;
	char buf_s[100];

	buf_s[0] = '\0';
	prev_bcnt = phase2n_baud_count;	// for debug

	 // ボーレート自動判定
	if (tmp_param->GetAutoBaud()) {
		s_r_pos = s_data->GetReadPos();
		sn_w_pos = sn_data->GetWritePos();

		bool match = false;
		for(idx = 0; idx < 4 && !match; idx++) {
			if (s_data->CompareRead(0, conv_baud_tbl_ff[idx].ptn, conv_baud_tbl_ff[idx].len) == 0) {
				match = true;
				if (phase2n_baud_count.Idx() < 0) {
					phase2n_baud_count.Idx(idx);
					phase2n_baud_count.Cnt(0);
				}
				pos = conv_baud_tbl_ff[idx].len;
				o_idx = phase2n_baud_count.Idx();
				cnt = phase2n_baud_count.Cnt();
				if (o_idx >= 0 && o_idx != idx) {
					//　別のボーレートの分をクリア
					OutBaudConvertedData(sn_data, c_baud_min_to_s1[o_idx], conv_baud_tbl_ff[idx].mag, cnt);
					phase2n_baud_count.Idx(idx);
					phase2n_baud_count.Cnt(0);
				}

				phase2n_baud_count.IncreaseCnt();
				cnt = phase2n_baud_count.Cnt();

				// マッチした以降次のスタートビットまでのデータを保持
				memset(buf_s, '0', conv_baud_tbl_ff[idx].mag);
				buf_s[conv_baud_tbl_ff[idx].mag] = '\0';
				int fidx = s_data->FindRead(pos + conv_baud_tbl_ff[idx].mag, (const uint8_t *)buf_s, conv_baud_tbl_ff[idx].mag);
				if (fidx >= 0) {
					fidx += pos + conv_baud_tbl_ff[idx].mag;
				} else {
					fidx = pos;
				}

				phase2n_baud_postfix[cnt-1].clear();

				int flen = fidx;
				int fmax = pos + conv_baud_tbl_ff[idx].mag * 4;

				if (flen > fmax) {
					flen = fmax;
				} else if (s_data->IsTail(flen)) {
					flen = s_data->RemainLength();
				}

				for(int i=0; i<flen; i++) {
					phase2n_baud_postfix[cnt-1].push_back(s_data->GetRead(i));
				}

				pos = flen;

				if (cnt > 4) {
					// 連続していたらボーレート切り替え
					phase3_baud = c_baud_min_to_s1[idx];
					OutBaudConvertedData(sn_data, phase3_baud, conv_baud_tbl_ff[idx].mag, cnt);
					phase2n_baud_count.Clear();
				}
			}
		}
		if (!match) {
			// クリア
			idx = phase2n_baud_count.Idx();
			cnt = phase2n_baud_count.Cnt();
			if (idx >= 0) {
				// 候補のボーレートと実際のボーレートの差を計算
				int n_idx;
				int n_mag = 1;
				switch(phase3_baud) {
				case 3:
					// 300
					n_idx = idx + 3;
					break;
				default:
					n_idx = idx + (2 - phase3_baud);
					break;
				}
				if (n_idx >= 0 && n_idx < 4) {
					n_mag = conv_baud_tbl_ff[idx].mag / conv_baud_tbl_ff[n_idx].mag;
					OutBaudConvertedData(sn_data, phase3_baud, n_mag, cnt);
				}
			}
			phase2n_baud_count.Clear();
		}
		// $FF以外の値
		if (pos == 0) {
			idx = c_baud_s1_to_min[phase3_baud];
			// 300ボーの場合は8バイトごと
			// 600ボーの場合は4バイトごと
			// 1200ボーの場合は2バイトごと
			// 2400ボーの場合は1バイトごと
			cnt = 1 << (3 - idx);
			CSampleData d = s_data->GetRead();
			d.Baud(phase3_baud);
			d.SnSta(1);
			if (s_data->SameAsRead(0, cnt, s_data->GetRead())) {
				sn_data->Add(d);
				pos = cnt;
			} else {
				// 一致しない
				if (process_mode == PROCESS_VIEWING) {
					d.Data('?');
					d.Err(0x8);
					sn_data->Add(d);
				}
				pos = 1;
			}
		}

		s_data->AddReadPos(pos);

		if (tmp_param->GetDebugMode() > 0) {
			s_r_pos = s_data->GetReadPos() - s_r_pos;
			sn_w_pos = sn_data->GetWritePos() - sn_w_pos;

			int df = s_data->Compare(s_data->GetReadPos() - s_r_pos, *sn_data, sn_data->GetWritePos() - sn_w_pos, sn_w_pos);

			CSampleString str(*sn_data, sn_data->GetWritePos() - sn_w_pos, sn_w_pos);
			gLogFile.Fprintf("p2n cnv b:%d cnt:%04x df:%2d s:%2d:%s -> sn:%2d:%s\n"
				,phase3_baud, (int)prev_bcnt.Cnt(), df, s_r_pos
				, buf_s
				, sn_w_pos
				, str.Get()
				);
		}

	} else {
		// go through
		CSampleData d = s_data->GetRead();
		d.Baud(param->GetBaud());
		d.SnSta(1);
		sn_data->Add(d);
		s_data->IncreaseReadPos();

	}

	int rc = 0;
	if (sn_data->IsFull(131)) {
		rc = 1;
	} else if (s_data->IsLastData() && s_data->IsTail()) {
		sn_data->LastData(true);
		rc = 1;
	} else if (!s_data->IsLastData() && s_data->IsTail(32)) {
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
	return rc;
}

/// @brief ボーレート変換でたまったデータを出力
void SerialParser::OutBaudConvertedData(SerialData *sn_data, int8_t baud, int mag, int cnt)
{
	uint8_t sn_sta = 1;
	for(int i=0; i<cnt; i++) {
		for(int j=0; j<(int)phase2n_baud_postfix[i].size(); j+=mag) {
			CSampleData d = phase2n_baud_postfix[i].at(j);
			d.Baud(baud);
			d.SnSta(sn_sta);
			sn_data->Add(d);
			sn_sta = 0;
		}
	}
}

/// @brief シリアルデータをバイナリデータに変換する
///
/// @param[in] s_data  シリアルデータ
/// @param[out] b_data バイナリデータ
/// @return 次の位置
int SerialParser::Decode(SerialData *s_data, BinaryData *b_data)
{
	int rc = 0;
	if (data_pos < 0) {
		FindStartSerialBit(s_data, b_data);
	} else {
		DecodeToBinary(s_data, b_data);
	}
	bool s_last = (s_data->IsLastData() && s_data->IsTail());
	b_data->LastData(s_last);

	if (b_data->IsFull() || s_last) {
		rc = 1;
	} else if (!s_data->IsLastData() && s_data->IsTail(32)) {
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
	return rc;
}

/// @brief スタートビット（と思われる）部分を探す
///
/// @param[in] s_data シリアルデータ
/// @param[in] b_data バイナリデータ
/// @return データ位置
int SerialParser::FindStartSerialBit(SerialData *s_data, BinaryData *b_data)
{
	start_data = s_data->GetRead();

	if ((start_data.Data() & 1) == 0 && start_data.Data() != '?') {
		data_pos = 0;

		/// データビット数
		bit_len = param->GetWordDataBitLen();
		/// -1:パリティなし 0:偶数パリティ 1:奇数パリティ 
		bit_parity = param->GetWordParityBit();
		/// ストップビット数
		bit_stop = param->GetWordStopBitLen();

		if (start_data.SnSta()) {
			mile_stone->ModifyMarkIfNeed(start_data.SPos(), start_data.Baud(), start_data.CPhase(), start_data.CFrip(), start_data.SnSta(), data_pos);
		}

	} else {
		data_pos = -1;

		if (tmp_param->GetDebugMode() > 1) {
			gLogFile.Fprintf("p3 s:% 8d %s: skip [%c]\n"
				, s_data->GetTotalReadPos()
				, UTILS::get_time_cstr(infile->CalcrateSampleUSec(s_data->GetRead().SPos()))
				, s_data->GetRead().Data());
		}

		if (start_data.SnSta()) {
			mile_stone->ModifyMarkIfNeed(start_data.SPos(), start_data.Baud(), start_data.CPhase(), start_data.CFrip(), start_data.SnSta(), data_pos);
		}

		if (process_mode == PROCESS_VIEWING) {
			start_data.Data(0);
			start_data.Err(0x9);
			b_data->Add(start_data);
		}
		s_data->IncreaseReadPos();
	}

	return data_pos;
}

/// @brief シリアルデータをバイナリデータに変換する
///
/// @param[in] s_data  シリアルデータ
/// @param[out] b_data バイナリデータ
/// @return 次の位置
int SerialParser::DecodeToBinary(SerialData *s_data, BinaryData *b_data)
{
	bool data_end = false;

	CSampleData d = s_data->GetRead();

	// skip
	if (d.Data() == '?') {
		s_data->IncreaseReadPos();
		return 1;
	}

	if (data_pos == 0) {
		// start bit
		bin_data = 0;
		bin_err  = 0;
		parity_count = 0;
		data_pos++;
	} else if (data_pos < bit_len + 1) {
		// data bit
		bin_data |= (d.Data() & 1) << (data_pos - 1);
		parity_count += (d.Data() & 1);
		data_pos++;
	} else if (bit_parity >= 0 && data_pos == bit_len + 1) {
		// parity bit
		if (bit_parity & 0x01) {
			// odd parity
			if (d.Data() == (parity_count & 0x01)) bin_err |= 0xc;		// error
		} else {
			// even parity
			if (d.Data() == (1 - (parity_count & 0x01))) bin_err |= 0xc;	// error
		}
		data_pos++;
	} else if (data_pos == bit_len + 1 + (bit_parity >= 0 ? 1 : 0)) {
		// stop bit 1
		if ((d.Data() & 1) == 0) bin_err |= 0xa;		// error

		if (bit_stop == 2 && (s_data->GetRead(1).Data() & 1) != 0) {
			// 2 stop bit
			data_pos++;
		} else {
			data_end = true;
			data_pos = -1;
		}
	} else if (data_pos == bit_len + bit_stop + (bit_parity >= 0 ? 1 : 0)) {
		// stop bit 2
		// no error check
		data_end = true;
		data_pos = -1;
	}

	// データ追加
	if (data_end) {
		int8_t baud = 0;
		if (infile->GetType() <= FILETYPE_L3C) {
			// baud select
			baud = tmp_param->GetAutoBaud() ? s_data->GetRead().Baud() : param->GetBaud();
		}

		if (bin_err == 0) {
			// OK
			b_data->Add(bin_data & 0xff, start_data.SPos(), baud);

			prev_err.Clear();
		} else {
			// エラーの場合
			if (tmp_param->GetDebugMode() > 0) {
				// デバッグログ
				gLogFile.Fprintf("p3 s:%12d(%s) %s error.\n"
					, start_data.SPos()
					, UTILS::get_time_cstr(infile->CalcrateSampleUSec(start_data.SPos()))
					, (bin_err & 0xc0) == 0xc0 ? "parity" : "frame"
				);
			}

			if (prev_err.SPos() == 0) {
				// エラーが連続していなければエラー情報を追加
				rep3.AddError(start_data.SPos());
			}
			prev_err.Data(bin_data & 0xff);
			prev_err.SPos(start_data.SPos());
			prev_err.Baud(baud);
			prev_err.Err(bin_err);

			if (process_mode == PROCESS_VIEWING || param->GetOutErrSerial()) {
				b_data->Add(bin_data & 0xff, start_data.SPos(), baud, bin_err);
			}

		}
	}
	s_data->IncreaseReadPos();

	return data_pos;
}

/// @brief バイナリデータをシリアルデータに変換する
///
/// @param[in] bin_data バイナリデータ1バイト
/// @param[out] s_data  シリアルデータ
/// @return シリアルデータに変換した長さ
int SerialParser::EncodeToSerial(uint8_t bin_data, SerialData *s_data)
{
	int pos = 0;
	int h_count = 0;	// H bitcount (use for parity check)
	uint8_t val;

	// start bit
	val = '0';
	s_data->Add(val, 0);

	// data bit
	for(pos = 0; pos < 7 + ((param->GetWordSelect() & 0x04) ? 1 : 0); pos++) {
		val = (bin_data & (1 << pos)) ? '1' : '0';
		s_data->Add(val, 0);
		h_count += (val == '1') ? 1 : 0;
	}
	pos++;

	// parity bit
	if ((param->GetWordSelect() & 0x07) != 0x04 && (param->GetWordSelect() & 0x07) != 0x05) {
		if (param->GetWordSelect() & 0x01) {
			// odd parity
			val = (h_count & 0x01) ? '1' : '0';
		} else {
			// even parity
			val = (h_count & 0x01) ? '0' : '1';
		}
		s_data->Add(val, 0);
		pos++;
	}

	// stop bit
	val = '1';
	s_data->Add(val, 0);
	pos++;
	if ((param->GetWordSelect() & 0x07) == 0x04 || (param->GetWordSelect() & 0x06) == 0x00) {
		// 2 stop bit
		s_data->Add(val, 0);
		pos++;
	}

	return pos;
}

/// @brief L3Bデータをファイルに出力
///
/// @param[in,out] outfile ファイル
/// @param[in]     s_data  シリアルデータ
/// @return        データ長さ
int SerialParser::WriteL3BData(OutputFile &outfile, SerialData *s_data)
{
	return WriteL3BData(outfile, s_data, 110, write_pos);
}

/// @brief データをファイルに出力
///
/// @param[in,out] outfile ファイル
/// @param[in]     s_data  シリアルデータ
/// @param[in]     width   1行の長さ
/// @param[in,out] redata  最終行の出力した長さ + 前のデータ
/// @return        データ長さ
int SerialParser::WriteL3BData(OutputFile &outfile, SerialData *s_data, int width, int &redata)
{
	int w = (redata >> 8);
	uint8_t prev = (redata & 0xff);

	for(int l=s_data->GetStartPos(); l<s_data->GetWritePos(); l++) {
		uint8_t data = s_data->At(l).Data();
		if (w >= width && prev == '1' && data == '0') {
			// 改行
			if (over_pos > 0) {
				outfile.Fwrite(over_buf, sizeof(uint8_t), over_pos);
			}
			outfile.Fwrite("\r\n", sizeof(uint8_t), 2);
			w = 0;
			over_pos = 0;
		}
		// 改行が入らないまま、幅が２倍になったら強引に改行を入れる
		if (w >= (width * 2)) {
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
		prev = data;
	}
	bool last = s_data->IsLastData();
	if (last && over_pos > 0) {
		// 最終データの場合、中途半端のデータも出力
		outfile.Fwrite(over_buf, sizeof(uint8_t), over_pos);
		over_pos = 0;
	}

	int len = s_data->Length();
	s_data->SetStartPos(s_data->GetWritePos());

	redata = (w << 8) | (prev);

	return len;
}

/// @brief T9Xデータをファイルに出力
///
/// @param[in,out] outfile ファイル
/// @param[in]     s_data  シリアルデータ
int SerialParser::WriteT9XData(OutputFile &outfile, SerialData *s_data)
{
	return WriteT9XData(outfile, s_data, write_pos);
}

/// @brief T9Xデータをファイルに出力
///
/// @param[in,out] outfile ファイル
/// @param[in]     s_data  シリアルデータ
/// @param[in]     redata  余りデータ+位置  
/// @return                長さ
int SerialParser::WriteT9XData(OutputFile &outfile, SerialData *s_data, int &redata)
{
	int w = (redata & 7);
	int c = (redata >> 4);
	
	for(int l=s_data->GetStartPos(); l<s_data->GetWritePos(); l++) {
		uint8_t data = s_data->At(l).Data();
		c |= (data & 1) ? (1 << w) : 0;
		w++;
		if (w >= 8) {
			outfile.Fputc(c);
			c = 0;
			w = 0;
		}
	}
	bool last = s_data->IsLastData();
	if (last && w > 0) {
		// 最終データの場合、中途半端のデータも出力
		outfile.Fputc(c);
	}

	int len = s_data->Length();
	s_data->SetStartPos(s_data->GetWritePos());

	redata = ((c << 4) | (w & 7));

	return len;
}

/// @brief デコード時のレポート
void SerialParser::DecordingReport(SerialData *s_data, wxString &buff, wxString *logbuf)
{
//	int spd = param->GetFskSpeed();

	gLogFile.SetLogBuf(logbuf);

	buff = _T(" [ l3b, t9x -> l3 ]");
	gLogFile.Write(buff, 1);

	if (param->GetWordDataBitLen() == 7) {
		// 7bit
		buff = _T(" 7bits");
	} else {
		// 8bit
		buff = _T(" 8bits");
	}

	switch(param->GetWordParityBit()) {
	case 1:
		buff += _T(" OddParity");
		break;
	case 2:
		buff += _T(" EvenParity");
		break;
	default:
		buff += _T(" NoParity");
		break;
	}
	if (param->GetWordStopBitLen() == 1) {
		buff += _T(" 1stopbit");
	} else {
		buff += _T(" 2stopbit");
	}
	gLogFile.Write(buff, 1);

	if (s_data->GetTotalReadPos() > 0) {
		buff.Printf(_T(" %d / %d errors. (%.2f%%)"),rep3.GetErrorNum(), s_data->GetTotalReadPos(), (rep3.GetErrorNum() * 100.0 / s_data->GetTotalReadPos()));
		gLogFile.Write(buff, 1);
		int col_max = 5;
		int col = 0;
		buff.Empty();
		for(int i=0; i<rep3.GetErrorCount(); i++) {
			int start_pos = rep3.GetError(i);
			buff += (col == 0 ? _T("  ") : _T(", "));
			buff += wxString::Format(_T("%d (")
				,start_pos);
			buff += UTILS::get_time_str(infile->CalcrateSampleUSec(start_pos));
			buff += _T(")");
			col++;
			if (col == col_max) {
				gLogFile.Write(buff, 1);
				col = 0;
				buff.Empty();
			}
		}
		for(int i=0; i<(rep3.IsOverError() ? 1 : 0); i++) {
			buff += _T("  and more...");
			col++;
			if (col == col_max) {
				gLogFile.Write(buff, 1);
				col = 0;
				buff.Empty();
			}
		}
		if (col > 0) {
			gLogFile.Write(buff, 1);
		}
	}

	gLogFile.Write(_T(""), 1);
}

/// @brief エンコード時のレポート
void SerialParser::EncordingReport(SerialData *s_data, wxString &buff, wxString *logbuf)
{
	if (infile->GetType() == FILETYPE_PLAIN) {
		buff = _T(" [ plain");
	} else {
		buff = _T(" [ l3");
	}
	buff += _T(" -> l3b, t9x ]");
	gLogFile.Write(buff, 1);

	if (param->GetWordDataBitLen() == 7) {
		// 7bit
		buff = _T(" 7bits");
	} else {
		// 8bit
		buff = _T(" 8bits");
	}

	switch(param->GetWordParityBit()) {
	case 1:
		buff += _T(" OddParity");
		break;
	case 2:
		buff += _T(" EvenParity");
		break;
	default:
		buff += _T(" NoParity");
		break;
	}
	if (param->GetWordStopBitLen() == 1) {
		buff += _T(" 1stopbit");
	} else {
		buff += _T(" 2stopbit");
	}
	gLogFile.Write(buff, 1);

	gLogFile.Write(_T(""), 1);
}

}; /* namespace PARSEWAV */
