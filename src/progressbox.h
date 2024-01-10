/// @file progressbox.h
///
/// progressbox.h
///


#ifndef _PROGRESSBOX_H_
#define _PROGRESSBOX_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/progdlg.h>
#include <wx/stopwatch.h>
#include <wx/datetime.h>

//#define USE_DATETIME_PROGRESS 1

/// 処理中プログレスダイアログ
class ProgressBox
{
private:
	wxProgressDialog *dlg;
	wxWindow *parent_window;

	bool cancel_button;
	long max_value;

#ifdef USE_DATETIME_PROGRESS
	wxDateTime current_time;
#else
	wxStopWatch swatch;
#endif

	ProgressBox();

public:
	ProgressBox(wxWindow *parent);
	~ProgressBox();

	void initProgress(int type, int min_val, int max_val);
	bool needSetProgress() const;
	bool setProgress(int val);
	bool setProgress(int num, int div);
	bool incProgress();
	bool viewProgress();
	void endProgress();
};

#endif /* _PROGRESSBOX_H_ */
