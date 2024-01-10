/// @file wavewindow.cpp
///
/// @brief 波形ウィンドウ
///

#include "wavewindow.h"
#include "findposbox.h"
#include "utils.h"

// Attach Event
BEGIN_EVENT_TABLE(WaveFrame, wxFrame)
	EVT_MENU( wxID_CLOSE, WaveFrame::OnClose )
	EVT_MENU( IDM_UPDATE, WaveFrame::OnUpdate )
	EVT_MENU( IDM_RELOAD, WaveFrame::OnReload )
	EVT_MENU( IDM_FIND, WaveFrame::OnFind )
	EVT_MENU( IDM_ZOOMIN, WaveFrame::OnZoomIn )
	EVT_MENU( IDM_ZOOMOUT, WaveFrame::OnZoomOut )
	EVT_MENU_RANGE(IDM_VIEW_TIME, IDM_VIEW_SPOS, WaveFrame::OnChangeMeasure )
	EVT_MENU_OPEN( WaveFrame::OnUpdateMenu )
END_EVENT_TABLE()

/// 波形ウィンドウフレーム
WaveFrame::WaveFrame(wxWindow* parent, wxWindowID id, ParseWav *wav)
	: wxFrame(parent, id, _("Wave"), wxDefaultPosition, wxSize(640,400), wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX)
{
	// menu
	menuFile = new wxMenu;
	menuFile->Append( wxID_CLOSE, _("&Close") );
	menuView = new wxMenu;
	menuView->Append( IDM_ZOOMIN, _("Zoom &In") );
	menuView->Append( IDM_ZOOMOUT, _("Zoom &Out") );
	menuView->AppendSeparator();
	menuView->Append( IDM_FIND, _("&Find...") );
	menuView->AppendSeparator();
	menuView->Append( IDM_UPDATE, _("&Update") );
	menuView->AppendSeparator();
	menuView->Append( IDM_RELOAD, _("&Reload from the Beginning") );
	menuView->AppendSeparator();
	menuView->AppendRadioItem( IDM_VIEW_TIME, _("Show in &Milli Second") );
	menuView->AppendRadioItem( IDM_VIEW_SPOS, _("Show in &Sample Position") );
	// menu bar
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, _("&File") );
	menuBar->Append( menuView, _("&View") );

	SetMenuBar( menuBar );

	// panel
	panel = new WavePanel(this, wxID_ANY, wav);
}

void WaveFrame::OnClose(wxCommandEvent &event)
{
	Close();
}

void WaveFrame::OnFind(wxCommandEvent &event)
{
	FindPosBox box(this, wxID_ANY, 0, 0);

	if (box.ShowModal() != wxID_OK) return;

	if (panel) {
		panel->Find(box.IsSelectedSec(), box.GetMSec(), box.GetSPos());
		panel->Refresh();
	}
}

void WaveFrame::OnUpdate(wxCommandEvent &event)
{
	if (panel) {
		panel->Refresh();
	}
}
void WaveFrame::OnReload(wxCommandEvent &event)
{
	Update(true);
}
void WaveFrame::OnZoomIn(wxCommandEvent &event)
{
	if (panel) {
		panel->ZoomIn();
	}
}
void WaveFrame::OnZoomOut(wxCommandEvent &event)
{
	if (panel) {
		panel->ZoomOut();
	}
}

void WaveFrame::OnChangeMeasure(wxCommandEvent &event)
{
	if (panel) {
		panel->ChangeMeasure(event.GetId() - IDM_VIEW_TIME);
		panel->Refresh();
	}
}

void WaveFrame::OnUpdateMenu(wxMenuEvent &event)
{
	int cm = 0;
	bool zi = false;
	bool zo = false;
	if (panel) {
		cm = panel->GetCurrentMeasure();
		zi = panel->CanZoomIn();
		zo = panel->CanZoomOut();
	}
	wxMenuItem *mi;
	mi = menuView->FindChildItem(IDM_VIEW_TIME + cm);
	mi->Check(true);
	mi = menuView->FindChildItem(IDM_ZOOMIN);
	mi->Enable(zi);
	mi = menuView->FindChildItem(IDM_ZOOMOUT);
	mi->Enable(zo);
}

