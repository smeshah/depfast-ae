#pragma once

#include "../__dep__.h"
#include "../coordinator.h"
#include "../frame.h"

namespace Janus {

    CoordinatorQuePaxa :: CoordinatorQuePaxa(uint32_t coo_id,
                                             int32_t benchmark,
                                             ClientControlServiceImpl* ccsi,
                                             uint32_t thread_id)
    : Coordinator(coo_id, benchmark, ccsi, thread_id)
}