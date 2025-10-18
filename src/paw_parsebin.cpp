/// @file paw_parsebin.cpp
///
/// @brief バイナリデータ解析
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2019.08.01
///
#include "paw_parsebin.h"
#include "paw_file.h"
#include "utils.h"


namespace PARSEWAV 
{

/// L3/S1 BASIC形式のヘッダ
static const uint8_t cmt_ident[4] = { 0xff, 0x01, 0x3c, 0x00 };

//

REPORT4::REPORT4()
{
	Clear();
}

REPORT4::~REPORT4()
{
}

void REPORT4::Clear()
{
	memset(save_data_name, 0, sizeof(save_data_name));
	flags = 0;
	baud = 0;
	data_count = 0;
	chksum_err_num = 0;
	chksum_err_pos.clear();
}

void REPORT4::AddChksumError(int start_pos, int end_pos)
{
	pos_t pos;

	pos.start_pos = start_pos;
	pos.end_pos = end_pos;
	chksum_err_pos.push_back(pos);

	chksum_err_num++;
}

void REPORT4::SetSaveDataName(const uint8_t *name, int len)
{
	memcpy(save_data_name, name, len);
}

void REPORT4::GetChksumError(int idx, int &start_pos, int &end_pos) const
{
	start_pos = chksum_err_pos[idx].start_pos;
	end_pos = chksum_err_pos[idx].end_pos;
}

//

BinaryParser::BinaryParser()
	: ParserBase()
{
	header_type = -1;
	memset(save_data_name, 0, sizeof(save_data_name));

	rep4_itm = NULL;
}

BinaryParser::~BinaryParser()
{
	ClearResult();
}

void BinaryParser::ClearResult()
{
	std::vector<REPORT4 *>::iterator itm = rep4.begin();
	while ( itm != rep4.end() ) {
		delete *itm;
		itm++;
	}
	rep4.clear();

//	delete rep4_itm;
	rep4_itm = NULL;
}

/// @brief デコード時の初期処理
///
void BinaryParser::InitForDecode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);

	memset(save_data_name, 0, sizeof(save_data_name));
	save_data_name[8]=0x7f;
	save_data_name[9]=0x7f;
}

/// @brief エンコード時の初期処理
///
void BinaryParser::InitForEncode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_)
{
	ParserBase::Init(process_mode_, tmp_param_, mile_stone_);
}

/// @brief l3ファイルのフォーマットをチェック（チェックしていないが）
///
/// @param[in] file 入力ファイル
/// @return pwOK
///
PwErrType BinaryParser::CheckL3Format(InputFile &file)
{
	SetInputFile(file);

	file.SampleNum(file.GetSize());

	// サンプルレートはダイアログのボーレートを基準にする
	int baud_mag = param->GetFskSpeed() + 1;
	int bit_len = param->GetWordAllBitLen();
	file.SampleRate((double)c_baud_rate[param->GetBaud()] * baud_mag / bit_len);

	return pwOK;
}

/// @brief l3ファイルからデータ読んでバッファに追記
///
/// @param[in,out] b_data バイナリデータ用のバッファ(追記していく)
/// @return 読み込んだデータの長さ
///
int BinaryParser::GetL3Sample(BinaryData *b_data)
{
	int l;
//	int baud_mag = param->GetFskSpeed() + 1;

	while(b_data->IsFull() != true && infile->SamplePos() < infile->SampleNum()) {
		l = infile->Fgetc();
		l &= 0xff;

//		file.CalcrateSampleUSec(c_baud_rate[param->GetBaud()] * baud_mag / 11);
		b_data->Add(l, infile->SamplePos());
		infile->IncreaseSamplePos();
	}
	if (infile->SamplePos() >= infile->SampleNum()) {
		b_data->LastData(true);
	}
	return b_data->GetWritePos();
}

