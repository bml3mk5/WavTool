/// @file rftypebox.cpp
///
/// @brief 実ファイル種類設定ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#include "rftypebox.h"

//////////////////////////////////////////////////////////////////////

RfTypeParam::RfTypeParam()
{
	Initialize();
}

void RfTypeParam::Initialize()
{
	data_format = 0;
	data_type   = 0;
	sel_file_type = 0;

	rf_name = _T("");
	rf_name_real[0] = '\0';
}

int RfTypeParam::GetRfDataName(uint8_t *name) const
{
	strncpy((char *)name, rf_name.mb_str(), 8);

	for(int i=0; i<8; i++) {
		if (name[i] == 0) name[i] = 0x20;
	}
	return 0;
}

const uint8_t *RfTypeParam::GetRfDataName()
{
	memset(rf_name_real, 0, sizeof(rf_name_real));
	GetRfDataName(rf_name_real);
	return rf_name_real;
}

int RfTypeParam::GetRfDataNameLen() const
{
	return 8;
}

//////////////////////////////////////////////////////////////////////

// Attach Event
BEGIN_EVENT_TABLE(RfTypeBox, wxDialog)
#ifndef USE_RADIOBOX
	EVT_RADIOBUTTON(IDC_RF_BASIC, RfTypeBox::OnClickDataFormat)
	EVT_RADIOBUTTON(IDC_RF_DATA, RfTypeBox::OnClickDataFormat)
	EVT_RADIOBUTTON(IDC_RF_MACHINE, RfTypeBox::OnClickDataFormat)
#else
	EVT_RADIOBOX(IDC_RF_TYPE1, RfTypeBox::OnClickType1)
#endif
	EVT_RADIOBUTTON(IDC_RF_TAPEFILE, RfTypeBox::OnClickFileType)
	EVT_RADIOBUTTON(IDC_RF_REALFILE, RfTypeBox::OnClickFileType)

	EVT_BUTTON(wxID_OK, RfTypeBox::OnClickOk)
END_EVENT_TABLE()

RfTypeBox::RfTypeBox(wxWindow* parent, RfTypeParam &nparam)
	: wxDialog(parent, wxID_ANY, _("File Type"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
	, param(&nparam)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	wxSizerFlags flagsW = wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 8);

	wxBoxSizer  *szrAll = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szr;

	szrAll->Add(new wxStaticText(this, wxID_ANY, _("Please select this file type.")), flags); // ファイルの種類を選択してください。

	radFiles[0] = new wxRadioButton(this, IDC_RF_TAPEFILE, _("Plain File (Add no header and footer.)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	szrAll->Add(radFiles[0], flags);
	radFiles[1] = new wxRadioButton(this, IDC_RF_REALFILE, _("Real File (Select following items.)"));
	szrAll->Add(radFiles[1], flags);

	szr = new wxBoxSizer(wxHORIZONTAL);
#ifndef USE_RADIOBOX
	wxBoxSizer *szrL = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _T("")), wxVERTICAL);
	radBasic = new wxRadioButton(this, IDC_RF_BASIC, _("BASIC"),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radData = new wxRadioButton(this, IDC_RF_DATA, _("Data"));
	radMachine = new wxRadioButton(this, IDC_RF_MACHINE, _("Machine"));
    szrL->Add(radBasic, flags);
    szrL->Add(radData, flags);
    szrL->Add(radMachine, flags);
	szr->Add(szrL, flagsW);
#else
	wxArrayString types1;
	types1.Add(_("BASIC"));
	types1.Add(_("Data"));
	types1.Add(_("Machine"));

	radTypes[0] = new wxRadioBox(this, IDC_RF_TYPE1, _T(""), wxDefaultPosition, wxDefaultSize, types1, 0, wxRA_SPECIFY_ROWS);
	szr->Add(radTypes[0], flagsW);
#endif
	wxBoxSizer *szrR = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _T("")), wxVERTICAL);
	radBinary = new wxRadioButton(this, IDC_RF_BINARY, _("Binary"),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radAscii = new wxRadioButton(this, IDC_RF_ASCII, _("Ascii"));
    szrR->Add(radBinary, flags);
    szrR->Add(radAscii, flags);
	szr->Add(szrR, flagsW);

	szrAll->Add(szr, flags);

	szrAll->Add(new wxStaticText(this, wxID_ANY, _("Please input internal file name.")), flags); // 内部ファイル名を入力してください。
	txtEdit = new wxTextCtrl(this, IDC_RF_EDIT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	szrAll->Add(txtEdit, flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK|wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);

	select_file_type();
}

// ダイアログ表示
int RfTypeBox::showRftypeBox(const _TCHAR *name, bool init_data)
{
	wxString sName = _T("");

	if (name != NULL) {
		wxString snName;
		sName = wxString(name, wxConvUTF8);
		trim_str(sName, snName);
		param->SetRfName(snName);
	}
	return showRftypeBox(init_data);
}

int RfTypeBox::showRftypeBox(const wxString &name, bool init_data)
{
	wxString snName;
	trim_str(name, snName);
	param->SetRfName(snName);

	return showRftypeBox(init_data);
}

int RfTypeBox::showRftypeBox(bool init_data)
{
	init_dialog(init_data);

	wxWindow *cancelBtn = FindWindowById(wxID_CANCEL, this);
	if (cancelBtn != NULL) {
		if (init_data) {
			cancelBtn->Enable(false);
		} else {
			cancelBtn->Enable(true);
		}
	}

	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
		return 1;
	}
	return 0;
}

