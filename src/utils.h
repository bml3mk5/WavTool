/// @file utils.h
///
/// @brief ユーティリティ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#ifndef _UTILS_H_
#define _UTILS_H_

#include "common.h"
#include <wx/wx.h>

namespace UTILS
{
	bool base_name(const _TCHAR *, _TCHAR *, size_t);
	bool prefix_name(const wxString &, wxString &);

	wxString get_time_str(uint32_t usec);
	const char *get_time_cstr(uint32_t usec);

	wxString conv_internal_name(const uint8_t *src);

	FILE *pw_fopen(const wxString &file, const wxString &mode);
//	int   write_data(FILE *fp, const char *data, int len, int width, int pos, int type);

	void  write_log(const wxString &buff, int crlf, FILE *fiolog = NULL, wxString *logbuf = NULL);

};

#endif /* _UTILS_H_ */