/// @brief l3ファイルから１データ読む
///
/// @return １バイトデータ
///
///
uint8_t BinaryParser::GetL3Sample()
{
	int l;

	l = infile->Fgetc();

	infile->IncreaseSamplePos();

	return (l & 0xff);
}

/// @brief BASIC形式のヘッダをさがす
///
/// @return -1:見つからない 0:name 1:body 0xff:footer
int BinaryParser::FindHeader(BinaryData *b_data)
{
	int rc = -1;
	// 0xff 0x01 0x3c を探す
	if(b_data->CompareRead(0, cmt_ident, 3) == 0) {
		// found
		if (rep4_itm == NULL) {
			rep4_itm = new REPORT4();
			rep4.push_back(rep4_itm);
		}

		// type?
		rc = b_data->GetRead(3).Data();
		if (rc != 0 && rc != 1 && rc != 0xff) {
			// ?? 謎のヘッダ
			rc = -1;
		}
	}
	header_type = rc;
	return rc;
}

/// @brief 名前セクション解析
///
/// @param[in]     b_data    バイナリデータ
/// @param[in,out] outfile   出力ファイル
/// @param[in]     outsfileb 分割時の出力ファイル名
/// @param[in]     outsext   分割時の出力ファイル拡張子
/// @param[in,out] outsfile  分割時の出力ファイル
/// @return -1:フラッシュが必要 0>サイズ
int BinaryParser::ParseNameSection(BinaryData *b_data, OutputFile &outfile, const wxString &outsfileb, const wxString &outsext, OutputFile &outsfile)
{
	int rc = 0;
	int data_len = 0;
	CSampleData sample;

	CSampleData start;

	int chk_sum_calc = 0;
	int chk_sum_data = 0;


	rep4_itm->OrFlags(1);
	rep4_itm->SetBaud(b_data->GetRead().Baud());
	data_len = b_data->GetRead().Data();
	if (!b_data->IsLastData() && b_data->IsTail(data_len + 6)) {
		// バッファをフラッシュ
		rc = -1;
		return rc;
	}
	sample = b_data->GetRead();
	b_data->IncreaseReadPos();

	start = b_data->GetRead();
	chk_sum_calc = 0;

	chk_sum_calc += data_len;
	for(int i=0; i<data_len && i<20; i++) {
		sample = b_data->GetRead();
		b_data->IncreaseReadPos();
		save_data_name[i] = sample.Data();
		chk_sum_calc += sample.Data();
	}
	save_data_name[20]='\0';

	rep4_itm->SetSaveDataName(save_data_name, 21);

	sample = b_data->GetRead();
	b_data->IncreaseReadPos();
	chk_sum_data = (int)sample.Data();

	chk_sum_calc = (chk_sum_calc & 0xff);

	if (chk_sum_calc != chk_sum_data) {
		// レポート用
		rep4_itm->AddChksumError(start.SPos(), sample.SPos());
	}

	// 実ファイルを分割して出力する時 open file
	if (outfile.GetType() == FILETYPE_REAL && outfile.GetType() >= infile->GetType()) {
		if (param->GetFileSplit()) {
			wxString outsfilen = outsfileb;
			outsfilen += wxString::Format(_T("_%03d"), (int)rep4.size());
			outsfilen += outsext;
			outsfile.Fopen(outsfilen, File::WRITE_BINARY);
		}
		prev_data_len = 0;
		memset(prev_data_body, 0, sizeof(prev_data_body));
	}

	return (int)rep4.size() + 1;
}

