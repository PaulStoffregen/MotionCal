#include "gui.h"
#include "imuread.h"






wxBEGIN_EVENT_TABLE(MyCanvas, wxGLCanvas)
    EVT_SIZE(MyCanvas::OnSize)
    EVT_PAINT(MyCanvas::OnPaint)
    //EVT_CHAR(MyCanvas::OnChar)
    //EVT_MOUSE_EVENTS(MyCanvas::OnMouseEvent)
wxEND_EVENT_TABLE()

MyCanvas::MyCanvas(wxWindow *parent, wxWindowID id, int* gl_attrib)
	: wxGLCanvas(parent, id, gl_attrib)
{
	//m_xrot = 0;
	//m_yrot = 0;
	//m_numverts = 0;
	// Explicitly create a new rendering context instance for this canvas.
	m_glRC = new wxGLContext(this);
}

MyCanvas::~MyCanvas()
{
	delete m_glRC;
}

void MyCanvas::OnSize(wxSizeEvent& event)
{
	//printf("OnSize\n");
	if (!IsShownOnScreen()) return;
	SetCurrent(*m_glRC);
	resize_callback(event.GetSize().x, event.GetSize().y);
}

void MyCanvas::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	//printf("OnPaint\n");
	wxPaintDC dc(this);
	SetCurrent(*m_glRC);
	display_callback();
	SwapBuffers();
}

void MyCanvas::InitGL()
{
	//printf("Init\n");
	SetCurrent(*m_glRC);
	visualize_init();
}





BEGIN_EVENT_TABLE(MyFrame,wxFrame)
	EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
	EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
	EVT_TIMER(ID_TIMER, MyFrame::OnTimer)
END_EVENT_TABLE()


MyFrame::MyFrame(wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style) :
    wxFrame( parent, id, title, position, size, style )
{
	wxMenuBar *menuBar;
	wxMenu *menu;
	//wxMenuItem *item;

	menuBar = new wxMenuBar;
	menu = new wxMenu;
	menu->Append(wxID_EXIT, wxT("Quit"));
	menuBar->Append(menu, wxT("&File"));

	menu = new wxMenu;
	//item = new wxMenuItem(menu, ID_ABOUT, "About");
	menu->Append(wxID_ABOUT, wxT("About"));
	menuBar->Append(menu, wxT("&Help"));

	SetMenuBar(menuBar);

	int gl_attrib[20] =
        { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1,
        WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1,
        WX_GL_DOUBLEBUFFER,
	0};


	m_canvas = new MyCanvas(this, wxID_ANY, gl_attrib);
	Show(true);
	Raise();
	m_canvas->InitGL();

	open_port(PORT);
	m_timer = new wxTimer(this, ID_TIMER);
	m_timer->Start(40, wxTIMER_CONTINUOUS);
}

void MyFrame::OnTimer(wxTimerEvent &event)
{
	//printf("OnTimer\n");
	read_serial_data();
	m_canvas->Refresh();
}

void MyFrame::OnAbout(wxCommandEvent &event)
{
        wxMessageDialog dialog(this,
                "IMU Read\n\n\nCopyright 2016, PJRC.COM, LLC.",
                "About IMUREAD", wxOK|wxICON_INFORMATION);
        dialog.ShowModal();
}

void MyFrame::OnQuit( wxCommandEvent &event )
{
        Close(true);
}

MyFrame::~MyFrame(void)
{
	close_port();
}




IMPLEMENT_APP(MyApp)

MyApp::MyApp()
{
}

bool MyApp::OnInit()
{
	// make sure we exit properly on macosx
	SetExitOnFrameDelete(true);

	wxPoint pos(100, 100);

	MyFrame *frame = new MyFrame(NULL, -1, wxT("Teensy"), pos, wxDefaultSize,
		wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER));
	frame->Show( true );
	return true;
}

int MyApp::OnExit()
{
	return 0;
}
