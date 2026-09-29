// SPDX-License-Identifier: MIT
/* Shared soc_init() for STM32 SoCs. The USARTs are devices in soc.yaml,
 * instantiated by dt_init(); no chip-level setup is needed before board_init(). */
#include "homecore/soc/soc.h"

void soc_init(void) {
}
