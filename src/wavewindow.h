/// @file wavewindow.h
///
/// @brief 波形ウィンドウ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _WAVEWINDOW_H_
#define _WAVEWINDOW_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/scrolwin.h>
#include "parsewav.h"


using namespace PARSEWAV;

#define SCROLLBAR_UNIT	8

class WavePanel;
class SampleDrawer;

/// 波形ウィンドウフレーム
class WaveFrame : public wxFrame
{
private:
	wxMenu *menuFile;
	wxMenu *menuView;

	WavePanel *panel;

public:
	WaveFrame(wxWindow* parent, wxWindowID id, ParseWav *wav);

	void OnClose(wxCommandEvent &event);
	void OnUpdate(wxCommandEvent &event);
	void OnReload(wxCommandEvent &event);
	void OnFind(wxCommandEvent &event);
	void OnZoomIn(wxCommandEvent &event);
	void OnZoomOut(wxCommandEvent &event);
	void OnChangeMeasure(wxCommandEvent &event);

	void OnUpdateMenu(wxMenuEvent &event);

	void UpdateAll(bool first = true);

	void SuspendDrawing();
	void ResumeDrawing();

	enum {
		IDM_UPDATE = 1,
		IDM_RELOAD,
		IDM_FIND,
		IDM_ZOOMIN,
		IDM_ZOOMOUT,
		IDM_VIEW_TIME,
		IDM_VIEW_SPOS,
	};

	DECLARE_EVENT_TABLE()
};

/// 波形ウィンドウパネル
class WavePanel : public wxScrolledWindow
{
private:
	// マウス操作
	wxPoint pt_mouse;

	ParseWav *wav;

	InputFile *file;
	WaveData *w_data[2];
	CarrierData *c_data;
	SerialData *s_data;
	SerialData *sn_data;
	BinaryData *b_data;

	int sample_num;
	int correct_type;

	double wmagnify;	///< ウィンドウの表示倍率
	double amagnify;	///< サンプリング1つに対する表示倍率

	int need_parse;
	int reopened;	// 入力ファイルが変わったか

	int measure_type;

	bool suspending;

	void SetScrollBarPos(int new_ux, int new_uy, int new_px, int new_py);
	void RecalcScrollBarPos(int num, int div);
	void OnDraw(wxDC &dc);

	CSampleArray *SelectAData(enum_file_type type);
	double SelectMeasureMagnify(enum_file_type type, int measure_type);

public:
	WavePanel(wxWindow* parent, wxWindowID id,	ParseWav *wav);

//	void OnPaint(wxPaintEvent &event);

	void ZoomIn();
	void ZoomOut();
	bool CanZoomIn() const;
	bool CanZoomOut() const;
	void Find(bool use_msec, uint32_t sample_msec, int sample_spos);

	void SetSampleNum(int num) { sample_num = num; }
	void NeedParse(bool first) { need_parse = first ? 2 : 3; }

	void ChangeMeasure(int num) { measure_type = (num & 1); }
	int GetCurrentMeasure() const { return measure_type; }

	void SuspendDrawing() { suspending = true; }
	void ResumeDrawing() { suspending = false; }

	void ScrollArea(int x, int y);
	void OnMouseLeftDown(wxMouseEvent &event);
	void OnMouseLeftUp(wxMouseEvent &event);
	void OnMouseMove(wxMouseEvent &event);
	void OnMouseWheel(wxMouseEvent& event);

	DECLARE_EVENT_TABLE()
};

/// データ描画クラス
class SampleDrawer
{
protected:
	CSampleArray *m_a_data; 
	CSampleArray *m_data;
	int m_data_pos;

	double m_xmag;
	double m_show_tmag;
	wxCoord m_ybase;
	wxCoord m_left;
	wxCoord m_right;
	wxCoord m_height;

	virtual void DrawOneX(wxDC &dc, int a_data_spos, wxCoord x);
	virtual void DrawOnePos(wxDC &dc, int data_spos, int a_data_spos, wxCoord x);

public:
	SampleDrawer(CSampleArray *a_data, CSampleArray *data, wxCoord left, wxCoord right, double xmag, double show_tmag, wxCoord ybase, wxCoord height);
	virtual ~SampleDrawer();

	virtual void Draw(wxDC &dc, int a_start_pos, int a_exp);
};

/// データ描画クラス
class FirstSampleDrawer : public SampleDrawer
{
protected:
	virtual void DrawOneX(wxDC &dc, int a_data_spos, wxCoord x);
public:
	FirstSampleDrawer(CSampleArray *data, wxCoord left, wxCoord right, double xmag, double show_tmag, wxCoord ybase, wxCoord height);

	virtual void Draw(wxDC &dc, int a_start_pos, int a_exp);
};

/// 波形描画クラス
class WaveDrawer : public FirstSampleDrawer
{
protected:
	bool m_correct;
	wxPoint m_prev_pt;
	bool m_first_point;

	virtual void DrawOneX(wxDC &dc, int a_data_spos, wxCoord x);
public:
	WaveDrawer(CSampleArray *data, wxCoord left, wxCoord right, double xmag, double show_tmag, wxCoord ybase, wxCoord height, bool correct);
};

/// バイナリデータ描画クラス
class BinaryDrawer : public SampleDrawer
{
protected:
	virtual void DrawOnePos(wxDC &dc, int data_spos, int a_data_spos, wxCoord x);
public:
	BinaryDrawer(CSampleArray *a_data, CSampleArray *data, wxCoord left, wxCoord right, double xmag, double show_tmag, wxCoord ybase, wxCoord height);
};


#endif /* _WAVEWINDOW_H_ */
