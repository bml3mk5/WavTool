/// @file findposbox.cpp
///
/// @brief 実ファイル種類設定ダイアログ
///


#include "findposbox.h"


// Attach Event
BEGIN_EVENT_TABLE(FindPosBox, wxDialog)
	EVT_RADIOBUTTON(IDC_RADIO_SEC, FindPosBox::OnChangeRadio)
	EVT_RADIOBUTTON(IDC_RADIO_POS, FindPosBox::OnChangeRadio)
//	EVT_BUTTON(wxID_OK, FindPosBox::OnClickOk)
END_EVENT_TABLE()

FindPosBox::FindPosBox()
{
	FindPosBox(NULL, wxID_ANY, 0, 0);
}

FindPosBox::FindPosBox(wxWindow* parent, wxWindowID id, uint32_t msec, int spos)
	: wxDialog(parent, id, _("Find Position"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
//	wxSizerFlags flagsW = wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 8);

	int sec = msec / 1000;
	int min = sec / 60;

	msec %= 1000;
	sec %= 60;
	min %= 60;

	wxBoxSizer  *szrAll = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szr;

	szr = new wxBoxSizer(wxHORIZONTAL);
	radSec = new wxRadioButton(this, IDC_RADIO_SEC, _("Find by time"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	szr->Add(radSec, flags);
	szrAll->Add(szr, flags);

	wxSize sz(64, -1);

	szr = new wxBoxSizer(wxHORIZONTAL);
	spinSec[0] = new wxSpinCtrl(this, IDC_SPIN_SEC0, wxEmptyString, wxDefaultPosition, sz, wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 59, min);
	szr->Add(spinSec[0], flags);
	szr->Add(new wxStaticText(this, wxID_ANY, _("min.")), flags);
	spinSec[1] = new wxSpinCtrl(this, IDC_SPIN_SEC1, wxEmptyString, wxDefaultPosition, sz, wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 59, sec);
	szr->Add(spinSec[1], flags);
	szr->Add(new wxStaticText(this, wxID_ANY, _("sec.")), flags);
	spinSec[2] = new wxSpinCtrl(this, IDC_SPIN_SEC2, wxEmptyString, wxDefaultPosition, sz, wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 999, msec);
	szr->Add(spinSec[2], flags);
	szr->Add(new wxStaticText(this, wxID_ANY, _("msec.")), flags);
	szrAll->Add(szr, flags);

	szr = new wxBoxSizer(wxHORIZONTAL);
	radPos = new wxRadioButton(this, IDC_RADIO_SEC, _("Find by sample position"));
	szr->Add(radPos, flags);
	szrAll->Add(szr, flags);

	szr = new wxBoxSizer(wxHORIZONTAL);
	spinPos = new wxSpinCtrl(this, IDC_SPIN_POS, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 99999999, spos);
	szr->Add(spinPos, flags);
	szrAll->Add(szr, flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK|wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);

	radSec->SetValue(true);

	ChangeRadio();
}

void FindPosBox::OnChangeRadio(wxCommandEvent &event)
{
	ChangeRadio();
}

void FindPosBox::ChangeRadio()
{
	if (radSec->GetValue()) {
		spinSec[0]->Enable(true);
		spinSec[1]->Enable(true);
		spinSec[2]->Enable(true);
		spinPos->Enable(false);
	} else {
		spinSec[0]->Enable(false);
		spinSec[1]->Enable(false);
		spinSec[2]->Enable(false);
		spinPos->Enable(true);
	}
}

bool FindPosBox::IsSelectedSec() const
{
	return radSec->GetValue();
}

uint32_t FindPosBox::GetMSec() const
{
	return (uint32_t)spinSec[0]->GetValue() * 60000 + (uint32_t)spinSec[1]->GetValue() * 1000 + (uint32_t)spinSec[2]->GetValue(); 
}

int FindPosBox::GetSPos() const
{
	return spinPos->GetValue();
}
