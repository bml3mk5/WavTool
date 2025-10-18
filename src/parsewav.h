/// @file parsewav.h
///
/// parsewav.h
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _PARSEWAV_H_
#define _PARSEWAV_H_

#include <stdio.h>
#include <string.h>
//#define _USE_MATH_DEFINES
//#include <math.h>
#include <vector>
#include "common.h"
#include <wx/wx.h>
#include "paw_defs.h"
#include "paw_param.h"
#include "paw_datas.h"
#include "paw_file.h"
#include "paw_parsewav.h"
#include "paw_parsecar.h"
#include "paw_parseser.h"
#include "paw_parsebin.h"
#include "progressbox.h"
#include "rftypebox.h"
#include "maddressbox.h"
#include "errorinfo.h"
#include "paw_format.h"
#include "paw_util.h"
#include "paw_dft.h"


namespace PARSEWAV
{

/// @brief データレコーダの変調波(FSK)を解析して実データに変換するクラス
///
/// waveファイル形式で保存されたデータレコーダの変調波(FSK)1200Hzと2400Hzの波を
/// 解析して搬送波データ、シリアルデータ、実データを取り出す。
/// 逆に実データから変調波のwaveファイルを作成する。
class ParseWav
{
private:
	ProgressBox *progbox;
	RfTypeParam rftypeparam;
	MAddressParam maddressparam;
	wxWindow *parent_window;
	PwErrInfo *errinfo;

	InputFile infile;
	OutputFile outfile;
	OutputFile outsfile;

	wxString logfilename;

//	wxString outsfilen;
	wxString outsfileb;
	wxString outsext;

	wxString *logbuf;

	bool include_header;
	uint8_t rf_header[5];
	uint8_t rf_footer[5];

	Parameter param;
	TempParameter tmp_param;

	Util  conv;
	Dft   dft;

	enum enum_phase {
		PHASE_NONE = -1,
		PHASE_IDLE = 0,
		PHASE1_GET_WAV_SAMPLE = 1,
		PHASE1_CORRECT_WAVE,
		PHASE1_DECODE_TO_CARRIER,
		PHASE1_GET_L3C_SAMPLE,
		PHASE1_PUT_L3C_SAMPLE,
		PHASE1_SHIFT_BUFFER,
		PHASE2_DECODE_TO_SERIAL = 11,
		PHASE2_PARSE_CARRIER,
		PHASE2_GET_L3B_SAMPLE,
		PHASE2_GET_T9X_SAMPLE,
		PHASE2_GOTO_PHASE2N,
		PHASE2_ENCODE_TO_CARRIER,
		PHASE2_GET_L3C_SAMPLE,
		PHASE2_ENCODE_TO_WAVE,
		PHASE2N_CONVERT_BAUD_RATE = 21,
		PHASE2N_GET_L3B_SAMPLE,
		PHASE2N_GET_T9X_SAMPLE,
		PHASE2N_PUT_L3B_SAMPLE,
		PHASE3_DECODE_TO_BINARY = 31,
		PHASE3_GET_L3_SAMPLE,
		PHASE3_PUT_L3_SAMPLE,
		PHASE3_GET_BIN_SAMPLE,
		PHASE3_ENCODE_TO_SERIAL,
		PHASE3_GET_L3B_SAMPLE,
		PHASE3_GET_T9X_SAMPLE,
		PHASE3_PUT_L3B_SAMPLE,
		PHASE4_FIND_HEADER = 41,
		PHASE4_PARSE_NAME_SECTION,
		PHASE4_PARSE_BODY_SECTION,
		PHASE4_PARSE_FOOTER_SECTION,
		PHASE4_PUT_HEADER,
		PHASE4_PUT_BODY_DATA,
		PHASE4_PUT_FOOTER_DATA,
	};

	enum_phase phase1;
	enum_phase phase2;
	enum_phase phase2n;
	enum_phase phase3;
	enum_phase phase4;

	WaveParser    wave_parser;
	CarrierParser carrier_parser;
	SerialParser  serial_parser;
	BinaryParser  binary_parser;

	MileStoneList mile_stone;
	int viewing_dir;

	WaveData     *wave_data;
	WaveData     *wave_correct_data;
	CarrierData  *carrier_data;
	SerialData   *serial_data;
	SerialData   *serial_new_data;
	BinaryData   *binary_data;

	PwErrCode err_num;

	enum_process_mode process_mode;

	ChkWave st_chkwav[2];
	int st_chkwav_analyzed_num;

	WaveFormat inwav;
	WaveFormat outwav;

	wxString buff;
//	char cbuff[1000];

	int progress_div;

	PwErrType check_rf_format(InputFile &file);
	PwErrType get_first_rf_data(InputFile &file);

	void decide_maddress(InputFile &file, enum_file_type &file_type);

	uint8_t get_rf_sample();

	int   decode_phase1(int fsk_spd, WaveData *w_data, WaveData *wc_data, CarrierData *c_data, SerialData *s_data, SerialData *sn_data, BinaryData *b_data, enum_phase start_phase);
	int   decode_phase2(int fsk_spd, CarrierData *c_data, SerialData *s_data, SerialData *sn_data, BinaryData *b_data, enum_phase start_phase);
	int   decode_phase2n(SerialData *s_data, SerialData *sn_data, BinaryData *b_data, enum_phase start_phase);
	int   decode_phase3(SerialData *s_data, BinaryData *b_data, enum_phase start_phase);
	int	  decode_phase4(BinaryData *b_data);
	int   decode_plain_data();

