/// @file mytextctrl.cpp
///
/// @brief テキストコントロール
///

#include "mytextctrl.h"
#include <wx/regex.h>
#include "colortag.h"

//

MyTextCtrl::MyTextCtrl()
#ifdef USE_RICHTEXTCTRL
	: wxRichTextCtrl()
#else
	: wxTextCtrl()
#endif
{
	default_attr = wxTextAttr(*wxBLACK);
#ifdef USE_RICHTEXTCTRL
	SetDefaultStyle(default_attr);
	SetMyScrollUnit();
#else
	SetDefaultStyle(default_attr);
#endif
#ifdef USE_MYTEXTCTRL_THREAD
	thread = NULL;
	mutex = new wxMutex();
#endif
}

MyTextCtrl::MyTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size)
#ifdef USE_RICHTEXTCTRL
	: wxRichTextCtrl(parent, id, value, pos, size, wxRE_READONLY | wxRE_MULTILINE)
#else
	: wxTextCtrl(parent, id, value, pos, size, wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH)
#endif
{
	default_attr = wxTextAttr(*wxBLACK);
#ifdef USE_RICHTEXTCTRL
	SetDefaultStyle(default_attr);
	SetMyScrollUnit();
#else
	SetDefaultStyle(default_attr);
#endif
#ifdef USE_MYTEXTCTRL_THREAD
	thread = NULL;
	mutex = new wxMutex();
#endif
}

MyTextCtrl::~MyTextCtrl()
{
#ifdef USE_MYTEXTCTRL_THREAD
	if (thread) {
		thread->Delete();
		delete thread;
	}
	delete mutex;
#endif
}

/// override on wxWidgets-3.0.x
void MyTextCtrl::SetupScrollbars(bool atTop)
{
#ifdef USE_RICHTEXTCTRL
	wxRichTextCtrl::SetupScrollbars(atTop);
	SetMyScrollUnit();
#endif
}

/// override on wxWidgets-3.1.x
void MyTextCtrl::SetupScrollbars(bool atTop, bool fromOnPaint)
{
#if wxCHECK_VERSION(3,1,0)
#ifdef USE_RICHTEXTCTRL
	wxRichTextCtrl::SetupScrollbars(atTop, fromOnPaint);
	SetMyScrollUnit();
#endif
#endif
}

/// スクロール単位を設定
void MyTextCtrl::SetMyScrollUnit()
{
#ifdef USE_RICHTEXTCTRL
	int ux, uy;
	int vx, vy;
	GetScrollPixelsPerUnit(&ux, &uy);
	GetVirtualSize(&vx, &vy);

	int nuy = GetFont().GetPixelSize().GetHeight();
	nuy += GetMargins().y;
	if (nuy > uy) {
		uy = nuy;
	}

	m_subindent = uy * 2;

	int nx = 0;
	if (ux > 0) nx = vx / ux;
	int ny = 0;
	if (uy > 0) ny = vy / uy;

	SetScrollbars(ux, uy, nx, ny);
#endif
}

/// デフォルトスタイルを設定
void MyTextCtrl::SetMyStyle()
{
	SetDefaultStyle(default_attr);
}

/// フォントを設定
bool MyTextCtrl::SetFont(const wxFont &font)
{
#ifdef USE_RICHTEXTCTRL
	bool rc =  wxRichTextCtrl::SetFont(font);
	SetMyScrollUnit();
	return rc;
#else
	default_attr.SetFont(font);
	return wxTextCtrl::SetFont(font);
#endif
}

void MyTextCtrl::SetValue(const wxString &value)
{
	SetMyStyle();

#ifdef USE_RICHTEXTCTRL
	wxRichTextCtrl::SetValue(value);
#else
	wxTextCtrl::SetValue(value);
#endif
}

#if 0
void MyTextCtrl::SetLine(const wxString &value)
{
	wxString str = value;

	const color_tag_t *tag = NULL;
	int mpos = wxNOT_FOUND;
	while(str.Len() > 0) {
		mpos = str.Find(wxT("\x1b"));
		if (mpos == wxNOT_FOUND) break;

		if (mpos > 0) {
#ifdef USE_RICHTEXTCTRL
			WriteText(str.Left(mpos));
#else
			AppendText(str.Left(mpos));
#endif
		}
		int idx = (int)str.GetChar(mpos+1) - 0x61;
		if (idx >= 0 && idx < 26) {
			tag = gColorTag.Get(idx);
			if (tag->start <= 1) {
#ifdef USE_RICHTEXTCTRL
				if (tag->start) {
					if (nest_nums > 0) {
						EndTextColour();
						nest_nums--;
					}
					nest_nums++;
					BeginTextColour(wxColour(tag->red, tag->green, tag->blue));
				} else {
					if (nest_nums > 0) {
						EndTextColour();
						nest_nums--;
					}
				}
#else
				if (tag->start) {
					SetDefaultStyle(wxTextAttr(wxColour(tag->red, tag->green, tag->blue)));
				} else {
					SetDefaultStyle(default_attr);
				}
#endif
			}
		}
		str = str.Mid(mpos+2);
	}
	if (str.Len() > 0) {
#ifdef USE_RICHTEXTCTRL
		WriteText(str);
#else
		AppendText(str);
#endif
	}

#ifdef USE_RICHTEXTCTRL
	Newline();
	EndAllStyles();
	DiscardEdits();
//	LineBreak();
#else
	AppendText(wxT("\n"));
#endif
}
#endif