void WaveFrame::Update(bool first)
{
	if (panel) {
		panel->NeedParse(first);
		panel->Refresh();
	}
	wxFrame::Update();
}


// Attach Event
BEGIN_EVENT_TABLE(WavePanel, wxScrolledWindow)
//	EVT_PAINT(WavePanel::OnPaint)
//	EVT_SCROLLWIN_BOTTOM(WavePanel::OnScrollBottom)
	EVT_LEFT_DOWN( WavePanel::OnMouseLeftDown )
	EVT_LEFT_UP( WavePanel::OnMouseLeftUp )
	EVT_MOTION( WavePanel::OnMouseMove )
	EVT_MOUSEWHEEL( WavePanel::OnMouseWheel )
END_EVENT_TABLE()

/// 波形ウィンドウパネル
WavePanel::WavePanel(wxWindow* parent, wxWindowID id, ParseWav *wav)
	: wxScrolledWindow(parent, id)
{
	pt_mouse.x = 0;
	pt_mouse.y = 0;

	wmagnify = 1.0;

	sample_num = 0;
	correct_type = 0;

	need_parse = 2;
	reopened = -1;

	measure_type = 0;

	SetWindowStyle(wxHSCROLL);

	SetScrollBarPos(0, 0, 0, 0);
	EnableScrolling(true, true);
	ShowScrollbars(wxSHOW_SB_ALWAYS, wxSHOW_SB_DEFAULT);

	this->wav = wav;

	file = wav->GetDataFile();

	w_data[0] = wav->GetWaveData();
	w_data[1] = wav->GetWaveCorrectData();
	c_data = wav->GetCarrierData();
	s_data = wav->GetSerialData();
	sn_data = wav->GetSerialNewData();
	b_data = wav->GetBinaryData();
}

/// スクロールバーを設定
void WavePanel::SetScrollBarPos(int new_ux, int new_uy, int new_px, int new_py)
{
	int ux, uy, px, py, sx, sy;
	GetVirtualSize(&ux, &uy);
	GetViewStart(&px, &py);
	px *= SCROLLBAR_UNIT;
	py *= SCROLLBAR_UNIT;
	GetClientSize(&sx, &sy);
	if (new_ux < sx) new_ux = sx;
	if (new_uy < sy) new_uy = sy;
	if (ux != new_ux || uy != new_uy || px != new_px || py != new_py) {
		SetScrollbars(SCROLLBAR_UNIT, SCROLLBAR_UNIT
			, new_ux / SCROLLBAR_UNIT, new_uy / SCROLLBAR_UNIT
			, new_px / SCROLLBAR_UNIT, new_py / SCROLLBAR_UNIT, true);
	}
}

/// マウス左ボタン押した
void WavePanel::OnMouseLeftDown(wxMouseEvent &event)
{
	pt_mouse = event.GetPosition();
}

/// マウス左ボタン離した
void WavePanel::OnMouseLeftUp(wxMouseEvent &event)
{
}

/// マウス移動
void WavePanel::OnMouseMove(wxMouseEvent &event)
{
	if (event.LeftIsDown()) {
		wxPoint pt = event.GetPosition();
		wxPoint delta = pt - pt_mouse;
		if (delta.x != 0 || delta.y != 0) {
			ScrollArea(- delta.x, delta.y);
		}
		pt_mouse = pt;
	}
}

/// マウスホイール
void WavePanel::OnMouseWheel(wxMouseEvent& event)
{
	int delta = event.GetWheelRotation();
	if (delta > 0) ZoomIn();
	else if (delta < 0) ZoomOut();
}

