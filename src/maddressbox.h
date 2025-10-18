/// @file maddressbox.h
///
/// @brief 開始アドレス設定ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _MADDRESSBOX_H_
#define _MADDRESSBOX_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include "paw_defs.h"

/// マシン語開始アドレス設定パラメータ
class MAddressParam
{
private:
	PARSEWAV::maddress_t p;
	bool  submitted;

public:
	MAddressParam();
	~MAddressParam() {}

	void Initialize();

	long GetStartAddr() const { return p.start_addr; }
	void SetStartAddr(long val) { p.start_addr = val; }
	long GetExecAddr() const { return p.exec_addr; }
	void SetExecAddr(long val) { p.exec_addr = val; }
	long GetDataSize() const { return p.data_size; }
	void SetDataSize(long val) { p.data_size = val; }
	long GetFileSize() const { return p.file_size; }
	void SetFileSize(long val) { p.file_size = val; }
	bool IncludeHeader() const { return p.include_header; }
	void IncludeHeader(bool val) { p.include_header = val; }
	bool Valid() const { return p.valid; }
	void Valid(bool val) { p.valid = val; }
	bool Submitted() const { return submitted; }
	void Submitted(bool val) { submitted = val; }
};

/// マシン語開始アドレス設定ダイアログ
class MAddressBox : public wxDialog
{
private:
	MAddressParam *param;
	bool  enable_cancel;

	wxStaticText *lblTitle;
	wxTextCtrl *txtStartAddr;
	wxTextCtrl *txtExecAddr;
	wxTextCtrl *txtDataSize;
	wxCheckBox *chkIncHeader;
	wxStaticText *lblNoHeader;

	bool chk_addr(wxString &str);
	bool chk_str(wxString &str);

	void init_dialog();
	void term_dialog();

public:
	MAddressBox(wxWindow* parent, MAddressParam &nparam);

	enum {
		IDC_STA_TITLE = 1,
		IDC_START_ADDR,
		IDC_EXEC_ADDR,
		IDC_DATA_SIZE,
		IDC_INC_HEADER,
		IDC_NO_HEADER,
	};

	// functions
	int showMAddressBox(bool);

	void OnClickOk(wxCommandEvent& event);
	void OnCheckIncHeader(wxCommandEvent& event);

	// properties
//	const PARSEWAV::maddress_t &get() const { return param; }
//	bool getValid() { return param.valid; }
//	long getStartAddr() { return start_addr; }
//	long getDataSize() { return data_size; }
//	long getExecAddr() { return exec_addr; }
//	bool getIncHeader() { return include_header; }
//	long getFileSize() { return file_size; }
//	void set(const PARSEWAV::maddress_t &val) { param = val; }
//	void setValid(bool val);
//	void setStartAddr(long val) { start_addr = val; }
//	void setDataSize(long val) { data_size = val; }
//	void setExecAddr(long val) { exec_addr = val; }
//	void setIncHeader(bool val) { include_header = val; }
//	void setFileSize(long val) { file_size = val; }

	DECLARE_EVENT_TABLE()
};

#endif /* _MADDRESSBOX_H_ */
