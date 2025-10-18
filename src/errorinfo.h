/// @file errorinfo.h
///
/// @brief エラー情報
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _ERRORINFO_H_
#define _ERRORINFO_H_

#include "common.h"
#include <wx/wx.h>


/// エラータイプ
typedef enum enumPwErrType {
	pwCancel = -1,
	pwOK = 0,
	pwError,
	pwWarning
} PwErrType;

/// エラーコード
typedef enum enumPwErrCode {
	pwErrNone = 0,
	pwErrFileNotFound,
	pwErrNotPCMFormat,
	pwErrSampleRate,
	pwErrCannotWrite,
	pwErrCannotWriteDebugLog,
	pwErrFileEmpty,
	pwErrSameFile,
	pwErrNotT9XFormat,
	pwErrNoBASICIntermediateLanguage = 401,
	pwErrUnknown = 9999
} PwErrCode;

/// エラー情報保存用
class PwErrInfo
{
private:
	PwErrType mType;
	PwErrCode mCode;
	wxString  mMsg;
	int       mLine;

public:
	PwErrInfo();
	~PwErrInfo();

	/// エラーメッセージ
	wxString ErrMsg(PwErrCode code);
	/// エラー情報セット
	void SetInfo(int line, PwErrType type, PwErrCode code, const wxString &msg = wxEmptyString);
	/// エラー情報セット
	void SetInfo(int line, PwErrType type, PwErrCode code1, PwErrCode code2, const wxString &msg = wxEmptyString);

	/// gui メッセージBOX
	void ShowMsgBox(wxWindow *win = 0);

};

#endif /* _ERRORINFO_H_ */