	int   encode_plain_data(BinaryData *b_data);
	int   encode_phase4(BinaryData *b_data, SerialData *s_data, CarrierData *c_data);
	int   encode_phase3(BinaryData *b_data, SerialData *s_data, CarrierData *c_data, enum_phase start_phase);
	int   encode_phase2(SerialData *s_data, CarrierData *c_data, enum_phase start_phase, bool end_data);
	int   encode_phase1(CarrierData *c_data, bool end_data);

	void  set_rf_info();

	void  reporting();
	void  reporting_analyze();
	void  write_log(const wxString &, int);

	bool check_extension(const wxString &filename, const wxString &ext);

	void out_dummy_tail_data(OutputFile &file);

public:
	ParseWav(wxWindow *parent);
	~ParseWav();

	bool OpenDataFile(const wxString &in_file);
	void CloseDataFile();
	PwErrType CheckFileFormat(InputFile &file);
	PwErrType SeekFileFormat(InputFile &file);

	bool OpenOutFile(const wxString &out_file, enum_file_type outfile_type);
	void CloseOutFile();

	int InitFileHeader(OutputFile &file);
	void SetFileHeader(OutputFile &file);
//	void OutConvSampleData(void *, uint32_t, size_t, size_t, FILE *);

	bool ExportData(enum_file_type file_type = FILETYPE_UNKNOWN);
	PwErrType DecodeData();
	PwErrType ViewData(int dir, double spos, CSampleArray *a_data);
	PwErrType EncodeData();
	int AnalyzeWave();

	void GetFileNameBase(wxString &);

	void SetLogBufferPtr(wxString *);

	bool ShowRfTypeBox();
	bool ShowMAddressBox(bool);

	InputFile *GetDataFile() { return &infile; }
	bool IsOpenedDataFile() { return infile.IsOpened(); }
	int OpenedDataFileCount(int *sample_num);
	enum_file_type GetDataFileType() { return infile.GetType(); }
	int GetRfDataFormat();

	Parameter &GetParam() { return param; }
	const Parameter &GetParam() const { return param; }
	void SetParam(const Parameter &data) { param = data; }

#if 0
	void SetSampleRate(int value) { param.SetSampleRate(value); }
	void SetSampleBits(int value) {	param.SetSampleBits(value); }
	void SetBaud(int value)		{ param.SetBaud(value); }
	void SetAutoBaud(int value) { param.SetAutoBaud(value); }
	void SetReverseWave(int value) { param.SetReverseWave(value); }
	void SetHalfWave(int value) { param.SetHalfWave(value); }
	void SetCorrectWave(int value) { param.SetCorrectWave(value); }
	void SetCorrectType(int value) { param.SetCorrectType(value); }
	void SetFskSpeed(int value) { param.SetFskSpeed(value); }
	void SetFreq(int num, int value) { param.SetFreq(num, value); }
	void SetFrequency(int magnify);
	void SetRange(int num, int value) { param.SetRange(num, value); }
	void SetWordSelect(int value) { param.SetWordSelect(value); }
	void SetChangeGapSize(int value) { param.SetChangeGapSize(value); }
	void SetDebugMode(int value) { param.SetDebugMode(value); }

	int GetSampleRate(void) { return param.GetSampleRate(); }
	int GetSampleBits(void) { return param.GetSampleBits(); }
	int GetBaud(void)		{ return param.GetBaud(); }
	int	GetAutoBaud(void)   { return param.GetAutoBaud(); }
	int GetReverseWave(void) { return param.GetReverseWave(); }
	int GetHalfWave(void)	{ return param.GetHalfWave(); }
	int GetCorrectWave(void) { return param.GetCorrectWave(); }
	int GetCorrectType(void) { return param.GetCorrectType(); }
	int GetFskSpeed(void) { return param.GetFskSpeed(); }
	int GetBaseFreq(void) { return param.GetBaseFreq(); }
	int GetFreq(int num) { return param.GetFreq(num); }
	int	GetRange(int num) { return param.GetRange(num); }
	int	GetWordSelect(void) { return param.GetWordSelect(); }
	int	GetChangeGapSize(void) { return param.GetChangeGapSize(); }
	int GetDebugMode(void) { return param.GetDebugMode(); }
#endif

	WaveData *GetWaveData(void) { return wave_data; }
	WaveData *GetWaveCorrectData(void) { return wave_correct_data; }
	CarrierData *GetCarrierData(void) { return carrier_data; }
	SerialData *GetSerialData(void) { return serial_data; }
	SerialData *GetSerialNewData(void) { return serial_new_data; }
	BinaryData *GetBinaryData(void) { return binary_data; }
	WaveFormat *GetInWavFormat(void) { return &inwav; }

	// wrapper

	void initProgress(int type, int min_val, int max_val);
	bool needSetProgress() const;
	bool setProgress(int val);
	bool setProgress(int num, int div);
	bool incProgress();
	bool viewProgress();
	void endProgress();

};

}; /* namespace PARSEWAV */

#endif