/// 移動
void WavePanel::ScrollArea(int x, int y)
{
	wxPoint pos = GetViewStart();
	pos.x *= SCROLLBAR_UNIT;
	pos.y *= SCROLLBAR_UNIT;
	Scroll((pos.x + x) / SCROLLBAR_UNIT, (pos.y + y) / SCROLLBAR_UNIT);

	Refresh();
}

/// 拡大
void WavePanel::ZoomIn()
{
	bool zoomd = false;
	if (CanZoomIn()) {
		wmagnify *= 2.0;
		zoomd = true;
	}
	if (zoomd) {
		RecalcScrollBarPos(2, 1);
		Refresh();
	}
}
/// 縮小
void WavePanel::ZoomOut()
{
	bool zoomd = false;
	if (CanZoomOut()) {
		wmagnify /= 2.0;
		zoomd = true;
	}
	if (zoomd) {
		RecalcScrollBarPos(1, 2);
		Refresh();
	}
}
/// 拡大できるか
bool WavePanel::CanZoomIn() const
{
	return (wmagnify < 4.0);
}
/// 縮小できるか
bool WavePanel::CanZoomOut() const
{
	return (wmagnify > 0.25);
}

/// スルロールバーの位置を再計算
void WavePanel::RecalcScrollBarPos(int num, int div)
{
	wxPoint pt_view;
	pt_view = GetViewStart();
	pt_view.x *= SCROLLBAR_UNIT;
	pt_view.y *= SCROLLBAR_UNIT;

	wxSize sz_window = GetClientSize();

	wxCoord vwindow_width = (wxCoord)((double)sample_num * wmagnify); // wnumerator / wdenominator;
	pt_view.x = pt_view.x * num / div;

	SetScrollBarPos(vwindow_width, sz_window.GetHeight(), pt_view.x, pt_view.y);
}
/// データ位置をさがす
void WavePanel::Find(bool use_msec, uint32_t sample_msec, int sample_spos)
{
	enum_file_type file_type = file->GetType();
	double amagnify = 1.0;

	CSampleArray *a_data = SelectAData(file_type, amagnify);

	bool viewing = (a_data != NULL);

	int ofc = wav->OpenedDataFileCount(NULL);
	if (!viewing || ofc < 0) {
		return;
	}

	if (use_msec) {
		sample_spos = (int)file->CalcrateSamplePos(sample_msec * 1000);
	}

	// 解析
	need_parse = 0;
	int first = 0;
	bool last = false;
	int find = -1;
	do {
		SetCursor(wxCursor(wxCURSOR_WAIT));
		wav->ViewData(first, sample_spos, a_data);
		reopened = ofc;
		first = 1;
		find = a_data->FindSPos(a_data->GetStartPos(), sample_spos);
		last = a_data->IsLastData();
	} while(find < 0 && !last);
	SetCursor(wxCursor(wxCURSOR_ARROW));

//	if (find < 0) {
//		return;
//	}

	wxPoint pt_view;
	pt_view = GetViewStart();
	pt_view.x *= SCROLLBAR_UNIT;
	pt_view.y *= SCROLLBAR_UNIT;

	wxSize sz_window = GetClientSize();

	if (find >= 0) {
		// find data
		pt_view.x = a_data->At(0).SPos(); // a_data->GetTotalWritePos() - a_data->GetWritePos();
		pt_view.x += find;
		if (pt_view.x < 0) {
			pt_view.x = 0;
		} else {
			pt_view.x *= wmagnify * amagnify;
		}
		pt_view.x -= (sz_window.GetWidth() / 2);
		if (pt_view.x < 0) {
			pt_view.x = 0;
		}
	} else {
		// no data found, so last data
		pt_view.x = a_data->GetWrite(-1).SPos();
		if (pt_view.x < 0) {
			pt_view.x = 0;
		} else {
			pt_view.x *= wmagnify * amagnify;
		}
		pt_view.x -= sz_window.GetWidth();
		if (pt_view.x < 0) {
			pt_view.x = 0;
		}
	}

	wxCoord vwindow_width = (wxCoord)((double)sample_num * wmagnify * amagnify);
	SetScrollBarPos(vwindow_width, sz_window.GetHeight(), pt_view.x, pt_view.y);
}

