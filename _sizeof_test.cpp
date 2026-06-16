#include <cstdio>
#include "Common/ClientMsg.h"
int main() {
    printf("header=%zu v1=%zu v2=%zu\n", sizeof(Msg_S2C_ZoneListRspHeader), ZONE_ENTRY_WIRE_V1_SIZE, sizeof(Msg_S2C_ZoneEntryWire));
    return 0;
}
