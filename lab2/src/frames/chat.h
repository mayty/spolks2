#pragma once

#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>

#include <chat/controller.h>

class ChatFrame : public wxFrame {
public:
    ChatFrame();

private:
    wxListBox* rooms_list;
    wxTextCtrl* message;
    wxRichTextCtrl* history;

    wxTimer timer;

    Controller controller;

    void onReceiveGUI(wxThreadEvent& event);
    void onSend(wxCommandEvent& event);
    void onClients(wxCommandEvent& event);
    void onNewRoom(wxCommandEvent& event);
    void onRoomSelect(wxCommandEvent& event);
    void onTimer(wxTimerEvent& event);

    void onReceive(in_addr_t source, std::string message);

    void updateRooms();
};
