/// @file paw_parseser.h
///
/// @brief シリアルデータ解析
///
/// @author Copyright (c) Sasaji. All rights reserved.
/// @date   2019.08.01
///
#ifndef _PARSEWAV_PARSESER_H_
#define _PARSEWAV_PARSESER_H_

#include "common.h"
#include <stdio.h>
#include <vector>
#include <wx/string.h>
#include "paw_parse.h"
#include "errorinfo.h"
#include "paw_defs.h"
#include "paw_datas.h"
#include "paw_param.h"
#include "paw_file.h"


namespace PARSEWAV 
{

/// ボーレート自動変換用
class Phase2nBaudCount
{
private:
	int16_t idx;
	int16_t cnt;
public:
	Phase2nBaudCount();
	void Clear();
	void IncreaseCnt() { cnt++; }
	void Idx(int16_t val) { idx = val; }
	void Cnt(int16_t val) { cnt = val; }
	int16_t Idx() const { return idx; }
	int16_t Cnt() const { return cnt; }
};

#define REPORT3_MAX_ERROR 50

/// @brief 結果レポート用クラス
class REPORT3
{
public:
	REPORT3();
	~REPORT3() {}

	int err_num;
	bool over_err;
	std::vector<int> err_pos;

	void Clear();
	void AddError(int pos);

	int GetErrorNum() const { return err_num; }
	int GetErrorCount() const { return (int)err_pos.size(); }
	bool IsOverError() const { return over_err; }
	int GetError(int idx) const;
};

/// シリアルデータ解析用クラス
class SerialParser : public ParserBase
{
private:
	/// レポート用
	REPORT3 rep3;

	/// ボーレート自動変換用
	Phase2nBaudCount phase2n_baud_count;
	CSampleList phase2n_baud_postfix[6];

	int8_t phase3_baud;

	/// シリアルデータモード
	int bit_len;	///< データビット数
	int bit_parity; ///< 0:パリティなし 1:奇数パリティ 2:偶数パリティ
	int bit_stop;	///< ストップビット数

	/// バイナリ変換中の位置
	CSampleData start_data;
	int8_t data_pos;
	int parity_count;
	CSampleData prev_err;

	uint16_t bin_data;
	uint8_t bin_err;

	// ファイル出力用
	/// 出力位置
	int write_pos;
	uint8_t over_buf[128];
	int over_pos;

	int CalcL3BSize(InputFile &file);
	int CalcT9XSize(InputFile &file);
	int WriteL3BData(OutputFile &outfile, SerialData *s_data, int width, int &redata);
	int WriteT9XData(OutputFile &outfile, SerialData *s_data, int &redata);

	void OutBaudConvertedData(SerialData *sn_data, int8_t baud, int mag, int cnt);

public:
	SerialParser();

	void ClearResult();
	void InitForDecode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_);
	void InitForEncode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_);

	PwErrType CheckL3BFormat(InputFile &file);
	PwErrType CheckT9XFormat(InputFile &file, PwErrCode &err_num, PwErrInfo &errinfo);

	int GetL3BSample(SerialData *s_data);
	int GetT9XSample(SerialData *s_data);

	int SkipL3BSample(int dir);
	int SkipT9XSample(int dir);

	int ConvertBaudRate(SerialData *s_data, SerialData *sn_data);

	int Decode(SerialData *s_data, BinaryData *b_data);

	int FindStartSerialBit(SerialData *s_data, BinaryData *b_data);
	int DecodeToBinary(SerialData *s_data, BinaryData *b_data);

	int EncodeToSerial(uint8_t bin_data, SerialData *s_data);

	int WriteL3BData(OutputFile &outfile, SerialData *s_data);
	int WriteT9XData(OutputFile &outfile, SerialData *s_data);

	void SetStartDataSPos(int val) { start_data.SPos(val); }
	void SetDataPos(int val) { data_pos = val; }
	void SetPhase3Baud(int8_t val) { phase3_baud = val; }

	void DecordingReport(SerialData *s_data, wxString &buff, wxString *logbuf);
	void EncordingReport(SerialData *s_data, wxString &buff, wxString *logbuf);
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_PARSESER_H_ */
