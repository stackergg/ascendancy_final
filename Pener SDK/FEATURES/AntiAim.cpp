#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/AntiAim.h"
#include "../FEATURES/AutoWall.h"
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

float randnumb(float Min, float Max)
{
	return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
}

float get_curtime(SDK::CUserCmd* ucmd) 
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) 
	{
		g_tick = (float)local_player->GetTickBase();
	}
	else 
	{
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

float anispeed_2d(SDK::CBaseEntity* LocalPlayer)
{
	if (LocalPlayer->GetAnimState() == nullptr)
		return false;

	int vel = LocalPlayer->GetAnimState()->speed_2d;
	return vel;
}

/*bool first_supress(const float yaw_to_break, SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return false;

	static float next_lby_update_time = 0;
	float curtime = get_curtime(cmd);

	auto animstate = local_player->GetAnimState();
	if (!animstate)
		return false;

	if (!(local_player->GetFlags() & FL_ONGROUND))
		return false;

	if (SETTINGS::settings.delta_val < 120)
		return false;

	if (animstate->speed_2d > 0.1)
		next_lby_update_time = curtime + 0.22 - TICKS_TO_TIME(1);

	if (next_lby_update_time < curtime)
	{
		next_lby_update_time = curtime + 1.1;
		return true;
	}

	return false;
}

bool second_supress(const float yaw_to_break, SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return false;

	static float next_lby_update_time = 0;
	float curtime = get_curtime(cmd);

	auto animstate = local_player->GetAnimState();
	if (!animstate)
		return false;

	if (!(local_player->GetFlags() & FL_ONGROUND))
		return false;

	if (SETTINGS::settings.delta_val < 120)
		return false;

	if (animstate->speed_2d > 0.1)
		next_lby_update_time = curtime + 0.22 + TICKS_TO_TIME(1);


	if (next_lby_update_time < curtime)
	{
		next_lby_update_time = curtime + 1.1;
		return true;
	}

	return false;
}*/

bool next_lby_update(const float yaw_to_break, SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return false;

	static float next_lby_update_time = 0;
	// yo the immigrant retard, we have to get predicted curtime for the lby breaker to work
	// yo the alipine retard, you already predict it in EngPred.cpp so why do it again.
	float curtime = INTERFACES::Globals->curtime;
	local_update = next_lby_update_time;

	auto animstate = local_player->GetAnimState();

	if (!animstate)
		return false;

	if (!(local_player->GetFlags() & FL_ONGROUND))
		return false;

	//in the sdk they dont use doubles. so im just gonna make this speed a float :)
	//ik the breaker works but its more proper

	if (animstate->speed_2d > 0.1f && !fake_walk)
	{
		next_lby_update_time = curtime + 0.22f;
	}

	if (next_lby_update_time < curtime)
	{
		next_lby_update_time = curtime + 1.1f;
		return true;
	}

	return false;
}

bool is_shooting_proper(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	auto cs_Grenade = reinterpret_cast<SDK::CBaseCSGrenade*>(weapon);
	if (weapon->is_knife())
	{
		if (cmd->buttons & IN_ATTACK || cmd->buttons & IN_ATTACK2)
			return true;
	}
	else if (weapon->is_grenade())
	{
		if (cs_Grenade->throw_time() > 0.f)
		{
			return true;
		}
	}
	else if (cmd->buttons & IN_ATTACK)
	{
		if (weapon->is_revolver() && SETTINGS::settings.aim_bool)
		{
			if (weapon->GetPostponeFireReadyTime() - get_curtime(cmd) <= 0.05f)
			{
				return true;
			}
		}
		else
			return true;
	}
	return false;
}

void CAntiAim::do_antiaim(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (cmd->buttons & IN_USE)
		return;

	if (cmd->buttons & IN_ATTACK && aimbot->can_shoot(cmd))
		return;

	if (local_player->GetMoveType() == SDK::MOVETYPE_NOCLIP || local_player->GetMoveType() == SDK::MOVETYPE_LADDER)
		return;

	auto cs_Grenade = reinterpret_cast<SDK::CBaseCSGrenade*>(weapon);
	if (weapon->is_grenade())
	{
		if (cs_Grenade->throw_time() > 0.f)
			return;
	}

	if (!SETTINGS::settings.aa_bool)
		return;

	//float delta = abs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw()));
	//std::cout << std::to_string(delta) << std::endl;

	switch (SETTINGS::settings.aa_pitch % 5)
	{
	case 0:
		break;
	case 1:
		cmd->viewangles.x = 89.000000; //emotion
		break;
	case 2:
		cmd->viewangles.x = -180.f; //fake down
		break;
	case 3:
		cmd->viewangles.x = 180.f; //fake up
		break;
	case 4:
		cmd->viewangles.x = 1080.f; //fake zero
		break;
	}

	switch (SETTINGS::settings.aa_type % 78)
	{
	case 0:
		break;
	case 1:
		//cmd->viewangles.x = 89.000000;
		backwards(cmd);
		break;
	case 2:
		//cmd->viewangles.x = 89.000000;
		sideways(cmd);
		break;
	case 3:
		sidespin(cmd);
		break;
	case 4:
		//cmd->viewangles.x = 89.000000;
		backjitter(cmd);
		break;
	case 5:
		//cmd->viewangles.x = 89.000000;
		if (GetAsyncKeyState(VK_XBUTTON1))
			backjitter(cmd);
		else
			lowerbody(cmd);
		break;
	case 6:
		legit(cmd);
		break;
	case 7:
		freestand(cmd);
		break;
	}
}

void CAntiAim::fix_movement(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (SETTINGS::settings.aa_pitch > 1)
	{
		cmd->forwardmove = clamp(cmd->forwardmove, -450.f, 450.f);
		cmd->sidemove = clamp(cmd->sidemove, -450.f, 450.f);
		cmd->upmove = clamp(cmd->upmove, -320.f, 320.f);

		cmd->viewangles.x = clamp(cmd->viewangles.x, -89.f, 89.f);
	}

	Vector real_viewangles;
	INTERFACES::Engine->GetViewAngles(real_viewangles);

	Vector vecMove(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	float speed = sqrt(vecMove.x * vecMove.x + vecMove.y * vecMove.y);

	Vector angMove;
	MATH::VectorAngles(vecMove, angMove);

	float yaw = DEG2RAD(cmd->viewangles.y - real_viewangles.y + angMove.y);

	cmd->forwardmove = cos(yaw) * speed;
	cmd->sidemove = sin(yaw) * speed;

	cmd->viewangles = MATH::NormalizeAngle(cmd->viewangles);

	if (cmd->viewangles.x < -89.f || cmd->viewangles.x > 89.f) cmd->forwardmove *= -1;
}

void CAntiAim::backwards(SDK::CUserCmd* cmd)
{
	if (GLOBAL::should_send_packet)
		cmd->viewangles.y += 179.000000; //cya fake angle checks
	else
		cmd->viewangles.y += 180.000000;
}

void CAntiAim::legit(SDK::CUserCmd* cmd)
{
	if (GLOBAL::should_send_packet)
		cmd->viewangles.y += 0;
	else
		cmd->viewangles.y += 90;
}

float ns_num;
float last_lby;

void CAntiAim::sidespin(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetFlags() & FL_ONGROUND)
		cmd->viewangles.y += 180;
	else
	{
		if (local_player->GetLowerBodyYaw() != last_lby)
		{
			last_lby = local_player->GetLowerBodyYaw();
			ns_num = randnumb(60, 110);
		}

		cmd->viewangles.y = local_player->GetLowerBodyYaw() + ns_num;
	}
}

float fov_player(Vector ViewOffSet, Vector View, SDK::CBaseEntity* entity, int hitbox)
{
	// Anything past 180 degrees is just going to wrap around
	CONST FLOAT MaxDegrees = 180.0f;

	// Get local angles
	Vector Angles = View;

	// Get local view / eye position
	Vector Origin = ViewOffSet;

	// Create and intiialize vectors for calculations below
	Vector Delta(0, 0, 0);
	//Vector Origin(0, 0, 0);
	Vector Forward(0, 0, 0);

	// Convert angles to normalized directional forward vector
	MATH::AngleVectors(Angles, &Forward);

	Vector AimPos = aimbot->get_hitbox_pos(entity, hitbox); //pvs fix disabled

	MATH::VectorSubtract(AimPos, Origin, Delta);
	//Delta = AimPos - Origin;

	// Normalize our delta vector
	MATH::NormalizeNum(Delta, Delta);

	// Get dot product between delta position and directional forward vectors
	FLOAT DotProduct = Forward.Dot(Delta);

	// Time to calculate the field of view
	return (acos(DotProduct) * (MaxDegrees / M_PI));
}

int closest_to_crosshair()
{
	int index = -1;
	float lowest_fov = INT_MAX;

	SDK::CBaseEntity* local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector angles;
	INTERFACES::Engine->GetViewAngles(angles);

	for (int i = 1; i <= INTERFACES::Globals->maxclients; i++)
	{
		SDK::CBaseEntity *entity = INTERFACES::ClientEntityList->GetClientEntity(i);

		if (!entity || !entity->IsAlive() || entity->GetTeam() == local_player->GetTeam() || entity->GetIsDormant() || entity == local_player)
			continue;

		float fov = fov_player(local_position, angles, entity, 0);

		if (fov < lowest_fov)
		{
			lowest_fov = fov;
			index = i;
		}
	}

	return index;
}

void CAntiAim::freestand(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	static float last_real;
	bool no_active = true;
	float bestrotation = 0.f;
	float highestthickness = 0.f;
	Vector besthead;

	auto leyepos = local_player->GetVecOrigin() + local_player->GetViewOffset();
	auto headpos = aimbot->get_hitbox_pos(local_player, 0);
	auto origin = local_player->GetAbsOrigin();

	auto checkWallThickness = [&](SDK::CBaseEntity* pPlayer, Vector newhead) -> float
	{
		Vector endpos1, endpos2;
		Vector eyepos = pPlayer->GetVecOrigin() + pPlayer->GetViewOffset();

		SDK::Ray_t ray;
		ray.Init(newhead, eyepos);

		SDK::CTraceFilterSkipTwoEntities filter(pPlayer, local_player);

		SDK::trace_t trace1, trace2;
		INTERFACES::Trace->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace1);

		if (trace1.DidHit())
			endpos1 = trace1.end;
		else
			return 0.f;

		ray.Init(eyepos, newhead);
		INTERFACES::Trace->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace2);

		if (trace2.DidHit())
			endpos2 = trace2.end;

		float add = newhead.Dist(eyepos) - leyepos.Dist(eyepos) + 3.f;
		return endpos1.Dist(endpos2) + add / 3;
	};

	int index = closest_to_crosshair();

	SDK::CBaseEntity* entity;

	if (index != -1)
		entity = INTERFACES::ClientEntityList->GetClientEntity(index);

	float step = (2 * M_PI) / 18.f; // One PI = half a circle ( for stacker cause low iq :sunglasses: ), 28

	float radius = fabs(Vector(headpos - origin).Length2D());

	if (index == -1)
	{
		no_active = true;
	}
	else
	{
		for (float rotation = 0; rotation < (M_PI * 2.0); rotation += step)
		{
			Vector newhead(radius * cos(rotation) + leyepos.x, radius * sin(rotation) + leyepos.y, leyepos.z);

			float totalthickness = 0.f;

			no_active = false;

			totalthickness += checkWallThickness(entity, newhead);

			if (totalthickness > highestthickness)
			{
				highestthickness = totalthickness;
				bestrotation = rotation;
				besthead = newhead;
			}
		}
	}

	if (GLOBAL::should_send_packet)
		cmd->viewangles.y += randnumb(-180, 180);
	else
	{
		if (next_lby_update(cmd->viewangles.y + SETTINGS::settings.delta_val, cmd))
		{
			cmd->viewangles.y = last_real + SETTINGS::settings.delta_val;
		}
		else
		{
			if (no_active)
				cmd->viewangles.y += 180.f;
			else
				cmd->viewangles.y = RAD2DEG(bestrotation);

			last_real = cmd->viewangles.y;
		}
	}
}

