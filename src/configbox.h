/// @file configbox.h
///
/// configbox.h
///


#ifndef _CONFIGBOX_H_
#define _CONFIGBOX_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include "paw_param.h"


/// 設定ダイアログ
class ConfigBox : public wxDialog
{
private:
	PARSEWAV::Parameter param;

	wxCheckBox *chkDblFsk;

	wxSpinCtrl *spinFreq[3];
	wxSpinCtrl *spinRange[2];

	wxCheckBox *chkReverse;
	wxCheckBox *chkHalfwave;

	wxRadioButton *radNoCorrect;
	wxRadioButton *radCorrCosw;
	wxRadioButton *radCorrSinw;

	wxStaticText *stsCorr1200;
	wxStaticText *stsCorr2400;
	wxSpinCtrl *spinCorrAmp[2];

	wxComboBox *comSRate;
	wxComboBox *comSBits;

	wxRadioButton *radBaud600;
	wxRadioButton *radBaud1200;
	wxRadioButton *radBaud2400;
	wxRadioButton *radBaud300;
	wxCheckBox *chkBaudAuto;

	wxRadioButton *radData7bit;
	wxRadioButton *radData8bit;
	wxRadioButton *radParityNo;
	wxRadioButton *radParityEven;
	wxRadioButton *radParityOdd;
	wxRadioButton *radStop1bit;
	wxRadioButton *radStop2bit;
	wxCheckBox *chkOutErrSer;

	wxCheckBox *chkGapSize;

	wxCheckBox *chkSplit;
	wxCheckBox *chkMHead;

	wxComboBox *comDebugLevel;

//	int ranges[3];
//	int freqs[3];

	void init_dialog();
	void term_dialog();
	void update_baud();

public:
	ConfigBox(wxWindow* parent, wxWindowID id);

	enum {
		IDC_CHK_DBL_FSK = 1,
		IDC_SPIN_FREQ1200,
		IDC_SPIN_FREQ2400,
		IDC_SPIN_FREQ4800,
		IDC_SPIN_RANGE1200,
		IDC_SPIN_RANGE2400,
		IDC_SPIN_RANGE4800,
		IDC_CHK_REVERSE,
		IDC_CHK_HALFWAVE,
		IDC_RADIO_NOCORRECT,
		IDC_RADIO_COSW,
		IDC_RADIO_SINW,
		IDC_STATIC1200,
		IDC_STATIC2400,
		IDC_SPIN_CORRAMP1200,
		IDC_SPIN_CORRAMP2400,

		IDC_RADIO_BAUD_600,
		IDC_RADIO_BAUD_1200,
		IDC_RADIO_BAUD_2400,
		IDC_RADIO_BAUD_300,
		IDC_CHK_BAUDAUTO,

		IDC_COMBO_SRATE,
		IDC_COMBO_SBITS,


		IDC_RADIO_7BIT,
		IDC_RADIO_8BIT,

		IDC_RADIO_PARITY_NO,
		IDC_RADIO_PARITY_EVEN,
		IDC_RADIO_PARITY_ODD,

		IDC_RADIO_STOP1BIT,
		IDC_RADIO_STOP2BIT,

		IDC_CHK_OUTERRSER,

		IDC_CHK_CHGGAP,

		IDC_CHK_SPLIT,
		IDC_CHK_MHEAD,

		IDC_COMBO_DEBUG,

	};

	// functions
	int ShowModal();

	// event procedures
	void OnClickDblFsk(wxCommandEvent& event);
	void OnClickCorrect(wxCommandEvent& event);
	void OnClick7bit(wxCommandEvent& event);
	void OnClick8bit(wxCommandEvent& event);
	void OnClickStop1bit(wxCommandEvent& event);
	void OnClickStop2bit(wxCommandEvent& event);

	// properties
	PARSEWAV::Parameter &GetParam() { return param; }
	const PARSEWAV::Parameter &GetParam() const { return param; }
	void SetParam(const PARSEWAV::Parameter &data) { param = data; }

	DECLARE_EVENT_TABLE()
};

#endif /* _CONFIGBOX_H_ */
