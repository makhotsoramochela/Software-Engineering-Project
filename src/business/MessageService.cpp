#include "MessageService.h"

#include "Validation.h"

namespace lms {

MessageService::MessageService(IMessageRepository& messages, Clock& clock) : messages_(messages), clock_(clock) {}

void MessageService::notify(int userId, const std::string& body) {
    messages_.add(Message(0, userId, validate::text(body, "A message", 1000), clock_.now()));
}

std::vector<Message> MessageService::takeWaiting(const User& actor) {
    std::vector<Message> waiting = messages_.listUnseen(actor.getId());
    for (const Message& m : waiting) messages_.markSeen(m.getId());
    return waiting;
}

}  // namespace lms
