/// @file wavtool.h
///
/// wavtool.h
///


#ifndef _WAVTOOL_H_
#define _WAVTOOL_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dnd.h>
#include "config.h"
#include "parsewav.h"


class WavtoolApp;
class WavtoolFrame;
class WavtoolPanel;
class WavtoolFileDialog;
class WavtoolFileDropTarget;

//class PARSEWAV::ParseWav;
class ConfigBox;
class WaveFrame;

/// @brief Window
class WavtoolApp: public wxApp
{
private:
	wxString app_path;
	wxString ini_path;
	wxString res_path;
	wxLocale mLocale;

	void SetAppPath();
public:
	WavtoolApp() : mLocale(wxLANGUAGE_DEFAULT) {}
	bool OnInit();
	int  OnExit();
	const wxString &GetAppPath();
	const wxString &GetIniPath();
	const wxString &GetResPath();
};

DECLARE_APP(WavtoolApp)

/// @brief Frame
class WavtoolFrame: public wxFrame
{
private:
	// gui
	wxMenu *menuFile;
	wxMenu *menuRecentFiles;
	wxMenu *menuSets;
	wxMenu *menuView;
	wxMenu *menuHelp;
	WavtoolPanel *panel;

	PARSEWAV::ParseWav *wav;

//	ConfigBox *cfgbox;
//	WaveFrame *wavewin;

	wxString text_buffer;

public:

    WavtoolFrame(const wxString& title, const wxSize& size);
	~WavtoolFrame();

	// event procedures
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	void OnOpenFile(wxCommandEvent& event);
	void OnOpenRecentFile(wxCommandEvent& event);
	void OnCloseFile(wxCommandEvent& event);

	void OnExportFile(wxCommandEvent& event);

	void OnAnalyzeWave(wxCommandEvent& event);
	void OnAnalyzeFiles(wxCommandEvent& event);

	void OnSetsRftype(wxCommandEvent& event);
	void OnSetsMachine(wxCommandEvent& event);

	void OnSetsBaudRate(wxCommandEvent& event);

	void OnSetsBaudDblFsk(wxCommandEvent& event);

	void OnSetsCorrectType(wxCommandEvent& event);

	void OnConfigure(wxCommandEvent& event);

	void OnOpenWaveWindow(wxCommandEvent& event);

	void OnMenuOpen(wxMenuEvent& event);

//	void OnDropFiles(wxDropFilesEvent& event);

	// functions
	void OpenDataFile(wxString &path);
	void OpenedDataFile();
	void CloseDataFile();
	void ExportFile(int id);
	void OpenDroppedFile(wxString &path);
	void UpdateMenu(wxMenu *menu);
	void UpdateFileMenu();
	void UpdateSettingMenu();
	void UpdateMenuRecentFiles();

	void UpdateWaveFrame();

	// properties
	WavtoolPanel *GetWavtoolPanel() { return panel; }
	PARSEWAV::ParseWav *GetParseWav() { return wav; }
	WaveFrame *GetWaveFrame();

	enum
	{
		// menu id
		IDM_EXIT = 1,
		IDM_ABOUT,
		IDM_OPEN_FILE,
		IDM_CLOSE_FILE,
		IDM_EXPORT,
		IDM_EXPORT_REAL = 6,
		IDM_EXPORT_L3,
		IDM_EXPORT_T9X,
		IDM_EXPORT_L3B,
		IDM_EXPORT_L3C,
		IDM_EXPORT_WAV,
		IDM_ANALYZE_WAV,
		IDM_ANALYZE_FILES,
		IDM_SETS_RFTYPE,
		IDM_SETS_MACHINE,
		IDM_SETS_BAUD,
		IDM_SETS_BAUD_AUTO,
		IDM_SETS_BAUD_600,
		IDM_SETS_BAUD_1200,
		IDM_SETS_BAUD_2400,
		IDM_SETS_BAUD_300,
		IDM_SETS_BAUD_DBLFSK,
		IDM_SETS_CORRECT,
		IDM_SETS_CORRECT_NONE,
		IDM_SETS_CORRECT_COS,
		IDM_SETS_CORRECT_SIN,
		IDM_SETS_CONFIGURE,

