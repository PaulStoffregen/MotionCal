#include "gui.h"
#include "imuread.h"


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
	EVT_MENU(ID_SENDCAL_MENU, MyFrame::OnSendCal)
	EVT_BUTTON(ID_CLEAR_BUTTON, MyFrame::OnClear)
	EVT_BUTTON(ID_SENDCAL_BUTTON, MyFrame::OnSendCal)
	EVT_TIMER(ID_TIMER, MyFrame::OnTimer)
	EVT_MENU_RANGE(9000, 9999, MyFrame::OnPortMenu)
	EVT_MENU_OPEN(MyFrame::OnShowMenu)
	EVT_COMBOBOX(ID_PORTLIST, MyFrame::OnPortList)
	EVT_COMBOBOX_DROPDOWN(ID_PORTLIST, MyFrame::OnShowPortList)
END_EVENT_TABLE()



MyFrame::MyFrame(wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style) :
    wxFrame( parent, id, title, position, size, style )
{
	wxMenuBar *menuBar;
	wxMenu *menu;
	wxSizer *hsizer, *vsizer, *calsizer;
	wxStaticText *text;
	int i, j;

	menuBar = new wxMenuBar;
	menu = new wxMenu;
	menu->Append(ID_SENDCAL_MENU, wxT("Send Calibration"));
	m_sendcal_menu = menu;
	m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
	menu->Append(wxID_EXIT, wxT("Quit"));
	menuBar->Append(menu, wxT("&File"));

	menu = new wxMenu;
	menuBar->Append(menu, "Port");
	m_port_menu = menu;

	menu = new wxMenu;
	menu->Append(wxID_ABOUT, wxT("About"));
	menuBar->Append(menu, wxT("&Help"));
	SetMenuBar(menuBar);

	wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *leftsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Communication");
	wxBoxSizer *middlesizer = new wxStaticBoxSizer(wxVERTICAL, this, "Magnetometer");
	wxBoxSizer *rightsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Calibration");

	topsizer->Add(leftsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);
	topsizer->Add(middlesizer, 1, wxALL | wxEXPAND, 5);
	topsizer->Add(rightsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);

	vsizer = new wxBoxSizer(wxVERTICAL);
	leftsizer->Add(vsizer, 0, wxALL, 8);
	text = new wxStaticText(this, wxID_ANY, "Port");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_port_list = new wxComboBox(this, ID_PORTLIST, "",
		wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	m_port_list->Append("(none)");
	m_port_list->Append(SAMPLE_PORT_NAME); // never seen, only for initial size
	m_port_list->SetSelection(0);
	vsizer->Add(m_port_list, 1, wxEXPAND, 0);

	vsizer->AddSpacer(8);
	text = new wxStaticText(this, wxID_ANY, "Actions");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_button_clear = new wxButton(this, ID_CLEAR_BUTTON, "Clear");
	m_button_clear->Enable(false);
	vsizer->Add(m_button_clear, 1, wxEXPAND, 0);
	m_button_sendcal = new wxButton(this, ID_SENDCAL_BUTTON, "Send Cal");
	vsizer->Add(m_button_sendcal, 1, wxEXPAND, 0);
	m_button_sendcal->Enable(false);

	vsizer = new wxBoxSizer(wxVERTICAL);
	middlesizer->Add(vsizer, 1, wxEXPAND | wxALL, 8);

	text = new wxStaticText(this, wxID_ANY, "");
	text->SetLabelMarkup("<small><i>Ideal calibration is a perfectly centered sphere</i></small>");
	vsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	int gl_attrib[20] = { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1,
		WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1, WX_GL_DOUBLEBUFFER, 0};
	m_canvas = new MyCanvas(this, wxID_ANY, gl_attrib);
	m_canvas->SetMinSize(wxSize(400,400));
	vsizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);


	hsizer = new wxGridSizer(4, 0, 15);
	middlesizer->Add(hsizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(this, wxID_ANY, "Gaps");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_coverage = new wxStaticText(this, wxID_ANY, "100.0%");
	vsizer->Add(m_err_coverage, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(this, wxID_ANY, "Variance");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_variance = new wxStaticText(this, wxID_ANY, "100.0%");
	vsizer->Add(m_err_variance, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(this, wxID_ANY, "Wobble");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_wobble = new wxStaticText(this, wxID_ANY, "100.0%");
	vsizer->Add(m_err_wobble, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(this, wxID_ANY, "Fit Error");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_fit = new wxStaticText(this, wxID_ANY, "100.0%");
	vsizer->Add(m_err_fit, 1, wxALIGN_CENTER_HORIZONTAL);

	calsizer = new wxBoxSizer(wxVERTICAL);
	rightsizer->Add(calsizer, 0, wxALL, 8);
	text = new wxStaticText(this, wxID_ANY, "Magnetic Offset");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_mag_offset[i] = new wxStaticText(this, wxID_ANY, "0.00");
		vsizer->Add(m_mag_offset[i], 1);
	}
	text = new wxStaticText(this, wxID_ANY, "Magnetic Mapping");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(3, 0, 12);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		for (j=0; j < 3; j++) {
			m_mag_mapping[i][j] = new wxStaticText(this, wxID_ANY,
				((i == j) ? "+1.000" : "+0.000"));
			vsizer->Add(m_mag_mapping[i][j], 1);
		}
	}
	text = new wxStaticText(this, wxID_ANY, "Magnetic Field");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_mag_field = new wxStaticText(this, wxID_ANY, "0.00");
	calsizer->Add(m_mag_field, 0, wxLEFT, 20);
	text = new wxStaticText(this, wxID_ANY, "Accelerometer");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_accel[i] = new wxStaticText(this, wxID_ANY, "0.000");
		vsizer->Add(m_accel[i], 1);
	}
	text = new wxStaticText(this, wxID_ANY, "Gyroscope");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_gyro[i] = new wxStaticText(this, wxID_ANY, "0.000");
		vsizer->Add(m_gyro[i], 1);
	}

	calsizer->AddSpacer(8);
	text = new wxStaticText(this, wxID_ANY, "");
	text->SetLabelMarkup("<small>Calibration should be performed\n<b>after</b> final installation.  Presence\nof magnets and ferrous metals\ncan alter magnetic calibration.\nMechanical stress during\nassembly can alter accelerometer\nand gyroscope calibration.</small>");
	//text->Wrap(200);
	//calsizer->Add(text, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 0);
	calsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	topsizer->SetSizeHints(this);
	SetSizerAndFit(topsizer);
	Show(true);
	Raise();

	m_canvas->InitGL();
	raw_data_reset();
	//open_port(PORT);
	m_timer = new wxTimer(this, ID_TIMER);
	m_timer->Start(14, wxTIMER_CONTINUOUS);
}

