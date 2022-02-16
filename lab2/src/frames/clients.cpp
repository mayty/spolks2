#include "clients.h"

#include <arpa/inet.h>

ClientsFrame::ClientsFrame(Controller& controller) :
    wxFrame(NULL, wxID_ANY, "Clients"),
    timer(this),
    controller(controller)
{
    list = new wxListCtrl(this);
    list->SetSingleStyle(wxLC_REPORT);
    list->SetSingleStyle(wxLC_SINGLE_SEL);
    list->AppendColumn("IP address");
    list->AppendColumn("Muted");

    auto mute = new wxButton(this, wxID_ANY, "Mute/Unmute");
    mute->Bind(wxEVT_BUTTON, &ClientsFrame::onMute, this);

    auto root = new wxBoxSizer(wxVERTICAL);
    root->Add(list, 1, wxEXPAND);
    root->Add(mute, 0, wxEXPAND);

    Bind(wxEVT_TIMER, &ClientsFrame::onTimer, this);

    SetSizerAndFit(root);
    SetMinSize(wxSize(200, 300));

    updateClients();

    timer.Start(1000);
}

void ClientsFrame::onTimer(wxTimerEvent& event) {
    updateClients();
}

void ClientsFrame::onMute(wxCommandEvent& event) {
    if (controller.getCurrentRoom() == BROADCAST_ROOM) {
        wxMessageDialog error(this, "Can't mute source address during being in broadcast room!");
        error.ShowModal();
        return;
    }

    in_addr_t addr = getSelectedItemData();
    if (!addr) {
        return;
    }

    auto mutedClients = controller.getMuted();

    auto it = std::find(mutedClients.begin(), mutedClients.end(), addr);
    if (it != mutedClients.end()) {
        controller.unmute(addr);
    } else {
        controller.mute(addr);
    }
}

wxUIntPtr ClientsFrame::getSelectedItemData() {
    int count = list->GetSelectedItemCount();
    if (count != 1) {
        return 0;
    }

    auto sel = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    auto data = list->GetItemData(sel);

    return data;
}

void ClientsFrame::updateClients() {
    auto prev_data = getSelectedItemData();

    list->DeleteAllItems();

    auto clients = controller.getClients();
    auto mutedClients = controller.getMuted();

    for (auto client : clients) {
        const char* str = inet_ntoa({ client });
        list->InsertItem(0, str);
        list->SetItemData(0, client);

        if (prev_data == client) {
            list->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        }

        auto it = std::find(mutedClients.begin(), mutedClients.end(), client);
        if (it != mutedClients.end()) {
            list->SetItem(0, 1, "+");
        }
    }

    list->SetColumnWidth(0, wxLIST_AUTOSIZE);
}
