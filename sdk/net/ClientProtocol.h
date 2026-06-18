/**
 * @file    ClientProtocol.h
 * @brief   客户端常用 wire 协议头聚合（转发 Common 域头文件）
 *
 * Common 已按域拆分（LoginMsg.h、ZoneMsg.h 等），wire v2 body 前两字节为 module/sub。
 */

#pragma once

#include "ClientMsgBody.h"
#include "ClientTypes.h"
#include "EquipCommon.h"
#include "LoginCommon.h"
#include "LoginMsg.h"
#include "MapDataCommon.h"
#include "MapDataMsg.h"
#include "MsgId.h"
#include "NetDefine.h"
#include "PropertyCommon.h"
#include "ZoneCommon.h"
#include "ZoneMsg.h"
