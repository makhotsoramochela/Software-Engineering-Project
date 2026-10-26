// Layer 2 (business logic): messages shown at the next log in (FR-23).
#pragma once

#include <string>
#include <vector>

#include "Ports.h"

namespace lms {

class MessageService {
public:
    MessageService(IMessageRepository& messages, Clock& clock);

    // Leaves a message for a user. It uses the transaction of the caller if there is one.
    void notify(int userId, const std::string& body);

    // Returns the messages that the user has not seen and marks them as seen.
    std::vector<Message> takeWaiting(const User& actor);

private:
    IMessageRepository& messages_;
    Clock& clock_;
};

}  // namespace lms
