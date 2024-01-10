/// @file paw_parsebin.h
///
/// @author Sasaji
/// @date   2019.08.01
///


#ifndef _PARSEWAV_PARSEBIN_H_
#define _PARSEWAV_PARSEBIN_H_

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

/// @brief 結果レポート用クラス
class REPORT4
{
private:
	uint8_t save_data_name[21];
	uint8_t flags;
	int8_t  baud;
	int data_count;
	int chksum_err_num;
	std::vector<pos_t> chksum_err_pos;

public:
	REPORT4();
	~REPORT4();

	void Clear();
	void AddChksumError(int start_pos, int end_pos);

	void SetSaveDataName(const uint8_t *name, int len);
	void GetFlags(uint8_t val) { flags = val; }
	void OrFlags(uint8_t val) { flags |= val; }
	void SetBaud(int8_t val) { baud = val; }
	void SetDataCount(int val) { data_count = val; }
	void IncDataCount() { data_count++; }

	const uint8_t *GetSaveDataName() const { return save_data_name; }
	uint8_t GetSaveDataName(int pos) const { return save_data_name[pos]; }
	uint8_t GetFlags() const { return flags; }
	int8_t GetBaud() const { return baud; }
	int GetDataCount() const { return data_count; }
	int GetChksumErrorNum() const { return chksum_err_num; }
	void GetChksumError(int idx, int &start_pos, int &end_pos) const;
};

/// バイナリデータ解析用クラス
class BinaryParser : public ParserBase
{
private:
	/// データ種類
	int header_type;
	/// データ内のファイル名
	uint8_t save_data_name[21];

	/// データバッファ
	int prev_data_len;
	uint8_t prev_data_body[257];

	/// レポート用
	std::vector<REPORT4 *> rep4;
	REPORT4 *rep4_itm;

public:
	BinaryParser();
	~BinaryParser();

	void ClearResult();
	void InitForDecode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_);
	void InitForEncode(enum_process_mode process_mode_, TempParameter &tmp_param_, MileStoneList &mile_stone_);

	PwErrType CheckL3Format(InputFile &file);

	int     GetL3Sample(BinaryData *b_data);
	uint8_t GetL3Sample();

	int FindHeader(BinaryData *b_data);
	int ParseNameSection(BinaryData *b_data, OutputFile &outfile, const wxString &outsfileb, const wxString &outsext, OutputFile &outsfile);
	int ParseBodySection(BinaryData *b_data, OutputFile &outfile, OutputFile &outsfile);
	int ParseFooterSection(BinaryData *b_data, OutputFile &outfile, OutputFile &outsfile);

	int PutHeaderSection(BinaryData *b_data, int gap_len);
	int PutBodySection(BinaryData *b_data, int gap_len);
	int PutFooterSection(BinaryData *b_data, int gap_len);

//	void FlushResult();

	void SetSaveDataInfo(const uint8_t *name, int len, uint8_t fmt, uint8_t type);
	void ClearPrevData();
	void AddPrevData(uint8_t data);

	int WriteL3Data(OutputFile &outfile, BinaryData *b_data);

	void DecordingReport(BinaryData *b_data, wxString &buff, wxString *logbuf);

	uint8_t GetSaveDataFormat() { return save_data_name[8]; }
	bool IsMachineData() { return (save_data_name[8] == 2); }
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_PARSEBIN_H_ */