/// 入力ファイルのバッファと表示倍率を選択する
/// @param [in]  type     ファイル種類
/// @param [out] amagnify 倍率
/// @return サンプルデータ
CSampleArray *WavePanel::SelectAData(enum_file_type type, double &amagnify)
{
	CSampleArray *a_data = NULL;
	switch(type) {
	case FILETYPE_WAV:
		a_data = w_data[0];
		if (file->SampleRate() < 40000.0) {
			amagnify = 2.0;
		} else if (file->SampleRate() < 20000.0) {
			amagnify = 4.0;
		} else {
			amagnify = 1.0;
		}
		break;
	case FILETYPE_L3C:
		a_data = c_data;
		amagnify = 8.0;
		break;
	case FILETYPE_L3B:
	case FILETYPE_T9X:
		a_data = s_data;
		amagnify = 8.0;
		break;
	default:
		break;
	}
	return a_data;
}

/// 目盛りの表示間隔を選択する
/// @param [in] type ファイル種類
/// @param [in] measure_type 目盛りの種類 0:ミリ秒表示 1:サンプル位置表示
double WavePanel::SelectMeasureMagnify(enum_file_type type, int measure_type)
{
	double mmagnify = 1.0;
	switch(type) {
	case FILETYPE_WAV:
		if (file->SampleRate() < 40000.0) {
			mmagnify = (measure_type != 0 ? 5.0 : 2.0);
		} else if (file->SampleRate() < 20000.0) {
			mmagnify = (measure_type != 0 ? 10.0 : 4.0);
		} else {
			mmagnify = (measure_type != 0 ? 5.0 : 1.0);
		}
		break;
	case FILETYPE_L3C:
		mmagnify = (measure_type != 0 ? 0.5 : 1.0);
		break;
	case FILETYPE_L3B:
	case FILETYPE_T9X:
		mmagnify = (measure_type != 0 ? 0.5 : 16.0);
		break;
	default:
		break;
	}
	return mmagnify;
}

