#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Movement.h"

#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))
#define CheckIfNonValidNumber(x) (fpclassify(x) == FP_INFINITE || fpclassify(x) == FP_NAN || fpclassify(x) == FP_SUBNORMAL)

void CMovement::bunnyhop(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (cmd->buttons & IN_JUMP)
	{
		if (!(local_player->GetFlags() & FL_ONGROUND))
		{
			cmd->buttons &= ~IN_JUMP;
		}
	}
}

void CMovement::autostrafer(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetMoveType() == SDK::MOVETYPE_NOCLIP || local_player->GetMoveType() == SDK::MOVETYPE_LADDER || !local_player->IsAlive()) 
		return;

	if (local_player->GetFlags() & FL_ONGROUND)
		return;

	if (local_player->GetVelocity().Length2D() == 0 || local_player->GetVelocity().Length2D() == NAN || local_player->GetVelocity().Length2D() == INFINITE)
	{
		cmd->forwardmove = 450;
		return;
	}

	if (GetAsyncKeyState(65) || GetAsyncKeyState(83) || GetAsyncKeyState(68)) //A, S, D
		return;

	if (cmd->buttons & (IN_FORWARD | IN_MOVERIGHT | IN_MOVELEFT | IN_BACK))
		return;

	if (cmd->buttons & IN_JUMP || !(local_player->GetFlags() & FL_ONGROUND)) {
		if (cmd->mousedx > 1 || cmd->mousedx < -1) {
			cmd->sidemove = cmd->mousedx < 0.f ? -450.f : 450.f;
		}
		else {
			cmd->forwardmove = (10000.f) / (local_player->GetVelocity().Length2D() + 1);
			cmd->sidemove = (cmd->command_number % 2) == 0 ? -450.f : 450.f;
		}
	}

	/*cmd->forwardmove = clamp(5850.f / local_player->GetVelocity().Length2D(), -450, 450);

	if (cmd->forwardmove < -450 || cmd->forwardmove > 450)
		cmd->forwardmove = 0;

	cmd->sidemove = clamp((cmd->command_number % 2) == 0 ? -450.f : 450.f, -450, 450); //perfect 1 tick strafing

	if (cmd->sidemove < -450 || cmd->sidemove > 450)
		cmd->sidemove = 0;*/
}

void CMovement::quick_stop(SDK::CBaseEntity* entity, SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer()); //initialize local player

	if (!local_player) //make sure this nigga aint null
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset(); //get eye position

	if (entity->GetImmunity()) //make sure u dont stop when they in spawn protect
		return;

	if (entity->GetIsDormant()) //fuck dormant niggas
		return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex())); //initialize weapon

	if (!weapon) //make sure u aint holdin aiR niGGa
		return;

	if (weapon->is_knife() || weapon->is_grenade() || weapon->GetItemDefenitionIndex() == SDK::WEAPON_TASER) //we dont wanna stop if we holdin a knife, grenade or zeus
		return;

	if (!aimbot->can_shoot(cmd)) //so it doesn't just fully freeze us while reloading or cocking
		return;

	if (autowall->CalculateDamage(local_position, aimbot->full_point(entity), local_player, entity).damage > SETTINGS::settings.damage_val) //autowall to the enemies position and see if they meet minimum damage
	{
		cmd->forwardmove = 450; //because fuck you
		aimbot->rotate_movement(UTILS::CalcAngle(Vector(0, 0, 0), local_player->GetVelocity()).y + 180.f, cmd); //negate direction to fully stop
	}
}

CMovement* movement = new CMovement(); //memory leak