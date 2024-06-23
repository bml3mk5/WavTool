/// @file wavtool.cpp
///
/// wavtool.cpp
///
#include "parsewav.h"
#include "wavtool.h"
#include "configbox.h"
#include <wx/filename.h>
#include "wavewindow.h"
#include "res/wavtool.xpm"
#include "version.h"

IMPLEMENT_APP(WavtoolApp)

bool WavtoolApp::OnInit()
{
	SetAppPath();
	SetAppName(_T(APPLICATION_NAME));

	// load ini file
	gConfig.Load(ini_path + GetAppName() + _T(".ini"));

	// set locale search path and catalog name
	mLocale.AddCatalogLookupPathPrefix(res_path + _T("lang"));
	mLocale.AddCatalogLookupPathPrefix(_T("lang"));
	mLocale.AddCatalog(_T(APPLICATION_NAME));

	if (!wxApp::OnInit()) {
		return false;
	}

	WavtoolFrame *frame = new WavtoolFrame(GetAppName(), wxSize(480, 400) );
	frame->Show(true);
	SetTopWindow(frame);
	return true;
}

int WavtoolApp::OnExit()
{
	// save ini file
	gConfig.Save();

	return 0;
}

void WavtoolApp::SetAppPath()
{
	app_path = wxFileName::FileName(argv[0]).GetPath(wxPATH_GET_SEPARATOR);
#ifdef __WXOSX__
	if (app_path.Find(_T("MacOS")) >= 0) {
		wxFileName file = wxFileName::FileName(app_path+"../../../");
		file.Normalize();
		ini_path = file.GetPath(wxPATH_GET_SEPARATOR);
		file = wxFileName::FileName(app_path+"../../Contents/Resources/");
		file.Normalize();
		res_path = file.GetPath(wxPATH_GET_SEPARATOR);
	} else
#endif
	{
		ini_path = app_path;
		res_path = app_path;
	}
}

const wxString &WavtoolApp::GetAppPath()
{
	return app_path;
}

const wxString &WavtoolApp::GetIniPath()
{
	return ini_path;
}

const wxString &WavtoolApp::GetResPath()
{
	return res_path;
}

//
// Frame
//
// Attach Event
BEGIN_EVENT_TABLE(WavtoolFrame, wxFrame)
	// menu event
	EVT_MENU(wxID_EXIT,  WavtoolFrame::OnQuit)
	EVT_MENU(wxID_ABOUT, WavtoolFrame::OnAbout)

	EVT_MENU(IDM_OPEN_FILE, WavtoolFrame::OnOpenFile)
	EVT_MENU(IDM_CLOSE_FILE, WavtoolFrame::OnCloseFile)

	EVT_MENU(IDM_EXPORT_REAL, WavtoolFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_L3,   WavtoolFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_T9X,  WavtoolFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_L3B,  WavtoolFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_L3C,  WavtoolFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_WAV,  WavtoolFrame::OnExportFile)

	EVT_MENU(IDM_ANALYZE_WAV, WavtoolFrame::OnAnalyzeWave)
	EVT_MENU(IDM_ANALYZE_FILES, WavtoolFrame::OnAnalyzeFiles)

	EVT_MENU_RANGE(IDM_RECENT_FILE_0, IDM_RECENT_FILE_0 + MAX_RECENT_FILES - 1, WavtoolFrame::OnOpenRecentFile)

	EVT_MENU(IDM_SETS_RFTYPE,  WavtoolFrame::OnSetsRftype)
	EVT_MENU(IDM_SETS_MACHINE, WavtoolFrame::OnSetsMachine)

	EVT_MENU(IDM_SETS_BAUD_AUTO, WavtoolFrame::OnSetsBaudRate)
	EVT_MENU(IDM_SETS_BAUD_600, WavtoolFrame::OnSetsBaudRate)
	EVT_MENU(IDM_SETS_BAUD_1200, WavtoolFrame::OnSetsBaudRate)
	EVT_MENU(IDM_SETS_BAUD_2400, WavtoolFrame::OnSetsBaudRate)
	EVT_MENU(IDM_SETS_BAUD_300, WavtoolFrame::OnSetsBaudRate)
	EVT_MENU(IDM_SETS_BAUD_DBLFSK, WavtoolFrame::OnSetsBaudDblFsk)

	EVT_MENU(IDM_SETS_CORRECT_NONE, WavtoolFrame::OnSetsCorrectType)
	EVT_MENU(IDM_SETS_CORRECT_COS, WavtoolFrame::OnSetsCorrectType)
	EVT_MENU(IDM_SETS_CORRECT_SIN, WavtoolFrame::OnSetsCorrectType)

	EVT_MENU(IDM_SETS_CONFIGURE,  WavtoolFrame::OnConfigure)

	EVT_MENU(IDM_WINDOW_WAVE, WavtoolFrame::OnOpenWaveWindow)

	//	EVT_MENU_OPEN(WavtoolFrame::OnMenuOpen)

//	EVT_DROP_FILES(WavtoolFrame::OnDropFiles)
END_EVENT_TABLE()

// 翻訳用
#define APPLE_MENU_STRING _TX("Hide wavtool"),_TX("Hide Others"),_TX("Show All"),_TX("Quit wavtool"),_TX("Services"),_TX("Preferences…")
#define DIALOG_STRING _TX("OK"),_TX("Cancel")

