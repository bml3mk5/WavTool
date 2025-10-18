/// @file config.cpp
///
/// @brief 設定ファイル入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#include "config.h"
#include "wx/filename.h"
#include "wx/fileconf.h"

Config gConfig;

Config::Config()
{
	ini_file = _T("");

	wav_file_path = _T("");
	sample_rate = 3;
	sample_bits = 0;
	baud = 0;
	auto_baud = true;
	correct_type = 0;
	correct_amp[0] = 1000;
	correct_amp[1] = 1000;
	chg_gap_size = false;
	out_err_ser = false;
}

Config::~Config()
{
}

void Config::SetFileName(const wxString &file)
{
	ini_file = file;
}

void Config::Load()
{
	if (ini_file.IsEmpty()) return;

	// load ini file
	wxFileConfig *ini = new wxFileConfig(wxEmptyString,wxEmptyString,ini_file,wxEmptyString
		,wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);

	int lv;
	ini->Read(_T("Path"), &wav_file_path);
	if (!ini->Read(_T("SampleRate"), &lv)) lv = sample_rate;
	if (lv < 0 || 3 < lv) lv = sample_rate;
	sample_rate = lv;
	if (!ini->Read(_T("SampleBits"), &lv)) lv = sample_bits;
	if (lv < 0 || 1 < lv) lv = sample_bits;
	sample_bits = lv;
	if (!ini->Read(_T("Baud"), &lv)) lv = baud;
	if (lv < 0 || 3 < lv) lv = baud;
	baud = lv;
	ini->Read(_T("AutoBaud"), &auto_baud);
	if (!ini->Read(_T("CorrectType"), &lv)) lv = correct_type;
	if (lv < 0 || 2 < lv) lv = correct_type;
	correct_type = lv;
	if (!ini->Read(_T("CorrectAmp0"), &lv)) lv = correct_amp[0];
	if (lv < 100 || 5000 < lv) lv = correct_amp[0];
	correct_amp[0] = lv;
	if (!ini->Read(_T("CorrectAmp1"), &lv)) lv = correct_amp[1];
	if (lv < 100 || 5000 < lv) lv = correct_amp[1];
	correct_amp[1] = lv;
	ini->Read(_T("ChangeGapSize"), &chg_gap_size);
	ini->Read(_T("OutputErrorSerial"), &out_err_ser);
	for(int i=0; i<MAX_RECENT_FILES; i++) {
		wxString sval;
		ini->Read(wxString::Format(_T("Recent%d"), i), &sval);
		if (!sval.IsEmpty()) {
			mRecentFiles.Add(sval);
		}
	}

	delete ini;
}

void Config::Load(const wxString &file)
{
	SetFileName(file);
	Load();
}

void Config::Save()
{
	if (ini_file.IsEmpty()) return;

	// save ini file
	wxFileConfig *ini = new wxFileConfig(wxEmptyString,wxEmptyString,ini_file,wxEmptyString
		,wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);
	ini->Write(_T("Path"), wav_file_path);
	ini->Write(_T("SampleRate"), sample_rate);
	ini->Write(_T("SampleBits"), sample_bits);
	ini->Write(_T("Baud"), baud);
	ini->Write(_T("AutoBaud"), auto_baud);
	ini->Write(_T("CorrectType"), correct_type);
	ini->Write(_T("CorrectAmp0"), correct_amp[0]);
	ini->Write(_T("CorrectAmp1"), correct_amp[1]);
	ini->Write(_T("ChangeGapSize"), chg_gap_size);
	ini->Write(_T("OutputErrorSerial"), out_err_ser);
	for(int i=0,row=0; row<MAX_RECENT_FILES && i<(int)mRecentFiles.Count(); i++) {
		wxString sval = mRecentFiles.Item(i);
		if (sval.IsEmpty()) continue;
		ini->Write(wxString::Format(_T("Recent%d"), row), sval);
		row++;
	}
	// write
	delete ini;
}

void Config::AddRecentFile(const wxString &val)
{
	wxFileName fpath = wxFileName::FileName(val);
	wav_file_path = fpath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	// 同じファイルがあるか
	int pos = mRecentFiles.Index(fpath.GetFullPath());
	if (pos >= 0) {
		// 消す
		mRecentFiles.RemoveAt(pos);
	}
	// 追加
	mRecentFiles.Insert(fpath.GetFullPath(), 0);
	// 10を超える分は消す
	if (mRecentFiles.Count() > MAX_RECENT_FILES) {
		mRecentFiles.RemoveAt(MAX_RECENT_FILES);
	}
}

wxString &Config::GetRecentFile()
{
	return mRecentFiles[0];
}

void Config::GetRecentFiles(wxArrayString &vals)
{
	vals = mRecentFiles;
}