/// 画面描画
void WavePanel::OnDraw(wxDC &dc)
{
	wxPoint pt_view;
	pt_view = GetViewStart();
	pt_view.x *= SCROLLBAR_UNIT;
	pt_view.y *= SCROLLBAR_UNIT;

	// current window size
	wxSize sz_window = GetClientSize();

	wxPen bluedotpen(*wxBLUE, 1, wxPENSTYLE_DOT);

	enum_file_type file_type = file->GetType();
	double amagnify = 1.0;

	CSampleArray *a_data = SelectAData(file_type, amagnify);
	bool viewing = (a_data != NULL);

	double measure_magnify = SelectMeasureMagnify(file_type, measure_type);

	// ファイルが開いているか
	int ofc = wav->OpenedDataFileCount(&sample_num);
	if (!viewing || ofc < 0) {
		return;
	}

	// 解析が必要ならここで解析する
	do {
		if (ofc >= 0 && (need_parse != 0 || ofc != reopened)) {
			SetCursor(wxCursor(wxCURSOR_WAIT));
			int dir = (ofc != reopened || need_parse >= 2) ? 0 : need_parse;
			wav->ViewData(dir, (pt_view.x / wmagnify / amagnify), a_data);
			if (ofc != reopened || need_parse == 2) {
				pt_view.x = 0;
				pt_view.y = 0;
			}
			reopened = ofc;
		}
		need_parse = 0;
		// 次のデータの解析が必要か
		if (!a_data->IsLastData() && a_data->GetWrite(-1).SPos() < (pt_view.x / wmagnify / amagnify) + 16384) {
			// 先のデータがないので読み込みが必要
			need_parse = 1;
		} else if (a_data->At(0).SPos() > (pt_view.x / wmagnify / amagnify)) {
			// 戻りのデータがないので読み込みが必要
			need_parse = -1;
		}
	} while(need_parse != 0);
	SetCursor(wxCursor(wxCURSOR_ARROW));

	wxCoord vwindow_width = (wxCoord)((double)sample_num * wmagnify * amagnify);
	SetScrollBarPos(vwindow_width, sz_window.GetHeight(), pt_view.x, pt_view.y);

	wxCoord w_yamp = 100;
	wxCoord h = 8;
	wxCoord m_ybase = 0;
	wxCoord mw_bound = m_ybase + h + h + 2;
	wxCoord w_ybase = mw_bound + 2 + w_yamp;
	wxCoord c_ybase = w_ybase + w_yamp + 4 + h;
	wxCoord s_ybase = c_ybase + h + 4 + h;
	wxCoord sn_ybase = s_ybase + h + 4 + h;
	wxCoord b_ybase = sn_ybase + h + 4 + h;

	correct_type = wav->GetParam().GetCorrectType();

	int a_start_pos;
	bool invalid = false;
	a_start_pos = a_data->FindRevSPos(0, (int)((double)pt_view.x / wmagnify / amagnify));
	if (a_start_pos < 0) {
		invalid = true;
	}
//	a_start_pos = w_data_pos[0];

//	view_right += (wxCoord)(480.0 * wmagnify * amagnify); // wnumerator / wdenominator);
//	view_left -= (wxCoord)(480.0 * wmagnify * amagnify); // wnumerator / wdenominator);
	wxCoord view_right = pt_view.x + sz_window.GetWidth();
	wxCoord view_left  = pt_view.x;

//	int w_data_pos = 0;
//	int c_data_pos = 0;
//	int s_data_pos = 0;
//	int sn_data_pos = 0;
//	int b_data_pos = 0;

//	wxPoint w_data_pt(-1, -1);
//	wxPoint wc_data_pt(-1, -1);
//	wxPoint c_data_pt(-1, -1);
//	wxPoint s_data_pt(-1, -1);
//	wxPoint sn_data_pt(-1, -1);
//	wxPoint b_data_pt(-1, -1);

	dc.SetPen(*wxBLUE_PEN);
	dc.DrawLine(view_left, mw_bound, view_right, mw_bound);
	dc.DrawLine(view_left, w_ybase, view_right, w_ybase);
//	dc.DrawLine(view_left, c_ybase, view_right, c_ybase);
//	dc.DrawLine(view_left, s_ybase, view_right, s_ybase);
//	dc.DrawLine(view_left, sn_ybase, view_right, sn_ybase);

//	double xdiv;

	// 目盛りを書く
	const int c_measure_mspitch[] = { 1, 2, 5, 10, 20, 50, 100, 0 };

	int measure_mspitch = (int)(measure_magnify / wmagnify);
	for(int i=0; c_measure_mspitch[i] > 0; i++) {
		if (measure_mspitch < c_measure_mspitch[i]) {
			measure_mspitch = c_measure_mspitch[i];
			break;
		}
	}

	double measure_pitch;
	if (measure_type != 0) {
		// sample position
		measure_pitch = measure_mspitch * 10;
	} else {
		// mill second
		measure_pitch = file->CalcrateSamplePos(measure_mspitch * 1000);
	}
	double measure_xpitch = measure_pitch * amagnify * wmagnify;

	int measure_npitch = (int)((double)pt_view.x / measure_xpitch);

	double measure_left = (double)measure_npitch * measure_xpitch;
	double measure_right = pt_view.x + sz_window.GetWidth();
	int measure_msec = measure_npitch * measure_mspitch;

	dc.SetPen(bluedotpen);

	for(double dx = measure_left; dx < measure_right;) {
		wxCoord x = (wxCoord)(dx + 0.5);
		dc.DrawLine(x, m_ybase, x, w_ybase + w_yamp);
		if (measure_type != 0) {
			// sample position
			dc.DrawText(wxString::Format(wxT("%d"), measure_msec * 10), x, m_ybase);
		} else {
			// mill second
			dc.DrawText(UTILS::get_time_str(measure_msec * 1000), x, m_ybase);
		}
		measure_msec += measure_mspitch;
		dx += measure_xpitch;
	}

	if (invalid) {
		return;
	}

	dc.SetBackground(*wxWHITE_BRUSH);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.SetPen(*wxBLACK_PEN);

	int apitch = (int)((double)pt_view.x / wmagnify / amagnify);
	apitch *= (wmagnify * amagnify);

//	view_left += (apitch - pt_view.x);
//	int a_data_pos;
	int a_exp = 2;

	if (file_type == FILETYPE_WAV) {
		WaveDrawer drawer(w_data[0], apitch, view_right, wmagnify * amagnify, w_ybase, w_yamp, false);
		drawer.Draw(dc, a_start_pos, a_exp);
		if (correct_type > 0) {
			WaveDrawer drawer(w_data[1], apitch, view_right, wmagnify * amagnify, w_ybase, w_yamp, true);
			drawer.Draw(dc, a_start_pos, a_exp);
		}
	}

	if (file_type == FILETYPE_L3C) {
		FirstSampleDrawer drawer(c_data, apitch, view_right, wmagnify * amagnify, c_ybase, h);
		drawer.Draw(dc, a_start_pos, a_exp);
	} else if (file_type < FILETYPE_L3C) {
		a_exp *= w_data[0]->GetRate() / c_data->GetRate();
		SampleDrawer drawer(a_data, c_data, apitch, view_right, wmagnify * amagnify, c_ybase, h);
		drawer.Draw(dc, a_start_pos, a_exp);
	}

	if (file_type == FILETYPE_L3B || file_type == FILETYPE_T9X) {
		FirstSampleDrawer drawer(s_data, apitch, view_right, wmagnify * amagnify, s_ybase, h);
		drawer.Draw(dc, a_start_pos, a_exp);
	} else if (file_type < FILETYPE_L3B) {
		a_exp *= 4;
		SampleDrawer drawer(a_data, s_data, apitch, view_right, wmagnify * amagnify, s_ybase, h);
		drawer.Draw(dc, a_start_pos, a_exp);
	}

	a_exp *= 4;
	SampleDrawer sn_drawer(a_data, sn_data, apitch, view_right, wmagnify * amagnify, sn_ybase, h);
	sn_drawer.Draw(dc, a_start_pos, a_exp);

	a_exp *= 4;
	BinaryDrawer b_drawer(a_data, b_data, apitch, view_right, wmagnify * amagnify, b_ybase, h);
	b_drawer.Draw(dc, a_start_pos, a_exp);
}

