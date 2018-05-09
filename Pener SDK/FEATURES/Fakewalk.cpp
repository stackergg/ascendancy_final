#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/NetChannel.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/ConVar.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/Fakewalk.h"
#include "../FEATURES/Aimbot.h"

#include <time.h>
#include <iostream>

float fakewalk_curtime(SDK::CUserCmd* ucmd) 
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

void CFakewalk::do_fakewalk(SDK::CUserCmd* cmd)
{
	if (GetAsyncKeyState(VK_SHIFT)) //make sure fakelag is set to max when u trigger fakewalk!
	{
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!local_player || local_player->GetHealth() <= 0)
			return;

		auto net_channel = INTERFACES::Engine->GetNetChannel();
		if (!net_channel)
			return;

		auto animstate = local_player->GetAnimState();
		if (!animstate)
			return;

		const int choked_ticks = net_channel->m_nChokedPackets;

		fake_walk = true;

		if (fabs(local_update - INTERFACES::Globals->curtime) <= 0.1)
		{
			cmd->forwardmove = 450;
			aimbot->rotate_movement(UTILS::CalcAngle(Vector(0, 0, 0), local_player->GetVelocity()).y + 180.f, cmd);
		}
/*
		if (!choked_ticks || choked_ticks > 8)
		{
			cmd->sidemove = 0;
			cmd->forwardmove = 450;

			if (!choked_ticks || animstate->speed_2d < 20.f)
				cmd->forwardmove = 0;

			aimbot->rotate_movement(UTILS::CalcAngle(Vector(0, 0, 0), local_player->GetVelocity()).y + 180.f, cmd); //rotates the movement duh
		}*/

		choked = choked > 7 ? 0 : choked + 1;
		cmd->forwardmove = choked < 2 || choked > 5 ? 0 : cmd->forwardmove;
		cmd->sidemove = choked < 2 || choked > 5 ? 0 : cmd->sidemove;
	}
	else
		fake_walk = false;
}

CFakewalk* slidebitch = new CFakewalk();