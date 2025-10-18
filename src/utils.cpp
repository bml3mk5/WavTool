/// @file utils.cpp
///
/// @brief ユーティリティ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///
#include "utils.h"

namespace UTILS
{

/// @brief ファイルパスからファイル名取得
///
/// @param[in] src ファイルパス
/// @param[out] dst ファイル名を格納するバッファ
/// @param[in] size バッファサイズ
/// @return true
bool base_name(const _TCHAR *src, _TCHAR *dst, size_t size)
{
	const _TCHAR *p;
#ifdef _WIN32
	p = _tcsrchr(src, '\\');
#else
	p = _tcsrchr(src, '/');
#endif
	if (dst != NULL && size > 0 && p != NULL) {
		if (_tcslen(p+1) < size) size = _tcslen(p);
		_tcsncpy(dst, p+1, size);
		dst[size-1]=_T('\0');
	}
	return true;
}

/// @brief ファイルパスからファイル名取得
///
/// @param[in] src ファイルパス
/// @param[out] dst ファイル名
/// @return true
bool prefix_name(const wxString &src, wxString &dst)
{
	int p = wxNOT_FOUND;

#ifdef __WXMSW__
	if ((p = src.Find('\\', true)) != wxNOT_FOUND) {
#else
	if ((p = src.Find('/', true)) != wxNOT_FOUND) {
#endif
		dst = src.Mid(p+1);
	} else {
		dst = src;
	}
	if ((p = dst.Find('.', true)) != wxNOT_FOUND) {
		dst = dst.Left(p);
	}
	return true;
}

/// @brief 時間（文字列）を返す
///
/// @param[in] usec マイクロ秒
/// @return 文字列 mm'ss"ms
///
wxString get_time_str(uint32_t usec)
{
	wxString str;
	usec += 500;
	int ms = usec / 1000;
	int sec = ms / 1000;
	int msec = ms % 1000;
	int min = sec / 60;
	sec = sec % 60;

	str.Printf(_T("%d\'%02d\".%03d"), min, sec, msec);
	return str;
}

/// @brief 時間（文字列）を返す
///
/// @param[in] usec マイクロ秒
/// @return 文字列 mm'ss"ms
///
const char *get_time_cstr(uint32_t usec)
{
	static char str[50];
	usec += 500;
	int ms = usec / 1000;
	int sec = ms / 1000;
	int msec = ms % 1000;
	int min = sec / 60;
	sec = sec % 60;

#if defined(__WXMSW__)
	_snprintf(str, sizeof(str), "%d\'%02d\".%03d", min, sec, msec);
#else
	snprintf(str, sizeof(str), "%d\'%02d\".%03d", min, sec, msec);
#endif
	return str;
}

/// @brief 内部ファイル名のキャラクターコードを変換するテーブル
static const wxString chr2utf8tbl[128] = {
	_T("年"),_T("月"),_T("日"),_T("市"),_T("区"),_T("町"),_T("を"),_T("ぁ"),_T("ぃ"),_T("ぅ"),_T("ぇ"),_T("ぉ"),_T("ゃ"),_T("ゅ"),_T("ょ"),_T("っ"),
	_T("π"),_T("あ"),_T("い"),_T("う"),_T("え"),_T("お"),_T("か"),_T("き"),_T("く"),_T("け"),_T("こ"),_T("さ"),_T("し"),_T("す"),_T("せ"),_T("そ"),
	_T("〒"),_T("。"),_T("「"),_T("」"),_T("、"),_T("・"),_T("ヲ"),_T("ァ"),_T("ィ"),_T("ゥ"),_T("ェ"),_T("ォ"),_T("ャ"),_T("ュ"),_T("ョ"),_T("ッ"),
	_T("ー"),_T("ア"),_T("イ"),_T("ウ"),_T("エ"),_T("オ"),_T("カ"),_T("キ"),_T("ク"),_T("ケ"),_T("コ"),_T("サ"),_T("シ"),_T("ス"),_T("セ"),_T("ソ"),
	_T("タ"),_T("チ"),_T("ツ"),_T("テ"),_T("ト"),_T("ナ"),_T("ニ"),_T("ヌ"),_T("ネ"),_T("ノ"),_T("ハ"),_T("ヒ"),_T("フ"),_T("ヘ"),_T("ホ"),_T("マ"),
	_T("ミ"),_T("ム"),_T("メ"),_T("モ"),_T("ヤ"),_T("ユ"),_T("ヨ"),_T("ラ"),_T("リ"),_T("ル"),_T("レ"),_T("ロ"),_T("ワ"),_T("ン"),_T("゛"),_T("゜"),
	_T("た"),_T("ち"),_T("つ"),_T("て"),_T("と"),_T("な"),_T("に"),_T("ぬ"),_T("ね"),_T("の"),_T("は"),_T("ひ"),_T("ふ"),_T("へ"),_T("ほ"),_T("ま"),
	_T("み"),_T("む"),_T("め"),_T("も"),_T("や"),_T("ゆ"),_T("よ"),_T("ら"),_T("り"),_T("る"),_T("れ"),_T("ろ"),_T("わ"),_T("ん"),_T("■"),_T("　")
};

/// @brief 内部ファイル名のキャラクターコードをUTF-8に変換
///
/// @param[in] src 内部ファイル名
/// @return 変換後の文字列
wxString conv_internal_name(const uint8_t *src)
{
	wxString dst = _T("");

	for(int i=0; i<8; i++) {
		const uint8_t *p = &src[i];
		if (*p >= 0x80 && *p <= 0xff) {
			dst += chr2utf8tbl[(*p)-0x80];
		} else if (*p >= 0x20 && *p <= 0x7f) {
			dst += wxString::FromUTF8((const char *)p, 1);
		} else {
			dst += _T("?");
		}
	}
	return dst;
}

/// @brief ファイルをオープンする
///
/// @param[in] file ファイル名
/// @param[in] mode ファイルモード
/// @return ファイルポインタ
FILE *pw_fopen(const wxString &file, const wxString &mode)
{
#ifdef __WXMSW__
	return _tfopen(file, mode);
#else
	return fopen(file.fn_str(), mode.fn_str());
#endif
}

#if 0
///
static int compare_bit(const char *str, const char *sub, int len)
{
	int cmp = 0;
	for(int n = 0; n < len; n++) {
		cmp = (((int)sub[n] & 1) - ((int)str[n] & 1));
		if (cmp != 0) break;
	}
	return cmp;
}

///
static const char *find_bit(const char *str, const char *sub)
{
	const char *p = NULL;
	int len = (int)strlen(sub);
	int tail = (int)strlen(str) - len;
	for(int pos = 0; pos < tail; pos++) {
		if (compare_bit(str, sub, len) == 0) {
			p = &str[pos];
			break;
		}
	}
	return p;
}
#endif

#if 0
/// @brief データをファイルに出力
///
/// @param[in,out] fp ファイル
/// @param[in]     data  出力データ
/// @param[in]     len   出力データ長さ
/// @param[in]     width 1行の長さ
/// @param[in]     pos   1行の長さ
/// @param[in]     type  改行位置の判定方法
///                      0:width固定で改行
///                      1:widthを越えた"10"の位置で改行
///                      2:widthを越えた"10"または"01"の位置で改行
/// @return        最終行の出力した長さ
int write_data(FILE *fp, const char *data, int len, int width, int pos, int type)
{
	int i = 0;
	int w = 0;
	bool last = false;
	const char *p0, *p1;
	int pos0, pos1;
	char buf[8192];

	if (fp == NULL) return w;

	while (last != true) {
		w = (w == 0) ? (width - (pos % width)) : width;
		if ((i+w) > len) {
			w = len - i;
			last = true;
		}

		if (type >= 1 && last != true) {
			// 改行を入れる位置を判定する
			pos0 = -1;
			if (type >= 2) {
				p0 = strstr(&data[i+w-1],"01");
			} else {
				p0 = NULL;
			}
			if (p0 != NULL) {
				pos0 = (int)(p0 - data);
			}
			pos1 = -1;
			p1 = strstr(&data[i+w-1],"10");
			if (p1 != NULL) {
				pos1 = (int)(p1 - data);
			}
			if ((pos0 >= 0 && pos1 < 0) || (pos0 >= 0 && pos0 < pos1)) {
				w = pos0 - i + 1;
			} else if ((pos0 < 0 && pos1 >= 0) || (pos1 >= 0 && pos0 >= pos1)) {
				w = pos1 - i + 1;
			}
			if (w >= (width * 2)) {
				w = width;
			}
		}
		for (int n=0; n<w; n++) {
			buf[n] = data[i + n];
		}

		fwrite(buf, sizeof(char), w, fp);
		if (last != true || (w / width) >= 1) fwrite("\r\n", sizeof(char), 2, fp);

		i += w;
	}

	return w;
}
#endif

/// @brief ログファイルに出力
///
/// @param[in] buff メッセージ
/// @param[in] crlf 改行する行数
/// @param[in] fiolog ログファイル
/// @param[in] logbuf ログバッファ
///
void write_log(const wxString &buff, int crlf, FILE *fiolog, wxString *logbuf)
{
	if (buff.Length() > 0) {
		if (fiolog) _fputts(buff.c_str(), fiolog);
		if (logbuf) *logbuf += buff;
	}
	for (int i=0; i<crlf; i++) {
		if (fiolog) _fputts(_T("\n"), fiolog);
#ifdef _WIN32
		if (logbuf) *logbuf += _T("\r\n");
#else
		if (logbuf) *logbuf += _T("\n");
#endif
	}
}

};
