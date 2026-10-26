// Layer 2 (business logic): the audit log of final marks (FR-14, FR-26, BR-05).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Ports.h"

namespace lms {

class AuditService {
public:
    AuditService(IAuditRepository& audits, Clock& clock);

    // Adds one entry. It is called inside the transaction of the service that changes the mark (AR-5),
    // so a mark can never be saved without its entry. It does not start a transaction itself.
    void record(const User& actor, int submissionId, std::optional<double> oldFinal, double newFinal,
                const std::string& reason);

    // FR-26: the entries of the courses of the department of the Head of Department.
    std::vector<AuditEntry> listForMyDepartment(const User& actor);

private:
    IAuditRepository& audits_;
    Clock& clock_;
};

}  // namespace lms
