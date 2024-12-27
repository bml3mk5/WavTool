/// @file parsewav.cpp
///
/// @brief waveファイルを解析
///
#include "parsewav.h"
#include <wx/filename.h>
#include "utils.h"
#include "version.h"


namespace PARSEWAV
{

/// 処理中ダイアログを表示するか
#define USE_PROGRESSBOX 1

/// 解析時の読み込む秒数
#define ANALYZE_SEC	30

/// ギャップの長さ
static const int c_gap_length[4] = { 0x5a, 0xc0, 0x19b, 0x36f };

/// @brief コンストラクタ
///
ParseWav::ParseWav(wxWindow *parent)
{
	parent_window = parent;

	//
	wave_data = new WaveData();
	wave_correct_data = new WaveData();
	carrier_data = new CarrierData();
	serial_data = new SerialData();
	serial_new_data = new SerialData();
	binary_data = new BinaryData();

	wave_parser.SetParameter(param);
	carrier_parser.SetParameter(param);
	serial_parser.SetParameter(param);
	binary_parser.SetParameter(param);

	wave_parser.SetInputFile(infile);
	carrier_parser.SetInputFile(infile);
	serial_parser.SetInputFile(infile);
	binary_parser.SetInputFile(infile);

	logbuf = NULL;
	logfilename = _T("wavtool.log");

	include_header = true;

	err_num = pwErrNone;

	process_mode = PROCESS_IDLE;

#ifdef USE_PROGRESSBOX
	progbox = new ProgressBox(parent_window);
#endif
//	rftypebox = new RfTypeBox(parent_window, wxID_ANY);
//	maddressbox = new MAddressBox(parent_window, wxID_ANY);
	errinfo = new PwErrInfo();

	viewing_dir = 0;

	// wavファイルヘッダ(出力用)
	outwav.Init(48000, 8, 1);
}

/// @brief デストラクタ
///
ParseWav::~ParseWav()
{
	CloseDataFile();
	CloseOutFile();

	delete errinfo;
//	delete maddressbox;
//	delete rftypebox;
#ifdef USE_PROGRESSBOX
	delete progbox;
#endif

	delete binary_data;
	delete serial_new_data;
	delete serial_data;
	delete carrier_data;
	delete wave_correct_data;
	delete wave_data;
}


/// @brief 入力ファイルのフォーマットをチェックする
///
/// @param[in]     file        入力ファイル
/// @return    pwOK:正常 pwError:エラーあり pwWarn:警告あり
PwErrType ParseWav::CheckFileFormat(InputFile &file) {
	PwErrType rc;

	// check file size
	char buf[64];
	size_t len = file.Fread(buf, sizeof(char), sizeof(buf));
	if (len == 0) {
		err_num = pwErrFileEmpty;
		errinfo->SetInfo(__LINE__, pwError, err_num);
		errinfo->ShowMsgBox();
		return pwError;
	}
	file.Fseek(0, SEEK_SET);

	// check wave header
	enum_file_type file_type = file.GetType();
	if (file_type == FILETYPE_WAV) {
		rc = wave_parser.CheckWaveFormat(file, inwav.GetHead(), inwav.GetFmtChank(), inwav.GetDataChank(), conv, err_num, *errinfo);
	} else if (file_type == FILETYPE_L3C) {
		rc = carrier_parser.CheckL3CFormat(file);
	} else if (file_type == FILETYPE_L3B) {
		rc = serial_parser.CheckL3BFormat(file);
	} else if (file_type == FILETYPE_T9X) {
		rc = serial_parser.CheckT9XFormat(file, err_num, *errinfo);
	} else if (file_type == FILETYPE_L3) {
		rc = binary_parser.CheckL3Format(file);
	} else {
		rc = check_rf_format(file);
	}

	return rc;
}

/// @brief 入力ファイルのフォーマットからデータ開始位置をシーク
///
/// @param[in] file      入力ファイル
/// @return    pwOK:正常 pwError:エラーあり pwWarn:警告あり
PwErrType ParseWav::SeekFileFormat(InputFile &file) {
	PwErrType rc = pwOK;

	// check file size
	char buf[64];
	size_t len = file.Fread(buf, sizeof(char), sizeof(buf));
	if (len == 0) {
		err_num = pwErrFileEmpty;
		errinfo->SetInfo(__LINE__, pwError, err_num);
		errinfo->ShowMsgBox();
		return pwError;
	}
	file.Fseek(0, SEEK_SET);

	// seek header
	enum_file_type file_type = file.GetType();
	if (file_type == FILETYPE_WAV) {
		rc = wave_parser.CheckWaveFormat(file, inwav.GetHead(), inwav.GetFmtChank(), inwav.GetDataChank(), conv, err_num, *errinfo);
	} else if (file_type == FILETYPE_T9X) {
		rc = serial_parser.CheckT9XFormat(file, err_num, *errinfo);
	}

	return rc;
}

/// @brief ファイルのヘッダをファイルに出力する
///
/// @param[in,out] file      出力ファイル
/// @return ヘッダ長さ
///
int ParseWav::InitFileHeader(OutputFile &file)
{
	int len_all = 0;

	if (!file.IsOpened()) return len_all;

	if (file.GetType() == FILETYPE_WAV) {
		// wavファイルのサンプルレート、ビットなどを設定
		outwav.Init(param.GetSampleRate(), (param.GetSampleBitsPos() == 0 ? 8 : 16), 1);

		conv.InitConvSampleData();

		len_all = outwav.Out(file);

	} else if (file.GetType() == FILETYPE_T9X) {
		t9x_header_t head;

		memset(&head, 0, sizeof(head));
		memcpy(head.ident, T9X_IDENTIFIER, sizeof(head.ident));
		file.Fwrite((void *)&head, sizeof(head), 1);
		len_all = sizeof(head);
	}

	return len_all;
}

/// @brief ファイルにファイルサイズを書き込む
///
/// @param[in,out] file          出力ファイル
///
///
void ParseWav::SetFileHeader(OutputFile &file)
{
	if (!file.IsOpened()) return;

	if (file.GetType() == FILETYPE_WAV) {
		// WAVファイル以外からの変換の場合、データ末尾に2400/4800Hzを0.5sec出力
		out_dummy_tail_data(file);
		// ファイルサイズ、データサイズを書き込む
		WaveFormat::OutSize(file);

	} else if (file.GetType() == FILETYPE_T9X) {
		t9x_header_t head;

		memset(&head, 0, sizeof(head));
		memcpy(head.ident, T9X_IDENTIFIER, sizeof(head.ident));
		head.data2 = 9;	// TODO: what's this?

		file.Fseek(0, SEEK_SET);
		file.Fwrite((void *)&head, sizeof(head), 1);

	}

	return;
}

/// WAVファイル以外からの変換の場合、データ末尾に2400/4800Hzを0.5sec出力
void ParseWav::out_dummy_tail_data(OutputFile &file)
{
	if (infile.GetType() != FILETYPE_WAV) {
		int spd = param.GetFskSpeed();
		int samples = 48000 / 2400 / (spd + 1);
		uint8_t w_data[200];
		for(int j=0; j<120; j++) {
			uint8_t c = carrier_data->GetRead(-1).Data();	// 最後のデータ
			carrier_data->Shift();
			for(int i=0; i<samples; i++) {
				c = ((1 - (c & 1)) | 0x30);
				carrier_data->Add(c, 0);
			}
			int pos = 0;
			while(!carrier_data->IsTail()) {
				pos += wave_parser.EncodeToWave(carrier_data, &w_data[pos], sizeof(w_data) - pos);
			}
			// convert
			conv.OutConvSampleData(w_data, 48000, 1, pos, outwav.GetSampleRate(), outwav.GetBlockSize(), file);
		}
	}
}

/// @brief 実ファイルのフォーマットをチェック
///
/// @param[in] file 入力ファイル
/// @return pwOK:OK pwCancel:キャンセル終了
PwErrType ParseWav::check_rf_format(InputFile &file)
{
	int rc;
	_TCHAR bname[_MAX_PATH];

	UTILS::base_name(file.GetName(), bname, _MAX_PATH);

	// ファイルの種類を選択
	rftypeparam.Initialize();
	RfTypeBox rftypebox(parent_window, rftypeparam);
	rc = rftypebox.showRftypeBox(bname, true);
	if (rc != 1) {
		// cancel button
		return pwCancel;
	}

	PwErrType rt = get_first_rf_data(file);

	return rt;
}

/// @brief 実ファイルを読みそのファイル種類を判定する
///
/// @param[in] file 入力ファイル
/// @return pwOK:OK pwWarning:警告あり
///
PwErrType ParseWav::get_first_rf_data(InputFile &file)
{
	PwErrType rc = pwOK;
	int dfmt;
	int dtype;

	include_header = true;

	// ファイルサイズ計算
	int sample_num = file.GetSize();
	file.SampleNum(sample_num);

	enum_file_type file_type = infile.GetType();

	switch(rftypeparam.GetRfDataFileType()) {
	case 0:
		// ただのファイル指定の場合
		file_type = FILETYPE_PLAIN;
		break;
	default:
		// 実ファイル指定の場合
		dfmt = rftypeparam.GetRfDataFormat();
		dtype = rftypeparam.GetRfDataType();
		switch(dfmt) {
			case 0:	// basic
				file_type = FILETYPE_REAL;
				if (dtype != 0xff) { // 中間言語形式の場合
					// ファイルの最初を読み込む
					file.Fread(rf_header, sizeof(uint8_t), sizeof(rf_header));
					if (rf_header[0] != 0xff && rf_header[0] != 0xfe) {
						// maybe not have header
						err_num = pwErrNoBASICIntermediateLanguage;
						rc = pwWarning;
						errinfo->SetInfo(__LINE__, rc, err_num);
						errinfo->ShowMsgBox();
					}
				}
				break;

			case 2: // machine
				// マシン語アドレス決定ダイアログを表示
				decide_maddress(file, file_type);
				break;
		}
		break;
	}
	file.SetType(file_type);
	file.Fseek(0, SEEK_SET);

	return rc;
}

/// @brief マシン語アドレス決定ダイアログを表示
void ParseWav::decide_maddress(InputFile &file, enum_file_type &file_type)
{
	int sample_num = file.SampleNum();
	int addr;
//	maddress_t val;

	// ファイルの最初/最後を読み込む
	file.Fread(rf_header, sizeof(uint8_t), sizeof(rf_header));
	file.Fseek(-5, SEEK_END);
	file.Fread(rf_footer, sizeof(uint8_t), sizeof(rf_footer));

	if (rf_header[0] != 0x00 || rf_footer[0] != 0xff || rf_footer[1] != 0x00) {
		// maybe not have header and footer
		include_header = false;
//		val = maddressbox->get();
		maddressparam.Valid(false);
		maddressparam.SetDataSize(sample_num);
		maddressparam.SetFileSize(sample_num);
		maddressparam.IncludeHeader(include_header);
//		maddressbox->set(val);
	} else {
//		val = maddressbox->get();
		maddressparam.Valid(true);
		addr = rf_header[3] * 256 + rf_header[4];
		maddressparam.SetStartAddr(addr);
		addr = rf_header[1] * 256 + rf_header[2];
		maddressparam.SetDataSize(addr);
		addr = rf_footer[3] * 256 + rf_footer[4];
		maddressparam.SetExecAddr(addr);
		maddressparam.IncludeHeader(include_header);
		maddressparam.SetFileSize(sample_num);
//		maddressbox->set(val);
		file_type = FILETYPE_REAL;
	}
	// ダイアログ表示
	ShowMAddressBox(true);
}

/// @brief 実ファイルから１データ読む
///
/// @return １バイトデータ
///
///
uint8_t ParseWav::get_rf_sample()
{
	int l;

	if (binary_parser.IsMachineData()) {
		// マシン語
		if (infile.SamplePos() < 5) {
			if (include_header) l = infile.Fgetc();
			l = rf_header[infile.SamplePos()];
		} else if (infile.SampleNum() - infile.SamplePos() <= 5) {
			if (include_header) l = infile.Fgetc();
			l = rf_footer[5 - infile.SampleNum() + infile.SamplePos()];
		} else {
			l = infile.Fgetc();
		}
	} else {
		l = infile.Fgetc();
	}
	infile.IncreaseSamplePos();

	return (l & 0xff);
}

/// @brief waveサンプルデータから搬送波(1200/2400Hz)を解析し
/// @brief ビットデータに変換する。(FSK)
///
/// @param[in]  fsk_spd 1:倍速FSK
/// @param[in]  w_data waveデータ
/// @param[out] wc_data wave補正データ
/// @param[out] c_data 搬送波データ
/// @param[out] s_data シリアルデータ
/// @param[out] sn_data 変換後シリアルデータ
/// @param[out] b_data バイナリデータ
/// @param[in]  start_phase 開始フェーズ
/// @return フェーズ番号
int ParseWav::decode_phase1(int fsk_spd, WaveData *w_data, WaveData *wc_data, CarrierData *c_data, SerialData *s_data, SerialData *sn_data, BinaryData *b_data, enum_phase start_phase)
{
	int rc = 0;
	int progress_num;
	int w_sum, wc_sum;
	int correct_type = tmp_param.GetCorrectType();
	bool reverse_wave = tmp_param.GetReverseWave();
	WaveData *wn_data;

	if (infile.GetType() == FILETYPE_WAV && (correct_type > 0 || process_mode == PROCESS_ANALYZING)) {
		// 補正あり
		wn_data = wc_data;
		dft.Init(wave_parser.GetLamda().samples[fsk_spd], correct_type, param.GetCorrectAmp(0), param.GetCorrectAmp(1));
	} else {
		// 補正なし
		wn_data = w_data;
	}

	while(phase1 > PHASE_NONE) {
		if (process_mode == PROCESS_ANALYZING) {
			progress_num = st_chkwav_analyzed_num + infile.SamplePos();
		} else {
			progress_num = infile.SamplePos();
		}
		if (tmp_param.GetViewProgBox() && needSetProgress()) {
			if (setProgress(progress_num, progress_div)) {
				phase1 = PHASE_NONE;
				break;
			}
		}
		switch(phase1) {
			case PHASE1_GET_WAV_SAMPLE:
				// WAVファイル読み込み
				wave_parser.GetWaveSample(w_data, reverse_wave);
				// チェックモードの場合は約30秒まで解析
				if (process_mode == PROCESS_ANALYZING && infile.SamplePos() >= st_chkwav[fsk_spd].analyze_num) {
					w_data->LastData(true);
				}
				phase1 = PHASE1_CORRECT_WAVE;
				break;
			case PHASE1_CORRECT_WAVE:
				// WAVサンプルデータを補正する
				if (infile.GetType() == FILETYPE_WAV && (correct_type > 0 || process_mode == PROCESS_ANALYZING)) {
					dft.Calcrate(w_data, wc_data);
				}
				if (outfile.GetType() == FILETYPE_WAV && outfile.GetType() >= infile.GetType()) {
					// WAV ファイル出力
					conv.OutConvSampleData(*wn_data, inwav.GetSampleRate(), outwav.GetSampleRate(), outwav.GetBlockSize(), outfile);
				}
				// データ最後まで読んだ場合
				if (correct_type > 0 || process_mode == PROCESS_ANALYZING) {
					if (w_data->IsLastData() && w_data->IsTail((int)wave_parser.GetLamda().samples[1] + 2)) {
						wc_data->LastData(true);
					}
				}
				// 表示モードで戻る場合
				if (process_mode == PROCESS_VIEWING && viewing_dir < 0) {
					w_data->Shift((int)wave_parser.GetLamda().samples[1] + 2);
					wc_data->Shift((int)wave_parser.GetLamda().samples[1] + 2);
				}

				phase1 = PHASE1_DECODE_TO_CARRIER;
				break;
			case PHASE1_DECODE_TO_CARRIER:
				// L3Cへ変換
				rc = wave_parser.DecodeToCarrier(fsk_spd, wn_data, c_data);
				if (rc & 0x10) {
					// cバッファがいっぱいになったら吐き出す
					phase1 = PHASE1_PUT_L3C_SAMPLE;
				} else if (process_mode == PROCESS_VIEWING) {
					// 波形ウィンドウ表示モードの場合
					if(rc & 0x01) {
						// wバッファをすべて解析した
						if (rc & 0x02) c_data->LastData(true);
						phase1 = PHASE1_PUT_L3C_SAMPLE;
					}
				} else {
#ifdef PARSEWAV_FILL_BUFFER
					if ((rc & 0x03) == 0x03) {
						// 最後のデータ
						c_data->LastData(true);
						phase1 = PHASE1_PUT_L3C_SAMPLE;
					} else if((rc & 0x03) == 0x01) {
						// cバッファがいっぱいになるまで繰り返す
						phase1 = PHASE1_SHIFT_BUFFER;
					}
#else
					if(rc & 0x01) {
						// wバッファをすべて解析した
						if (rc & 0x02) c_data->LastData(true);
						phase1 = PHASE1_PUT_L3C_SAMPLE;
					}
#endif
				}
				break;
			case PHASE1_GET_L3C_SAMPLE:
				// L3Cファイル読み込み
				carrier_parser.GetL3CSample(c_data);
				phase1 = PHASE1_PUT_L3C_SAMPLE;
				break;
			case PHASE1_PUT_L3C_SAMPLE:
				if (outfile.GetType() == FILETYPE_L3C && outfile.GetType() >= infile.GetType()) {
					// L3C ファイル出力
					carrier_parser.WriteL3CData(outfile, c_data);
				}
				// 更に変換(L3C -> L3B -> L3)
				if (outfile.GetType() >= FILETYPE_WAV) {
					if (process_mode == PROCESS_ANALYZING) {
						phase2 = PHASE2_PARSE_CARRIER;
						decode_phase2(fsk_spd, c_data, s_data, sn_data, b_data, PHASE2_PARSE_CARRIER);
					} else {
						decode_phase2(fsk_spd, c_data, s_data, sn_data, b_data, PHASE2_DECODE_TO_SERIAL);
					}
				}
				// 波形ウィンドウ表示の場合は解析終り
				if (process_mode == PROCESS_VIEWING) {
					phase1 = PHASE_NONE;
				} else {
					phase1 = PHASE1_SHIFT_BUFFER;
				}
				break;
			case PHASE1_SHIFT_BUFFER:
				// 残りをバッファの最初にコピー→次のターンで解析をするため
				w_sum = w_data->GetReadPos() - (int)wave_parser.GetLamda().samples[1] - 2;
				if (correct_type > 0 || process_mode == PROCESS_ANALYZING) {
					wc_sum = wc_data->GetReadPos() - 2;
					w_sum = (w_sum > wc_sum ? wc_sum : w_sum);
				}
				if (w_sum > 0) {
					w_data->Shift(w_sum);
					wc_data->Shift(w_sum);
				}

				if (c_data->IsLastData() && c_data->IsTail()) {
					phase1 = PHASE_NONE;
					break;
				}

				if (process_mode == PROCESS_ANALYZING && w_data->IsLastData()) {
					phase1 = PHASE_NONE;
				} else {
					phase1 = start_phase;
				}
				break;
			default:
				// ??
				phase1 = PHASE_NONE;
				break;
		}
	}

	return phase1;
}

/// @brief 搬送波(1200/2400Hz)データからシリアルデータに変換
/// ここでボーレートによってシリアルデータが変わってくる
///
/// @param[in]  fsk_spd 1:倍速FSK
/// @param[in] c_data  搬送波データ
/// @param[out] s_data シリアルデータ
/// @param[out] sn_data 変換後シリアルデータ
/// @param[out] b_data バイナリデータ
/// @param[in] start_phase 開始フェーズ
/// @return フェーズ番号
int ParseWav::decode_phase2(int fsk_spd, CarrierData *c_data, SerialData *s_data, SerialData *sn_data, BinaryData *b_data, enum_phase start_phase)
{
	int step = 0;
	int rc = 0;
	bool break_data = false;
	int8_t baud = (tmp_param.GetAutoBaud() ? IDX_PTN_2400 : param.GetBaud());	///< 自動判定のときは2400ボーにする
	int progress_num;
	int c_read_pos = c_data->GetReadPos();

	while(phase2 > PHASE_NONE) {
		if (process_mode == PROCESS_ANALYZING) {
			progress_num = st_chkwav_analyzed_num + infile.SamplePos();
		} else {
			progress_num = infile.SamplePos();
		}
		if (tmp_param.GetViewProgBox() && needSetProgress()) {
			if (setProgress(progress_num, progress_div)) {
				phase2 = PHASE_NONE;
				phase1 = PHASE_NONE;
				break;
			}
		}
		// バッファの末尾
//		if (!c_data->IsLastData() && c_data->IsTail(32)) {
//			if (process_mode == PROCESS_VIEWING) {
//				phase2 = PHASE2_GOTO_PHASE2N;
//			} else {
//				break_data = true;
//			}
//		}
		if (break_data == true) {
			break;
		}
		switch (phase2) {
			case PHASE2_DECODE_TO_SERIAL:
				// 搬送波からシリアルデータに変換できる位置をさがし
				// 搬送波をシリアルデータに変換
				rc = carrier_parser.Decode(c_data, s_data, baud, step);
				rc &= 0xffff;
				if (rc == 1) {
					if (process_mode == PROCESS_ANALYZING) {
						phase2 = start_phase;
						break_data = true;
					} else {
						// sバッファを解析する
						phase2 = PHASE2_GOTO_PHASE2N;
					}
				} else if (rc == 2) {
					// sバッファを埋めるため phase1へ戻る
					break_data = true;
				}
				break;
			case PHASE2_PARSE_CARRIER:
				// 搬送波からボーレートを解析
				rc = carrier_parser.ParseBaudRate(fsk_spd, c_data, st_chkwav, step);
				if (rc == 1) {
					// sバッファを解析する
					c_data->AddReadPos(c_read_pos - c_data->GetReadPos());
					phase2 = PHASE2_DECODE_TO_SERIAL;
				} else if (rc == 2) {
					// sバッファを埋めるため phase1へ戻る
					break_data = true;
				}
				break;
			case PHASE2_GET_L3B_SAMPLE:
				serial_parser.GetL3BSample(s_data);
				phase2 = PHASE2_GOTO_PHASE2N;
				break;
			case PHASE2_GET_T9X_SAMPLE:
				serial_parser.GetT9XSample(s_data);
				phase2 = PHASE2_GOTO_PHASE2N;
				break;
			case PHASE2_GOTO_PHASE2N:
				if (outfile.GetType() >= FILETYPE_WAV) {
					decode_phase2n(s_data, sn_data, b_data, PHASE2N_CONVERT_BAUD_RATE);
				}
				if (s_data->IsLastData() && s_data->IsTail()) {
					phase2 = PHASE_NONE;
					break;
				}
				if (process_mode == PROCESS_VIEWING) {
					phase2 = PHASE_NONE;
					break;
				}
				phase2 = start_phase;
				if (infile.GetType() < FILETYPE_L3B) {
					break_data = true;
				}
				break;
			default:
				// ??
				phase2 = PHASE_NONE;
				break;
		}
	}
	if (process_mode != PROCESS_VIEWING && !c_data->IsLastData() && break_data == true) {
		// 残りをバッファの最初にコピー→次のターンで解析をするため
		c_data->Shift();
	}
	return phase2;
}

/// @brief L3B -> ボーレート変換 ->  L3B
///
/// @param[in] s_data シリアルデータ
/// @param[out] sn_data 変換後シリアルデータ
/// @param[out] b_data バイナリデータ
/// @param[in] start_phase 開始フェーズ
/// @return フェーズ番号
int ParseWav::decode_phase2n(SerialData *s_data, SerialData *sn_data, BinaryData *b_data, enum_phase start_phase)
{
//	int step = 0;
	int rc = 0;
//	int baud = 0;
	bool break_data = false;

	while(phase2n > PHASE_NONE) {
		if (tmp_param.GetViewProgBox() && needSetProgress()) {
			if (setProgress(infile.SamplePos(), infile.SampleNum())) {
				phase1 = PHASE_NONE;
				phase2 = PHASE_NONE;
				phase2n = PHASE_NONE;
				break;
			}
		}
		// バッファの末尾
//		if (s_data->IsLastData() != true && s_data->IsTail(32)) {
//			if (process_mode == PROCESS_VIEWING) {
//				phase2n = PHASE2N_PUT_L3B_SAMPLE;
//			} else {
//				break_data = true;
//			}
//		}
		if (break_data == true) {
			break;
		}
		switch (phase2n) {
			case PHASE2N_CONVERT_BAUD_RATE:
				rc = serial_parser.ConvertBaudRate(s_data, sn_data);
				if (rc == 1) {
					phase2n = PHASE2N_PUT_L3B_SAMPLE;
				} else if (rc == 2) {
					break_data = true;
				}
				break;
			case PHASE2N_GET_L3B_SAMPLE:
				serial_parser.GetL3BSample(sn_data);
				phase2n = PHASE2N_PUT_L3B_SAMPLE;
				break;
			case PHASE2N_GET_T9X_SAMPLE:
				serial_parser.GetT9XSample(sn_data);
				phase2n = PHASE2N_PUT_L3B_SAMPLE;
				break;
			case PHASE2N_PUT_L3B_SAMPLE:
				if (outfile.GetType() == FILETYPE_L3B && outfile.GetType() >= infile.GetType()) {
					// L3Bファイル出力
					serial_parser.WriteL3BData(outfile, sn_data);
				}
				else if (outfile.GetType() == FILETYPE_T9X && outfile.GetType() >= infile.GetType()) {
					// T9Xファイル出力
					serial_parser.WriteT9XData(outfile, sn_data);
				}
				if (outfile.GetType() >= FILETYPE_WAV) {
					decode_phase3(sn_data, b_data, PHASE3_DECODE_TO_BINARY);
				}
				if (s_data->IsLastData() && s_data->IsTail()) {
					phase2n = PHASE_NONE;
					break;
				}
				if (sn_data->IsLastData() && sn_data->IsTail()) {
					phase2n = PHASE_NONE;
					break;
				}
				if (process_mode == PROCESS_VIEWING) {
					phase2n = PHASE_NONE;
					break;
				}
				phase2n = start_phase;
				break_data = true;
				break;
			default:
				// ??
				phase2n = PHASE_NONE;
				break;
		}
	}
	if (process_mode != PROCESS_VIEWING && !s_data->IsLastData() && break_data == true) {
		// 残りをバッファの最初にコピー→次のターンで解析をするため
		s_data->Shift();
	}
	return phase2n;
}

/// @brief L3B -> L3 シリアルからバイナリに変換
///
/// @param[in] s_data シリアルデータ
/// @param[out] b_data バイナりデータ
/// @param[in] start_phase 開始フェーズ
/// @return フェーズ番号
int ParseWav::decode_phase3(SerialData *s_data, BinaryData *b_data, enum_phase start_phase)
{
//	int step = 0;
	int rc = 0;
	bool break_data = false;

	while(phase3 > PHASE_NONE) {
		if (tmp_param.GetViewProgBox() && needSetProgress()) {
			if (setProgress(infile.SamplePos(), infile.SampleNum())) {
				phase1 = PHASE_NONE;
				phase2 = PHASE_NONE;
				phase2n = PHASE_NONE;
				phase3 = PHASE_NONE;
				break;
			}
		}
		// バッファの末尾
//		if (!s_data->IsLastData() && s_data->IsTail(32)) {
//			if (process_mode == PROCESS_VIEWING) {
//				phase3 = PHASE3_PUT_L3_SAMPLE;
//			} else {
//				break_data = true;
//			}
//		}
		if (break_data == true) {
			break;
		}
		switch (phase3) {
			case PHASE3_DECODE_TO_BINARY:
				// find start bit / decode to binary data
				rc = serial_parser.Decode(s_data, b_data);
				if (rc == 1) {
					phase3 = PHASE3_PUT_L3_SAMPLE;
				} else if (rc == 2) {
					break_data = true;
				}
				break;
			case PHASE3_GET_L3_SAMPLE:
				binary_parser.GetL3Sample(b_data);
				phase3 = PHASE3_PUT_L3_SAMPLE;
				break;
			case PHASE3_PUT_L3_SAMPLE:
				if (outfile.GetType() == FILETYPE_L3 && outfile.GetType() >= infile.GetType()) {
					// L3ファイル出力
					binary_parser.WriteL3Data(outfile, b_data);
				}
				if (outfile.GetType() >= FILETYPE_WAV) {
					decode_phase4(b_data);
				}
				if (b_data->IsLastData() && b_data->IsTail()) {
					phase3 = PHASE_NONE;
					break;
				}
				if (process_mode == PROCESS_VIEWING) {
					phase3 = PHASE_NONE;
					break;
				}

				phase3 = start_phase;
				if (infile.GetType() < FILETYPE_L3) {
					break_data = true;
				}
				break;
			default:
				// ??
				phase3 = PHASE_NONE;
				break;
		}
	}
	if (process_mode != PROCESS_VIEWING && s_data->IsLastData() != true && break_data == true) {
		// 残りをバッファの最初にコピー→次のターンで解析をするため
		s_data->Shift();
	}
	return phase3;
}

/// @brief L3→実データ変換
///
/// @param[in] b_data バイナリデータ
/// @return フェーズ番号
///
int ParseWav::decode_phase4(BinaryData *b_data)
{
	int rc = 0;
	bool break_data = false;

	// ヘッダ解析
	while(phase4 > PHASE_NONE) {
		// バッファの末尾か？
		if (!b_data->IsLastData() && b_data->IsTail(256)) {
			break_data = true;
		} else if (b_data->IsLastData() && b_data->IsTail(0)) {
			phase4 = PHASE_NONE;
			break_data = true;
		}
		if (break_data == true) {
			break;
		}

		switch(phase4) {
			case PHASE4_FIND_HEADER:
				// 0xff 0x01 0x3c を探す
				rc = binary_parser.FindHeader(b_data);
				if (rc >= 0) {
					// found
					// type?
					switch(rc) {
						case 0:
							// file name section
							phase4 = PHASE4_PARSE_NAME_SECTION;
							break;
						case 1:
							// body data section
							phase4 = PHASE4_PARSE_BODY_SECTION;
							break;
						case 0xff:
							// footer data section
							phase4 = PHASE4_PARSE_FOOTER_SECTION;
							break;
						default:
							break;
					}
					b_data->AddReadPos(4);
				} else {
					b_data->IncreaseReadPos();
				}
				break;
			case PHASE4_PARSE_NAME_SECTION:
				// file name section
				rc = binary_parser.ParseNameSection(b_data, outfile, outsfileb, outsext, outsfile);
				if (rc < 0) {
					// バッファをフラッシュ
					break_data = true;
					break;
				}

				phase4 = PHASE4_FIND_HEADER;
				break;
			case PHASE4_PARSE_BODY_SECTION:
				// body data section
				rc = binary_parser.ParseBodySection(b_data, outfile, outsfile);
				if (rc < 0) {
					// バッファをフラッシュ
					break_data = true;
					break;
				}

				phase4 = PHASE4_FIND_HEADER;
				break;
			case PHASE4_PARSE_FOOTER_SECTION:
				// footer data section
				rc = binary_parser.ParseFooterSection(b_data, outfile, outsfile);
				if (rc < 0) {
					// バッファをフラッシュ
					break_data = true;
					if (rc == -1) {
						// そのままfooterに
						break;
					}
				}

				phase4 = PHASE4_FIND_HEADER;	// end -> parse next data
				break;
			default:
				// ??
				phase4 = PHASE_NONE;
				break;
		}
	}
	if (process_mode != PROCESS_VIEWING && !b_data->IsLastData() && break_data == true) {
		// 残りをバッファの最初にコピー→次のターンで解析をするため
		b_data->Shift();
	}
//	if (phase4 <= PHASE_NONE) {
//		// add report data
//		binary_parser.FlushResult();
//	}

	return phase4;
}

/// @brief 実データ → ベタデータ
///
/// @return フェーズ番号
///
int ParseWav::decode_plain_data()
{
	return 0;
}

/// @brief ベタデータ → 実データ
///
/// @param[out] b_data バイナリデータ
/// @return フェーズ番号
///
int ParseWav::encode_plain_data(BinaryData *b_data)
{
	while(infile.SamplePos() < infile.SampleNum()) {
		b_data->Add(get_rf_sample(), 0);
	}
	outfile.WriteData(*b_data);
	return 0;
}

/// @brief 実ファイル -> L3
///
/// @param[out] b_data バイナリデータ
/// @param[out] s_data シリアルデータ
/// @param[out] c_data 搬送波データ
/// @return フェーズ番号
int ParseWav::encode_phase4(BinaryData *b_data, SerialData *s_data, CarrierData *c_data)
{
	int rc = 0;
	int hlen = c_gap_length[0];

	if (param.GetChangeGapSize() != 0 || outfile.GetType() <= FILETYPE_L3C) {
		hlen = param.GetBaud();
		if (hlen >= 3) hlen = 0;
		hlen += (param.GetFskSpeed() & 1);
		hlen = c_gap_length[hlen];
	}

	while (phase4 > PHASE_NONE) {
		switch(phase4) {
			case PHASE4_PUT_HEADER:
				binary_parser.PutHeaderSection(b_data, hlen);

				phase4 = PHASE4_PUT_BODY_DATA;
				break;

			case PHASE4_PUT_BODY_DATA:
				// body data
				binary_parser.ClearPrevData();
				for(int i=0; i<255 && infile.SamplePos() < infile.SampleNum(); i++) {
					binary_parser.AddPrevData(get_rf_sample());
				}
				rc = binary_parser.PutBodySection(b_data, hlen);
				if (rc == 1) {
					// gap reset when it's binary save
					hlen = 10;
				}
				if (infile.SamplePos() >= infile.SampleNum()) {
					phase4 = PHASE4_PUT_FOOTER_DATA;
				}
				break;

			case PHASE4_PUT_FOOTER_DATA:
				// footer data
				binary_parser.PutFooterSection(b_data, hlen);

				phase4 = PHASE_NONE;
				break;
			default:
				// ??
				phase4 = PHASE_NONE;
				break;
		}
		if (b_data->IsFull(300) || phase4 <= PHASE_NONE) {
			if (outfile.GetType() == FILETYPE_L3) {
				outfile.WriteData(*b_data);
			}
			phase3 = PHASE3_GET_BIN_SAMPLE;
			if (outfile.GetType() < FILETYPE_L3) encode_phase3(b_data, s_data, c_data, PHASE3_GET_BIN_SAMPLE);
			b_data->Clear();
		}
	}

	return phase4;
}

/// @brief L3 -> L3B バイナリからシリアルに変換
///
/// @param[in] b_data バイナリデータ
/// @param[out] s_data シリアルデータ
/// @param[out] c_data 搬送波データ
/// @param[in] start_phase 開始フェーズ
/// @return フェーズ番号
int ParseWav::encode_phase3(BinaryData *b_data, SerialData *s_data, CarrierData *c_data, enum_phase start_phase)
{
//	int pos = 0;
//	int step = 0;
	bool last = false;
	uint8_t bin_data = 0;

//	pos = strlen(serial_data);

//	int prog_cnt = 0;
	while(phase3 > PHASE_NONE) {
//		if (prog_cnt < 16) {
			if (tmp_param.GetViewProgBox() && needSetProgress()) {
				if (setProgress(infile.SamplePos(), infile.SampleNum())) {
					phase3 = PHASE_NONE;
					break;
				}
			}
//			prog_cnt = 0;
//		} else {
//			prog_cnt++;
//		}
		switch (phase3) {
			case PHASE3_GET_BIN_SAMPLE:
				// バイナリデータ読み込み
				bin_data = b_data->GetRead().Data();
				b_data->IncreaseReadPos();
				if (b_data->IsTail()) {
					b_data->LastData(true);
					last = true;
				}
				phase3 = PHASE3_ENCODE_TO_SERIAL;
				break;
			case PHASE3_GET_L3_SAMPLE:
				// l3データ読み込み or PLAINデータ読み込み
				bin_data = binary_parser.GetL3Sample();
				if (infile.SamplePos() >= infile.SampleNum()) {
					b_data->LastData(true);
					last = true;
				}
				phase3 = PHASE3_ENCODE_TO_SERIAL;
				break;
			case PHASE3_ENCODE_TO_SERIAL:
				// シリアルデータに変換
				serial_parser.EncodeToSerial(bin_data, s_data);
				phase3 = start_phase;
				if (s_data->IsFull(10)) {
					phase3 = PHASE3_PUT_L3B_SAMPLE;
				}
				if (b_data->IsLastData()) {
					s_data->LastData(true);
					phase3 = PHASE3_PUT_L3B_SAMPLE;
				}
				break;
			case PHASE3_GET_L3B_SAMPLE:
				// l3bデータ読み込み
				serial_parser.GetL3BSample(s_data);
				phase3 = PHASE3_PUT_L3B_SAMPLE;
				if (s_data->IsLastData()) {
					last = true;
				}
				break;
			case PHASE3_GET_T9X_SAMPLE:
				// t9xデータ読み込み
				serial_parser.GetT9XSample(s_data);
				phase3 = PHASE3_PUT_L3B_SAMPLE;
				if (s_data->IsLastData()) {
					last = true;
				}
				break;
			case PHASE3_PUT_L3B_SAMPLE:
				// 次のフェーズへ
				if (outfile.GetType() == FILETYPE_L3B) {
					serial_parser.WriteL3BData(outfile, s_data);
					s_data->Clear();
				}
				else if (outfile.GetType() == FILETYPE_T9X) {
					serial_parser.WriteT9XData(outfile, s_data);
					s_data->Clear();
				}
				if (outfile.GetType() < FILETYPE_L3B) {
					encode_phase2(s_data, c_data, PHASE2_ENCODE_TO_CARRIER, last);
				}

				if (last) {
					phase3 = PHASE_NONE;
					break;
				}
				phase3 = start_phase;
				break;
			default:
				// ??
				phase3 = PHASE_NONE;
				break;
		}
	}
	return phase3;

}

/// @brief l3b -> l3c シリアルデータを搬送波ビットデータに変換する
///
/// @param[in] s_data シリアルデータ
/// @param[out] c_data 搬送波データ
/// @param[in] start_phase 開始フェーズ
/// @param[in] end_data 最終データか？
/// @return フェーズ番号
int ParseWav::encode_phase2(SerialData *s_data, CarrierData *c_data, enum_phase start_phase, bool end_data)
{
//	int pos = 0;
//	int step = 0;
//	int pos_s = 0;
//	int len = 0;
	bool last = false;
	bool break_data = false;

//	len = strlen(serial_data);
//	pos = strlen(carrier_data);
//	pos = c_data->get_w_pos();

//	int prog_cnt = 0;
	while(phase2 > PHASE_NONE) {
//		if (prog_cnt < 16) {
			if (tmp_param.GetViewProgBox() && needSetProgress()) {
				if (setProgress(infile.SamplePos(), infile.SampleNum())) {
					phase3 = PHASE_NONE;
					phase2 = PHASE_NONE;
					break;
				}
			}
//			prog_cnt = 0;
//		} else {
//			prog_cnt++;
//		}

		// バッファの末尾
		if (end_data != true && s_data->IsTail(32)) {
			break_data = true;
		}
		if (break_data == true) {
			break;
		}

		switch (phase2) {
			case PHASE2_ENCODE_TO_CARRIER:
				// 搬送波データに変換
				carrier_parser.EncodeToCarrier(s_data, c_data);
				phase2 = start_phase;
				if (c_data->IsFull(7)) {
					phase2 = PHASE2_ENCODE_TO_WAVE;
				}
				if (s_data->IsTail()) {
					phase2 = PHASE2_ENCODE_TO_WAVE;
					last = true;
				}
				break;
			case PHASE2_GET_L3C_SAMPLE:
				carrier_parser.GetL3CSample(c_data);
				phase2 = PHASE2_ENCODE_TO_WAVE;
				if (c_data->IsLastData()) {
					last = true;
				}
				break;
			case PHASE2_ENCODE_TO_WAVE:
				if (outfile.GetType() == FILETYPE_L3C) {
					carrier_parser.WriteL3CData(outfile, c_data);
					c_data->Clear();
				}
				if (outfile.GetType() < FILETYPE_L3C) {
					encode_phase1(c_data, last);
				}
				if (last) {
					phase2 = PHASE_NONE;
					break;
				}
				phase2 = start_phase;
				break;
			default:
				// ??
				phase2 = PHASE_NONE;
				break;
		}
	}
	if (end_data != true && break_data == true) {
		// 残りをバッファの最初にコピー→次のターンで解析をするため
		s_data->Shift();
	}
	return phase2;

}

/// @brief L3C -> WAV 搬送波からwaveサンプルデータに変換
///
/// @param[in] c_data 搬送波データ
/// @param[in] end_data 最終データか？
/// @return フェーズ番号
int ParseWav::encode_phase1(CarrierData *c_data, bool end_data)
{
	int pos = 0;
	int step = 0;
	bool last = false;
	bool break_data = false;
	uint8_t wav_data[20];

//	int prog_cnt = 0;
	while(phase1 > PHASE_NONE) {
//		if (prog_cnt < 256) {
//			prog_cnt++;
//		} else {
			if (tmp_param.GetViewProgBox() && needSetProgress()) {
				if (setProgress(infile.SamplePos(), infile.SampleNum())) {
					phase1 = PHASE_NONE;
					break;
				}
			}
//			prog_cnt = 0;
//		}

		// バッファの末尾
		if (end_data != true && c_data->IsTail(32)) {
			break_data = true;
		}
		if (break_data == true) {
			break;
		}

		step = wave_parser.EncodeToWave(c_data, wav_data, 20);
		pos += step;

		if (c_data->IsTail()) {
			last = true;
		}

		if (outfile.GetType() == FILETYPE_WAV) {
			conv.OutConvSampleData(wav_data, 48000, 1, step, outwav.GetSampleRate(), outwav.GetBlockSize(), outfile);
			pos = 0;
		}

		if (last) {
			phase1 = PHASE_NONE;
			break;
		}
	}

	if (end_data != true && break_data == true) {
		// 残りをバッファの最初にコピー→次のターンで解析をするため
		c_data->Shift();
	}
	return phase1;

}

/// @brief レポートログ出力
///
///
///
///
void ParseWav::reporting()
{
//	size_t len;
//	int spd = param.GetFskSpeed();

	buff = _T("----- Result Report -----");
	write_log(buff, 1);

	// input
	buff = _T(" [ input ]");
	write_log(buff, 1);
	buff = _T(" ") + infile.GetName();
	write_log(buff, 1);
	if (infile.GetType() == FILETYPE_WAV) {
		buff.Printf(_T(" %dHz %dbit %dch"),inwav.GetSampleRate(), inwav.GetSampleBits(), inwav.GetChannels());
		write_log(buff, 1);
	}

	write_log(_T(""), 1);

	// output
	if (outfile.GetType() != FILETYPE_NO_FILE) {
		buff = _T(" [ output ]");
		write_log(buff, 1);
		buff = _T(" ") + outfile.GetName();
		write_log(buff, 1);

		write_log(_T(""), 1);
	}
	// phase1 report
	if (infile.GetType() == FILETYPE_WAV && outfile.GetType() >= FILETYPE_WAV) {
		wave_parser.DecordingReport(buff, logbuf);
	}

	// phase2 report
	if (infile.GetType() <= FILETYPE_L3C && outfile.GetType() >= FILETYPE_WAV) {
		carrier_parser.DecordingReport(carrier_data, buff, logbuf);
	}

	// phase3 report
	if (infile.GetType() <= FILETYPE_T9X && outfile.GetType() >= FILETYPE_WAV) {
		serial_parser.DecordingReport(serial_data, buff, logbuf);
	}

	// phase4 report
	if (infile.GetType() <= FILETYPE_L3 && outfile.GetType() >= FILETYPE_WAV) {
		binary_parser.DecordingReport(binary_data, buff, logbuf);
	}

	// phase3 report
	if (infile.GetType() >= FILETYPE_L3 && outfile.GetType() <= FILETYPE_T9X) {
		serial_parser.EncordingReport(serial_data, buff, logbuf);
	}

	// phase2 report
	if (infile.GetType() >= FILETYPE_L3B && outfile.GetType() <= FILETYPE_L3C) {
		carrier_parser.EncordingReport(carrier_data, buff, logbuf);
	}

	// phase1 report
	if (infile.GetType() >= FILETYPE_L3C && outfile.GetType() <= FILETYPE_WAV) {
		wave_parser.EncordingReport(outwav, buff, logbuf);
	}
}


/// @brief 波形解析レポートログ出力
///
///
///
///
void ParseWav::reporting_analyze()
{
	buff = _T("----- Result Report -----");
	write_log(buff, 1);

	// input
	buff = _T(" [ input ]");
	write_log(buff, 1);
	buff = _T(" ") + infile.GetName();
	write_log(buff, 1);
	if (infile.GetType() == FILETYPE_WAV) {
		buff.Printf(_T(" %dHz %dbit %dch"), inwav.GetSampleRate(), inwav.GetSampleBits(), inwav.GetChannels());
		write_log(buff, 1);
	}

	for(int spd=0; spd<2; spd++) {
		write_log(_T(""), 1);
		buff = _T("  Wave Type: ");
		buff += spd ? _T("Double Speed FSK") : _T("Standard FSK");
		write_log(buff, 1);
		for(int cor=0; cor<2; cor++) {
			switch(cor) {
				case 0:
					buff = _T("    Cos Wave:");
					break;
				case 1:
					buff = _T("    Sin Wave:");
					break;
			}
			write_log(buff, 1);
			buff.Printf(_T("      Long(0)  : %8d Cent:%6.1fHz Avg:%6.1fHz")
				, st_chkwav[spd].sample_num[cor][0]
				, 1000000.0 / wave_parser.GetLamda().us[spd]
				, 1000000.0 / st_chkwav[spd].us0avg[cor]
			);
			write_log(buff, 1);
			buff.Printf(_T("      Short(1) : %8d Cent:%6.1fHz Avg:%6.1fHz")
				, st_chkwav[spd].sample_num[cor][1]
				, 1000000.0 / wave_parser.GetLamda().us[spd+1]
				, 1000000.0 / st_chkwav[spd].us1avg[cor]
			);
			write_log(buff, 1);
			buff.Printf(_T("      Middle   : %8d"),st_chkwav[spd].sample_num[cor][2]);
			write_log(buff, 1);
			buff.Printf(_T("      Too Long : %8d"),st_chkwav[spd].sample_num[cor][3]);
			write_log(buff, 1);
			buff.Printf(_T("      Too Short: %8d"),st_chkwav[spd].sample_num[cor][4]);
			write_log(buff, 1);
			buff.Printf(_T("      %dbaud: %4d  %dbaud: %4d  %dbaud: %4d  %dbaud: %4d")
				,(int)c_baud_rate[0] * (spd + 1)
				,st_chkwav[spd].baud_num[cor][0]
				,(int)c_baud_rate[1] * (spd + 1)
				,st_chkwav[spd].baud_num[cor][1]
				,(int)c_baud_rate[2] * (spd + 1)
				,st_chkwav[spd].baud_num[cor][2]
				,(int)c_baud_rate[3] * (spd + 1)
				,st_chkwav[spd].baud_num[cor][3]
			);
			write_log(buff, 1);
			buff.Printf(_T("      Normal : %4d  Reverse : %4d  AmpMax : %4d  AmpMin : %4d  Serial Err:%d")
				,st_chkwav[spd].rev_num[cor][0], st_chkwav[spd].rev_num[cor][1]
				,st_chkwav[spd].amp_max[cor], st_chkwav[spd].amp_min[cor]
				,st_chkwav[spd].ser_err[cor]
			);
			write_log(buff, 1);
		}
	}
	write_log(_T(""), 1);

	// report
	if (infile.GetType() <= FILETYPE_WAV && outfile.GetType() >= FILETYPE_WAV) {
		int fsk_spd = param.GetFskSpeed();

		buff = _T(" [ result ]");
		write_log(buff, 1);

		buff = _T("  Wave Type: ");
		buff += fsk_spd ? _T("Double Speed FSK") : _T("Standard FSK");
		write_log(buff, 1);

		buff.Printf(_T("  Wave Reverse: %s"),(param.GetReverseWave() ? _T("on") : _T("off")));
		write_log(buff, 1);
		buff.Printf(_T("  Correct Type: %s"),(param.GetCorrectType() == 2 ? _T("sin wave") : _T("cos wave")));
		write_log(buff, 1);
//		buff.Printf(_T("  Avg: %4dHz  %4dHz")
//			,param.freq1200,param.freq2400);
//		write_log(buff, 1);

		buff.Printf(_T("  %4d Baud"),(int)c_baud_rate[param.GetBaud()] * (fsk_spd + 1));
		write_log(buff, 1);

		write_log(_T(""), 1);
	}
}

/// @brief ログファイルに出力
///
/// @param[in] buff メッセージ
/// @param[in] crlf 改行する行数
///
void ParseWav::write_log(const wxString &buff, int crlf)
{
	if (buff.Length() > 0) {
		gLogFile.Fputs(buff);
		if (logbuf) *logbuf += buff;
	}
	for (int i=0; i<crlf; i++) {
		gLogFile.Fputts(_T("\n"));
#ifdef _WIN32
		if (logbuf) *logbuf += _T("\r\n");
#else
		if (logbuf) *logbuf += _T("\n");
#endif
	}
}

/// @brief ファイル名取得
///
/// @param[out] filename ファイル名
///
///
void ParseWav::GetFileNameBase(wxString &filename)
{
	UTILS::prefix_name(infile.GetName(), filename);
}

/// @brief 拡張子のチェック
///
/// @param[in] filename ファイル名
/// @param[in] ext 拡張子名
/// @return true 拡張子が一致 / false 一致しない
bool ParseWav::check_extension(const wxString &filename, const wxString &ext)
{
	size_t nam_len = filename.Length();
	size_t ext_len = ext.Length();

	if(nam_len >= ext_len) {
		if (filename.Right(ext_len).IsSameAs(ext, false) == true) {
			return true;
		}
	}
	return false;
}

/// @brief ログバッファへのポインタを設定
///
/// @param[in,out] buf バッファポインタ
///
///
void ParseWav::SetLogBufferPtr(wxString *buf)
{
	logbuf = buf;
}

/// @brief 実ファイルのファイルフォーマット種類を得る
///
/// @return 種類 0 1 2
///
///
int  ParseWav::GetRfDataFormat()
{
	return rftypeparam.GetRfDataFormat();
}

/// @brief 実ファイルのファイルフォーマット種類ダイアログを表示
///
/// @return true OK / false キャンセル
///
///
bool ParseWav::ShowRfTypeBox()
{
	// ファイルの種類を選択
	RfTypeBox rftypebox(parent_window, rftypeparam);
	int rc = rftypebox.showRftypeBox(false);
	if (rc != 1) {
		// cancel button
		return false;
	}

	if (infile.IsOpened()) {
		PwErrType rt = get_first_rf_data(infile);
		if (rt == pwError) {
			return false;
		}
	}
	return true;
}

/// @brief マシン語情報ダイアログを表示
///
/// @param[in] hide_no_header_info ヘッダなし情報を表示しないか
/// @return true OK / false キャンセル
///
bool ParseWav::ShowMAddressBox(bool hide_no_header_info) {
	int rc;
	int addr;

	MAddressBox maddressbox(parent_window, maddressparam);
	rc = maddressbox.showMAddressBox(hide_no_header_info);
	if (rc == 1) {
//		maddress_t val = maddressbox->get();
		rf_header[0] = 0;
		addr = (int)maddressparam.GetStartAddr();
		rf_header[3] = addr / 256;
		rf_header[4] = addr % 256;
		addr = (int)maddressparam.GetDataSize();
		rf_header[1] = addr / 256;
		rf_header[2] = addr % 256;

		rf_footer[0] = 0xff;
		rf_footer[1] = 0;
		rf_footer[2] = 0;
		addr = (int)maddressparam.GetExecAddr();
		rf_footer[3] = addr / 256;
		rf_footer[4] = addr % 256;

		include_header = maddressparam.IncludeHeader();

		return true;
	}
	return false;
}

/// @brief 音データからバイナリデータに変換
///
/// @return pwOK 正常
///
///
PwErrType ParseWav::DecodeData()
{
	PwErrType rc = pwOK;

	process_mode = PROCESS_DECODING;

	wave_data->Init();
	wave_correct_data->Init();
	carrier_data->Init();
	serial_data->Init();
	serial_new_data->Init();
	binary_data->Init();

	tmp_param.SetHalfWave(param.GetHalfWave());
	tmp_param.SetAutoBaud(param.GetAutoBaud());
	tmp_param.SetCorrectType(param.GetCorrectType());
	tmp_param.SetReverseWave(param.GetReverseWave());
	tmp_param.SetViewProgBox(true);

	wave_parser.InitForDecode(process_mode, inwav, tmp_param, mile_stone);
	carrier_parser.InitForDecode(process_mode, tmp_param, mile_stone);
	serial_parser.InitForDecode(process_mode, tmp_param, mile_stone);
	binary_parser.InitForDecode(process_mode, tmp_param, mile_stone);

	mile_stone.Clear(DATA_ARRAY_SIZE / 2);

	wave_parser.ClearResult();
	carrier_parser.ClearResult();
	serial_parser.ClearResult();
	binary_parser.ClearResult();

	wave_data->SetRate(inwav.GetSampleRate());
	wave_correct_data->SetRate(inwav.GetSampleRate());
	carrier_data->SetRate(carrier_parser.GetSampleRate());

//	memset(&inwav, 0, sizeof(inwav));

	// 入力ファイル先頭にセット
	infile.First();

	int fsk_spd = param.GetFskSpeed();

	// 入力ファイルのシーク
	if ((rc = SeekFileFormat(infile)) != pwOK) {
		goto FIN;
	}

	// ファイルのヘッダを出力
	if (outfile.GetType() >= infile.GetType()) {
		InitFileHeader(outfile);
	}

	progress_div = infile.SampleNum();

	if (tmp_param.GetViewProgBox()) {
		initProgress(outfile.GetType() == FILETYPE_NO_FILE ? 1 : 0, 0, 100);
	}

	switch(infile.GetType()) {
	case FILETYPE_WAV:
		// wav
		phase1 = PHASE1_GET_WAV_SAMPLE;
		phase2 = PHASE2_DECODE_TO_SERIAL;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		decode_phase1(fsk_spd, wave_data, wave_correct_data, carrier_data, serial_data, serial_new_data, binary_data, PHASE1_GET_WAV_SAMPLE);
		break;
	case FILETYPE_L3C:
		// l3c
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_DECODE_TO_SERIAL;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		decode_phase1(fsk_spd, wave_data, wave_correct_data, carrier_data, serial_data, serial_new_data, binary_data, PHASE1_GET_L3C_SAMPLE);
		break;
	case FILETYPE_L3B:
		// l3b
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_GET_L3B_SAMPLE;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		carrier_data->LastData(true);
		decode_phase2(fsk_spd, carrier_data, serial_data, serial_new_data, binary_data, PHASE2_GET_L3B_SAMPLE);
		break;
	case FILETYPE_T9X:
		// t9x
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_GET_T9X_SAMPLE;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		carrier_data->LastData(true);
		decode_phase2(fsk_spd, carrier_data, serial_data, serial_new_data, binary_data, PHASE2_GET_T9X_SAMPLE);
		break;
	case FILETYPE_L3:
		// l3 -> real data
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_GET_L3B_SAMPLE;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_GET_L3_SAMPLE;
		phase4 = PHASE4_FIND_HEADER;
		serial_data->LastData(true);
		decode_phase3(serial_data, binary_data, PHASE3_GET_L3_SAMPLE);
		break;
	case FILETYPE_REAL:
		// TODO: real data -> plain data
		decode_plain_data();
	default:
		// unknown
		break;
	}

	// ファイルのヘッダを更新
	if (outfile.GetType() >= infile.GetType()) {
		SetFileHeader(outfile);
	}

FIN:
	process_mode = PROCESS_IDLE;

	return rc;
}

/// @brief 音データからバイナリデータに変換(波形画面表示用)
///
/// @param [in]     dir    0:最初から 1:続き -1:戻す
/// @param [in]     spos   サンプリング位置
/// @param [in,out] a_data 入力データ
/// @return pwOK 正常
PwErrType ParseWav::ViewData(int dir, double spos, CSampleArray *a_data)
{
	PwErrType rc = pwOK;

	process_mode = PROCESS_VIEWING;

	outfile.SetType(FILETYPE_NO_FILE);

//	tmp_param.SetViewProgBox(false);
	tmp_param.SetDebugMode(0);

	int fsk_spd = param.GetFskSpeed();

	if (dir < 0) {
		// 戻る場合
		mile_stone.UnshiftBySPos((int)spos);
		if (mile_stone.GetCurrentSPos() <= 0) {
			dir = 0;
		}
	}
	viewing_dir = dir;

	if (dir == 0) {
		// 最初からデータを読み込む

		wave_data->Init();
		wave_data->Repeat(128, wave_parser.GetLamda().samples[0], -1);
		wave_correct_data->Init();
		carrier_data->Init();
		serial_data->Init();
		serial_new_data->Init();
		binary_data->Init();

		tmp_param.SetHalfWave(param.GetHalfWave());
		tmp_param.SetAutoBaud(param.GetAutoBaud());
		tmp_param.SetCorrectType(param.GetCorrectType());
		tmp_param.SetReverseWave(param.GetReverseWave());

		wave_parser.InitForDecode(process_mode, inwav, tmp_param, mile_stone);
		carrier_parser.InitForDecode(process_mode, tmp_param, mile_stone);
		serial_parser.InitForDecode(process_mode, tmp_param, mile_stone);
		binary_parser.InitForDecode(process_mode, tmp_param, mile_stone);

		mile_stone.Clear(DATA_ARRAY_SIZE / 2);

		wave_parser.ClearResult();
		carrier_parser.ClearResult();
		serial_parser.ClearResult();
		binary_parser.ClearResult();

		wave_data->SetRate(inwav.GetSampleRate());
		wave_correct_data->SetRate(inwav.GetSampleRate());
		carrier_data->SetRate(carrier_parser.GetSampleRate());

		// 入力ファイル先頭にセット
		infile.First();

		// 入力ファイルのシーク
		if ((rc = SeekFileFormat(infile)) != pwOK) {
			goto FIN;
		}

		// ファイルのヘッダを出力
		if (outfile.GetType() >= infile.GetType()) {
			InitFileHeader(outfile);
		}
	} else if (dir > 0) {
		// 続きを読み込む

		// 全データをシフト
		int w_r_pos = 0;
		int sft_pos = 0;

		w_r_pos = a_data->GetReadPos();
		sft_pos = a_data->FindSPos(0, mile_stone.GetCurrentSPos());

		if (w_r_pos >= sft_pos) {
			CSampleData d = a_data->At(sft_pos);
			if (infile.GetType() == FILETYPE_WAV) {

				wave_data->Shift(sft_pos);
				if (param.GetCorrectType() > 0) {
					wave_correct_data->Shift(sft_pos);
				}

				sft_pos = carrier_data->FindSPos(0, d.SPos());
			}
			if (infile.GetType() <= FILETYPE_L3C) {
				if (sft_pos >= 0) {
					carrier_data->Shift(sft_pos);
				}
				sft_pos = serial_data->FindSPos(0, d.SPos());
			}
			if (infile.GetType() <= FILETYPE_T9X) {
				if (sft_pos >= 0) {
					serial_data->Shift(sft_pos);
				}
				sft_pos = serial_new_data->FindSPos(0, d.SPos());
				if (sft_pos >= 0) {
					serial_new_data->Shift(sft_pos);
				}
				sft_pos = binary_data->FindSPos(0, d.SPos());
			}
			if (sft_pos >= 0) {
				binary_data->Shift(sft_pos);
			}
		}
	} else {
		// 戻して読み込む

		MileStone unsft_spos = mile_stone.GetCurrent();
//		int unsft_pos = 0;
		switch(infile.GetType()) {
		case FILETYPE_WAV:
			wave_parser.SkipWaveSample(unsft_spos.SPos() - (int)wave_parser.GetLamda().samples[1] - 2 - a_data->GetWrite(-1).SPos());
			break;
		case FILETYPE_L3C:
			carrier_parser.SkipL3CSample(unsft_spos.SPos() - a_data->GetWrite(-1).SPos());
			break;
		case FILETYPE_L3B:
			serial_parser.SkipL3BSample(unsft_spos.SPos() - a_data->GetWrite(-1).SPos());
			break;
		case FILETYPE_T9X:
			serial_parser.SkipT9XSample(unsft_spos.SPos() - a_data->GetWrite(-1).SPos());
			break;
		default:
			// unknown
			break;
		}
//		unsft_pos = a_data->GetWrite(-1).SPos() - unsft_spos.SPos();

		if (infile.GetType() == FILETYPE_WAV) {
			wave_data->Revert();
			wave_data->LastData(false);
			if (param.GetCorrectType() > 0) {
				wave_correct_data->Revert();
				wave_correct_data->LastData(false);
			}
			wave_parser.SetPrevCross(unsft_spos.SPos());
		}
		if (infile.GetType() <= FILETYPE_L3C) {
			carrier_data->Revert();
			carrier_data->LastData(false);
			carrier_parser.SetPhase(unsft_spos.CPhase());
			carrier_parser.SetFrip(unsft_spos.CFrip());
		}
		if (infile.GetType() <= FILETYPE_T9X) {
			serial_data->Revert();
			serial_data->LastData(false);
			serial_new_data->Revert();
			serial_new_data->LastData(false);
			serial_parser.SetStartDataSPos(unsft_spos.SPos());
			serial_parser.SetDataPos(unsft_spos.SDataPos());
			serial_parser.SetPhase3Baud(unsft_spos.Baud() >= 0 ? unsft_spos.Baud() : 0);
		}
		if (infile.GetType() <= FILETYPE_L3) {
			binary_data->Revert();
			binary_data->LastData(false);
		}
	}

//	if (tmp_param.GetViewProgBox()) {
//		initProgress(outfile.GetType() == FILETYPE_NO_FILE ? 1 : 0, 0, 100);
//	}

	switch(infile.GetType()) {
	case FILETYPE_WAV:
		// wav
		phase1 = PHASE1_GET_WAV_SAMPLE;
		phase2 = PHASE2_DECODE_TO_SERIAL;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		decode_phase1(fsk_spd, wave_data, wave_correct_data, carrier_data, serial_data, serial_new_data, binary_data, PHASE1_GET_WAV_SAMPLE);
		break;
	case FILETYPE_L3C:
		// l3c
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_DECODE_TO_SERIAL;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		decode_phase1(fsk_spd, wave_data, wave_correct_data, carrier_data, serial_data, serial_new_data, binary_data, PHASE1_GET_L3C_SAMPLE);
		break;
	case FILETYPE_L3B:
		// l3b
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_GET_L3B_SAMPLE;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		carrier_data->LastData(true);
		decode_phase2(fsk_spd, carrier_data, serial_data, serial_new_data, binary_data, PHASE2_GET_L3B_SAMPLE);
		break;
	case FILETYPE_T9X:
		// t9x
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_GET_T9X_SAMPLE;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_DECODE_TO_BINARY;
		phase4 = PHASE4_FIND_HEADER;
		carrier_data->LastData(true);
		decode_phase2(fsk_spd, carrier_data, serial_data, serial_new_data, binary_data, PHASE2_GET_T9X_SAMPLE);
		break;
	case FILETYPE_L3:
		// l3
		phase1 = PHASE1_GET_L3C_SAMPLE;
		phase2 = PHASE2_GET_L3B_SAMPLE;
		phase2n = PHASE2N_CONVERT_BAUD_RATE;
		phase3 = PHASE3_GET_L3_SAMPLE;
		phase4 = PHASE4_FIND_HEADER;
		serial_data->LastData(true);
		decode_phase3(serial_data, binary_data, PHASE3_GET_L3_SAMPLE);
		break;
	default:
		// unknown
		break;
	}

FIN:
	process_mode = PROCESS_IDLE;

	return rc;
}

/// @brief バイナリデータから音データに変換
///
/// @return pwOK:OK pwCancel:キャンセルボタンを押した
///
///
PwErrType ParseWav::EncodeData()
{
	PwErrType rc = pwOK;

	process_mode = PROCESS_ENCODING;

	wave_data->Init();
	carrier_data->Init();
	serial_data->Init();
	serial_new_data->Init();
	binary_data->Init();

	wave_parser.InitForEncode(process_mode, inwav, tmp_param, mile_stone);
	carrier_parser.InitForEncode(process_mode, tmp_param, mile_stone);
	serial_parser.InitForEncode(process_mode, tmp_param, mile_stone);
	binary_parser.InitForEncode(process_mode, tmp_param, mile_stone);

	mile_stone.Clear(DATA_ARRAY_SIZE / 2);

	wave_parser.ClearResult();
	carrier_parser.ClearResult();
	serial_parser.ClearResult();
	binary_parser.ClearResult();

	wave_data->SetRate(inwav.GetSampleRate());
	wave_correct_data->SetRate(inwav.GetSampleRate());
	carrier_data->SetRate(carrier_parser.GetSampleRate());

	// 実ファイルの情報をセット
	if (infile.GetType() == FILETYPE_REAL || infile.GetType() == FILETYPE_PLAIN) {
		set_rf_info();
	}

	infile.First();

	// 入力ファイルの位置をシーク
	if ((rc = SeekFileFormat(infile)) != pwOK) {
		goto FIN;
	}

	// ファイルのヘッダを出力
	InitFileHeader(outfile);

	if (tmp_param.GetViewProgBox()) {
		initProgress(outfile.GetType() == FILETYPE_NO_FILE ? 1 : 0, 0, 100);
	}

	switch(infile.GetType()) {
	case FILETYPE_L3C:
		// l3c
		phase4 = PHASE4_PUT_HEADER;
		phase3 = PHASE3_GET_L3B_SAMPLE;
		phase2n = PHASE_NONE;
		phase2 = PHASE2_GET_L3C_SAMPLE;
		phase1 = PHASE1_GET_L3C_SAMPLE;
		encode_phase2(serial_data, carrier_data, PHASE2_GET_L3C_SAMPLE, true);
		break;
	case FILETYPE_T9X:
		// t9x
		phase4 = PHASE4_PUT_HEADER;
		phase3 = PHASE3_GET_T9X_SAMPLE;
		phase2n = PHASE_NONE;
		phase2 = PHASE2_ENCODE_TO_CARRIER;
		phase1 = PHASE1_GET_L3C_SAMPLE;
		encode_phase3(binary_data, serial_data, carrier_data, PHASE3_GET_T9X_SAMPLE);
		break;
	case FILETYPE_L3B:
		// l3b
		phase4 = PHASE4_PUT_HEADER;
		phase3 = PHASE3_GET_L3B_SAMPLE;
		phase2n = PHASE_NONE;
		phase2 = PHASE2_ENCODE_TO_CARRIER;
		phase1 = PHASE1_GET_L3C_SAMPLE;
		encode_phase3(binary_data, serial_data, carrier_data, PHASE3_GET_L3B_SAMPLE);
		break;
	case FILETYPE_L3:
		// l3 -> l3b
		phase4 = PHASE4_PUT_HEADER;
		phase3 = PHASE3_GET_L3_SAMPLE;
		phase2n = PHASE_NONE;
		phase2 = PHASE2_ENCODE_TO_CARRIER;
		phase1 = PHASE1_GET_L3C_SAMPLE;
		encode_phase3(binary_data, serial_data, carrier_data, PHASE3_GET_L3_SAMPLE);
		break;
	case FILETYPE_REAL:
		// real -> L3
		phase4 = PHASE4_PUT_HEADER;
		phase3 = PHASE3_GET_BIN_SAMPLE;
		phase2n = PHASE_NONE;
		phase2 = PHASE2_ENCODE_TO_CARRIER;
		phase1 = PHASE1_GET_L3C_SAMPLE;
		encode_phase4(binary_data, serial_data, carrier_data);
		break;
	case FILETYPE_PLAIN:
		if (outfile.GetType() == FILETYPE_REAL) {
			// plain -> real
			encode_plain_data(binary_data);
		} else {
			// plain -> L3
			phase4 = PHASE4_PUT_HEADER;
			phase3 = PHASE3_GET_BIN_SAMPLE;
			phase2n = PHASE_NONE;
			phase2 = PHASE2_ENCODE_TO_CARRIER;
			phase1 = PHASE1_GET_L3C_SAMPLE;
			encode_phase4(binary_data, serial_data, carrier_data);
		}
		break;
	default:
		// unknown
		break;
	}

	// 必要ならファイルのヘッダを更新
	SetFileHeader(outfile);

FIN:
	process_mode = PROCESS_IDLE;

	return pwOK;
}

/// @brief 実ファイルの情報を設定する
void ParseWav::set_rf_info()
{
	binary_parser.SetSaveDataInfo(
		rftypeparam.GetRfDataName(),
		rftypeparam.GetRfDataNameLen(),
		rftypeparam.GetRfDataFormat(),
		rftypeparam.GetRfDataType()
	);

	// マシン語の場合
	if (binary_parser.IsMachineData() && !maddressparam.Valid()) {
		// ヘッダがない場合、情報入力を促す
		if (ShowMAddressBox(false) == false) {
			include_header = true;
//			return pwCancel;
		}
	}

	if (include_header) {
		infile.RestoreSampleNum();
	} else {
		infile.RestoreSampleNum(10);
	}
}

/// @brief 波形を解析
///
/// @return 0
///
///
int ParseWav::AnalyzeWave()
{
	int rc = 0;

	process_mode = PROCESS_ANALYZING;

	outfile.SetType(FILETYPE_NO_FILE);	// 出力ファイルなし

	tmp_param.SetHalfWave(true);	// 常に半波で解析
	tmp_param.SetAutoBaud(true);	// 常に自動
	tmp_param.SetCorrectType(param.GetCorrectType());
	tmp_param.SetReverseWave(false);
	tmp_param.SetViewProgBox(true);
	tmp_param.SetDebugMode(0);

	if (logbuf) {
		*logbuf = _T("");
	}

	for(int spd=0; spd < 2; spd++) {
		st_chkwav[spd].Clear();
	}
	st_chkwav_analyzed_num = 0;

	progress_div = inwav.GetSampleRate() * ANALYZE_SEC * 4;

	if (tmp_param.GetViewProgBox()) {
		initProgress(1, 0, 100);
	}

	for(int spd=0; spd < 2; spd++) {
		param.SetFskSpeed(spd);
		for(int cor=0; cor < 2; cor++) {
			st_chkwav[spd].num = cor;
			// fseekする
			infile.First();

			wave_parser.CheckWaveFormat(infile, inwav.GetHead(), inwav.GetFmtChank(), inwav.GetDataChank(), conv, err_num, *errinfo);

			wave_data->Init();
			wave_correct_data->Init();
			carrier_data->Init();
			serial_data->Init();
			serial_new_data->Init();
			binary_data->Init();

			st_chkwav[spd].analyze_num = inwav.GetSampleRate() * ANALYZE_SEC;	// 30秒解析

			wave_parser.InitForDecode(process_mode, inwav, tmp_param, mile_stone);
			carrier_parser.InitForDecode(process_mode, tmp_param, mile_stone);
			serial_parser.InitForDecode(process_mode, tmp_param, mile_stone);
			binary_parser.InitForDecode(process_mode, tmp_param, mile_stone);

			mile_stone.Clear(DATA_ARRAY_SIZE / 2);

			wave_parser.ClearResult();
			carrier_parser.ClearResult();
			serial_parser.ClearResult();
			binary_parser.ClearResult();

			tmp_param.SetCorrectType(cor + 1);

			phase1 = PHASE1_GET_WAV_SAMPLE;
			phase2 = PHASE2_PARSE_CARRIER;
			phase2n = PHASE2N_CONVERT_BAUD_RATE;
			phase3 = PHASE3_DECODE_TO_BINARY;
			phase4 = PHASE4_FIND_HEADER;

			decode_phase1(spd, wave_data, wave_correct_data, carrier_data, serial_data, serial_new_data, binary_data, PHASE1_GET_WAV_SAMPLE);
			for(int i=0; i<5; i++) {
				st_chkwav[spd].sample_num[cor][i]=wave_parser.GetReport().GetSampleNum(i);
//				st_chkwav[spd].sample_odd[cor][i]=wave_parser.GetReport().GetSampleOdd(i);
			}

			st_chkwav[spd].us0avg[cor] = wave_parser.GetLamda().us_avg[spd];
			st_chkwav[spd].us1avg[cor] = wave_parser.GetLamda().us_avg[spd+1];
			st_chkwav[spd].amp_max[cor] = dft.GetAmpMax();
			st_chkwav[spd].amp_min[cor] = dft.GetAmpMin();

			st_chkwav[spd].ser_err[cor] = carrier_parser.GetReport().GetErrorNum();

			st_chkwav_analyzed_num += st_chkwav[spd].analyze_num;
		}
	}

	if (tmp_param.GetViewProgBox()) {
		endProgress();
	}

	// 尤もらしいものを判定

	int best_fsk_spd = -1;
	int best_baud = -1;
	int best_correct = 0;

	// 倍速かどうか判定

	if ((st_chkwav[1].sample_num[0][0] > (st_chkwav[1].sample_num[0][1] * 10))
	 || (st_chkwav[1].sample_num[1][0] > (st_chkwav[1].sample_num[1][1] * 10))) {
		// Double FSKで、Long と Short Wave の比率が10:1未満
		best_fsk_spd = 0;
	} else if (((st_chkwav[0].sample_num[0][0] * 10) < st_chkwav[0].sample_num[0][1])
	 || ((st_chkwav[0].sample_num[1][0] * 10) < st_chkwav[1].sample_num[1][1])) {
		// Single FSKで、Long と Short Wave の比率が1:10未満
		best_fsk_spd = 1;
	} else {
		best_fsk_spd = 0;
	}
	param.SetFskSpeed(best_fsk_spd);

	// ボーレートから補正波形を判定
	int best_correct_per_baud[4] = {-1, -1, -1, -1};
	int best_baud_cnt[4] = {-1, -1, -1, -1};
	for(int i=0; i<4; i++) {
		int num0 = st_chkwav[best_fsk_spd].baud_num[0][i];	// cos
		int num1 = st_chkwav[best_fsk_spd].baud_num[1][i];	// sin
		if (num0 > 10 || num1 > 10) {
			if (num0 >= num1) {
				best_correct_per_baud[i] = 0;
				best_baud_cnt[i] = num0;
			} else {
				best_correct_per_baud[i] = 1;
				best_baud_cnt[i] = num1;
			}
		}
	}

	// ボーレート判定
	int baud_cnt = -1;
	for(int i=0; i<4; i++) {
		if (baud_cnt < best_baud_cnt[i]) {
			best_baud = i;
			baud_cnt = best_baud_cnt[i];
		}
	}
	if (best_baud >= 0) {
		param.SetBaud(best_baud);
	}

	// 補正波形を判定
	if (best_baud >= 0) {
		best_correct = best_correct_per_baud[best_baud];
	}
	if (best_correct >= 0) {
		param.SetCorrectType((best_correct % 2) + 1);
	}

	// 波正逆判定
	if (st_chkwav[best_fsk_spd].rev_num[best_correct][0] < st_chkwav[best_fsk_spd].rev_num[best_correct][1]) {
		// 逆
		param.SetReverseWave(true);
	} else {
		// 正
		param.SetReverseWave(false);
	}

	// パラメータをセット

	// 波長の平均値を設定
//	param.freq1200 = (int)(1000000.0 / st_chkwav.us1200avg[best_num] /  (1 << half_wave));
//	param.freq2400 = (int)(1000000.0 / st_chkwav.us2400avg[best_num] /  (1 << half_wave));

	// レポート
	reporting_analyze();

	process_mode = PROCESS_IDLE;

	return rc;
}

/// @brief 変換したデータをファイルに出力
///
/// @return true 正常 / false エラーあり
///
///
bool ParseWav::ExportData(enum_file_type file_type)
{
	PwErrType rc = pwOK;
	if (file_type != FILETYPE_UNKNOWN) {
		outfile.SetType(file_type);
	}

	gLogFile.SetLogBuf(logbuf);
	if (logbuf) {
		*logbuf = _T("");
	}

	tmp_param.SetDebugMode(param.GetDebugMode());

	// デバッグログオープン
	if (tmp_param.GetDebugMode() > 0) {
		if (!gLogFile.Open(logfilename)) {
			err_num = pwErrCannotWriteDebugLog;
			errinfo->SetInfo(__LINE__, pwWarning, err_num);
			errinfo->ShowMsgBox();
		}
	}

	if (infile.GetType() <= outfile.GetType()) {
		// decode
		rc = DecodeData();
	} else if (infile.GetType() > outfile.GetType()) {
		// encode
		rc = EncodeData();
		// decode チェック
		if (rc == pwOK) rc = DecodeData();
	}

	reporting();

	gLogFile.Close();

	if (tmp_param.GetViewProgBox()) {
		endProgress();
	}

	if (rc == pwCancel || rc == pwError) {
		return false;
	}

	return true;
}

/// @brief 出力用ファイルを開く
///
/// @param[in] out_file     ファイルパス名
/// @param[in] outfile_type ファイル種類
/// @return true:正常 false:エラーあり
///
bool ParseWav::OpenOutFile(const wxString &out_file, enum_file_type outfile_type)
{
	// 同じファイルはダメ
	wxFileName infile_name = wxFileName::FileName(infile.GetName());
	wxFileName outfile_name = wxFileName::FileName(out_file);
	if (outfile_name.SameAs(infile_name)) {
		err_num = pwErrSameFile;
		errinfo->SetInfo(__LINE__, pwError, err_num);
		errinfo->ShowMsgBox();
		return false;
	}

	logfilename = out_file + _T(".log");

	if (outfile_type == FILETYPE_UNKNOWN) {
		if (check_extension(out_file, _T(".WAV"))) {
			outfile_type = FILETYPE_WAV;
		} else if (check_extension(out_file, _T(".L3C"))) {
			outfile_type = FILETYPE_L3C;
		} else if (check_extension(out_file, _T(".L3B"))) {
			outfile_type = FILETYPE_L3B;
		} else if (check_extension(out_file, _T(".T9X"))) {
			outfile_type = FILETYPE_T9X;
		} else if (check_extension(out_file, _T(".L3"))) {
			outfile_type = FILETYPE_L3;
		} else {
			outfile_type = FILETYPE_REAL;
		}
	}

	if (param.GetFileSplit() && outfile_type == FILETYPE_REAL) {
		// 分割する場合
#ifdef _WIN32
		int seppos = out_file.Find(wxChar('\\'), true);
#else
		int seppos = out_file.Find(wxChar('/'), true);
#endif
		int pos = out_file.Find(wxChar('.'), true);
		if (pos != wxNOT_FOUND && seppos < pos) {
			outsext = out_file.Mid(pos);
			outsfileb = out_file.Left(pos);
		} else {
			outsext = _T("");
			outsfileb = out_file;
		}

		wxString outsfilen = outsfileb;
		outsfilen += wxString::Format(_T("_%03d"), 1);
		outsfilen += outsext;
		if (!outsfile.Fopen(outsfilen, File::WRITE_BINARY)) {
			err_num = pwErrCannotWrite;
			errinfo->SetInfo(__LINE__, pwError, err_num);
			errinfo->ShowMsgBox();
			return false;
		}
		outsfile.Fclose();

	} else {
		// 分割しない
		if (!outfile.Fopen(out_file, File::WRITE_BINARY)) {
			err_num = pwErrCannotWrite;
			errinfo->SetInfo(__LINE__, pwError, err_num);
			errinfo->ShowMsgBox();
			return false;
		}
	}

	outfile.SetType(outfile_type);

//	outfile = out_file;

	return true;
}

/// @brief 出力用ファイルを閉じる
///
///
///
///
void ParseWav::CloseOutFile()
{
	outsfile.Fclose();
	outfile.Fclose();

	logfilename = _T("wavtool.log");
}

/// @brief 入力ファイルを開く
///
/// @param[in] in_file ファイルパス名
/// @return true:正常 false:エラー
///
bool ParseWav::OpenDataFile(const wxString &in_file)
{
	PwErrType rc;


	enum_file_type infile_type = FILETYPE_UNKNOWN;
	if (check_extension(in_file, _T(".WAV"))) {
		infile_type = FILETYPE_WAV;
	} else if (check_extension(in_file, _T(".L3C"))) {
		infile_type = FILETYPE_L3C;
	} else if (check_extension(in_file, _T(".L3B"))) {
		infile_type = FILETYPE_L3B;
	} else if (check_extension(in_file, _T(".T9X"))) {
		infile_type = FILETYPE_T9X;
	} else if (check_extension(in_file, _T(".L3"))) {
		infile_type = FILETYPE_L3;
	} else {
		infile_type = FILETYPE_PLAIN;
	}

	CloseDataFile();

	if (!infile.Fopen(in_file, File::READ_BINARY)) {
		err_num = pwErrFileNotFound;
		errinfo->SetInfo(__LINE__, pwError, err_num);
		errinfo->ShowMsgBox();
		return false;
	}

	// ファイルのチェック
	infile.SetType(infile_type);
	rc = CheckFileFormat(infile);
	if (rc == pwError || rc == pwCancel) {
		CloseDataFile();
		return false;
	}

//	infile = in_file;

//	param.freq1200 = 1200;
//	param.freq2400 = 2400;

	return true;

}

/// @brief 入力ファイルを閉じる
///
///
///
///
void ParseWav::CloseDataFile()
{
	infile.Fclose();
}

/// @brief 入力ファイルを開いた数を返す
///
///
///
///
int ParseWav::OpenedDataFileCount(int *sample_num)
{
	if (sample_num) *sample_num = infile.SampleNum();
	return infile.OpenedFileCount();
}

//
//
// wrapper
//
//

void ParseWav::initProgress(int type, int min_val, int max_val)
{
#ifdef USE_PROGRESSBOX
	progbox->initProgress(type, min_val, max_val);
#endif
}
bool ParseWav::needSetProgress() const
{
#ifdef USE_PROGRESSBOX
	return progbox->needSetProgress();
#else
	return false;
#endif
}
bool ParseWav::setProgress(int val)
{
#ifdef USE_PROGRESSBOX
	return progbox->setProgress(val);
#else
	return false;
#endif
}
bool ParseWav::setProgress(int num, int div)
{
#ifdef USE_PROGRESSBOX
	return progbox->setProgress(num, div);
#else
	return false;
#endif
}
bool ParseWav::incProgress()
{
#ifdef USE_PROGRESSBOX
	return progbox->incProgress();
#else
	return false;
#endif
}
bool ParseWav::viewProgress()
{
#ifdef USE_PROGRESSBOX
	return progbox->viewProgress();
#else
	return false;
#endif
}
void ParseWav::endProgress()
{
#ifdef USE_PROGRESSBOX
	progbox->endProgress();
#endif
}

}; /* namespace PARSEWAV */