/// 1行分のテキストを設定
void MyTextCtrl::SetLine(const wxString &value)
{
	const color_tag_t *tag = NULL;
	int pos = 0;
	int len = (int)value.Len();
	m_line.Empty();
	for(pos = 0; pos < len; pos++) {
		wxUniChar ch = value.GetChar(pos);
		if (ch != 0x1b) {
			m_line += ch;
			continue;
		}

		// Set color
		pos++;
		int idx = (int)value.GetChar(pos) - 0x61;
		if (idx >= 0 && idx < 26) {
			tag = gColorTag.Get(idx);
			if (tag->start <= 1) {
				if (m_line.Len() > 0) {
#ifdef USE_RICHTEXTCTRL
					WriteText(m_line);
#else
					AppendText(m_line);
#endif
					m_line.Empty();
				}

#ifdef USE_RICHTEXTCTRL
				if (tag->start) {
					if (nest_nums > 0) {
						EndTextColour();
						nest_nums--;
					}
					nest_nums++;
					BeginTextColour(wxColour(tag->red, tag->green, tag->blue));
				} else {
					if (nest_nums > 0) {
						EndTextColour();
						nest_nums--;
					}
				}
#else
				if (tag->start) {
					SetDefaultStyle(wxTextAttr(wxColour(tag->red, tag->green, tag->blue)));
				} else {
					SetDefaultStyle(default_attr);
				}
#endif
			}
		}
	}
	if (m_line.Len() > 0) {
#ifdef USE_RICHTEXTCTRL
		WriteText(m_line);
#else
		AppendText(m_line);
#endif
	}

#ifdef USE_RICHTEXTCTRL
	Newline();
//	EndAllStyles();
	DiscardEdits();
//	LineBreak();
#else
	AppendText(wxT("\n"));
#endif
}

/// 複数行テキストを設定
void MyTextCtrl::SetLines(const wxArrayString &values)
{
#ifdef USE_MYTEXTCTRL_THREAD
	mutex->Lock();
	mutex->Unlock();
	if (thread) {
		thread->Delete();
		while(thread) {
			wxSleep(1);
		}
	}
	if (!thread) {
		thread = new MyTextCtrlThread(this, values);
		thread->Run();
	}
#else
	SetLinesMain(values);
#endif
}

/// 複数行テキストを設定
void MyTextCtrl::SetLinesMain(const wxArrayString &values)
{
	wxStopWatch sw;
	wxCursor cr;

#ifndef USE_MYTEXTCTRL_THREAD
	sw.Start();
#else
	wxMutexGuiEnter();
#endif
	Freeze();
	Clear();

	SetMyStyle();
#ifdef USE_RICHTEXTCTRL
	BeginSuppressUndo();
	BeginLeftIndent(0, m_subindent);
#endif
#ifdef USE_MYTEXTCTRL_THREAD
	wxMutexGuiLeave();
#endif
	nest_nums = 0;
	for(size_t i=0; i<values.GetCount()
#ifdef USE_MYTEXTCTRL_THREAD
		&& !thread->TestDestroy()
#endif
		; i++) {
#ifdef USE_MYTEXTCTRL_THREAD
		wxMutexGuiEnter();
#endif

		SetLine(values[i]);

#ifndef USE_MYTEXTCTRL_THREAD
		if (sw.Time() > 500) {
			cr = wxCursor(wxCURSOR_WAIT);
			wxSetCursor(cr);
			sw.Start();
		}
#else
		wxMutexGuiLeave();
#endif
	}

#ifdef USE_RICHTEXTCTRL
#ifdef USE_MYTEXTCTRL_THREAD
	wxMutexGuiEnter();
#endif

	EndLeftIndent();
	EndSuppressUndo();
	Thaw();
#ifdef USE_MYTEXTCTRL_THREAD
	wxMutexGuiLeave();
#endif
#endif

#ifndef USE_MYTEXTCTRL_THREAD
	cr = wxCursor(wxCURSOR_ARROW);
	wxSetCursor(cr);
#endif
}

#ifdef USE_MYTEXTCTRL_THREAD
void MyTextCtrl::ReleaseThread()
{
	mutex->Lock();
	thread = NULL;
	mutex->Unlock();
}
#endif

#ifdef USE_MYTEXTCTRL_THREAD
MyTextCtrlThread::MyTextCtrlThread(MyTextCtrl *ctrl, const wxArrayString &values)
	: wxThread()
{
	mCtrl = ctrl;
	mValues = wxArrayString(values);
}

MyTextCtrlThread::~MyTextCtrlThread()
{
	mCtrl->ReleaseThread();
}

MyTextCtrlThread::ExitCode MyTextCtrlThread::Entry()
{
	mCtrl->SetLinesMain(mValues);
	return (MyTextCtrlThread::ExitCode)0;
}
#endif
