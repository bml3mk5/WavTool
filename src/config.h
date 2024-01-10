/// @file config.h
///
/// config.h
///


#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "common.h"
#include <wx/wx.h>


#define MAX_RECENT_FILES 20

/// 設定ファイル入出力
class Config
{
private:
	wxString ini_file;

	wxString wav_file_path;
	int      sample_rate;
	int      sample_bits;
	int      baud;
	bool     auto_baud;
	int      correct_type;
	int      correct_amp[2];
	bool     chg_gap_size;
	bool     out_err_ser;
	wxArrayString mRecentFiles;

public:
	Config();
	~Config();
	void SetFileName(const wxString &file);
	void Load(const wxString &file);
	void Load();
	void Save();
	const wxString &GetFilePath() const { return wav_file_path; }
	int GetSampleRatePos() const { return sample_rate; }
	void SetSampleRatePos(int val) { sample_rate = val; }
	int GetSampleBitsPos() const { return sample_bits; }
	void SetSampleBitsPos(int val) { sample_bits = val; }
	int GetBaud() const { return baud; }
	void SetBaud(int val) { baud = val; }
	bool GetAutoBaud() const { return auto_baud; }
	void SetAutoBaud(bool val) { auto_baud = val; }
	int GetCorrectType() const { return correct_type; }
	int GetCorrectAmp(int idx) const { return correct_amp[idx]; }
	void SetCorrectType(int val) { correct_type = val; }
	void SetCorrectAmp(int idx, int val) { correct_amp[idx] = val; }
	bool GetChangeGapSize() const { return chg_gap_size; }
	void SetChangeGapSize(bool val) { chg_gap_size = val; }
	bool GetOutErrSerial() const { return out_err_ser; }
	void SetOutErrSerial(bool val) { out_err_ser = val; }
	void AddRecentFile(const wxString &val);
	wxString &GetRecentFile();
	void GetRecentFiles(wxArrayString &vals);
};

extern Config gConfig;

#endif /* _CONFIG_H_ */
