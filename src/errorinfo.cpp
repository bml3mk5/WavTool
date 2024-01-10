/// @file errorinfo.cpp
///
/// @brief エラー情報
///
#include "errorinfo.h"

PwErrInfo::PwErrInfo()
{
	mType = pwOK;
	mCode = pwErrNone;
	mMsg = _T("");
	mLine = 0;
}

PwErrInfo::~PwErrInfo()
{
}

// エラーメッセージ
wxString PwErrInfo::ErrMsg(PwErrCode code)
{
	wxString str;
	switch(code) {
		case pwErrNone:
			// no error
			str = _T("");
			break;
		case pwErrFileNotFound:
			// ファイルがみつかりません。
			str = _("File not found.");
			break;
		case pwErrNotPCMFormat:
			// PCMフォーマットのwavファイルではありません。
			str = _("This is not PCM format in the wav file.");
			break;
		case pwErrSampleRate:
			// サンプルレートは11025～48000Hzをサポートします。
			str = _("Sample rate is supported between 11025 and 48000Hz.");
			break;
		case pwErrCannotWrite:
			// ファイルを出力できません。
			str = _("Cannot write file.");
			break;
		case pwErrCannotWriteDebugLog:
			// デバッグログを出力できませんが、処理を続けます。
			str = _("Cannot write debug log. Continue this process.");
			break;
		case pwErrFileEmpty:
			// ファイルが空です。
			str = _("File is empty.");
			break;
		case pwErrSameFile:
			// 同じファイルを指定することはできません。
			str = _("Cannot specify the same file.");
			break;
		case pwErrNotT9XFormat:
			// t9xフォーマットのファイルではありません。
			str = _("This file is not t9x format.");
			break;
		case pwErrNoBASICIntermediateLanguage:
			// BASIC中間言語形式のファイルではありませんが、処理を続けます。
			str = _("This is not BASIC intermediate language file. Continue this process.");
			break;
		default:
			// 不明なエラー: %d
			str.Printf(_("Unknown error: %d"), code);
			break;
	}
	return str;
}

// エラー情報セット
void PwErrInfo::SetInfo(int line, PwErrType type, PwErrCode code, const wxString &msg)
{
		mType = type;
		mCode = code;
		mMsg = ErrMsg(code);
		if (!msg.IsEmpty()) {
			mMsg += _T(" (") + msg + _T(")");
		}
		mLine = line;
}

void PwErrInfo::SetInfo(int line, PwErrType type, PwErrCode code1, PwErrCode code2, const wxString &msg)
{
		mType = type;
		mCode = code1;
		mMsg = ErrMsg(code1);
		if (!msg.IsEmpty()) {
			mMsg += _T(" (") + msg + _T(")");
		}
		mMsg += _T("\n") + ErrMsg(code2);
		mLine = line;
}

// gui メッセージBOX
void PwErrInfo::ShowMsgBox(wxWindow *win)
{
	switch(mType) {
		case pwError:
			wxMessageBox(mMsg, _("Error"), wxOK | wxICON_ERROR, win);
			break;
		case pwWarning:
			wxMessageBox(mMsg, _("Warning"), wxOK | wxICON_WARNING, win);
			break;
		default:
			break;
	}
}