/// @param [in] a_data 元になるデータ
/// @param [in] data   描画対象データ
/// @param [in] left   X軸の左端
/// @param [in] right  X軸の右端
/// @param [in] xmag   X軸の倍率
/// @param [in] ybase  Y軸の描画中心
/// @param [in] height Y軸の描画範囲(ybase±heightが範囲)
SampleDrawer::SampleDrawer(CSampleArray *a_data, CSampleArray *data, wxCoord left, wxCoord right, double xmag, wxCoord ybase, wxCoord height)
{
	m_a_data = a_data;
	m_data = data;
	m_data_pos = 0;
	m_left = left;
	m_right = right;
	m_xmag = xmag;
	m_ybase = ybase;
	m_height = height;
}
SampleDrawer::~SampleDrawer()
{
}
/// 描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] a_start_pos データ取得開始位置
/// @param [in] a_exp       ウィンドウ左右のマージン
void SampleDrawer::Draw(wxDC &dc, int a_start_pos, int a_exp)
{
	double xdiv = 0.0;
	int a_data_pos = a_start_pos - a_exp;
	wxCoord view_left = m_left - a_exp * m_xmag;
	wxCoord view_right = m_right + a_exp * m_xmag;
	for(wxCoord x = view_left; x < view_right;) {
		if (a_data_pos >= 0 && a_data_pos < m_a_data->GetWritePos()) {
			const CSampleData *d = &m_a_data->At(a_data_pos);
			int a_data_spos = d->SPos();
			if (a_data_spos >= 0) {
				DrawOneX(dc, a_data_spos, x);
			}
		}
		a_data_pos++;

		xdiv += m_xmag;
		if (xdiv >= 1.0) {
			x += (wxCoord)xdiv;
			xdiv = 0.0;
		}
	}
}
/// １座標に入るサンプルデータを描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] a_data_spos 元データのサンプリング位置
/// @param [in] x           X座標
void SampleDrawer::DrawOneX(wxDC &dc, int a_data_spos, wxCoord x)
{
	while(m_data_pos < m_data->GetWritePos()) {
		int data_spos = m_data->At(m_data_pos).SPos();
		if (data_spos == a_data_spos) {
			DrawOnePos(dc, data_spos, a_data_spos, x);
			break;
		} else if (data_spos > a_data_spos) {
			break;
		}
		m_data_pos++;
	}

	dc.SetTextForeground(*wxBLACK);
}
/// １サンプルデータを描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] data_spos   サンプリング位置
/// @param [in] a_data_spos 元データのサンプリング位置
/// @param [in] x           X座標
void SampleDrawer::DrawOnePos(wxDC &dc, int data_spos, int a_data_spos, wxCoord x)
{
	wxString str;
	int dir = m_data->At(m_data_pos).Data() & 1 ? -1 : 1;
	bool err = (m_data->At(m_data_pos).Err() != 0);
	bool tail = false;
	do {
		str += m_data->At(m_data_pos).Data();
		m_data_pos++;
		if (m_data_pos >= m_data->GetWritePos()) {
			tail = true;
			break;
		}
		data_spos = m_data->At(m_data_pos).SPos();
	} while(data_spos == a_data_spos);

	double next_x = 0.0;
	if (!tail) {
		next_x = m_xmag * (data_spos - a_data_spos) + x;
	} else {
		next_x = m_right;
	}

	wxCoord y = m_ybase + m_height * dir;
	dc.DrawLine(x, m_ybase - m_height, x, m_ybase + m_height);
	dc.DrawLine(x, y, next_x, y);

//	if (prev_pt.x < 0 && prev_pt.y < 0) {
//		dc.DrawPoint(x, y);
//		dc.DrawLine(x, ybase - h, x, ybase + h);
//	} else {
//	}
//	prev_pt.x = x;
//	prev_pt.y = y;
	dc.SetTextForeground(err ? *wxRED : *wxBLACK);
	dc.DrawText(str, x, m_ybase - m_height);
}

