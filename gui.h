#ifndef gui__h_
#define gui__h_

#include <wx/wx.h>
#include "wx/timer.h"
#include "wx/glcanvas.h"
#include "wx/math.h"
#include "wx/log.h"
#include "wx/wfstream.h"
#include "wx/zstream.h"
#include "wx/txtstrm.h"
#if defined(__WXMAC__) || defined(__WXCOCOA__)
#ifdef __DARWIN__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <gl.h>
#include <glu.h>
#endif
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


#define ID_TIMER		10000
#define ID_SENDCAL_MENU		10001
#define ID_CLEAR_BUTTON		10002
#define ID_SENDCAL_BUTTON	10003
#define ID_PORTLIST		10004

class MyCanvas : public wxGLCanvas
{
public:
	MyCanvas(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		int *gl_attrib = NULL);

	virtual ~MyCanvas();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMouseEvent(wxMouseEvent& event);

	void LoadSurface(const wxString& filename);
	void InitMaterials();
	void InitGL();

private:
	wxGLContext *m_glRC;
	wxDECLARE_NO_COPY_CLASS(MyCanvas);
	wxDECLARE_EVENT_TABLE();
};


class MyFrame: public wxFrame
{
public:
	MyFrame(wxWindow *parent, wxWindowID id,
		const wxString &title,
		const wxPoint &pos = wxDefaultPosition,
		const wxSize &size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE);
	~MyFrame(void);
private:
	wxStaticText *m_err_coverage;
	wxStaticText *m_err_variance;
	wxStaticText *m_err_wobble;
	wxStaticText *m_err_fit;

	wxStaticText *m_mag_offset[3];
	wxStaticText *m_mag_mapping[3][3];
	wxStaticText *m_mag_field;
	wxStaticText *m_accel[3];
	wxStaticText *m_gyro[3];

	MyCanvas *m_canvas;
	wxTimer *m_timer;
	wxButton *m_button_clear;
	wxButton *m_button_sendcal;
	wxStaticBitmap *m_confirm_icon;
	wxMenu *m_port_menu;
	wxComboBox *m_port_list;
	wxMenu *m_sendcal_menu;
	void OnSendCal(wxCommandEvent &event);
	void OnClear(wxCommandEvent &event);
	void OnShowMenu(wxMenuEvent &event);
	void OnShowPortList(wxCommandEvent &event);
	void OnPortList(wxCommandEvent& event);
	void OnPortMenu(wxCommandEvent &event);
	void OnTimer(wxTimerEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnQuit(wxCommandEvent &event);
	DECLARE_EVENT_TABLE()
};


class MyApp: public wxApp
{
public:
	MyApp();
	virtual bool OnInit();
	virtual int OnExit();
private:
        //wxSingleInstanceChecker *m_instance;
};

// portlist.cpp
wxArrayString serial_port_list();

// images.cpp
wxBitmap MyBitmap(const char *name);

// sample port name, for initial sizing of left panel
#if defined(LINUX)
#define SAMPLE_PORT_NAME "/dev/ttyACM5."
#elif defined(WINDOWS)
#define SAMPLE_PORT_NAME "COM22:."
#elif defined(MACOSX)
#define SAMPLE_PORT_NAME "/dev/cu.usbmodem2457891..."
#endif


#endif
