/// @file rftypebox.h
///
/// @brief 実ファイル種類設定ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _RFTYPEBOX_H_
#define _RFTYPEBOX_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>

/// 実ファイル種類選択パラメータ
class RfTypeParam
{
private:
	uint8_t  data_format;
	uint8_t  data_type;

	wxString rf_name;
	uint8_t rf_name_real[12];

	int sel_file_type;

public:
	RfTypeParam();
	~RfTypeParam() {}

	void Initialize();

	int		GetRfDataName(uint8_t *) const;
	const uint8_t *GetRfDataName();
	int		GetRfDataNameLen() const;
	const wxString &GetRfName() const { return rf_name; }
	uint8_t	GetRfDataFormat() const { return data_format; }
	uint8_t	GetRfDataType() const { return data_type; }
	int     GetRfDataFileType() const { return sel_file_type; }
	void    SetRfDataFormat(uint8_t val) { data_format = val; }
	void    SetRfDataType(uint8_t val) { data_type = val; }
	void    SetRfName(const wxString &val) { rf_name = val; }
	void    SetRfDataFileType(int val) { sel_file_type = val; }
};

/// 実ファイル種類選択ダイアログ
class RfTypeBox : public wxDialog
{
private:
	RfTypeParam *param;

	wxRadioButton *radFiles[2];

#ifndef USE_RADIOBOX
	wxRadioButton *radBasic;
	wxRadioButton *radData;
	wxRadioButton *radMachine;
#else
	wxRadioBox *radTypes[1];
#endif
	wxRadioButton *radBinary;
	wxRadioButton *radAscii;
	wxTextCtrl *txtEdit;

	void init_dialog(bool init_data);
	void term_dialog();

	void select_file_type();
	void select_data_format();
	bool chk_str(const wxString &str);
	void trim_str(const wxString &str, wxString &nstr);

public:
	RfTypeBox(wxWindow* parent, RfTypeParam &nparam);

	enum {
		IDC_RF_TAPEFILE = 1,
		IDC_RF_REALFILE,
#ifndef USE_RADIOBOX
		IDC_RF_BASIC,
		IDC_RF_DATA,
		IDC_RF_MACHINE,
#else
		IDC_RF_TYPE1,
#endif
		IDC_RF_BINARY,
		IDC_RF_ASCII,
		IDC_RF_EDIT,
	};

#ifndef USE_RADIOBOX
	void OnClickDataFormat(wxCommandEvent& event);
#else
	void OnClickType1(wxCommandEvent& event);
#endif
	void OnClickFileType(wxCommandEvent& event);
	void OnClickOk(wxCommandEvent& event);

//	int		getRfDataName(uint8_t *);
//	const uint8_t *getRfDataName();
//	int		getRfDataNameLen();
//	wxString getRfName() { return rf_name; }
//	uint8_t	getRfDataFormat() { return data_format; }
//	uint8_t	getRfDataType()   { return data_type; }
//	int     getRfDataFileType() { return sel_file_type; }

	int		showRftypeBox(const _TCHAR *name, bool init_data = true);
	int     showRftypeBox(const wxString &name, bool init_data = true);
	int     showRftypeBox(bool init_data = true);

	DECLARE_EVENT_TABLE()
};

#endif /* _RFTYPEBOX_H_ */
