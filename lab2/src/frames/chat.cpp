#include "chat.h"

#include <functional>
#include <arpa/inet.h>

#include <wx/listctrl.h>

#include <chat/network.h>
#include "clients.h"

class RoomData : public wxClientData {
public:
    RoomData(Room room): room(room) {}
    const Room& getRoom() { return room; }

private:
    Room room;
};

ChatFrame::ChatFrame() : wxFrame(NULL, wxID_ANY, "Chat"), timer(this) {
    // create widgets
    auto root = new wxBoxSizer(wxHORIZONTAL);
    auto tools = new wxBoxSizer(wxVERTICAL);
    auto control = new wxBoxSizer(wxVERTICAL);
    auto edit = new wxBoxSizer(wxHORIZONTAL);

    rooms_list = new wxListBox(this, wxID_ANY);
    history = new wxRichTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxRE_READONLY);
    message = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    auto address = new wxStaticText(this, wxID_ANY, "Can't retrieve interface info", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    auto send = new wxButton(this, wxID_ANY, "Send");
    auto clients = new wxButton(this, wxID_ANY, "Clients");
    auto add_room = new wxButton(this, wxID_ANY, "New room");

    // add widgets to sizers
    root->Add(tools, 4, wxEXPAND);
    root->Add(control, 10, wxEXPAND);
    tools->Add(add_room, 0, wxEXPAND);
    tools->Add(rooms_list, 1, wxEXPAND);
    tools->Add(clients, 0, wxEXPAND);
    control->Add(address, 0, wxEXPAND);
    control->Add(history, 1, wxEXPAND);
    control->Add(edit, 0, wxEXPAND);
    edit->Add(message, 1, wxEXPAND);
    edit->Add(send);

    // initialize widgets
    history->BeginItalic();
    history->WriteText("Select rooms to start chatting...\n");
    history->EndItalic();

    {
        auto info_opt = getIfInfo();
        if (info_opt.has_value()) {
            auto info = info_opt.value();

            std::string addr = inet_ntoa({ info.addr }) + std::string("/") + std::to_string(__builtin_popcount(info.mask));

            address->SetLabel(addr);
        }
    }

    // bind widgets events
    rooms_list->Bind(wxEVT_LISTBOX, &ChatFrame::onRoomSelect, this);
    rooms_list->Bind(wxEVT_CHECKLISTBOX, &ChatFrame::onRoomSelect, this);
    message->Bind(wxEVT_TEXT_ENTER, &ChatFrame::onSend, this);
    send->Bind(wxEVT_BUTTON, &ChatFrame::onSend, this);
    clients->Bind(wxEVT_BUTTON, &ChatFrame::onClients, this);
    add_room->Bind(wxEVT_BUTTON, &ChatFrame::onNewRoom, this);
    Bind(wxEVT_TIMER, &ChatFrame::onTimer, this);
    Bind(wxEVT_THREAD, &ChatFrame::onReceiveGUI, this);

    // configure frame
    SetSizerAndFit(root);
    SetMinSize(wxSize(400, 400));
    message->SetFocus();

    // configure controller
    auto bindedCallback = std::bind(
        &ChatFrame::onReceive,
        this,
        std::placeholders::_1,
        std::placeholders::_2
    );
    controller.setOnReceive(bindedCallback);

    updateRooms();

    timer.Start(1000);
}

struct ReceiveData {
    in_addr_t source;
    std::string message;
};

void ChatFrame::onReceive(in_addr_t source, std::string message) {
    ReceiveData data;
    data.source = source;
    data.message = message;

    wxThreadEvent event(wxEVT_THREAD);
    event.SetPayload(data);
    wxQueueEvent(this, event.Clone());
}

void ChatFrame::onReceiveGUI(wxThreadEvent& event) {
    const ReceiveData& data = event.GetPayload<ReceiveData>();

    std::string source_str = inet_ntoa({ data.source });

    history->BeginBold();
    history->WriteText(source_str + ": ");
    history->EndBold();

    history->WriteText(data.message + "\n");
}

void ChatFrame::onSend(wxCommandEvent &event) {
    std::string value = message->GetValue().ToStdString();
    if (value.size() == 0) {
        return;
    }

    message->Clear();

    controller.send(value);
}

void ChatFrame::onClients(wxCommandEvent &event) {
    auto frame = new ClientsFrame(controller);
    frame->Show(true);
}

void ChatFrame::onNewRoom(wxCommandEvent &event) {
    wxTextEntryDialog dialog(this, "Enter new room name:");
    dialog.SetTextValidator(wxFILTER_EMPTY);

    int result = dialog.ShowModal();
    if (result != wxID_OK) {
        return;
    }

    std::string name = dialog.GetValue().ToStdString();

    const Room& room = controller.createRoom(name);
    controller.join(room);
}

void ChatFrame::onRoomSelect(wxCommandEvent &event) {
    int sel = event.GetSelection();
    RoomData* data = (RoomData*) rooms_list->GetClientObject(sel);
    Room room = data->getRoom();

    controller.join(room);
}

// I'm really sorry about it
void ChatFrame::onTimer(wxTimerEvent& event) {
    static unsigned int tick_progress = 0;

    if (tick_progress == 20) {
        tick_progress = 0;

        controller.announceClient();
        controller.announceCurrentRoom();
    }

    tick_progress++;

    updateRooms();
}

void ChatFrame::updateRooms() {
    Room current_room = controller.getCurrentRoom();

    rooms_list->Clear();

    rooms_list->Append("All", new RoomData(BROADCAST_ROOM));
    if (current_room == BROADCAST_ROOM) {
        rooms_list->SetSelection(0);
    }

    auto rooms = controller.getRooms();
    for (auto& room : rooms) {
        rooms_list->Append(room.name, new RoomData(room));

        if (room == current_room) {
            int count = rooms_list->GetCount();
            rooms_list->SetSelection(count-1);
        }
    }
}