void CAntiAim::sideways(SDK::CUserCmd* cmd)
{
	if (SETTINGS::settings.flip_bool)
	{
		if (GLOBAL::should_send_packet)
			cmd->viewangles.y += randnumb(-180, 180);
		else
			cmd->viewangles.y -= 90;
	}
	else
	{
		if (GLOBAL::should_send_packet)
			cmd->viewangles.y += randnumb(-180, 180);
		else
			cmd->viewangles.y += 90;
	}
}

void CAntiAim::backjitter(SDK::CUserCmd* cmd)
{
	if (GLOBAL::should_send_packet)
		cmd->viewangles.y += randnumb(-180, 180);
	else
		cmd->viewangles.y += 180 + ((rand() % 35) - (35 * 0.5f));
}

void CAntiAim::lowerbody(SDK::CUserCmd* cmd)
{
	static float last_real;
	auto nci = INTERFACES::Engine->GetNetChannelInfo();

	if (SETTINGS::settings.flip_bool)
	{
		if (GLOBAL::should_send_packet)
			cmd->viewangles.y += randnumb(-180, 180);
		else
		{
			if (next_lby_update(cmd->viewangles.y + SETTINGS::settings.delta_val, cmd)) //else if
			{
				cmd->viewangles.y = last_real + SETTINGS::settings.delta_val;
			}
			else
			{
				cmd->viewangles.y -= 90;
				last_real = cmd->viewangles.y;
			}
		}
	}
	else
	{
		if (GLOBAL::should_send_packet)
			cmd->viewangles.y += randnumb(-180, 180);
		else
		{
			if (next_lby_update(cmd->viewangles.y - SETTINGS::settings.delta_val, cmd))
			{
				cmd->viewangles.y = last_real - SETTINGS::settings.delta_val;
			}
			else
			{
				cmd->viewangles.y += 90;
				last_real = cmd->viewangles.y;
			}
		}
	}
}

CAntiAim* antiaim = new CAntiAim();