//

/// @param [in] data   描画対象データ
/// @param [in] left   X軸の左端
/// @param [in] right  X軸の右端
/// @param [in] xmag   X軸の倍率
/// @param [in] ybase  Y軸の描画中心
/// @param [in] height Y軸の描画範囲(ybase±heightが範囲)
FirstSampleDrawer::FirstSampleDrawer(CSampleArray *data, wxCoord left, wxCoord right, double xmag, wxCoord ybase, wxCoord height)
	: SampleDrawer(NULL, data, left, right, xmag, ybase, height)
{
}
/// 描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] a_start_pos データ取得開始位置
/// @param [in] a_exp       ウィンドウ左右のマージン
void FirstSampleDrawer::Draw(wxDC &dc, int a_start_pos, int a_exp)
{
	double xdiv = 0.0;
	m_data_pos = a_start_pos - a_exp;
	wxCoord view_left = m_left - a_exp * m_xmag;
	wxCoord view_right = m_right + a_exp * m_xmag;
	for(wxCoord x = view_left; x < view_right;) {
		int prev_pos = m_data_pos;
		if (m_data_pos >= 0 && m_data_pos < m_data->GetWritePos()) {
			const CSampleData *d = &m_data->At(m_data_pos);
			int a_data_spos = d->SPos();
			if (a_data_spos >= 0) {
				DrawOneX(dc, a_data_spos, x);
			} else {
				m_data_pos++;
			}
		} else {
			m_data_pos++;
		}

		xdiv += m_xmag * (m_data_pos - prev_pos);
		if (xdiv >= 1.0) {
			x += (wxCoord)xdiv;
			xdiv = 0.0;
		}
	}
}
/// １座標に入るサンプルデータを描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] a_data_spos 元データのサンプリング位置
/// @param [in] x           X座標
void FirstSampleDrawer::DrawOneX(wxDC &dc, int a_data_spos, wxCoord x)
{
	int data_spos = m_data->At(m_data_pos).SPos();
	DrawOnePos(dc, data_spos, a_data_spos, x);

	dc.SetTextForeground(*wxBLACK);
}

