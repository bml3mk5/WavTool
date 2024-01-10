/// @file paw_parsecar.h
///
/// @author Sasaji
/// @date   2019.08.01
///


#ifndef _PARSEWAV_PARSECAR_H_
#define _PARSEWAV_PARSECAR_H_

#include "common.h"
#include <stdio.h>
#include <wx/string.h>
#include "paw_parse.h"
#include "errorinfo.h"
#include "paw_datas.h"
#include "paw_param.h"
#include "paw_file.h"


namespace PARSEWAV 
{

#ifdef PARSEWAV_USE_REPORT
/// レポート用
class REPORT2
{
private:
	int error_num;
public:
	REPORT2();
	~REPORT2() {}

	void Clear();
	void IncErrorNum() { error_num++; }
	int GetErrorNum() const { return error_num; }
};
#endif

/// 搬送波データ解析用クラス
class CarrierParser : public ParserBase
{
private:
	/// デコード時のフェーズ
	uint8_t phase;
	/// デコード時のフリップ有無
	uint8_t frip;

	/// 2400ボーエンコード時のフリップ有無
	int baud24_frip;

#ifdef PARSEWAV_USE_REPORT
	REPORT2 rep2;
#endif

	/// 出力位置
	uint32_t prev_data;
	int prev_width;
	uint8_t over_buf[128];
	int over_pos;

	int CalcL3CSize(InputFile &file);

	int FindStartCarrierBit(CarrierData *c_data, SerialData *s_data, int8_t baud, int &step);
	int DecodeToSerial(CarrierData *c_data, SerialData *s_data, int8_t baud, int &step);
	int WriteL3CData(OutputFile &outfile, CarrierData *c_data, int width, uint32_t &pdata, int &pwidth);

public:
	CarrierParser();

	void ClearResult();
	void InitForDecode(enum_process_mode process_mode, TempParameter &tmp_param_, MileStoneList &mile_stone_);
	void InitForEncode(enum_process_mode process_mode, TempParameter &tmp_param_, MileStoneList &mile_stone_);

	PwErrType CheckL3CFormat(InputFile &file);

	int GetL3CSample(CarrierData *c_data);

	int SkipL3CSample(int dir);

	int Decode(CarrierData *c_data, SerialData *s_data, int8_t baud, int &step);

	int ParseBaudRate(int fsk_spd, CarrierData *c_data, ChkWave *st_chkwav, int &step);

	int EncodeToCarrier(SerialData *s_data, CarrierData *c_data);

	int WriteL3CData(OutputFile &outfile, CarrierData *c_data);

	void SetPhase(int val) { phase = val; }
	void SetFrip(int val) { frip = val; }

#ifdef PARSEWAV_USE_REPORT
	void DecordingReport(CarrierData *c_data, wxString &buff, wxString *logbuf);
	void EncordingReport(CarrierData *c_data, wxString &buff, wxString *logbuf);
#endif

	double GetSampleRate();

#ifdef PARSEWAV_USE_REPORT
	const REPORT2 &GetReport() const { return rep2; }
#endif
};

}; /* namespace PARSEWAV */

#endif /* _PARSEWAV_PARSECAR_H_ */
