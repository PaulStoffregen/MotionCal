#include "gui.h"
#include "imuread.h"



wxMenu *port_menu;
wxMenu *sendcal_menu;
wxString port_name;


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



/*****************************************************************************/

BEGIN_EVENT_TABLE(MyFrame,wxFrame)
	EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
	EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
	EVT_MENU(ID_SENDCAL, MyFrame::OnSendCal)
	EVT_TIMER(ID_TIMER, MyFrame::OnTimer)
	EVT_MENU_RANGE(9000, 9999, MyFrame::OnPort)
	EVT_MENU_OPEN(MyMenu::OnShowPortList)
	EVT_MENU_HIGHLIGHT(-1, MyMenu::OnHighlight)
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
	menu->Append(ID_SENDCAL, wxT("Send Calibration"));
	sendcal_menu = menu;
	sendcal_menu->Enable(ID_SENDCAL, false);
	menu->Append(wxID_EXIT, wxT("Quit"));
	menuBar->Append(menu, wxT("&File"));

	menu = new wxMenu;
	menuBar->Append(menu, "Port");
	port_menu = menu;

	menu = new wxMenu;
	//item = new wxMenuItem(menu, ID_ABOUT, "About");
	menu->Append(wxID_ABOUT, wxT("About"));
	menuBar->Append(menu, wxT("&Help"));
	SetMenuBar(menuBar);

	wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *leftsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Communication");
	wxBoxSizer *middlesizer = new wxStaticBoxSizer(wxVERTICAL, this, "Magnetometer");
	wxBoxSizer *rightsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Gyroscope");

	topsizer->Add(leftsizer, 0, wxALL, 5);
	topsizer->Add(middlesizer, 1, wxALL | wxEXPAND, 5);
	topsizer->Add(rightsizer, 0, wxALL, 5);

	int gl_attrib[20] =
        { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1,
        WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1,
        WX_GL_DOUBLEBUFFER,
	0};
	m_canvas = new MyCanvas(this, wxID_ANY, gl_attrib);
	m_canvas->SetMinSize(wxSize(340,340));

	middlesizer->Add(m_canvas, 1, wxEXPAND | wxALL, 10);
	topsizer->SetSizeHints(this);
	SetSizerAndFit(topsizer);
	Show(true);
	Raise();

	m_canvas->InitGL();
	raw_data_reset();
	//open_port(PORT);
	m_timer = new wxTimer(this, ID_TIMER);
	m_timer->Start(40, wxTIMER_CONTINUOUS);
}

void MyFrame::OnTimer(wxTimerEvent &event)
{
	//printf("OnTimer\n");
	if (port_is_open()) {
		read_serial_data();
		m_canvas->Refresh();
		if (magcal.FitError < 6.0f) {
			sendcal_menu->Enable(ID_SENDCAL, true);
		} else if (magcal.FitError > 7.0f) {
			sendcal_menu->Enable(ID_SENDCAL, false);
		}
	} else {
		sendcal_menu->Enable(ID_SENDCAL, false);
	}
}

void MyFrame::OnSendCal(wxCommandEvent &event)
{
	printf("OnSendCal\n");
	printf("Magnetic Calibration:   (%.1f%% fit error)\n", magcal.FitError);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2]);
	send_calibration();
}

void MyFrame::OnPort(wxCommandEvent &event)
{
        int id = event.GetId();
        wxString name = port_menu->FindItem(id)->GetItemLabelText();

	close_port();
        //printf("OnPort, id = %d, name = %s\n", id, (const char *)name);
	sendcal_menu->Enable(ID_SENDCAL, false);
	port_name = name;
        if (id == 9000) return;
	raw_data_reset();
	open_port((const char *)name);
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


/*****************************************************************************/
// Port Menu

MyMenu::MyMenu(const wxString& title, long style) : wxMenu(title, style)
{
}

void MyMenu::OnShowPortList(wxMenuEvent &event)
{
        wxMenu *menu;
	int any=0;
        int num;

        menu = event.GetMenu();
        //printf("OnShowPortList, %s\n", (const char *)menu->GetTitle());
        if (menu != port_menu) return;

        wxMenuItemList old_items = menu->GetMenuItems();
        num = old_items.GetCount();
        for (int i = old_items.GetCount() - 1; i >= 0; i--) {
                menu->Delete(old_items[i]);
        }
        menu->AppendRadioItem(9000, " (none)");
        wxArrayString list = serial_port_list();
        num = list.GetCount();
	bool isopen = port_is_open();
        for (int i=0; i < num; i++) {
                //printf("%d: port %s\n", i, (const char *)list[i]);
                menu->AppendRadioItem(9001 + i, list[i]);
                if (isopen && port_name.IsSameAs(list[i])) {
                        menu->Check(9001 + i, true);
                        any = 1;
                }
        }
        if (!any) menu->Check(9000, true);
}

void MyMenu::OnHighlight(wxMenuEvent &event)
{
	//printf("OnHighlight\n");
}



/*****************************************************************************/

IMPLEMENT_APP(MyApp)

MyApp::MyApp()
{
}

bool MyApp::OnInit()
{
	// make sure we exit properly on macosx
	SetExitOnFrameDelete(true);

	wxPoint pos(100, 100);

	MyFrame *frame = new MyFrame(NULL, -1, wxT("IMU Read"), pos, wxSize(1120,760),
		wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER));
	frame->Show( true );
	return true;
}

int MyApp::OnExit()
{
	return 0;
}