void MyFrame::OnTimer(wxTimerEvent &event)
{
	float gaps, variance, wobble, fiterror;
	char buf[32];
	int i, j;

	//printf("OnTimer\n");
	if (port_is_open()) {
		read_serial_data();
		m_canvas->Refresh();
		gaps = quality_surface_gap_error();
		variance = quality_magnitude_variance_error();
		wobble = quality_wobble_error();
		fiterror = quality_spherical_fit_error();
		if (gaps < 15.0f && variance < 4.5f && wobble < 4.0f && fiterror < 5.0f) {
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, true);
			m_button_sendcal->Enable(true);
		} else if (gaps > 20.0f && variance > 5.0f && wobble > 5.0f && fiterror > 6.0f) {
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
			m_button_sendcal->Enable(false);
		}
		snprintf(buf, sizeof(buf), "%.1f%%", quality_surface_gap_error());
		m_err_coverage->SetLabelText(buf);
		snprintf(buf, sizeof(buf), "%.1f%%", quality_magnitude_variance_error());
		m_err_variance->SetLabelText(buf);
		snprintf(buf, sizeof(buf), "%.1f%%", quality_wobble_error());
		m_err_wobble->SetLabelText(buf);
		snprintf(buf, sizeof(buf), "%.1f%%", quality_spherical_fit_error());
		m_err_fit->SetLabelText(buf);
		for (i=0; i < 3; i++) {
			snprintf(buf, sizeof(buf), "%.2f", magcal.V[i]);
			m_mag_offset[i]->SetLabelText(buf);
		}
		for (i=0; i < 3; i++) {
			for (j=0; j < 3; j++) {
				snprintf(buf, sizeof(buf), "%+.3f", magcal.invW[i][j]);
				m_mag_mapping[i][j]->SetLabelText(buf);
			}
		}
		snprintf(buf, sizeof(buf), "%.2f", magcal.B);
		m_mag_field->SetLabelText(buf);
		for (i=0; i < 3; i++) {
			snprintf(buf, sizeof(buf), "%.3f", 0.0f); // TODO...
			m_accel[i]->SetLabelText(buf);
		}
		for (i=0; i < 3; i++) {
			snprintf(buf, sizeof(buf), "%.3f", 0.0f); // TODO...
			m_gyro[i]->SetLabelText(buf);
		}
	} else {
		if (!port_name.IsEmpty()) {
			//printf("port has closed, updating stuff\n");
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
			m_button_clear->Enable(false);
			m_button_sendcal->Enable(false);
			m_port_list->Clear();
			m_port_list->Append("(none)");
			m_port_list->SetSelection(0);
			port_name = "";
		}
	}
}