		IDM_WINDOW_WAVE,

		IDD_CONFIGBOX,

		IDM_RECENT_FILE_0 = 80,

		IDD_WAVEWINDOW = 16000,
	};

	DECLARE_EVENT_TABLE()
};

/// @brief Panel
class WavtoolPanel: public wxPanel
{
private:
	wxTextCtrl *textName;
	wxTextCtrl *textInfo;
	wxButton *btnExportReal;
	wxButton *btnExportL3;
	wxButton *btnExportT9X;
	wxButton *btnExportL3B;
	wxButton *btnExportL3C;
	wxButton *btnExportWAV;
	wxCheckBox *chkBaudAuto;
	wxRadioButton *radBaud600;
	wxRadioButton *radBaud1200;
	wxRadioButton *radBaud2400;
	wxRadioButton *radBaud300;
	wxCheckBox *chkDblFsk;
	wxRadioButton *radCorrNone;
	wxRadioButton *radCorrCosw;
	wxRadioButton *radCorrSinw;
	wxButton *btnAnalyzeWav;
	wxButton *btnAnalyzeFile;

	void UpdateBaudStr();
public:
	WavtoolPanel(WavtoolFrame *parent);
	void UpdateExportButton();
	void DisableExportButton();
	void UpdateBaudAndCorr();

	// event procedures
	void OnSize(wxSizeEvent& event);
	void OnClickExport(wxCommandEvent& event);
	void OnSetsBaudRate(wxCommandEvent& event);
	void OnSetsCorrectType(wxCommandEvent& event);
	void OnAnalyzeWave(wxCommandEvent& event);
	void OnAnalyzeFiles(wxCommandEvent& event);

	// properties
	wxTextCtrl *GetTextName() { return textName; }
	wxTextCtrl *GetTextInfo() { return textInfo; }

	WavtoolFrame *GetFrame() { return (WavtoolFrame *)GetParent(); }

	enum {
		IDC_TEXT_NAME = 1,
		IDC_TEXT_INFO,
		IDC_BTN_EXPORT_REAL = 6,
		IDC_BTN_EXPORT_L3,
		IDC_BTN_EXPORT_T9X,
		IDC_BTN_EXPORT_L3B,
		IDC_BTN_EXPORT_L3C,
		IDC_BTN_EXPORT_WAV,
		IDC_BTN_ANALYZE_WAV,
		IDC_BTN_ANALYZE_FILES,
		IDC_CHK_BAUD_AUTO = 17,
		IDC_RADIO_BAUD_600,
		IDC_RADIO_BAUD_1200,
		IDC_RADIO_BAUD_2400,
		IDC_RADIO_BAUD_300,
		IDC_CHK_BAUD_DBL_FSK,
		IDC_RADIO_CORR_NONE = 23,
		IDC_RADIO_CORR_COSW,
		IDC_RADIO_CORR_SINW
	};

	DECLARE_EVENT_TABLE()
};

/// @brief for file dialog
class WavtoolFileDialog: public wxFileDialog
{
public:
	WavtoolFileDialog(const wxString& message, const wxString& defaultDir = wxEmptyString, const wxString& defaultFile = wxEmptyString, const wxString& wildcard = wxFileSelectorDefaultWildcardStr, long style = wxFD_DEFAULT_STYLE);

};

/// @brief for drop
class WavtoolFileDropTarget : public wxFileDropTarget
{
    WavtoolFrame *frame;
public:
    WavtoolFileDropTarget(WavtoolFrame *parent);
    bool OnDropFiles(wxCoord x, wxCoord y ,const wxArrayString &filenames);
};

/// @brief About dialog
class WavtoolAbout : public wxDialog
{
public:
	WavtoolAbout(wxWindow* parent, wxWindowID id);
};

#endif /* _WAVTOOL_H_ */