/// @brief ボディ（データ）セクション解析
///
/// @param[in]     b_data    バイナリデータ
/// @param[in,out] outfile   出力ファイル
/// @param[in,out] outsfile  分割時の出力ファイル
/// @return -1:フラッシュが必要 0
int BinaryParser::ParseBodySection(BinaryData *b_data, OutputFile &outfile, OutputFile &outsfile)
{
	int rc = 0;
	int data_len = 0;
	CSampleData sample;

	CSampleData start;

	int chk_sum_calc = 0;
	int chk_sum_data = 0;


	uint8_t data_body[257];
	uint8_t *pdata;


	rep4_itm->OrFlags(2);
	data_len = b_data->GetRead().Data();
	if (!b_data->IsLastData() && b_data->IsTail(data_len + 6)) {
		// バッファをフラッシュ
		rc = -1;
		return rc;
	}
	sample = b_data->GetRead();
	b_data->IncreaseReadPos();

	start = b_data->GetRead();
	chk_sum_calc = 1;

	rep4_itm->IncDataCount();

	chk_sum_calc += data_len;
	for(int i=0; i<data_len && i<255; i++) {
		sample = b_data->GetRead();
		b_data->IncreaseReadPos();
		data_body[i] = sample.Data();
		chk_sum_calc += sample.Data();
	}
	data_body[255]='\0';

	sample = b_data->GetRead();
	b_data->IncreaseReadPos();
	chk_sum_data = (int)sample.Data();

	chk_sum_calc = (chk_sum_calc & 0xff);

	if (chk_sum_calc != chk_sum_data) {
		// レポート用
		rep4_itm->AddChksumError(start.SPos(), sample.SPos());
	}

	// 実ファイルを分割して出力する時 write to file
	if (outfile.GetType() == FILETYPE_REAL && outfile.GetType() >= infile->GetType()) {
		// マシン語のヘッダを取り除く場合
		if (param->GetDeleteMHead() && rep4_itm->GetSaveDataName(8) == 2 && rep4_itm->GetDataCount() == 1) {
			pdata = &data_body[5];
			data_len -= 5;
		} else {
			pdata = data_body;
		}

		if (prev_data_len > 0) {
			if (!param->GetFileSplit()) {
				outfile.Fwrite(prev_data_body, sizeof(uint8_t), prev_data_len);
			} else {
				outsfile.Fwrite(prev_data_body, sizeof(uint8_t), prev_data_len);
			}
		}
		prev_data_len = data_len;
		memcpy(prev_data_body, pdata, data_len);
	}

	return rc;
}

/// @brief フッタセクション解析
///
/// @param[in]     b_data    バイナリデータ
/// @param[in,out] outfile   出力ファイル
/// @param[in,out] outsfile  分割時の出力ファイル
/// @return -1:フラッシュが必要 0
int BinaryParser::ParseFooterSection(BinaryData *b_data, OutputFile &outfile, OutputFile &outsfile)
{
	int rc = 0;
	int data_len = 0;
	CSampleData sample;

	CSampleData start;

	int chk_sum_calc = 0;
	int chk_sum_data = 0;


	rep4_itm->OrFlags(4);
	data_len = b_data->GetRead().Data();
	if (!b_data->IsLastData() && b_data->IsTail(data_len + 6)) {
		// バッファをフラッシュ
		rc = -1;
		return rc;
	}
	sample = b_data->GetRead();
	b_data->IncreaseReadPos();

	start = b_data->GetRead();
	chk_sum_calc = 255;

	chk_sum_calc += data_len;
	for(int i=0; i<data_len && i<255; i++) {
		sample = b_data->GetRead();
		b_data->IncreaseReadPos();
		chk_sum_calc += sample.Data();
	}
	sample = b_data->GetRead();
	b_data->IncreaseReadPos();
	chk_sum_data = (int)sample.Data();

	chk_sum_calc = (chk_sum_calc & 0xff);

	if (chk_sum_calc != chk_sum_data) {
		// レポート用
		rep4_itm->AddChksumError(start.SPos(), sample.SPos());
	}

	// write file
	if (outfile.GetType() == FILETYPE_REAL && outfile.GetType() >= infile->GetType()) {
		// マシン語のヘッダを取り除く場合
		if (param->GetDeleteMHead() && rep4_itm->GetSaveDataName(8) == 2) {
			prev_data_len -= 5;
		}
		if (prev_data_len > 0) {
			if (!param->GetFileSplit()) {
				outfile.Fwrite(prev_data_body, sizeof(uint8_t), prev_data_len);
			} else {
				outsfile.Fwrite(prev_data_body, sizeof(uint8_t), prev_data_len);
			}
		}
		if (param->GetFileSplit()) {
			// close file
			outsfile.Fclose();
//			// 一旦バッファをフラッシュする。
//			rc = -2;
		}
	}

	rep4_itm = NULL;
//	FlushResult();

	return rc;
}

