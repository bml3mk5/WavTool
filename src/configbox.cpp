/// @file configbox.cpp
///
/// @brief 設定ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#include "configbox.h"

static wxString dbg_list[] = {
	_T("0"), _T("1"), _T("2")
};
static wxString rate_list[] = {
	_T("11025"), _T("22050"), _T("44100"), _T("48000")
};
static wxString bits_list[] = {
	_T("8"), _T("16")
};

// Attach Event
BEGIN_EVENT_TABLE(ConfigBox, wxDialog)
	EVT_CHECKBOX(IDC_CHK_DBL_FSK, ConfigBox::OnClickDblFsk)
//	EVT_CHECKBOX(IDC_CHK_CORRECT, ConfigBox::OnClickCorrect)
	EVT_RADIOBUTTON(IDC_RADIO_7BIT, ConfigBox::OnClick7bit)
	EVT_RADIOBUTTON(IDC_RADIO_8BIT, ConfigBox::OnClick8bit)
	EVT_RADIOBUTTON(IDC_RADIO_STOP1BIT, ConfigBox::OnClickStop1bit)
	EVT_RADIOBUTTON(IDC_RADIO_STOP2BIT, ConfigBox::OnClickStop2bit)
END_EVENT_TABLE()

ConfigBox::ConfigBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Configure"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrLeft   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrRight  = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrMain   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *szrAll    = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *bszr;
	wxGridSizer *gszr;


	wxBoxSizer *szrWav2L3c = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("How to parse a wav file")), wxVERTICAL);
#ifdef __WXGTK__
	wxSize size(-1,-1);
#else
	wxSize size(64,-1);
