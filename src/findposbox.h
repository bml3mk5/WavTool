/// @file findposbox.h
///
/// @brief 表示位置検索ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _FINDPOSBOX_H_
#define _FINDPOSBOX_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>


/// 表示位置検索ダイアログ
class FindPosBox : public wxDialog
{
private:
	wxRadioButton *radSec;
	wxRadioButton *radPos;
	wxSpinCtrl *spinSec[3];
	wxSpinCtrl *spinPos;

public:
	FindPosBox();
	FindPosBox(wxWindow* parent, wxWindowID id, uint32_t msec, int spos);

	enum {
		IDC_RADIO_SEC = 1,
		IDC_RADIO_POS,
		IDC_SPIN_SEC0,
		IDC_SPIN_SEC1,
		IDC_SPIN_SEC2,
		IDC_SPIN_POS,
	};

	void OnChangeRadio(wxCommandEvent &event);

	void ChangeRadio();

	bool IsSelectedSec() const;
	uint32_t GetMSec() const;
	int  GetSPos() const;

	DECLARE_EVENT_TABLE()
};

#endif /* _FINDPOSBOX_H_ */
