/// @file mytextctrl.h
///
/// @brief テキストコントロール
///

#ifndef _MYTEXTCTRL_H_
#define _MYTEXTCTRL_H_

#define USE_RICHTEXTCTRL 1
//#define USE_MYTEXTCTRL_THREAD 1

#include "common.h"
#include <wx/string.h>
#ifdef USE_RICHTEXTCTRL
#include <wx/richtext/richtextctrl.h>
#else
#include <wx/textctrl.h>
#endif
#ifdef USE_MYTEXTCTRL_THREAD
#include <wx/thread.h>

class MyTextCtrlThread;
#endif

///
/// @brief カスタム・テキストコントロール
///
/// エスケープシーケンスで色を指定できるようにしている。
///
class MyTextCtrl
#ifdef USE_RICHTEXTCTRL
	: public wxRichTextCtrl
#else
	: public wxTextCtrl
#endif
{
private:
	int nest_nums;
	wxString m_line;
	
#ifdef USE_RICHTEXTCTRL
	wxRichTextAttr default_attr;
#else
	wxTextAttr default_attr;
#endif
#ifdef USE_MYTEXTCTRL_THREAD
	MyTextCtrlThread *thread;
	wxMutex *mutex;
#endif
	int m_subindent;
	void SetMyScrollUnit();

public:
	MyTextCtrl();
	MyTextCtrl(wxWindow *parent, wxWindowID id=-1, const wxString &value=wxEmptyString, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize);
	virtual ~MyTextCtrl();

	virtual void SetupScrollbars(bool atTop = false);
    virtual void SetupScrollbars(bool atTop, bool fromOnPaint);

	void SetMyStyle();
	virtual bool SetFont(const wxFont &font);
	virtual void SetValue(const wxString &value);
	virtual void SetLine(const wxString &value);
	virtual void SetLines(const wxArrayString &values);
	void SetLinesMain(const wxArrayString &values);

#ifdef USE_MYTEXTCTRL_THREAD
	void ReleaseThread();
#endif
};

#ifdef USE_MYTEXTCTRL_THREAD
class MyTextCtrlThread : public wxThread
{
protected:
	MyTextCtrl *mCtrl;
	wxArrayString mValues;

	virtual ExitCode Entry();

public:
	MyTextCtrlThread(MyTextCtrl *ctrl, const wxArrayString &values);
	~MyTextCtrlThread();
};
#endif

#endif /* _MYTEXTCTRL_H_ */
