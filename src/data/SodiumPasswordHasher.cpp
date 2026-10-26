#include "SodiumPasswordHasher.h"

#include <sodium.h>

#include "Exceptions.h"

namespace lms {

namespace {
void initialiseSodium() {
    // sodium_init is safe to call more than once. It returns -1 only if the library cannot start.
    if (sodium_init() < 0) throw RuleError("The password library could not start.");
}
}  // namespace

SodiumPasswordHasher::SodiumPasswordHasher() : SodiumPasswordHasher(4ULL, static_cast<std::size_t>(64) * 1024 * 1024) {}

SodiumPasswordHasher::SodiumPasswordHasher(unsigned long long opsLimit, std::size_t memLimit)
    : opsLimit_(opsLimit), memLimit_(memLimit) {
    initialiseSodium();
}

std::string SodiumPasswordHasher::hash(const std::string& password) const {
    char out[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str_alg(out, password.c_str(), password.size(), opsLimit_, memLimit_,
                              crypto_pwhash_ALG_ARGON2ID13) != 0) {
        throw RuleError("The password could not be protected (not enough memory).");
    }
    return std::string(out);
}

bool SodiumPasswordHasher::verify(const std::string& hash, const std::string& password) const {
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.size()) == 0;
}

}  // namespace lms