void RfTypeBox::init_dialog(bool init_data)
{
	uint8_t data_format = param->GetRfDataFormat();
	uint8_t data_type = param->GetRfDataType();
	int sel_file_type = param->GetRfDataFileType();

	radFiles[0]->SetValue(sel_file_type == 0);
	radFiles[1]->SetValue(sel_file_type == 1);

#ifndef USE_RADIOBOX
	radBasic->SetValue(data_format == 0);
	radData->SetValue(data_format == 1);
	radMachine->SetValue(data_format == 2);
#else
	radTypes[0]->SetSelection(data_format);
#endif
	radBinary->SetValue(data_type == 0);
	radAscii->SetValue(data_type == 0xff);

	txtEdit->SetValue(param->GetRfName());
	txtEdit->SetMaxLength(8);

	select_file_type();
}

void RfTypeBox::term_dialog()
{
	uint8_t data_format = 0;
	uint8_t data_type = 0;

	// ラジオボタン
#ifndef USE_RADIOBOX
	if (radBasic->GetValue()) {
		data_format = 0;
	} else if (radData->GetValue()) {
		data_format = 1;
	} else {
		data_format = 2;
	}
#else
	data_format = radTypes[0]->GetSelection();
	if (data_format < 0) data_format = 2;
#endif
	if (radBinary->GetValue()) {
		data_type = 0;
	} else {
		data_type = 0xff;
	}

	param->SetRfDataFormat(data_format);
	param->SetRfDataType(data_type);
}

#ifndef USE_RADIOBOX
void RfTypeBox::OnClickDataFormat(wxCommandEvent& event)
{
	param->SetRfDataFormat((uint8_t)(event.GetId() - IDC_RF_BASIC));
	select_data_format();
}
#else
void RfTypeBox::OnClickType1(wxCommandEvent& event)
{
	data_format = (uint8_t)event.GetSelection();
	select_data_format();
}
#endif

void RfTypeBox::OnClickFileType(wxCommandEvent& event)
{
	param->SetRfDataFileType(event.GetId() - IDC_RF_TAPEFILE);
	select_file_type();
}

void RfTypeBox::OnClickOk(wxCommandEvent& event)
{
	// テキスト
	param->SetRfName(txtEdit->GetValue());
	if (!chk_str(param->GetRfName())) {
		wxMessageBox(_("Internal file name accepts only alphabets and digits.") // 内部ファイル名は半角英数字で入力してください。
			, _("error"), wxOK | wxICON_ERROR, this);
		return;
	}
	EndModal(wxID_OK);
}

void RfTypeBox::select_file_type()
{
	bool sel = (param->GetRfDataFileType() != 0);
#ifndef USE_RADIOBOX
	radBasic->Enable(sel);
	radData->Enable(sel);
	radMachine->Enable(sel);
#else
	radTypes[0]->Enable(sel);
#endif
	radBinary->Enable(sel);
	radAscii->Enable(sel);
	txtEdit->Enable(sel);
	if (sel) select_data_format();
}

void RfTypeBox::select_data_format()
{
	int data_format = param->GetRfDataFormat();
	radBinary->Enable(data_format != 1);
	radAscii->Enable(data_format != 2);
	if (!radBinary->IsEnabled()) {
		radBinary->SetValue(false);
		radAscii->SetValue(true);
	} else if (!radAscii->IsEnabled()) {
		radBinary->SetValue(true);
		radAscii->SetValue(false);
	}
}

bool RfTypeBox::chk_str(const wxString &str)
{
	bool rc = true;
	size_t len;

	len = str.Length();

	if (len <= 0) {
			rc = false;
	}
	for(size_t i=0; i < len; i++) {
		wxChar c = str.GetChar(i);
		if (c < _T(' ') || c == _T('(') || c == _T(')') || c == _T(':')) {
			rc = false;
			break;
		}
	}
	return rc;
}

void RfTypeBox::trim_str(const wxString &str, wxString &nstr)
{
	size_t len;

	nstr = _T("");

	len = str.Length();
	for(size_t i=0; i < len && i < 8; i++) {
		wxChar c = str.GetChar(i);
		if (c < _T(' ') || c == _T('.') || c == _T('(') || c == _T(')') || c == _T(':')) {
			break;
		}
		nstr += c;
	}
}
