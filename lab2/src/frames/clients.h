#pragma once

#include <wx/listctrl.h>
#include <wx/wx.h>

#include <chat/controller.h>

class ClientsFrame : public wxFrame {
public:
    ClientsFrame(Controller& controller);

private:
    wxListCtrl* list;
    wxTimer timer;

    Controller& controller;

    void onTimer(wxTimerEvent& event);
    void onMute(wxCommandEvent& event);

    wxUIntPtr getSelectedItemData();

    void updateClients();
};
