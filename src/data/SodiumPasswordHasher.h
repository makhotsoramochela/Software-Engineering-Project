// Layer 3 (infrastructure): password hashing with libsodium, algorithm Argon2id (NFR-03).
// The library makes a new random salt for each password and stores the salt and the settings
// inside the text it returns, so the database needs only one column (password_hash).
#pragma once

#include "Ports.h"

namespace lms {

class SodiumPasswordHasher : public PasswordHasher {
public:
    // Default settings: Argon2id, 4 passes, 64 MiB. Calibrate for the reference computer (see README):
    // one check should take between 100 and 500 milliseconds (NFR-03).
    SodiumPasswordHasher();
    SodiumPasswordHasher(unsigned long long opsLimit, std::size_t memLimit);

    std::string hash(const std::string& password) const override;
    bool verify(const std::string& hash, const std::string& password) const override;

private:
    unsigned long long opsLimit_;
    std::size_t memLimit_;
};

}  // namespace lms
