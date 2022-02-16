#include "app.h"

#include "frames/chat.h"

bool App::OnInit() {
    auto frame = new ChatFrame;
    frame->Show(true);

    return true;
}