WavtoolFrame::WavtoolFrame(const wxString& title, const wxSize& size)
       : wxFrame(NULL, -1, title, wxDefaultPosition, size)
{
	// icon
#ifdef __WXMSW__
	SetIcon(wxIcon(_T(APPLICATION_NAME)));
#elif defined(__WXGTK__) || defined(__WXMOTIF__)
	SetIcon(wxIcon(APPLICATION_XPMICON_NAME));
#endif

//	wavewin = NULL;

	// menu
	menuFile = new wxMenu;
	menuSets = new wxMenu;
	menuHelp = new wxMenu;
	wxMenu *smenu;

	menuFile->Append( IDM_OPEN_FILE, _("&Open...") );
	menuFile->Append( IDM_CLOSE_FILE, _("&Close") );
	menuFile->AppendSeparator();
	smenu = new wxMenu;
	smenu->Append( IDM_EXPORT_REAL, _("&Real File...") );
	smenu->AppendSeparator();
	smenu->Append( IDM_EXPORT_L3, _("&L3 File...") );
	smenu->Append( IDM_EXPORT_T9X, _("&T9X File...") );
	smenu->Append( IDM_EXPORT_L3B, _("L3&B File...") );
	smenu->Append( IDM_EXPORT_L3C, _("L3&C File...") );
	smenu->Append( IDM_EXPORT_WAV, _("&WAV File...") );
	menuFile->Append( IDM_EXPORT, _("&Export To"), smenu );
	menuFile->AppendSeparator();
	menuFile->Append( IDM_ANALYZE_WAV, _("&Analyze Wave") );
	menuFile->Append( IDM_ANALYZE_FILES, _("Analyze &Files") );
	menuFile->AppendSeparator();
	menuRecentFiles = new wxMenu();
	UpdateMenuRecentFiles();
	menuFile->AppendSubMenu(menuRecentFiles, _("&Reccent Files") );
	menuFile->AppendSeparator();
	menuFile->Append( wxID_EXIT, _("E&xit") );
	// settings menu
	menuSets->Append( IDM_SETS_RFTYPE, _("File &Type...") );
	menuSets->Append( IDM_SETS_MACHINE, _("&Machine Code...") );
	menuSets->AppendSeparator();
	smenu = new wxMenu;
	smenu->AppendCheckItem( IDM_SETS_BAUD_AUTO, _("&Auto Detect") );
	smenu->AppendSeparator();
	smenu->AppendRadioItem( IDM_SETS_BAUD_600, _T("600 ") );
	smenu->AppendRadioItem( IDM_SETS_BAUD_1200, _T("1200") );
	smenu->AppendRadioItem( IDM_SETS_BAUD_2400, _T("2400") );
	smenu->AppendRadioItem( IDM_SETS_BAUD_300, _T("300 ") );
	smenu->AppendSeparator();
	smenu->AppendCheckItem( IDM_SETS_BAUD_DBLFSK, _("&Double Speed FSK") );
	menuSets->Append( IDM_SETS_BAUD, _("&Baud Rate"), smenu );
	menuSets->AppendSeparator();
	smenu = new wxMenu;
	smenu->AppendRadioItem( IDM_SETS_CORRECT_NONE, _("None") );
	smenu->AppendRadioItem( IDM_SETS_CORRECT_COS, _("COS Wave") );
	smenu->AppendRadioItem( IDM_SETS_CORRECT_SIN, _("SIN Wave") );
	menuSets->Append( IDM_SETS_CORRECT, _("Wave &Correct"), smenu );
	menuSets->AppendSeparator();
	menuSets->Append( IDM_SETS_CONFIGURE, _("&Details...") );
	// window menu
	menuView = new wxMenu();
	menuView->Append( IDM_WINDOW_WAVE, _("&Wave Window") );
	// help menu
	menuHelp->Append( wxID_ABOUT, _("&About...") );

	// menu bar
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, _("&File") );
	menuBar->Append( menuSets, _("&Settings") );
	menuBar->Append( menuView, _("&View") );
	menuBar->Append( menuHelp, _("&Help") );

	SetMenuBar( menuBar );

	// initialize
	wav = new ParseWav(this);
	wav->SetLogBufferPtr(&text_buffer);

//	cfgbox = new ConfigBox(this, IDD_CONFIGBOX);

	// load ini file
	wav->GetParam().SetSampleRatePos(gConfig.GetSampleRatePos());
	wav->GetParam().SetSampleBitsPos(gConfig.GetSampleBitsPos());
	wav->GetParam().SetBaud(gConfig.GetBaud());
	wav->GetParam().SetAutoBaud(gConfig.GetAutoBaud());
	wav->GetParam().SetCorrectType(gConfig.GetCorrectType());
	wav->GetParam().SetCorrectAmp(0, gConfig.GetCorrectAmp(0));
	wav->GetParam().SetCorrectAmp(1, gConfig.GetCorrectAmp(1));
	wav->GetParam().SetChangeGapSize(gConfig.GetChangeGapSize() ? 1 : 0);
	wav->GetParam().SetOutErrSerial(gConfig.GetOutErrSerial());

	// control panel
	panel = new WavtoolPanel(this);

	//
//	DragAcceptFiles(true);

	// update menu
	UpdateMenu(menuFile);
	UpdateMenu(menuSets);

}

