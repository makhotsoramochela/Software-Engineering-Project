// Layer 3 (infrastructure): the real clock of the computer (local time, to the minute).
#pragma once

#include "Ports.h"

namespace lms {

class SystemClock : public Clock {
public:
    DateTime now() const override;
};

}  // namespace lms