/// @brief 名前セクション出力
///
/// @param[in]     b_data    バイナリデータ
/// @param[in]     gap_len   ギャップの長さ
int BinaryParser::PutHeaderSection(BinaryData *b_data, int gap_len)
{
	int chk_sum = 0;

	b_data->Repeat(0xff, gap_len, 0);
	b_data->AddString(&cmt_ident[1], 2, 0);
	// header_type
	b_data->Add(0, 0);
	chk_sum += 0;
	// 20バイト
	int dlen = 20;
	b_data->Add(dlen, 0);
	chk_sum += dlen;
	for(int i=0; i<dlen; i++) {
		uint8_t data = save_data_name[i];
		b_data->Add(data, 0);
		chk_sum += data;
	}
	// chk sum
	b_data->Add((chk_sum & 0xff), 0);
	// 0 x4
	b_data->Repeat(0, 4, 0);

	return 0;
}

/// @brief ボディセクション出力
///
/// @param[in]     b_data    バイナリデータ
/// @param[in]     gap_len   ギャップの長さ
int BinaryParser::PutBodySection(BinaryData *b_data, int gap_len)
{
	int chk_sum = 0;
	int data_size = prev_data_len;

	b_data->Repeat(0xff, gap_len, 0);
	b_data->AddString(&cmt_ident[1], 2, 0);
	// header_type
	b_data->Add(1, 0);
	chk_sum += 1;
	// 255バイト最大
	if (data_size >= 255) {
		data_size = 255;
	}
	b_data->Add(data_size, 0);
	chk_sum += data_size;
	for(int i=0; i<data_size; i++) {
		uint8_t data = prev_data_body[i];
		b_data->Add(data, 0);
		chk_sum += data;
	}
	// chk sum
	b_data->Add((chk_sum & 0xff), 0);
	// 0 x 4
	b_data->Repeat(0, 4, 0);

	// binary file is 1
	return (save_data_name[9] == 0) ? 1 :0;
}

/// @brief フッタセクション出力
///
/// @param[in]     b_data    バイナリデータ
/// @param[in]     gap_len   ギャップの長さ
int BinaryParser::PutFooterSection(BinaryData *b_data, int gap_len)
{
	int chk_sum = 0;

	b_data->Repeat(0xff, gap_len, 0);
	b_data->AddString(&cmt_ident[1], 2, 0);
	// header_type
	b_data->Add(0xff, 0);
	chk_sum += 0xff;
	//
	int dlen = 0;
	b_data->Add(dlen, 0);
	chk_sum += dlen;
	// chk sum
	b_data->Add((chk_sum & 0xff), 0);
	// 0 x 4
	b_data->Repeat(0, 4, 0);

	return 0;
}

#if 0
void BinaryParser::FlushResult()
{
	if (rep4_itm != NULL) {
		rep4.push_back(rep4_itm);
		rep4_itm = NULL;
	}
}
#endif

/// @brief セーブデータ情報（名前、ファイル種類）を設定
///
/// @param[in]     name    名前
/// @param[in]     len     名前長さ
/// @param[in]     fmt     ファイル種類
/// @param[in]     type    ファイル種類２
void BinaryParser::SetSaveDataInfo(const uint8_t *name, int len, uint8_t fmt, uint8_t type)
{
	memset(save_data_name, 0, sizeof(save_data_name));
	for(int i=0; i<len && i<8; i++) {
		save_data_name[i] = name[i];
	}
	save_data_name[8]  = fmt;
	save_data_name[9]  = type;
	save_data_name[10] = type;
}

