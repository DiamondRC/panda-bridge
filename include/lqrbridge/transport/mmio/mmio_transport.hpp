#pragma once
#include "lqrbridge/transport/transport.hpp"
#include "lqrbridge/types.hpp"
 
#include <cstddef>
#include <cstdint>
  
namespace lqr {

    class MmioTransport {
        public:
            // Offsets into register window.
            // TODO - replace with block's register map.
            struct Reg {
                std::size_t gains;
                std::size_t commit;
                std::size_t gen;
            };

            // 

        private:
    }

}