#include "Exceptions.h"
#include "SqliteRepositories.h"

namespace lms {

void SqliteMessageRepository::add(const Message& m) {
    Statement st = db_.prepare("INSERT INTO message (user_id, body, created_at, seen) VALUES (?, ?, ?, 0)");
    st.bindInt(1, m.getUserId());
    st.bindText(2, m.getBody());
    st.bindText(3, m.getCreatedAt().toString());
    st.step();
}

std::vector<Message> SqliteMessageRepository::listUnseen(int userId) {
    Statement st = db_.prepare(
        "SELECT message_id, user_id, body, created_at, seen FROM message WHERE user_id = ? AND seen = 0 "
        "ORDER BY message_id");
    st.bindInt(1, userId);
    std::vector<Message> rows;
    while (st.step()) {
        rows.emplace_back(static_cast<int>(st.columnInt(0)), static_cast<int>(st.columnInt(1)), st.columnText(2),
                          DateTime::parse(st.columnText(3)), st.columnInt(4) != 0);
    }
    return rows;
}

void SqliteMessageRepository::markSeen(int messageId) {
    Statement st = db_.prepare("UPDATE message SET seen = 1 WHERE message_id = ?");
    st.bindInt(1, messageId);
    st.step();
}

}  // namespace lms
