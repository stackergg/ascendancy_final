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
#include "../FEATURES/Fakewalk.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/FakeLag.h"

float fakelag_curtime(SDK::CUserCmd* ucmd)
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

int CFakeLag::lag_comp_break()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player || local_player->GetHealth() <= 0)
		return 1;	

	auto velocity = local_player->GetVelocity();
	velocity.z = 0.f;
	auto speed = velocity.Length();
	auto distance_per_tick = speed * INTERFACES::Globals->interval_per_tick;
	int choked_ticks = std::ceilf(64.f / distance_per_tick);
	return std::min<int>(choked_ticks, 14.9f);
}

void CFakeLag::do_fakelag(SDK::CUserCmd* cmd)
{
	GLOBAL::should_send_packet = true;
	int choke_amount;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player || local_player->GetHealth() <= 0)
		return;

	auto net_channel = INTERFACES::Engine->GetNetChannel();
	if (!net_channel)
		return;

	if (SETTINGS::settings.aa_type == 0 || SETTINGS::settings.aa_type == 6)
		return;

	//if u dont remember, 1 = no fakelag (no fucking shit)
	if (SETTINGS::settings.lag_bool)
	{
		Vector local_position = local_player->GetVecOrigin();

		if (local_update - INTERFACES::Globals->curtime < 0.1 && local_player->GetVelocity().Length2D() < 0.1 || INTERFACES::Engine->IsVoiceRecording())
			choke_amount = 1;
		else if (fake_walk)
		{
			if (local_update - INTERFACES::Globals->curtime < 0.1)
				choke_amount = 1;
			else
				choke_amount = slidebitch->choked < 1;
		}
		else
		{
			choke_amount = 14; // standing flag (1)
			if (!(local_player->GetFlags() & FL_ONGROUND))
			{
				if (SETTINGS::settings.lag_type == 1)
					choke_amount = lag_comp_break();
				else
					choke_amount = SETTINGS::settings.jump_lag; // jumping flag (6)
			}
			else if (local_player->GetVelocity().Length2D() > 0.1)
			{
				if (SETTINGS::settings.lag_type == 1)
					choke_amount = lag_comp_break();
				else
					choke_amount = SETTINGS::settings.move_lag; // moving flag (3)
			}
		}
	}
	else
		choke_amount = 1;

	if (net_channel->m_nChokedPackets >= min(15, choke_amount))
		GLOBAL::should_send_packet = true;
	else
		GLOBAL::should_send_packet = false;
}

CFakeLag* fakelag = new CFakeLag();