WavtoolFrame::~WavtoolFrame()
{
	// save ini file
	gConfig.SetSampleRatePos(wav->GetParam().GetSampleRatePos());
	gConfig.SetSampleBitsPos(wav->GetParam().GetSampleBitsPos());
	gConfig.SetBaud(wav->GetParam().GetBaud());
	gConfig.SetAutoBaud(wav->GetParam().GetAutoBaud());
	gConfig.SetCorrectType(wav->GetParam().GetCorrectType());
	gConfig.SetCorrectAmp(0, wav->GetParam().GetCorrectAmp(0));
	gConfig.SetCorrectAmp(1, wav->GetParam().GetCorrectAmp(1));
	gConfig.SetChangeGapSize(wav->GetParam().GetChangeGapSize() != 0);
	gConfig.SetOutErrSerial(wav->GetParam().GetOutErrSerial());

//	delete cfgbox;
	delete wav;
}

/// メニュー更新
void WavtoolFrame::OnMenuOpen(wxMenuEvent& event)
{
	wxMenu *menu = event.GetMenu();

	if (menu == NULL) return;

	UpdateMenu(menu);
}

/// メニュー更新
void WavtoolFrame::UpdateMenu(wxMenu* menu)
{
	if (menu == menuFile) {	// File...
		UpdateFileMenu();
	} else if (menu == menuSets) {
		UpdateSettingMenu();
	}
}

