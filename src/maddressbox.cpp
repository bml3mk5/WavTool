/// @file maddressbox.cpp
///
/// @brief 開始アドレス設定ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#include "maddressbox.h"

//////////////////////////////////////////////////////////////////////

MAddressParam::MAddressParam()
{
	Initialize();
}

void MAddressParam::Initialize()
{
	p.start_addr = 0;
	p.exec_addr  = 0;
	p.data_size  = 0;
	p.file_size  = 0;
	p.valid      = false;
	submitted    = false;
}

//////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MAddressBox, wxDialog)
	EVT_CHECKBOX(IDC_INC_HEADER, MAddressBox::OnCheckIncHeader)
	EVT_BUTTON(wxID_OK, MAddressBox::OnClickOk)
END_EVENT_TABLE()

MAddressBox::MAddressBox(wxWindow* parent, MAddressParam &nparam)
	: wxDialog(parent, wxID_ANY, _("Machine code information"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
	, param(&nparam)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer  *szrAll = new wxBoxSizer(wxVERTICAL);
	wxGridSizer *gszr;

	lblTitle = new wxStaticText(this, IDC_STA_TITLE, _T(""));
	szrAll->Add(lblTitle, flags);

	gszr = new wxFlexGridSizer(3, 3, 0, 0);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Start address")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("&&H")), flags);
	txtStartAddr = new wxTextCtrl(this, IDC_START_ADDR);
	gszr->Add(txtStartAddr, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Execute address")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("&&H")), flags);
	txtExecAddr = new wxTextCtrl(this, IDC_EXEC_ADDR);
	gszr->Add(txtExecAddr, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Data size")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("&&H")), flags);
	txtDataSize = new wxTextCtrl(this, IDC_DATA_SIZE);
	gszr->Add(txtDataSize, flags);
	szrAll->Add(gszr, flags);

	chkIncHeader = new wxCheckBox(this, IDC_INC_HEADER, _("Include header and footer in data."));
	szrAll->Add(chkIncHeader, flags);

	lblNoHeader = new wxStaticText(this, IDC_NO_HEADER, _("If cancel, file will output without header."));
	szrAll->Add(lblNoHeader, flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK|wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

// ダイアログ表示
int MAddressBox::showMAddressBox(bool hide_no_header_info)
{
	enable_cancel = true;
	
	if (hide_no_header_info) {
		lblNoHeader->Show(false);
	} else {
		lblNoHeader->Show(true);
	}
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
		return 1;
	}
	return 0;
}

void MAddressBox::init_dialog()
{
	wxString buf = _T("");

	if (param->Valid() || param->Submitted()) buf.Printf(_T("%04lx"), param->GetStartAddr());
	txtStartAddr->SetValue(buf);
	txtStartAddr->SetMaxLength(4);

	if (param->Valid() || param->Submitted()) buf.Printf(_T("%04lx"), param->GetExecAddr());
	txtExecAddr->SetValue(buf);
	txtExecAddr->SetMaxLength(4);

	buf.Printf(_T("%04lx"), (long)param->GetDataSize());
	txtDataSize->SetValue(buf);
	txtDataSize->SetMaxLength(4);
	txtDataSize->Enable(false);


	wxWindow *cancelBtn = FindWindowById(wxID_CANCEL, this);
	if (cancelBtn != NULL) {
		if (enable_cancel) {
			cancelBtn->Enable(true);
		} else {
			cancelBtn->Enable(false);
		}
	}

	chkIncHeader->SetValue(param->IncludeHeader());
	if (param->Valid()) {
		lblTitle->SetLabel(_("Please confirm this information."));	// 情報を確認して下さい。
		chkIncHeader->Enable(true);
	} else {
		lblTitle->SetLabel(_("Please input some information."));	// 情報を入力してください。
		chkIncHeader->Enable(false);
	}
}

void MAddressBox::term_dialog()
{
}

void MAddressBox::OnClickOk(wxCommandEvent& event)
{
	bool rc = true;
	wxString buf;
	long val;

	// テキスト
	buf = txtStartAddr->GetValue();
	rc = chk_addr(buf);
	if (rc != true) return;
	val = 0;
	if (buf.ToLong(&val, 16)) param->SetStartAddr(val);

	buf = txtExecAddr->GetValue();
	rc = chk_addr(buf);
	if (rc != true) return;
	val = 0;
	if (buf.ToLong(&val, 16)) param->SetExecAddr(val);

	param->IncludeHeader(chkIncHeader->IsChecked());
	if (!param->IncludeHeader()) {
		param->SetDataSize(param->GetFileSize());
	}

	param->Valid(true);
	param->Submitted(true);
	EndModal(wxID_OK);
}

void MAddressBox::OnCheckIncHeader(wxCommandEvent& event)
{
	if (!chkIncHeader->IsChecked()) {
		txtDataSize->SetValue(wxString::Format(_T("%04lx"), param->GetFileSize()));
	} else {
		txtDataSize->SetValue(wxString::Format(_T("%04lx"), param->GetDataSize()));
	}
}

#if 0
void MAddressBox::setValid(bool val)
{
	param.valid = val;
	if (!param.valid) {
//		param.start_addr = 0;
//		param.exec_addr  = 0;
		param.data_size  = 0;
		param.include_header = false;
	}
}
#endif

bool MAddressBox::chk_addr(wxString &str)
{
	if (str.Length() <= 0 || str.Length() > 4 || !chk_str(str)) {
		wxMessageBox(_("Address accepts only 4 digits hexadecimal.") // アドレスは16進数4桁で入力してください。
			, _("error"), wxOK | wxICON_ERROR, this);
		return false;
	}
	return true;
}

bool MAddressBox::chk_str(wxString &str)
{
	bool rc = true;
	size_t len;

	len = str.Length();

	if (len <= 0) {
			rc = false;
	}
	for(size_t i=0; i < len; i++) {
		if (!_istxdigit(str.GetChar(i))) {	// hex str
			rc = false;
			break;
		}
	}
	return rc;
}
