/// @file progressbox.cpp
///
/// @brief プログレスダイアログ
///
#include "progressbox.h"

ProgressBox::ProgressBox()
{
	ProgressBox(NULL);
}

ProgressBox::ProgressBox(wxWindow *parent)
{
	dlg = NULL;
	parent_window = parent;
	cancel_button = false;

#ifdef USE_DATETIME_PROGRESS
	current_time = wxDateTime::Now();
#else
	swatch.Pause();
#endif
}

ProgressBox::~ProgressBox()
{
	endProgress();
}

void ProgressBox::initProgress(int type, int min_val, int max_val)
{
	wxString title;
	if (dlg == NULL) {
		max_value = (max_val - min_val);
		// 変換中...
		if (type == 1) {
			title = _("Analyzing...");
		} else {
			title = _("Converting...");
		}
		dlg = new wxProgressDialog(title, title, (int)(max_value + 1), parent_window, wxPD_CAN_ABORT | wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH);
		wxSize sz = dlg->GetSize();
		sz.SetWidth(sz.GetWidth() + 100);
		dlg->SetSize(sz);
		dlg->Update(0);
		dlg->Show();

#ifdef USE_DATETIME_PROGRESS
		current_time = wxDateTime::Now();
#else
		swatch.Start();
#endif

		cancel_button = false;
	}
}

bool ProgressBox::needSetProgress() const
{
#ifdef USE_DATETIME_PROGRESS
	wxDateTime now = wxDateTime::Now();
	if (now.GetTicks() - current_time.GetTicks() < 1 && now.GetMillisecond() - current_time.GetMillisecond() < 500) {
		return false;
	}
	return true;
#else
	return (swatch.Time() >= 500);
#endif
}

bool ProgressBox::setProgress(int val)
{
	if (dlg != NULL) {
		if (val > max_value) val = (int)max_value;
		if (val < 0) val = 0;
		cancel_button = dlg->Update(val) ? false : true;

#ifdef USE_DATETIME_PROGRESS
		current_time = wxDateTime::Now();
#else
		swatch.Start();
#endif
	}
	return cancel_button;
}

bool ProgressBox::setProgress(int num, int div)
{
	if (dlg != NULL) {
		int val = (int)((double)num * max_value / div);

		if (val > max_value) val = (int)max_value;
		if (val < 0) val = 0;
		cancel_button = dlg->Update(val) ? false : true;

#ifdef USE_DATETIME_PROGRESS
		current_time = wxDateTime::Now();
#else
		swatch.Start();
#endif
	}
	return cancel_button;
}

bool ProgressBox::incProgress()
{
	if (dlg != NULL) {
		cancel_button = dlg->Pulse() ? false : true;

#ifdef USE_DATETIME_PROGRESS
		current_time = wxDateTime::Now();
#else
		swatch.Start();
#endif
	}
	return cancel_button;
}

bool ProgressBox::viewProgress()
{
	return cancel_button;
}

void ProgressBox::endProgress()
{
	if (dlg != NULL) {
		dlg->Hide();

		delete dlg;
		dlg = NULL;

#ifndef USE_DATETIME_PROGRESS
		swatch.Pause();
#endif
	}
}