//

WaveDrawer::WaveDrawer(CSampleArray *data, wxCoord left, wxCoord right, double xmag, wxCoord ybase, wxCoord height, bool correct)
	: FirstSampleDrawer(data, left, right, xmag, ybase, height)
{
	m_correct = correct;
	m_prev_pt.x = 0;
	m_prev_pt.y = 0;
	m_first_point = true;
}
/// １座標に入るサンプルデータを描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] a_data_spos 元データのサンプリング位置
/// @param [in] x           X座標
void WaveDrawer::DrawOneX(wxDC &dc, int a_data_spos, wxCoord x)
{
	wxCoord y = m_data->At(m_data_pos).Data();
	y -= 128;
	y = -y * m_height / 128;

	if (m_correct) {
		dc.SetPen(*wxRED_PEN);
	} else {
		dc.SetPen(*wxBLACK_PEN);
	}

	if (m_first_point) {
		dc.DrawPoint(x, y + m_ybase);
		m_first_point = false;
	} else {
		dc.DrawLine(m_prev_pt.x, m_prev_pt.y + m_ybase, x, y + m_ybase);
	}
	m_prev_pt.x = x;
	m_prev_pt.y = y;
	m_data_pos++;
}

//

BinaryDrawer::BinaryDrawer(CSampleArray *a_data, CSampleArray *data, wxCoord left, wxCoord right, double xmag, wxCoord ybase, wxCoord height)
	: SampleDrawer(a_data, data, left, right, xmag, ybase, height)
{
}
/// １サンプルデータを描画
/// @param [in] dc          デバイスコンテキスト
/// @param [in] data_spos   サンプリング位置
/// @param [in] a_data_spos 元データのサンプリング位置
/// @param [in] x           X座標
void BinaryDrawer::DrawOnePos(wxDC &dc, int data_spos, int a_data_spos, wxCoord x)
{
	wxString str;
	uint8_t err = m_data->At(m_data_pos).Err();

	bool tail = false;
	do {
		if ((err & 0x1) == 0) {
			str += wxString::Format(_T("%02X"), m_data->At(m_data_pos).Data());
		}
		m_data_pos++;
		if (m_data_pos >= m_data->GetWritePos()) {
			tail = true;
			break;
		}
		data_spos = m_data->At(m_data_pos).SPos();

	} while(data_spos == a_data_spos);

	double next_x = 0.0;
	if (!tail) {
		next_x = m_xmag * (data_spos - a_data_spos) + x;
	} else {
		next_x = m_right;
	}

	if (err == 0x9) {
		str = _T("?");
	}
	if (err == 0xc) {
		str += _T(" Parity Error");
	} else if (err == 0xa) {
		str += _T(" Frame Error");
	}

	dc.DrawLine(x, m_ybase - m_height, next_x, m_ybase - m_height);
	dc.DrawLine(x, m_ybase + m_height, next_x, m_ybase + m_height);
	dc.DrawLine(x, m_ybase - m_height, x, m_ybase + m_height);

	dc.SetTextForeground(err ? *wxRED : *wxBLACK);
	dc.DrawText(str, x, m_ybase - m_height);
}