#endif
	gszr = new wxFlexGridSizer(2, 4, 0, 0);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("- 1200 -"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("- 2400 -"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("- 4800 -"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Center(Hz)")), flags);
	for(int n=0; n<3; n++) {
		spinFreq[n] = new wxSpinCtrl(this, IDC_SPIN_FREQ1200 + n, wxEmptyString, wxDefaultPosition, size);
		gszr->Add(spinFreq[n], flags);
	}
	szrWav2L3c->Add(gszr, flags);

	gszr = new wxFlexGridSizer(2, 3, 0, 0);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("Long(0)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("Short(1)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Error(%)")), flags);
	for(int n=0; n<2; n++) {
		spinRange[n] = new wxSpinCtrl(this, IDC_SPIN_RANGE1200 + n, wxEmptyString, wxDefaultPosition, size);
		gszr->Add(spinRange[n], flags);
	}
	szrWav2L3c->Add(gszr, flags);

	bszr = new wxBoxSizer(wxHORIZONTAL);
	chkReverse = new wxCheckBox(this, IDC_CHK_REVERSE, _("Reverse Wave"));
	chkHalfwave = new wxCheckBox(this, IDC_CHK_HALFWAVE, _("Detect Half Wave"));
	bszr->Add(chkReverse, flags);
	bszr->Add(chkHalfwave, flags);
	szrWav2L3c->Add(bszr, flags);
	bszr = new wxBoxSizer(wxHORIZONTAL);
//	chkCorrect = new wxCheckBox(this, IDC_CHK_CORRECT, _("Correct"));
	bszr->Add(new wxStaticText(this, wxID_ANY, _("Correct")), flags);
	radNoCorrect = new wxRadioButton(this, IDC_RADIO_NOCORRECT, _("No"),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radCorrCosw = new wxRadioButton(this, IDC_RADIO_COSW, _("COS Wave"));
	radCorrSinw = new wxRadioButton(this, IDC_RADIO_SINW, _("SIN Wave"));
//	bszr->Add(chkCorrect, flags);
	bszr->Add(radNoCorrect, flags);
	bszr->Add(radCorrCosw, flags);
	bszr->Add(radCorrSinw, flags);
	szrWav2L3c->Add(bszr);

	bszr = new wxBoxSizer(wxHORIZONTAL);
	bszr->Add(new wxStaticText(this, wxID_ANY, _("Amplitude of Correct Wave")), flags);
	szrWav2L3c->Add(bszr);

	gszr = new wxFlexGridSizer(1, 7, 0, 0);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T(" "), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	stsCorr1200 = new wxStaticText(this, IDC_STATIC1200, _T("1200"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	gszr->Add(stsCorr1200, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("Hz"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	spinCorrAmp[0] = new wxSpinCtrl(this, IDC_SPIN_CORRAMP1200, wxEmptyString, wxDefaultPosition, size);
	gszr->Add(spinCorrAmp[0], flags);
	stsCorr2400 = new wxStaticText(this, IDC_STATIC2400, _T("2400"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	gszr->Add(stsCorr2400, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("Hz"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL), flags);
	spinCorrAmp[1] = new wxSpinCtrl(this, IDC_SPIN_CORRAMP2400, wxEmptyString, wxDefaultPosition, size);
	gszr->Add(spinCorrAmp[1], flags);
	szrWav2L3c->Add(gszr);

	szrLeft->Add(szrWav2L3c);

	wxBoxSizer *szrWavFile = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Output a wav file")), wxVERTICAL);
	gszr = new wxFlexGridSizer(2, 3, 0, 0);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Sample Rate")), flags);
	comSRate = new wxComboBox(this, IDC_COMBO_SRATE, _T(""), wxDefaultPosition, wxDefaultSize, 4, rate_list, wxCB_DROPDOWN | wxCB_READONLY);
	gszr->Add(comSRate, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Hz")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Sample Bits")), flags);
	comSBits = new wxComboBox(this, IDC_COMBO_SBITS, _T(""), wxDefaultPosition, wxDefaultSize, 2, bits_list, wxCB_DROPDOWN | wxCB_READONLY);
	gszr->Add(comSBits, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("bits")), flags);
	szrWavFile->Add(gszr, flags);
	szrLeft->Add(szrWavFile, flags);

	bszr = new wxBoxSizer(wxHORIZONTAL);
	bszr->Add(new wxStaticText(this, wxID_ANY, _("DebugLogLevel")), flags);
	comDebugLevel = new wxComboBox(this, IDC_COMBO_DEBUG, _T(""), wxDefaultPosition, wxDefaultSize, 3, dbg_list, wxCB_DROPDOWN | wxCB_READONLY);
	bszr->Add(comDebugLevel, flags);
	szrLeft->Add(bszr, flags);

	/* */
	wxBoxSizer *szrL3c2L3b = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Baud rate")), wxVERTICAL);
	chkDblFsk = new wxCheckBox(this, IDC_CHK_DBL_FSK, _("Use Double Speed FSK"));
	szrL3c2L3b->Add(chkDblFsk, flags);
	bszr = new wxBoxSizer(wxHORIZONTAL);
    bszr->Add(new wxStaticText(this, wxID_ANY, _("Baud")), flags);
	radBaud600 = new wxRadioButton(this, IDC_RADIO_BAUD_600, _T("600 "),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radBaud1200 = new wxRadioButton(this, IDC_RADIO_BAUD_1200, _T("1200"));
	radBaud2400 = new wxRadioButton(this, IDC_RADIO_BAUD_2400, _T("2400"));
	radBaud300 = new wxRadioButton(this, IDC_RADIO_BAUD_300, _T("300 "));
	bszr->Add(radBaud600, flags);
	bszr->Add(radBaud1200, flags);
	bszr->Add(radBaud2400, flags);
	bszr->Add(radBaud300, flags);
	szrL3c2L3b->Add(bszr);
	bszr = new wxBoxSizer(wxHORIZONTAL);
    bszr->Add(new wxStaticText(this, wxID_ANY, wxT("    ")), flags);
	chkBaudAuto = new wxCheckBox(this, IDC_CHK_BAUDAUTO, _("Auto Detect"));
	bszr->Add(chkBaudAuto, flags);
	szrL3c2L3b->Add(bszr);
	szrRight->Add(szrL3c2L3b, flags);

	wxBoxSizer *szrL3b2L3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Bits length of one word")), wxVERTICAL);
	gszr = new wxFlexGridSizer(3, 4, 0, 0);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Data bits")), flags);
	radData7bit = new wxRadioButton(this, IDC_RADIO_7BIT, _("7bits"),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radData8bit = new wxRadioButton(this, IDC_RADIO_8BIT, _("8bits"));
	gszr->Add(radData7bit, flags);
	gszr->Add(radData8bit, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("")), flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Parity")), flags);
	radParityNo = new wxRadioButton(this, IDC_RADIO_PARITY_NO, _("No"),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radParityEven = new wxRadioButton(this, IDC_RADIO_PARITY_EVEN, _("Even"));
	radParityOdd = new wxRadioButton(this, IDC_RADIO_PARITY_ODD, _("Odd"));
	gszr->Add(radParityNo, flags);
	gszr->Add(radParityEven, flags);
	gszr->Add(radParityOdd, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _("Stopbit")), flags);
	radStop1bit = new wxRadioButton(this, IDC_RADIO_STOP1BIT, _("1bit"),wxDefaultPosition,wxDefaultSize,wxRB_GROUP);
	radStop2bit = new wxRadioButton(this, IDC_RADIO_STOP2BIT, _("2bit"));
	gszr->Add(radStop1bit, flags);
	gszr->Add(radStop2bit, flags);
	gszr->Add(new wxStaticText(this, wxID_ANY, _T("")), flags);
	szrL3b2L3->Add(gszr, flags);
	bszr = new wxBoxSizer(wxHORIZONTAL);
	chkOutErrSer = new wxCheckBox(this, IDC_CHK_OUTERRSER, _("Output a data even if parity or frame error."));
	bszr->Add(chkOutErrSer, flags);
	szrL3b2L3->Add(bszr, flags);
	szrRight->Add(szrL3b2L3, flags);

	wxBoxSizer *szrGapSize = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Gap size")), wxVERTICAL);
	chkGapSize = new wxCheckBox(this, IDC_CHK_CHGGAP, _("Change the long gap size according to a baud rate."));
	szrGapSize->Add(chkGapSize, flags);
	szrRight->Add(szrGapSize, flags);

	wxBoxSizer *szrRealFile = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Real file")), wxVERTICAL);
	chkSplit = new wxCheckBox(this, IDC_CHK_SPLIT, _("Split files per program"));
	chkMHead = new wxCheckBox(this, IDC_CHK_MHEAD, _("Trim header and footer in machine code"));

	szrRealFile->Add(chkSplit, flags);
	szrRealFile->Add(chkMHead, flags);

	szrRealFile->Add(new wxStaticText(this, wxID_ANY, _("(Enable when exporting from tape image.)")), flags);

	szrRight->Add(szrRealFile, flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK|wxCANCEL);
	szrMain->Add(szrLeft, flags);
	szrMain->Add(szrRight, flags);
	szrAll->Add(szrMain, flags);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int ConfigBox::ShowModal()
{
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void ConfigBox::init_dialog()
{
		chkDblFsk->SetValue(param.GetFskSpeed() != 0);

		for(int n=0; n<2; n++) {
			spinRange[n]->SetRange(1, 100);
			spinRange[n]->SetValue(param.GetRange(n));
		}

		for(int n=0; n<3; n++) {
			spinFreq[n]->SetRange((1200 << n) - 600, (1200 << n) + 600);
			spinFreq[n]->SetValue(param.GetFreq(n));
		}

		chkReverse->SetValue(param.GetReverseWave());
		chkHalfwave->SetValue(param.GetHalfWave());

		radNoCorrect->SetValue(param.GetCorrectType() == 0);
		radCorrCosw->SetValue(param.GetCorrectType() == 1);
		radCorrSinw->SetValue(param.GetCorrectType() == 2);

		spinCorrAmp[0]->SetRange(100, 5000);
		spinCorrAmp[0]->SetIncrement(100);
		spinCorrAmp[0]->SetValue(param.GetCorrectAmp(0));
		spinCorrAmp[1]->SetRange(100, 5000);
		spinCorrAmp[1]->SetIncrement(100);
		spinCorrAmp[1]->SetValue(param.GetCorrectAmp(1));

		//
		comSRate->Select(param.GetSampleRatePos());
		comSBits->Select(param.GetSampleBitsPos());

		//
		radBaud600->SetValue(false);
		radBaud1200->SetValue(false);
		radBaud2400->SetValue(false);
		radBaud300->SetValue(false);
		switch(param.GetBaud()) {
		case 0:
			radBaud600->SetValue(true);
			break;
		case 1:
			radBaud1200->SetValue(true);
			break;
		case 2:
			radBaud2400->SetValue(true);
			break;
		case 3:
			radBaud300->SetValue(true);
			break;
		}
		chkBaudAuto->SetValue(param.GetAutoBaud());

		//
		if ((param.GetWordSelect() & 0x04) == 0) {
			// 7bit
			radData7bit->SetValue(true);
			radData8bit->SetValue(false);
			radParityNo->Enable(false);
		} else {
			// 8bit
			radData7bit->SetValue(false);
			radData8bit->SetValue(true);
			radParityNo->Enable(true);
		}
		if ((param.GetWordSelect() & 0x02) == 0 && param.GetWordSelect() != 5) {
			radStop1bit->SetValue(false);
			radStop2bit->SetValue(true);
		} else {
			radStop1bit->SetValue(true);
			radStop2bit->SetValue(false);
		}

		radParityEven->Enable(true);
		radParityOdd->Enable(true);
		if (param.GetWordSelect() == 4 || param.GetWordSelect() == 5) {
			radParityNo->SetValue(true);
			radParityEven->SetValue(false);
			radParityOdd->SetValue(false);
			if (param.GetWordSelect() == 4) {
				radParityEven->Enable(false);
				radParityOdd->Enable(false);
			}
		} else if ((param.GetWordSelect() & 0x01) == 0) {
			radParityNo->SetValue(false);
			radParityEven->SetValue(true);
			radParityOdd->SetValue(false);
		} else {
			radParityNo->SetValue(false);
			radParityEven->SetValue(false);
			radParityOdd->SetValue(true);
		}

		chkOutErrSer->SetValue(param.GetOutErrSerial());

		chkSplit->SetValue(param.GetFileSplit() ? true : false);
		chkMHead->SetValue(param.GetDeleteMHead() ? true : false);

		chkGapSize->SetValue(param.GetChangeGapSize() ? true : false);

		comDebugLevel->Select(param.GetDebugMode());

		update_baud();
}

void ConfigBox::update_baud()
{
	int mag = chkDblFsk->GetValue() ? 2 : 1;
	radBaud600->SetLabel(wxString::Format(_T("%d"), 600 * mag));
	radBaud1200->SetLabel(wxString::Format(_T("%d"), 1200 * mag));
	radBaud2400->SetLabel(wxString::Format(_T("%d"), 2400 * mag));
	radBaud300->SetLabel(wxString::Format(_T("%d"), 300 * mag));

	stsCorr1200->SetLabel(wxString::Format(_T("%d"), 1200 * mag));
	stsCorr2400->SetLabel(wxString::Format(_T("%d"), 2400 * mag));
}

void ConfigBox::OnClickDblFsk(wxCommandEvent& event)
{
	update_baud();
}

void ConfigBox::OnClickCorrect(wxCommandEvent& event)
{
//	radCorrCosw->Enable(true);
//	radCorrSinw->Enable(true);
}

void ConfigBox::OnClick7bit(wxCommandEvent& event)
{
	if (radData7bit->GetValue()) {
		if (radParityNo->GetValue()) {
			radParityNo->SetValue(false);
			radParityEven->SetValue(true);
		}
		radParityNo->Enable(false);
		radParityEven->Enable(true);
		radParityOdd->Enable(true);
	}
}

void ConfigBox::OnClick8bit(wxCommandEvent& event)
{
	if (radData8bit->GetValue()) {
		radParityNo->Enable(true);
		if (radStop2bit->GetValue()) {
			radParityNo->SetValue(true);
			radParityEven->SetValue(false);
			radParityOdd->SetValue(false);
			radParityEven->Enable(false);
			radParityOdd->Enable(false);
		} else {
			radParityEven->Enable(true);
			radParityOdd->Enable(true);
		}
	}
}

void ConfigBox::OnClickStop1bit(wxCommandEvent& event)
{
	if (radStop1bit->GetValue()) {
		radParityEven->Enable(true);
		radParityOdd->Enable(true);
		if (radData8bit->GetValue()) {
			radParityNo->Enable(true);
		} else {
			radParityNo->Enable(false);
		}
	}
}

void ConfigBox::OnClickStop2bit(wxCommandEvent& event)
{
	if (radStop2bit->GetValue()) {
		if (radData8bit->GetValue()) {
			radParityNo->SetValue(true);
			radParityEven->SetValue(false);
			radParityOdd->SetValue(false);
			radParityNo->Enable(true);
			radParityEven->Enable(false);
			radParityOdd->Enable(false);
		} else {
			if (radParityNo->GetValue()) {
				radParityNo->SetValue(false);
				radParityEven->SetValue(true);
			}
			radParityNo->Enable(false);
			radParityEven->Enable(true);
			radParityOdd->Enable(true);
		}
	}
}

void ConfigBox::term_dialog()
{
	param.SetFskSpeed(chkDblFsk->GetValue() ? 1 : 0);

	for(int n=0; n<3; n++) {
		param.SetFreq(n, spinFreq[n]->GetValue());
	}

	for(int n=0; n<2; n++) {
		param.SetRange(n, spinRange[n]->GetValue());
	}

	param.SetReverseWave(chkReverse->GetValue());
	param.SetHalfWave(chkHalfwave->GetValue());

	if (radCorrCosw->GetValue()) param.SetCorrectType(1);
	else if (radCorrSinw->GetValue()) param.SetCorrectType(2);
	else param.SetCorrectType(0);

	param.SetCorrectAmp(0, spinCorrAmp[0]->GetValue());
	param.SetCorrectAmp(1, spinCorrAmp[1]->GetValue());

	param.SetSampleRatePos(comSRate->GetCurrentSelection());
	param.SetSampleBitsPos(comSBits->GetCurrentSelection());

	if (radBaud600->GetValue()) {
		param.SetBaud(0);
	} else if (radBaud1200->GetValue()) {
		param.SetBaud(1);
	} else if (radBaud2400->GetValue()) {
		param.SetBaud(2);
	} else {
		param.SetBaud(3);
	}
	param.SetAutoBaud(chkBaudAuto->GetValue());

	int word_select = 0;
	if (radData8bit->GetValue()) {
		word_select |= 0x04;
	}
	if (radStop1bit->GetValue()) {
		word_select |= 0x02;
	}
	if (radParityOdd->GetValue()) {
		word_select |= 0x01;
	}
	if (radData8bit->GetValue()
		&& radParityNo->GetValue()) {
		if (radStop1bit->GetValue()) {
			word_select = 5;
		} else {
			word_select = 4;
		}
	}
	param.SetWordSelect(word_select);

	param.SetOutErrSerial(chkOutErrSer->GetValue());

	param.SetDebugMode(comDebugLevel->GetCurrentSelection());

	param.SetFileSplit(chkSplit->GetValue() ? 1 : 0);
	param.SetDeleteMHead(chkMHead->GetValue() ? 1 : 0);

	param.SetChangeGapSize(chkGapSize->GetValue() ? 1 : 0);
}