/// ファイルメニュー更新
void WavtoolFrame::UpdateFileMenu()
{
	// Export to ...
	menuFile->Enable(IDM_CLOSE_FILE,  false);
	menuFile->Enable(IDM_EXPORT_L3,   false);
	menuFile->Enable(IDM_EXPORT_T9X,  false);
	menuFile->Enable(IDM_EXPORT_L3B,  false);
	menuFile->Enable(IDM_EXPORT_L3C,  false);
	menuFile->Enable(IDM_EXPORT_WAV,  false);
	menuFile->Enable(IDM_EXPORT_REAL, false);
	menuFile->Enable(IDM_ANALYZE_WAV, false);
	menuFile->Enable(IDM_ANALYZE_FILES, false);

	if (wav->IsOpenedDataFile()) {
		menuFile->Enable(IDM_CLOSE_FILE, true);

		switch(wav->GetDataFileType()) {
		case FILETYPE_WAV:	//wav
				menuFile->Enable(IDM_EXPORT_L3,   true);
				menuFile->Enable(IDM_EXPORT_T9X,  true);
				menuFile->Enable(IDM_EXPORT_L3B,  true);
				menuFile->Enable(IDM_EXPORT_L3C,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				menuFile->Enable(IDM_EXPORT_REAL, true);
				menuFile->Enable(IDM_ANALYZE_WAV, true);
				menuFile->Enable(IDM_ANALYZE_FILES, true);
				break;
			case FILETYPE_L3C:	//l3c
				menuFile->Enable(IDM_EXPORT_L3,   true);
				menuFile->Enable(IDM_EXPORT_T9X,  true);
				menuFile->Enable(IDM_EXPORT_L3B,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				menuFile->Enable(IDM_EXPORT_REAL, true);
				menuFile->Enable(IDM_ANALYZE_FILES, true);
				break;
			case FILETYPE_L3B:	//l3b
				menuFile->Enable(IDM_EXPORT_L3,   true);
				menuFile->Enable(IDM_EXPORT_T9X,  true);
				menuFile->Enable(IDM_EXPORT_L3C,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				menuFile->Enable(IDM_EXPORT_REAL, true);
				menuFile->Enable(IDM_ANALYZE_FILES, true);
				break;
			case FILETYPE_T9X:	//t9x
				menuFile->Enable(IDM_EXPORT_L3,   true);
				menuFile->Enable(IDM_EXPORT_L3B,  true);
				menuFile->Enable(IDM_EXPORT_L3C,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				menuFile->Enable(IDM_EXPORT_REAL, true);
				menuFile->Enable(IDM_ANALYZE_FILES, true);
				break;
			case FILETYPE_L3:	//l3
				menuFile->Enable(IDM_EXPORT_T9X,  true);
				menuFile->Enable(IDM_EXPORT_L3B,  true);
				menuFile->Enable(IDM_EXPORT_L3C,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				menuFile->Enable(IDM_EXPORT_REAL, true);
				menuFile->Enable(IDM_ANALYZE_FILES, true);
				break;
			case FILETYPE_REAL:	//real
				menuFile->Enable(IDM_EXPORT_L3,   true);
				menuFile->Enable(IDM_EXPORT_T9X,  true);
				menuFile->Enable(IDM_EXPORT_L3B,  true);
				menuFile->Enable(IDM_EXPORT_L3C,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				break;
			case FILETYPE_PLAIN:	//plain
				menuFile->Enable(IDM_EXPORT_L3,   true);
				menuFile->Enable(IDM_EXPORT_T9X,  true);
				menuFile->Enable(IDM_EXPORT_L3B,  true);
				menuFile->Enable(IDM_EXPORT_L3C,  true);
				menuFile->Enable(IDM_EXPORT_WAV,  true);
				menuFile->Enable(IDM_EXPORT_REAL, true);
				break;
			default:
				break;
		}
	}
}

/// 設定メニュー更新
void WavtoolFrame::UpdateSettingMenu()
{
	int i;
	int mag = (wav->GetParam().GetBaseFreq() == 1200 ? 1 : 2);

	menuSets->Enable(IDM_SETS_RFTYPE,  false);
	menuSets->Enable(IDM_SETS_MACHINE, false);

	switch(wav->GetDataFileType()) {
		case FILETYPE_PLAIN:
		case FILETYPE_REAL:
			menuSets->Enable(IDM_SETS_RFTYPE, true);
			if (wav->GetRfDataFormat() == 2) {
				menuSets->Enable(IDM_SETS_MACHINE, true);
			}
			break;
		default:
			break;
	}

	menuSets->Check(IDM_SETS_BAUD_AUTO, wav->GetParam().GetAutoBaud());
	i = wav->GetParam().GetBaud();
	menuSets->Check(IDM_SETS_BAUD_600 + i, true);

	for(int id=IDM_SETS_BAUD_600; id<=IDM_SETS_BAUD_300; id++) {
		menuSets->SetLabel(id, wxString::Format(_T("%d"), (300 * mag) << ((id-IDM_SETS_BAUD_600+1) % 4)));
	}
	menuSets->Check(IDM_SETS_BAUD_DBLFSK, mag > 1);

	i = wav->GetParam().GetCorrectType();
	menuSets->Check(IDM_SETS_CORRECT_NONE + i, true);
}

/// 最近使用したファイル一覧を更新
void WavtoolFrame::UpdateMenuRecentFiles()
{
	// メニューを更新
	wxArrayString names;
	gConfig.GetRecentFiles(names);
	for(int i=0; i<MAX_RECENT_FILES && i<(int)names.Count(); i++) {
		if (menuRecentFiles->FindItem(IDM_RECENT_FILE_0 + i)) menuRecentFiles->Delete(IDM_RECENT_FILE_0 + i);
		menuRecentFiles->Append(IDM_RECENT_FILE_0 + i, names[i]);
	}
}

/// ドロップされたファイルを開く
#if 0
void WavtoolFrame::OnDropFiles(wxDropFilesEvent& event)
{
	wxString *names = event.GetFiles();
	int num = event.GetNumberOfFiles();

	// 最初のファイル名を取得
	if (num > 0) {
		CloseDataFile();
		OpenDataFile(names[0]);
	}
}
#endif
void WavtoolFrame::OpenDroppedFile(wxString &path)
{
	CloseDataFile();
	OpenDataFile(path);
}

/// 最近使用したファイル
void WavtoolFrame::OnOpenRecentFile(wxCommandEvent& event)
{
	wxMenuItem *item = menuRecentFiles->FindItem(event.GetId());
	if (!item) return;
	wxString path = item->GetItemLabel();
	CloseDataFile();
	OpenDataFile(path);
}

void WavtoolFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
 	CloseDataFile();
	Close(true);
}

void WavtoolFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	WavtoolAbout(this, wxID_ANY).ShowModal();
}

void WavtoolFrame::OnOpenFile(wxCommandEvent& WXUNUSED(event))
{
	WavtoolFileDialog *dlg = new WavtoolFileDialog(
		_("Open file"),
		gConfig.GetFilePath(),
		wxEmptyString,
		_("Supported files (*.wav;*.l3c;*.l3b;*.l3;*.t9x;*.bin;*.bas;*.obj;*.dat)|*.wav;*.l3c;*.l3b;*.l3;*.t9x;*.bin;*.bas;*.obj;*.dat|All files (*.*)|*.*"),
		wxFD_OPEN);

	int rc = dlg->ShowModal();
	wxString path = dlg->GetPath();

	delete dlg;

	if (rc == wxID_OK) {
		OpenDataFile(path);
	}
}

void WavtoolFrame::OnCloseFile(wxCommandEvent& WXUNUSED(event))
{
	CloseDataFile();
}

/// エクスポート
void WavtoolFrame::OnExportFile(wxCommandEvent& event)
{
	int id = event.GetId();

	ExportFile(id);
}

/// 波形解析
void WavtoolFrame::OnAnalyzeWave(wxCommandEvent& event)
{
	if (!wav->IsOpenedDataFile()) return;
	if (wav->GetDataFileType() != FILETYPE_WAV) return;

	panel->DisableExportButton();

	wav->AnalyzeWave();

	// disp infomation on the window
	panel->GetTextInfo()->SetValue(text_buffer);

	UpdateMenu(menuSets);
	panel->UpdateBaudAndCorr();

	panel->UpdateExportButton();
}

/// ファイル内容解析
void WavtoolFrame::OnAnalyzeFiles(wxCommandEvent& event)
{
	if (!wav->IsOpenedDataFile()) return;

	panel->DisableExportButton();

	wav->ExportData(FILETYPE_NO_FILE);

	// disp infomation on the window
	panel->GetTextInfo()->SetValue(text_buffer);

	panel->UpdateExportButton();
}

/// データ種類設定ダイアログ
void WavtoolFrame::OnSetsRftype(wxCommandEvent& WXUNUSED(event))
{
	if (!wav->IsOpenedDataFile()) return;
	enum_file_type type = wav->GetDataFileType();
	if (type != FILETYPE_REAL && type != FILETYPE_PLAIN) return;

	bool rc;

	rc = wav->ShowRfTypeBox();
	if (!rc) {
		// cancel button or error
		return;
	}

	UpdateSettingMenu();
}

/// マシン語設定ダイアログ
void WavtoolFrame::OnSetsMachine(wxCommandEvent& WXUNUSED(event))
{
	if (!wav->IsOpenedDataFile()) return;
//	if (wav->GetDataFileType() != FILETYPE_REAL) return;
	if (wav->GetRfDataFormat() != 2) return;

	wav->ShowMAddressBox(true);

	UpdateSettingMenu();
}

/// ボーレート設定
void WavtoolFrame::OnSetsBaudRate(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id) {
	case IDM_SETS_BAUD_AUTO:
		wav->GetParam().SetAutoBaud(event.IsChecked());
		break;
	default:
		wav->GetParam().SetBaud(id-IDM_SETS_BAUD_600);
		break;
	}
	panel->UpdateBaudAndCorr();
	UpdateWaveFrame();
}

/// 倍速FSK設定
void WavtoolFrame::OnSetsBaudDblFsk(wxCommandEvent& event)
{
	int mag = event.IsChecked() ? 2 : 1;
	wav->GetParam().SetFrequency(mag);

	UpdateSettingMenu();
	panel->UpdateBaudAndCorr();
	UpdateWaveFrame();
}

/// 波形補正設定
void WavtoolFrame::OnSetsCorrectType(wxCommandEvent& event)
{
	int id = event.GetId();
	wav->GetParam().SetCorrectType(id-IDM_SETS_CORRECT_NONE);
	panel->UpdateBaudAndCorr();
	UpdateWaveFrame();
}

/// 設定ダイアログ
void WavtoolFrame::OnConfigure(wxCommandEvent& WXUNUSED(event))
{
	ConfigBox cfgbox(this, IDD_CONFIGBOX);
	cfgbox.SetParam(wav->GetParam());
	cfgbox.ShowModal();
	wav->SetParam(cfgbox.GetParam());

	UpdateMenu(menuSets);
	panel->UpdateBaudAndCorr();
	UpdateWaveFrame();
}

// Waveウィンドウを開く
void WavtoolFrame::OnOpenWaveWindow(wxCommandEvent& WXUNUSED(event))
{
	wxWindow *wavewin = FindWindowById(IDD_WAVEWINDOW);
	if (!wavewin) {
		wavewin = new WaveFrame(this, IDD_WAVEWINDOW, wav);
		wavewin->Show();
	} else {
		wavewin->Close();
	}
}

WaveFrame *WavtoolFrame::GetWaveFrame()
{
	return(WaveFrame *)FindWindowById(IDD_WAVEWINDOW);
}

/// 波形ウィンドウを更新
void WavtoolFrame::UpdateWaveFrame()
{
	WaveFrame *wavewin = GetWaveFrame();
	if (wavewin) {
		wavewin->Update();
	}
}

/// 指定したファイルを開く
void WavtoolFrame::OpenDataFile(wxString &path)
{
	bool rc;
	wxString title;

	gConfig.AddRecentFile(path);
	UpdateMenuRecentFiles();

	rc = wav->OpenDataFile(path);
	if (!rc) {
		// cancel button or error
		return;
	}

	// update window
	title = wxGetApp().GetAppName() + _T(" - ") + path;
	SetTitle(title);
	panel->GetTextName()->SetValue(path);
	panel->GetTextInfo()->SetValue(_T(""));
	panel->UpdateExportButton();

	UpdateMenu(menuFile);
	UpdateMenu(menuSets);

	// update wave window
	UpdateWaveFrame();
}

void WavtoolFrame::OpenedDataFile()
{
}

/// ファイルを閉じる
void WavtoolFrame::CloseDataFile()
{
	wav->CloseDataFile();

	// update window
	wxString title = wxGetApp().GetAppName();
	SetTitle(title);
	panel->GetTextName()->SetValue(_T(""));
	panel->UpdateExportButton();

	UpdateMenu(menuFile);
	UpdateMenu(menuSets);
}

/// エクスポート
void WavtoolFrame::ExportFile(int id)
{
	int rc;
	wxString file_base;
	wxString wild_card;
	bool enable = true;

	if (!wav->IsOpenedDataFile()) return;

	wav->GetFileNameBase(file_base);
	enum_file_type infile_type = wav->GetDataFileType();
	enum_file_type outfile_type = FILETYPE_UNKNOWN;

	switch(id) {
		case IDM_EXPORT_L3:
			file_base += _T(".l3");
			wild_card = _("L3 file (*.l3)|*.l3");
			outfile_type = FILETYPE_L3;
			if (infile_type == FILETYPE_L3) enable = false;
			break;
		case IDM_EXPORT_T9X:
			file_base += _T(".t9x");
			wild_card = _("T9X file (*.t9x)|*.t9x");
			outfile_type = FILETYPE_T9X;
			if (infile_type == FILETYPE_T9X) enable = false;
			break;
		case IDM_EXPORT_L3B:
			file_base += _T(".l3b");
			wild_card = _("L3B file (*.l3b)|*.l3b");
			outfile_type = FILETYPE_L3B;
			if (infile_type == FILETYPE_L3B) enable = false;
			break;
		case IDM_EXPORT_L3C:
			file_base += _T(".l3c");
			wild_card = _("L3C file (*.l3c)|*.l3c");
			outfile_type = FILETYPE_L3C;
			if (infile_type == FILETYPE_L3C) enable = false;
			break;
		case IDM_EXPORT_WAV:
			file_base += _T(".wav");
			wild_card = _("Wave file (*.wav)|*.wav");
			outfile_type = FILETYPE_WAV;
			break;
		case IDM_EXPORT_REAL:
#if defined(__WXGTK__) || defined(__WXMOTIF__)
			file_base += _T(".bin");
			wild_card = _("Real file (*.bin)|*.bin|All Files (*.*)|*.*");
#else
			file_base += _T(".bin");
			wild_card = _("Real file (*.bin)|*.bin|BASIC file (*.bas)|*.bas|Data file (*.dat)|*.dat|All files (*.*)|*.*");
#endif
			outfile_type = FILETYPE_REAL;
			if (infile_type == FILETYPE_REAL) enable = false;
			break;
		default:
			file_base += _T("");
			wild_card = _("All files (*.*)|*.*");
			outfile_type = FILETYPE_PLAIN;
			break;
	}
	if (!enable) return;

	WavtoolFileDialog *dlg = new WavtoolFileDialog(
		_("Export file"),
		gConfig.GetFilePath(),
		file_base,
		wild_card,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	rc = dlg->ShowModal();
	wxString path = dlg->GetPath();

	delete dlg;

	if (rc == wxID_OK) {
		gConfig.AddRecentFile(path);
		UpdateMenuRecentFiles();

		bool rt = wav->OpenOutFile(path, outfile_type);
		if (!rt) {
			// cancel button or error
			return;
		}
		//
		panel->DisableExportButton();

		rt = wav->ExportData();
		wav->CloseOutFile();

		panel->UpdateExportButton();

		if (!rt) {
			// cancel button or error
			return;
		}

		// disp infomation on the window
		panel->GetTextInfo()->SetValue(text_buffer);
	}
}

//
// Control Panel
//
// Attach Event
BEGIN_EVENT_TABLE(WavtoolPanel, wxPanel)
	// event
	EVT_SIZE(WavtoolPanel::OnSize)

	EVT_BUTTON(IDC_BTN_EXPORT_REAL, WavtoolPanel::OnClickExport)
	EVT_BUTTON(IDC_BTN_EXPORT_L3, WavtoolPanel::OnClickExport)
	EVT_BUTTON(IDC_BTN_EXPORT_T9X, WavtoolPanel::OnClickExport)
	EVT_BUTTON(IDC_BTN_EXPORT_L3B, WavtoolPanel::OnClickExport)
	EVT_BUTTON(IDC_BTN_EXPORT_L3C, WavtoolPanel::OnClickExport)
	EVT_BUTTON(IDC_BTN_EXPORT_WAV, WavtoolPanel::OnClickExport)

	EVT_CHECKBOX(IDC_CHK_BAUD_AUTO, WavtoolPanel::OnSetsBaudRate)
	EVT_RADIOBUTTON(IDC_RADIO_BAUD_600, WavtoolPanel::OnSetsBaudRate)
	EVT_RADIOBUTTON(IDC_RADIO_BAUD_1200, WavtoolPanel::OnSetsBaudRate)
	EVT_RADIOBUTTON(IDC_RADIO_BAUD_2400, WavtoolPanel::OnSetsBaudRate)
	EVT_RADIOBUTTON(IDC_RADIO_BAUD_300, WavtoolPanel::OnSetsBaudRate)
	EVT_CHECKBOX(IDC_CHK_BAUD_DBL_FSK, WavtoolPanel::OnSetsBaudRate)

	EVT_RADIOBUTTON(IDC_RADIO_CORR_NONE, WavtoolPanel::OnSetsCorrectType)
	EVT_RADIOBUTTON(IDC_RADIO_CORR_COSW, WavtoolPanel::OnSetsCorrectType)
	EVT_RADIOBUTTON(IDC_RADIO_CORR_SINW, WavtoolPanel::OnSetsCorrectType)

	EVT_BUTTON(IDC_BTN_ANALYZE_WAV, WavtoolPanel::OnAnalyzeWave)
	EVT_BUTTON(IDC_BTN_ANALYZE_FILES, WavtoolPanel::OnAnalyzeFiles)
END_EVENT_TABLE()

WavtoolPanel::WavtoolPanel(WavtoolFrame *parent)
       : wxPanel(parent)
{
	wxPoint pos;
	wxSize  frame_size = parent->GetSize();
	wxSize  size;
	long style;

	pos = wxDefaultPosition;

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hboxAll,*hbox;

	// ファイルパス
	size.x = frame_size.x; size.y = -1;
	style = wxTE_READONLY;
	textName = new wxTextCtrl(this, IDC_TEXT_NAME, wxEmptyString, pos, size, style);
	szrAll->Add(textName);

	hboxAll = new wxBoxSizer(wxHORIZONTAL); 

	// 解析
	size.x = 64; size.y = -1;
	hbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Analyze")), wxHORIZONTAL);
	btnAnalyzeWav  = new wxButton(this, IDC_BTN_ANALYZE_WAV,   _("Wave"), pos, size);
	btnAnalyzeFile = new wxButton(this, IDC_BTN_ANALYZE_FILES, _("File"), pos, size);
	hbox->Add(btnAnalyzeWav);
	hbox->Add(btnAnalyzeFile);
	hboxAll->Add(hbox);

	// ボーレート
	size.x = -1; size.y = btnAnalyzeFile->GetSize().y;
	hbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Baud")), wxHORIZONTAL);
	chkBaudAuto = new wxCheckBox(this, IDC_CHK_BAUD_AUTO,      _("Auto Detect"), pos, size);
	radBaud600  = new wxRadioButton(this, IDC_RADIO_BAUD_600,  _T("600 "), pos, size, wxRB_GROUP);
	radBaud1200 = new wxRadioButton(this, IDC_RADIO_BAUD_1200, _T("1200"), pos, size);
	radBaud2400 = new wxRadioButton(this, IDC_RADIO_BAUD_2400, _T("2400"), pos, size);
	radBaud300  = new wxRadioButton(this, IDC_RADIO_BAUD_300,  _T("300 "), pos, size);
	chkDblFsk   = new wxCheckBox(this, IDC_CHK_BAUD_DBL_FSK,   _("Double FSK"), pos, size);
	hbox->Add(chkBaudAuto);
	hbox->Add(radBaud600);
	hbox->Add(radBaud1200);
	hbox->Add(radBaud2400);
	hbox->Add(radBaud300);
	hbox->Add(chkDblFsk);
	hboxAll->Add(hbox);

	// 補正
	hbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Correct")), wxHORIZONTAL);
	radCorrNone = new wxRadioButton(this, IDC_RADIO_CORR_NONE,  _("None"), pos, size, wxRB_GROUP);
	radCorrCosw = new wxRadioButton(this, IDC_RADIO_CORR_COSW,  _("COS Wave"), pos, size);
	radCorrSinw = new wxRadioButton(this, IDC_RADIO_CORR_SINW,  _("SIN Wave"), pos, size);
	hbox->Add(radCorrNone);
	hbox->Add(radCorrCosw);
	hbox->Add(radCorrSinw);
	hboxAll->Add(hbox);

	szrAll->Add(hboxAll);

	// エクスポートボタン
	size = wxDefaultSize;
	hbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Export to ...")), wxHORIZONTAL);
	btnExportReal = new wxButton(this, IDC_BTN_EXPORT_REAL, _("Real File"));
	btnExportL3   = new wxButton(this, IDC_BTN_EXPORT_L3,   _("L3 File"));
	btnExportT9X  = new wxButton(this, IDC_BTN_EXPORT_T9X,  _("T9X File"));
	btnExportL3B  = new wxButton(this, IDC_BTN_EXPORT_L3B,  _("L3B File"));
	btnExportL3C  = new wxButton(this, IDC_BTN_EXPORT_L3C,  _("L3C File"));
	btnExportWAV  = new wxButton(this, IDC_BTN_EXPORT_WAV,  _("Wav File"));
	hbox->Add(btnExportReal);
	hbox->Add(btnExportL3);
	hbox->Add(btnExportT9X);
	hbox->Add(btnExportL3B);
	hbox->Add(btnExportL3C);
	hbox->Add(btnExportWAV);
	szrAll->Add(hbox);

	// 情報テキスト
	size.x = frame_size.x; size.y = -1;
	style = wxTE_READONLY | wxTE_MULTILINE;
	textInfo = new wxTextCtrl(this, IDC_TEXT_INFO, wxEmptyString, pos, size, style);
	szrAll->Add(textInfo);

	SetSizerAndFit(szrAll);
	Layout();

	// adjust window width
	wxSize fsz = parent->GetClientSize();
	wxSize psz = this->GetSize();
	if (fsz.x < psz.x) fsz.x = psz.x;
	if (fsz.y < psz.y) fsz.y = psz.y;
	parent->SetClientSize(fsz);

	// drag and drop
	SetDropTarget(new WavtoolFileDropTarget(parent));
//	textName->SetDropTarget(new WavtoolFileDropTarget(parent));
//	textInfo->SetDropTarget(new WavtoolFileDropTarget(parent));

	// update button
	UpdateExportButton();
	UpdateBaudAndCorr();
}

/// サイズ変更
void WavtoolPanel::OnSize(wxSizeEvent& event)
{
	wxSize size = event.GetSize();

	textName->SetSize(size.x, textName->GetSize().y);
	textInfo->SetSize(size.x, size.y - textInfo->GetPosition().y);
}

/// エクスポート
void WavtoolPanel::OnClickExport(wxCommandEvent& event)
{
	WavtoolFrame *parent = GetFrame();
	int id = event.GetId();

	parent->ExportFile(id);
}

/// ボーレート更新
void WavtoolPanel::OnSetsBaudRate(wxCommandEvent& event)
{
	WavtoolFrame *parent = GetFrame();
	ParseWav *wav = parent->GetParseWav();

	int id = event.GetId();
	switch(id) {
	case IDC_CHK_BAUD_AUTO:
		wav->GetParam().SetAutoBaud(event.IsChecked());
		break;
	case IDC_CHK_BAUD_DBL_FSK:
		wav->GetParam().SetFskSpeed(event.IsChecked() ? 1 : 0);
		UpdateBaudStr();
		break;
	default:
		wav->GetParam().SetBaud(id-IDC_RADIO_BAUD_600);
		break;
	}
	parent->UpdateSettingMenu();
	parent->UpdateWaveFrame();
}

/// 波形補正設定
void WavtoolPanel::OnSetsCorrectType(wxCommandEvent& event)
{
	WavtoolFrame *parent = GetFrame();
	ParseWav *wav = parent->GetParseWav();

	int id = event.GetId();
	wav->GetParam().SetCorrectType(id-IDC_RADIO_CORR_NONE);
	parent->UpdateSettingMenu();
	parent->UpdateWaveFrame();
}

/// 波形解析
void WavtoolPanel::OnAnalyzeWave(wxCommandEvent& event)
{
	GetFrame()->OnAnalyzeWave(event);
}

/// ファイル内容解析
void WavtoolPanel::OnAnalyzeFiles(wxCommandEvent& event)
{
	GetFrame()->OnAnalyzeFiles(event);
}

/// ボタン無効
void WavtoolPanel::DisableExportButton()
{
	btnExportL3->Enable(false);
	btnExportT9X->Enable(false);
	btnExportL3B->Enable(false);
	btnExportL3C->Enable(false);
	btnExportWAV->Enable(false);
	btnExportReal->Enable(false);
	btnAnalyzeWav->Enable(false);
	btnAnalyzeFile->Enable(false);
}

/// ボタン更新
void WavtoolPanel::UpdateExportButton()
{
	WavtoolFrame *parent = GetFrame();
	ParseWav *wav = parent->GetParseWav();

	DisableExportButton();

	if (wav->IsOpenedDataFile()) {
		switch(wav->GetDataFileType()) {
		case FILETYPE_WAV:	//wav
			btnExportL3->Enable(true);
			btnExportT9X->Enable(true);
			btnExportL3B->Enable(true);
			btnExportL3C->Enable(true);
			btnExportWAV->Enable(true);
			btnExportReal->Enable(true);
			btnAnalyzeWav->Enable(true);
			btnAnalyzeFile->Enable(true);
			break;
		case FILETYPE_L3C:	//l3c
			btnExportL3->Enable(true);
			btnExportT9X->Enable(true);
			btnExportL3B->Enable(true);
			btnExportWAV->Enable(true);
			btnExportReal->Enable(true);
			btnAnalyzeFile->Enable(true);
			break;
		case FILETYPE_L3B:	//l3b
			btnExportL3->Enable(true);
			btnExportT9X->Enable(true);
			btnExportL3C->Enable(true);
			btnExportWAV->Enable(true);
			btnExportReal->Enable(true);
			btnAnalyzeFile->Enable(true);
			break;
		case FILETYPE_T9X:	//t9x
			btnExportL3->Enable(true);
			btnExportL3B->Enable(true);
			btnExportL3C->Enable(true);
			btnExportWAV->Enable(true);
			btnExportReal->Enable(true);
			btnAnalyzeFile->Enable(true);
			break;
		case FILETYPE_L3:	//l3
			btnExportT9X->Enable(true);
			btnExportL3B->Enable(true);
			btnExportL3C->Enable(true);
			btnExportWAV->Enable(true);
			btnExportReal->Enable(true);
			btnAnalyzeFile->Enable(true);
			break;
		case FILETYPE_REAL:	//real
			btnExportL3->Enable(true);
			btnExportT9X->Enable(true);
			btnExportL3B->Enable(true);
			btnExportL3C->Enable(true);
			btnExportWAV->Enable(true);
			break;
		case FILETYPE_PLAIN:	//plain
			btnExportL3->Enable(true);
			btnExportT9X->Enable(true);
			btnExportL3B->Enable(true);
			btnExportL3C->Enable(true);
			btnExportWAV->Enable(true);
			btnExportReal->Enable(true);
			break;
		default:
			break;
		}
	}
}

void WavtoolPanel::UpdateBaudAndCorr()
{
	WavtoolFrame *parent = GetFrame();
	ParseWav *wav = parent->GetParseWav();
	int num = wav->GetParam().GetBaud();
//	int mag = wav->GetParam().GetFskSpeed() + 1;

	chkBaudAuto->SetValue(wav->GetParam().GetAutoBaud());
	switch(num) {
	case 1:
		radBaud1200->SetValue(true);
		break;
	case 2:
		radBaud2400->SetValue(true);
		break;
	case 3:
		radBaud300->SetValue(true);
		break;
	default:
		radBaud600->SetValue(true);
		break;
	}
	UpdateBaudStr();
	chkDblFsk->SetValue(wav->GetParam().GetFskSpeed() != 0);

	num = wav->GetParam().GetCorrectType();
	switch(num) {
	case 1:
		radCorrCosw->SetValue(true);
		break;
	case 2:
		radCorrSinw->SetValue(true);
		break;
	default:
		radCorrNone->SetValue(true);
		break;
	}
}

void WavtoolPanel::UpdateBaudStr()
{
	WavtoolFrame *parent = GetFrame();
	ParseWav *wav = parent->GetParseWav();
	int mag = wav->GetParam().GetFskSpeed() + 1;
	radBaud1200->SetLabel(wxString::Format(_T("%d"), 1200 * mag));
	radBaud2400->SetLabel(wxString::Format(_T("%d"), 2400 * mag));
	radBaud300->SetLabel(wxString::Format(_T("%d"), 300 * mag));
	radBaud600->SetLabel(wxString::Format(_T("%d"), 600 * mag));
}

//
// File Dialog
//
WavtoolFileDialog::WavtoolFileDialog(const wxString& message, const wxString& defaultDir, const wxString& defaultFile, const wxString& wildcard, long style)
            : wxFileDialog(NULL, message, defaultDir, defaultFile, wildcard, style)
{
}

//
// File Drag and Drop
//
WavtoolFileDropTarget::WavtoolFileDropTarget(WavtoolFrame *parent)
			: frame(parent)
{
}

bool WavtoolFileDropTarget::OnDropFiles(wxCoord x, wxCoord y ,const wxArrayString &filenames) {
	if (filenames.Count() > 0) {
		wxString name = filenames.Item(0);
		frame->OpenDroppedFile(name);
	}
    return true;
}

//
// About dialog
//
WavtoolAbout::WavtoolAbout(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("About..."), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrLeft   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrRight  = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrMain   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *szrAll    = new wxBoxSizer(wxVERTICAL);

	szrLeft->Add(new wxStaticBitmap(this, wxID_ANY,
		wxBitmap(APPLICATION_XPMICON_NAME), wxDefaultPosition, wxSize(64, 64))
		, flags);

	wxString str = _T(APPLICATION_FULLNAME);
	str += _T(", Version ");
	str += _T(APPLICATION_VERSION);
	str += _T(" \"");
	str += _T(PLATFORM);
	str += _T("\"\n\n");
#ifdef _DEBUG
	str += _T("(Debug Version)\n\n");
#endif
	str	+= _T("using ");
	str += wxVERSION_STRING;
	str += _T("\n\n");
	str	+= _T(APP_COPYRIGHT);

	szrRight->Add(new wxStaticText(this, wxID_ANY, str), flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK);
	szrMain->Add(szrLeft, flags);
	szrMain->Add(szrRight, flags);
	szrAll->Add(szrMain, flags);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}