void MyFrame::OnClear(wxCommandEvent &event)
{
	//printf("OnClear\n");
	raw_data_reset();
}

void MyFrame::OnSendCal(wxCommandEvent &event)
{
	/*printf("OnSendCal\n");
	printf("Magnetic Calibration:   (%.1f%% fit error)\n", magcal.FitError);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2]);
	*/
	send_calibration();
}



void MyFrame::OnShowMenu(wxMenuEvent &event)
{
        wxMenu *menu = event.GetMenu();
        if (menu != m_port_menu) return;
        //printf("OnShow Port Menu, %s\n", (const char *)menu->GetTitle());
	while (menu->GetMenuItemCount() > 0) {
		menu->Delete(menu->GetMenuItems()[0]);
	}
        menu->AppendRadioItem(9000, " (none)");
	bool isopen = port_is_open();
	if (!isopen) menu->Check(9000, true);
        wxArrayString list = serial_port_list();
        int num = list.GetCount();
        for (int i=0; i < num; i++) {
                menu->AppendRadioItem(9001 + i, list[i]);
                if (isopen && port_name.IsSameAs(list[i])) {
                        menu->Check(9001 + i, true);
                }
        }
	menu->UpdateUI();
}

void MyFrame::OnShowPortList(wxCommandEvent& event)
{
	//printf("OnShowPortList\n");
	m_port_list->Clear();
	m_port_list->Append("(none)");
	wxArrayString list = serial_port_list();
	int num = list.GetCount();
	for (int i=0; i < num; i++) {
		m_port_list->Append(list[i]);
	}
}


void MyFrame::OnPortMenu(wxCommandEvent &event)
{
        int id = event.GetId();
        wxString name = m_port_menu->FindItem(id)->GetItemLabelText();

	close_port();
        //printf("OnPortMenu, id = %d, name = %s\n", id, (const char *)name);
	port_name = name;
	m_port_list->Clear();
	m_port_list->Append(port_name);
	m_port_list->SetSelection(0);
        if (id == 9000) return;
	raw_data_reset();
	open_port((const char *)name);
	m_button_clear->Enable(true);
}

void MyFrame::OnPortList(wxCommandEvent& event)
{
	int selected = m_port_list->GetSelection();
	if (selected == wxNOT_FOUND) return;
	wxString name = m_port_list->GetString(selected);
	//printf("OnPortList, %s\n", (const char *)name);
	close_port();
	port_name = name;
	if (name == "(none)") return;
	raw_data_reset();
	open_port((const char *)name);
	m_button_clear->Enable(true);
}




void MyFrame::OnAbout(wxCommandEvent &event)
{
        wxMessageDialog dialog(this,
                "MotionCal - Motion Sensor Calibration Tool\n\n"
		"Paul Stoffregen <paul@pjrc.com>\n"
		"http://www.pjrc.com/store/prop_shield.html\n"
		"https://github.com/PaulStoffregen/MotionCal\n\n"
		"Copyright 2016, PJRC.COM, LLC.",
                "About MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
        dialog.ShowModal();
}

void MyFrame::OnQuit( wxCommandEvent &event )
{
        Close(true);
}

MyFrame::~MyFrame(void)
{
	m_timer->Stop();
	close_port();
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

	MyFrame *frame = new MyFrame(NULL, -1, "Motion Sensor Calibration Tool",
		pos, wxSize(1120,760), wxDEFAULT_FRAME_STYLE);
#ifdef WINDOWS
	frame->SetIcon(wxIcon("MotionCal"));
#endif
	frame->Show( true );
	return true;
}

int MyApp::OnExit()
{
	return 0;
}