/// @brief 事前データクリア
void BinaryParser::ClearPrevData()
{
	prev_data_len = 0;
}

/// @brief 事前データにセット
///
/// @param[in]     data    1バイトデータ
void BinaryParser::AddPrevData(uint8_t data)
{
	prev_data_body[prev_data_len] = data;
	prev_data_len++;
}

/// @brief L3データ出力
///
/// @param[in] outfile 出力ファイル
/// @param[in] b_data  バイナリデータ
int BinaryParser::WriteL3Data(OutputFile &outfile, BinaryData *b_data)
{
	return outfile.WriteData(*b_data);
}

/// @brief デコード時のレポート
void BinaryParser::DecordingReport(BinaryData *b_data, wxString &buff, wxString *logbuf)
{
	int spd = param->GetFskSpeed();

	gLogFile.SetLogBuf(logbuf);

	buff = _T(" [ l3 -> real data ]");
	gLogFile.Write(buff, 1);

	if (rep4.empty()) {
		buff = _T(" data cannot parse.");
		gLogFile.Write(buff, 1);
	} else {
		int idx = 1;
		std::vector<REPORT4 *>::iterator itm = rep4.begin();
		while ( itm != rep4.end() ) {
			buff.Printf(_T("%03d:"), idx);
			gLogFile.Write(buff, 1);
			if ((*itm)->GetFlags() & 1) {
				buff = _T(" dataname: \"");
				buff += UTILS::conv_internal_name((*itm)->GetSaveDataName());
				buff += _T("\"   [");
				switch((*itm)->GetSaveDataName(8)) {
					case 0:
						buff += _T("BASIC");
						break;
					case 1:
						buff += _T("DATA");
						break;
					case 2:
						buff += _T("Machine");
						break;
					default:
						buff += _T("?");
						break;
				}
				switch((*itm)->GetSaveDataName(9)) {
					case 0:
						buff += _T(" - Binary save");
						break;
					case 0xff:
						buff += _T(" - Ascii save");
						break;
					default:
						buff += _T(" ?");
						break;
				}
				buff += _T("]");

				int idx = ((*itm)->GetBaud());
				if (idx >= 0 && idx < 4) {
					buff += wxString::Format(_T(" (%4dbaud)"), (int)c_baud_rate[idx] * (spd + 1));
				} else {
					buff += _T(" (----baud)");
				}
				gLogFile.Write(buff, 1);
			} else {
				buff = _T(" no filename");
				gLogFile.Write(buff, 1);
				buff = _T(" header section not found.");
				gLogFile.Write(buff, 1);
			}
			if (((*itm)->GetFlags() & 2) == 0) {
				buff = _T(" data section not found.");
				gLogFile.Write(buff, 1);
			}
			if (((*itm)->GetFlags() & 4) == 0) {
				buff = _T(" footer section not found.");
				gLogFile.Write(buff, 1);
			}

			if ((*itm)->GetChksumErrorNum() <= 0) {
				buff = _T(" check sum ok.");
			} else {
				buff = _T(" check sum error exists.");
			}
			gLogFile.Write(buff, 1);
			for(int i=0; i<(*itm)->GetChksumErrorNum(); i++) {
				int start_pos, end_pos;
				(*itm)->GetChksumError(i, start_pos, end_pos);
				buff.Printf(_T(" pos: %d-%d (")
					,start_pos, end_pos);
				buff += UTILS::get_time_str(infile->CalcrateSampleUSec(start_pos));
				buff += _T("-");
				buff += UTILS::get_time_str(infile->CalcrateSampleUSec(end_pos));
				buff += _T(")");
				gLogFile.Write(buff, 1);
			}

			itm++;
			idx++;
		}
	}

	gLogFile.Write(_T(""), 1);
}

}; /* namespace PARSEWAV */
