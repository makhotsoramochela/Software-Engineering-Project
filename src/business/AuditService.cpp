#include "AuditService.h"

#include "Validation.h"

namespace lms {

AuditService::AuditService(IAuditRepository& audits, Clock& clock) : audits_(audits), clock_(clock) {}

void AuditService::record(const User& actor, int submissionId, std::optional<double> oldFinal, double newFinal,
                          const std::string& reason) {
    const std::string cleanReason = validate::text(reason, "The reason", 500);
    audits_.add(AuditEntry(0, submissionId, actor.getId(), clock_.now(), oldFinal, newFinal, cleanReason));
}

std::vector<AuditEntry> AuditService::listForMyDepartment(const User& actor) {
    requirePermission(actor, Function::VIEW_AUDIT_LOG);
    return audits_.listByDepartment(departmentOf(actor));
}

}  // namespace lms
