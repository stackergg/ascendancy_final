#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/IViewRenderBeams.h"
#include "../FEATURES/Backtracking.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseWeapon.h"
#include "../FEATURES/AutoWall.h"
#include "../SDK/CTrace.h"	
#include "../FEATURES/Resolver.h"
#include "../SDK/CGlobalVars.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Visuals.h"
#include "../UTILS/render.h"
#include "../SDK/IVDebugOverlay.h"
#include <string.h>

//--- Misc Variable Initalization ---//
int alpha[65];
CColor breaking;
CColor backtrack;
static bool bPerformed = false, bLastSetting;
float fade_alpha[65];
float dormant_time[65];
CColor main_color;
CColor ammo;
SDK::CBaseEntity *BombCarrier;

void CVisuals::set_hitmarker_time( float time ) 
{
	GLOBAL::flHurtTime = time;
}

void CVisuals::Draw()
{
	if ( !INTERFACES::Engine->IsInGame( ) ) {
		GLOBAL::flHurtTime = 0.f;
		return;
	}

	for (int i = 1; i <= 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!entity)
			continue;

		if (!local_player)
			continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (is_teammate) 
			continue;

		if (entity->GetHealth() <= 0)
			continue;

		if (!entity->IsAlive())
			continue;

		if (entity->GetVecOrigin() == Vector(0, 0, 0))
			continue;

		//--- Colors ---//
		int enemy_hp = entity->GetHealth();
		int hp_red = 255 - (enemy_hp * 2.55);
		int hp_green = enemy_hp * 2.55;
		CColor health_color = CColor(hp_red, hp_green, 1, alpha[entity->GetIndex()]);
		CColor dormant_color = CColor(100, 100, 100, alpha[entity->GetIndex()]);
		CColor box_color;
		CColor still_health;
		CColor alt_color;
		CColor zoom_color;
		CColor bomb_color;

		//--- Domant ESP Checks ---//
		if (entity->GetIsDormant())
		{
			//if (alpha[entity->GetIndex()] > 150)
				//alpha[entity->GetIndex()] = 149;

			if (alpha[entity->GetIndex()] > 0)
				alpha[entity->GetIndex()] -= 0.01;
			main_color = dormant_color;
			still_health = dormant_color;
			alt_color = CColor(20, 20, 20, alpha[entity->GetIndex()]);
			zoom_color = dormant_color;
			breaking = dormant_color;
			backtrack = dormant_color;
			box_color = dormant_color;
			bomb_color = dormant_color;
			ammo = dormant_color;
		}
		else if (!entity->GetIsDormant())
		{
			alpha[entity->GetIndex()] = 165;
			main_color = CColor(255, 255, 255, alpha[entity->GetIndex()]); //heath_color
			still_health = health_color;
			alt_color = CColor(0, 0, 0, 165);
			zoom_color = CColor(150, 150, 220, 165);
			breaking = CColor(220, 150, 150, 165);
			backtrack = CColor(155, 220, 150, 165);
			box_color = SETTINGS::settings.box_col;
			bomb_color = CColor(244, 66, 66, 165);
			ammo = CColor(61, 135, 255, 165);
		}

		//--- Entity Related Rendering ---///
		switch (SETTINGS::settings.box_type)
		{
		case 0: break;
		case 1: DrawBox(entity, box_color); break;
		case 2: DrawCorners(entity, box_color); break;
		}
		if (SETTINGS::settings.name_bool) DrawName(entity, main_color, i);
		if (SETTINGS::settings.weap_bool) DrawWeapon(entity, main_color, i);
		if (SETTINGS::settings.health_bool) DrawHealth(entity, still_health, alt_color);
		if (SETTINGS::settings.dist_bool) DrawDistance(entity, main_color);
		if (SETTINGS::settings.fov_bool) DrawFovArrows(entity);
		if (SETTINGS::settings.ammo_bool) DrawAmmo(entity, ammo, alt_color);
		DrawInfo(entity, main_color, zoom_color);
	}
}

void CVisuals::ClientDraw()
{
	if (SETTINGS::settings.spread_bool) DrawInaccuracy();
	if (SETTINGS::settings.scope_bool) DrawBorderLines();

	DrawIndicator();
	DrawHitmarker();
}

std::string str_to_upper(std::string strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);

	return strToConvert;
}

void CVisuals::DrawBox(SDK::CBaseEntity* entity, CColor color)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;

		RENDER::DrawEmptyRect(pos.x - width / 2, top.y, (pos.x - width / 2) + width, top.y + height, color);
		RENDER::DrawEmptyRect((pos.x - width / 2) + 1, top.y + 1, (pos.x - width / 2) + width - 1, top.y + height - 1, CColor(20, 20, 20, alpha[entity->GetIndex()]));
		RENDER::DrawEmptyRect((pos.x - width / 2) - 1, top.y - 1, (pos.x - width / 2) + width + 1, top.y + height + 1, CColor(20, 20, 20, alpha[entity->GetIndex()]));
	}

	/*Vector bbmin; //another way of doing it, doesn't look gae tho
	Vector bbmax;
	Vector screen1;
	Vector screen2;
	entity->GetRenderBounds(bbmin, bbmax);
	Vector pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	Vector top3D = pos3D + Vector(0, 0, bbmax.z + 10);

	if (RENDER::WorldToScreen(pos3D, screen1) && RENDER::WorldToScreen(top3D, screen2))
	{
	float height = screen1.y - screen2.y;
	float width = height / 4;
	RENDER::DrawEmptyRect(screen2.x - width, screen2.y, screen1.x + width, screen1.y, color); //main
	RENDER::DrawEmptyRect(screen2.x - width - 1, screen2.y - 1, screen1.x + width + 1, screen1.y + 1, CColor(20, 20, 20, alpha[entity->GetIndex()])); //outline
	RENDER::DrawEmptyRect(screen2.x - width + 1, screen2.y + 1, screen1.x + width - 1, screen1.y - 1, CColor(20, 20, 20, alpha[entity->GetIndex()])); //inline
	}*/
}

void CVisuals::DrawCorners(SDK::CBaseEntity* entity, CColor color)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;

		RENDER::DrawEdges(pos.x - width / 2, top.y, (pos.x - width / 2) + width, top.y + height, 8, color);
		RENDER::DrawEdges((pos.x - width / 2) + 1, top.y + 1, (pos.x - width / 2) + width - 1, top.y + height - 1, 8, CColor(20, 20, 20, alpha[entity->GetIndex()]));
		RENDER::DrawEdges((pos.x - width / 2) - 1, top.y - 1, (pos.x - width / 2) + width + 1, top.y + height + 1, 8, CColor(20, 20, 20, alpha[entity->GetIndex()]));
	}
}

void CVisuals::DrawName(SDK::CBaseEntity* entity, CColor color, int index)
{
	SDK::player_info_t ent_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);
	INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		RENDER::DrawF(pos.x, top.y - 7, FONTS::visuals_name_font, true, true, color, ent_info.name); //numpad_menu_font
	}
/*	SDK::player_info_t ent_info;
	Vector bbmin;
	Vector bbmax;
	Vector screen1;
	Vector screen2;
	entity->GetRenderBounds(bbmin, bbmax);
	Vector bottom = entity->GetAbsOrigin();
	Vector top = entity->GetAbsOrigin() + Vector(0, 0, bbmax.z);
	INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	if (RENDER::WorldToScreen(bottom, screen1) && RENDER::WorldToScreen(top, screen2))
	{
		float height = screen1.y - screen2.y;
		float width = height / 4;
		RENDER::DrawF(screen2.x, screen2.y - 6, FONTS::numpad_menu_font, true, true, color, ent_info.name);
	}*/
}

float CVisuals::resolve_distance(Vector src, Vector dest)
{
	Vector delta = src - dest;

	float fl_dist = ::sqrtf((delta.Length()));

	if (fl_dist < 1.0f)
		return 1.0f;

	return fl_dist;
}

void CVisuals::DrawDistance(SDK::CBaseEntity* entity, CColor color)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	SDK::player_info_t ent_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);
	Vector vecOrigin = entity->GetVecOrigin();
	Vector vecOriginLocal = local_player->GetVecOrigin();

	char dist_to[32];
	sprintf_s(dist_to, "%.0f ft", resolve_distance(vecOriginLocal, vecOrigin));

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		RENDER::DrawF(pos.x, pos.y + 12, FONTS::visuals_esp_font, true, true, color, dist_to); //8
	}
}

std::string fix_item_name(std::string name)
{
	std::string cname = name;

	if (cname[0] == 'C')
		cname.erase(cname.begin());

	auto startOfWeap = cname.find("Weapon");
	if (startOfWeap != std::string::npos)
		cname.erase(cname.begin() + startOfWeap, cname.begin() + startOfWeap + 6);

	return cname;
}

void CVisuals::DrawWeapon(SDK::CBaseEntity* entity, CColor color, int index)
{
	SDK::player_info_t ent_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);
	INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (!weapon)
		return;

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		if (SETTINGS::settings.dist_bool)
		{
			if (c_baseweapon->GetLoadedAmmo() != -1)
			{
				RENDER::DrawF(pos.x, pos.y + 21, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));// + " (" + std::to_string(c_baseweapon->GetLoadedAmmo()) + ")"); //17

			}
			else
			{
				RENDER::DrawF(pos.x, pos.y + 21, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));// + " (" + std::to_string(c_baseweapon->GetLoadedAmmo()) + ")");
			}
		}
		else
		{
			if (c_baseweapon->GetLoadedAmmo() != -1)
			{
				RENDER::DrawF(pos.x, pos.y + 12, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));// +" (" + std::to_string(c_baseweapon->GetLoadedAmmo()) + ")"); //8

			}
			else
			{
				RENDER::DrawF(pos.x, pos.y + 12, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));// + " (" + std::to_string(c_baseweapon->GetLoadedAmmo()) + ")");
			}
		}
	}
}

void CVisuals::DrawHealth(SDK::CBaseEntity* entity, CColor color, CColor dormant)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	int enemy_hp = entity->GetHealth();
	int hp_red = 255 - (enemy_hp * 2.55);
	int hp_green = enemy_hp * 2.55;
	CColor health_color = CColor(hp_red, hp_green, 1, alpha[entity->GetIndex()]);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;

		float offset = (height / 4.f) + 5;
		UINT hp = height - (UINT)((height * enemy_hp) / 100);

		RENDER::DrawFilledRect((pos.x - width / 2) - 7, top.y - 1, (pos.x - width / 2) - 3, top.y + height + 2, dormant); //intense maths
		RENDER::DrawLine((pos.x - width / 2) - 5, top.y + hp, (pos.x - width / 2) - 5, top.y + height, color); //could have done a rect here,
		RENDER::DrawLine((pos.x - width / 2) - 6, top.y + hp, (pos.x - width / 2) - 6, top.y + height, color); //but fuck it

		if (entity->GetHealth() < 100)
			RENDER::DrawF((pos.x - width / 2) - 4, top.y + hp, FONTS::visuals_esp_font, true, true, main_color, std::to_string(enemy_hp));
	}
}

void CVisuals::BombPlanted(SDK::CBaseEntity* entity)
{
	BombCarrier = nullptr;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector vOrig; Vector vScreen;
	vOrig = entity->GetVecOrigin();
	SDK::CCSBomb* Bomb = (SDK::CCSBomb*)entity;

	float flBlow = Bomb->GetC4BlowTime();
	float TimeRemaining = flBlow;// -(INTERFACES::Globals->interval_per_tick * local_player->GetTickBase());
	char buffer[64];
	sprintf_s(buffer, "B - %.1fs", TimeRemaining);
	RENDER::DrawF(10, 10, FONTS::visuals_lby_font, false, false, CColor(124, 195, 13, 255), buffer);
}

void CVisuals::DrawDropped(SDK::CBaseEntity* entity)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	SDK::CBaseWeapon* weapon_cast = (SDK::CBaseWeapon*)entity;

	if (!weapon_cast)
		return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (!weapon)
		return;

	SDK::CBaseEntity* plr = INTERFACES::ClientEntityList->GetClientEntityFromHandle((HANDLE)weapon_cast->GetOwnerHandle());
	if (!plr && RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		std::string ItemName = fix_item_name(weapon->GetClientClass()->m_pNetworkName);
		int height = (pos.y - top.y);
		int width = height / 2;
		RENDER::DrawF(pos.x, pos.y, FONTS::visuals_esp_font, true, true, WHITE, ItemName.c_str()); //numpad_menu_font
	}
}

void CVisuals::DrawAmmo(SDK::CBaseEntity* entity, CColor color, CColor dormant)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	int enemy_hp = entity->GetHealth();
	int hp_red = 255 - (enemy_hp * 2.55);
	int hp_green = enemy_hp * 2.55;
	CColor health_color = CColor(hp_red, hp_green, 1, alpha[entity->GetIndex()]);

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);

		float offset = (height / 4.f) + 5;
		UINT hp = height - (UINT)((height * 3) / 100);

		auto animLayer = entity->GetAnimOverlay(1);
		if (!animLayer.m_pOwner)
			return;

		auto activity = entity->GetSequenceActivity(animLayer.m_nSequence);

		int iClip = c_baseweapon->GetLoadedAmmo();
		int iClipMax = c_baseweapon->get_full_info()->iMaxClip1;

		float box_w = (float)fabs(height / 2);
		float width;
		if (activity == 967 && animLayer.m_flWeight != 0.f)
		{
			float cycle = animLayer.m_flCycle; // 1 = finished 0 = just started
			width = (((box_w * cycle) / 1.f));
		}
		else
			width = (((box_w * iClip) / iClipMax));

		RENDER::DrawFilledRect((pos.x - box_w / 2), top.y + height + 3, (pos.x - box_w / 2) + box_w + 2, top.y + height + 7, dormant); //outline
		RENDER::DrawFilledRect((pos.x - box_w / 2) + 1, top.y + height + 4, (pos.x - box_w / 2) + width + 1, top.y + height + 6, color); //ammo
	}
}

void CVisuals::DrawInfo(SDK::CBaseEntity* entity, CColor color, CColor alt)
{
	std::vector<std::pair<std::string, CColor>> stored_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	//if (entity->GetHealth() > 0)
		//stored_info.push_back(std::pair<std::string, CColor>("hp: " + std::to_string(entity->GetHealth()), color));

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	//auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	//auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	//if (!weapon)
		//return;

	//if (!c_baseweapon)
		//return;

	//stored_info.push_back(std::pair<std::string, CColor>(weapon->GetClientClass()->m_pNetworkName, color));
	//stored_info.push_back(std::pair<std::string, CColor>("ammo: " + std::to_string(c_baseweapon->GetLoadedAmmo()), color));

	if (SETTINGS::settings.money_bool)
		stored_info.push_back(std::pair<std::string, CColor>("$" + std::to_string(entity->GetMoney()), backtrack));

	if (SETTINGS::settings.info_bool)
	{
		if (entity->GetArmor() > 0)
			stored_info.push_back(std::pair<std::string, CColor>(entity->GetArmorName(), color));

		if (entity->GetIsScoped())
			stored_info.push_back(std::pair<std::string, CColor>("zoom", alt));

		if (using_fake_angles[entity->GetIndex()] && entity->GetVelocity().Length2D() < 0.1 || using_fake_angles[entity->GetIndex()] && !(entity->GetFlags() & FL_ONGROUND) || using_fake_angles[entity->GetIndex()] && !backtrack_tick[entity->GetIndex()])
			stored_info.push_back(std::pair<std::string, CColor>("fake", color));
	}
	//auto &track = backtracking->get_records( entity->GetIndex( ) )->storedTicks;
	//if ( !track.empty( ) )
	//	stored_info.push_back( std::pair<std::string, CColor>( "oldest simtime : " + std::to_string( track.at( track.size( ) - 1 ).lag_record.m_flSimulationTime ), color ) );
	//stored_info.push_back( std::pair<std::string, CColor>( "current simtime : " + std::to_string( entity->GetSimTime( ) ), color ) );

	//if (entity->HasC4())
		//stored_info.push_back(std::pair<std::string, CColor>("B", backtrack));

	/*if (!SETTINGS::settings.aim_type == 2)
		stored_info.push_back(std::pair<std::string, CColor>("null", color));
	else
	{
		if (local_player->GetHealth() == 0)
			stored_info.push_back(std::pair<std::string, CColor>("null", color)); //not resolving
		else if (resolve_type[entity->GetIndex()] == 1)
			stored_info.push_back(std::pair<std::string, CColor>("fake 2", color)); //moving
		else if (resolve_type[entity->GetIndex()] == 2)
			stored_info.push_back(std::pair<std::string, CColor>("shuffle correct", color));
		else if (resolve_type[entity->GetIndex()] == 3)
			stored_info.push_back(std::pair<std::string, CColor>("fake 1", color)); //lby updates
		else if (resolve_type[entity->GetIndex()] == 4)
			stored_info.push_back(std::pair<std::string, CColor>("fake 4", color)); //bruteforce
		else if (resolve_type[entity->GetIndex()] == 5)
			stored_info.push_back(std::pair<std::string, CColor>("fake 3", color)); //logged angle
		else if (resolve_type[entity->GetIndex()] == 6)
			stored_info.push_back(std::pair<std::string, CColor>("lby inverse", color));
		else if (resolve_type[entity->GetIndex()] == 7)
			stored_info.push_back(std::pair<std::string, CColor>("fake 5", color)); //backtracking lby
		else
			stored_info.push_back(std::pair<std::string, CColor>("not resolving", color));

	}*/

	//stored_info.push_back(std::pair<std::string, CColor>("update: " + std::to_string(INTERFACES::Globals->curtime - update_time[entity->GetIndex()]), backtrack));

	/*if (entity->GetVelocity().Length2D() > 36)
		stored_info.push_back(std::pair<std::string, CColor>("velocity: " + std::to_string(entity->GetVelocity().Length2D()), CColor(0, 255, 0, alpha[entity->GetIndex()])));
	else if (entity->GetVelocity().Length2D() < 36 && entity->GetVelocity().Length2D() > 0.1)
		stored_info.push_back(std::pair<std::string, CColor>("velocity: " + std::to_string(entity->GetVelocity().Length2D()), CColor(255, 255, 0, alpha[entity->GetIndex()])));
	else
		stored_info.push_back(std::pair<std::string, CColor>("velocity: " + std::to_string(entity->GetVelocity().Length2D()), CColor(255, 0, 0, alpha[entity->GetIndex()])));*/

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		int i = 0;
		for (auto Text : stored_info)
		{
			RENDER::DrawF((pos.x + width / 2) + 5, top.y + i, FONTS::visuals_esp_font, false, false, Text.second, Text.first); //numpad_menu_font
			i += 9;
		}
	}
}

/*void CVisuals::LagCompHitbox(SDK::CBaseEntity* entity)
{
	float duration = 1;

	if (entity->GetIndex() < 0)
		return;

	CColor color = RED;

	if (!entity)
		return;

	SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());

	if (!pStudioModel)
		return;

	static matrix3x4_t pBoneToWorldOut[128];

	if (!entity->SetupBones(pBoneToWorldOut, MAXSTUDIOBONES, 256, INTERFACES::Globals->curtime))
		return;

	SDK::mstudiohitboxset_t* pHitboxSet = pStudioModel->GetHitboxSet(0);
	if (!pHitboxSet)
		return;

	for (int i = 0; i < pHitboxSet->numhitboxes; i++)
	{
		SDK::mstudiobbox_t* pHitbox = pHitboxSet->GetHitbox(i);
		if (!pHitbox)
			continue;

		Vector vMin, vMax;
		MATH::VectorTransform(pHitbox->bbmin, pBoneToWorldOut[pHitbox->bone], vMin); //nullptr???
		MATH::VectorTransform(pHitbox->bbmax, pBoneToWorldOut[pHitbox->bone], vMax);

		if (pHitbox->m_flRadius > -1)
		{
			INTERFACES::DebugOverlay->AddCapsuleOverlay(vMin, vMax, pHitbox->m_flRadius, color.RGBA[0], color.RGBA[1], color.RGBA[2], 100, duration);
		}
	}

}

/*void CVisuals::RenderHitbox(SDK::CBaseEntity* entity)
{
	CColor color = RED;

	matrix3x4_t matrix[MAXSTUDIOBONES];

	if (!entity->SetupBones(matrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, 0))
		return;

	SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());
	if (!pStudioModel)
		return;

	SDK::mstudiohitboxset_t* set = pStudioModel->GetHitboxSet(0);
	if (!set)
		return;

	for (int i = 0; i < set->numhitboxes; i++)
	{
		SDK::mstudiobbox_t* hitbox = set->GetHitbox(i);
		if (!hitbox)
			return;

		int bone = hitbox->bone;

		Vector vMaxUntransformed = hitbox->bbmax;
		Vector vMinUntransformed = hitbox->bbmin;

		if (hitbox->m_flRadius != -1.f)
		{
			vMinUntransformed -= Vector(hitbox->m_flRadius, hitbox->m_flRadius, hitbox->m_flRadius);
			vMaxUntransformed += Vector(hitbox->m_flRadius, hitbox->m_flRadius, hitbox->m_flRadius);
		}

		HitboxESP(matrix, vMinUntransformed, vMaxUntransformed, hitbox->bone, color);
	}
}

void CVisuals::HitboxESP(matrix3x4_t* matrix, Vector bbmin, Vector bbmax, int bone, CColor color)
{
	Vector points[] = {
		Vector(bbmin.x, bbmin.y, bbmin.z),
		Vector(bbmin.x, bbmax.y, bbmin.z),
		Vector(bbmax.x, bbmax.y, bbmin.z),
		Vector(bbmax.x, bbmin.y, bbmin.z),
		Vector(bbmax.x, bbmax.y, bbmax.z),
		Vector(bbmin.x, bbmax.y, bbmax.z),
		Vector(bbmin.x, bbmin.y, bbmax.z),
		Vector(bbmax.x, bbmin.y, bbmax.z)
	};

	Vector pointsTransformed[8];

	for (int index = 0; index < 8; ++index) {
		if (index != 0)
			points[index] = ((((points[index] + points[0]) * .5f) + points[index]) * .5f);

		pointsTransformed[index] = MATH::VectorTransformTest(points[index], matrix[bone]);
	}

}*/

void CVisuals::DrawSkeleton( SDK::CBaseEntity* entity, CColor color, VMatrix bone_matrix [128] ) //wut hek
{
	SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel( entity->GetModel( ) );
	if ( !pStudioModel )
		return;

	for ( int i = 0; i < pStudioModel->numbones; i++ ) {
		SDK::mstudiobone_t* pBone = pStudioModel->pBone( i );
		if ( !pBone || !( pBone->flags & 256 ) || pBone->parent == -1 )
			continue;

		Vector vBonePos1;
		if ( !RENDER::WorldToScreen( Vector( bone_matrix [i] [0] [3], bone_matrix [i] [1] [3], bone_matrix [i] [2] [3] ), vBonePos1 ) )
			continue;

		Vector vBonePos2;
		if ( !RENDER::WorldToScreen( Vector( bone_matrix [pBone->parent] [0] [3], bone_matrix [pBone->parent] [1] [3], bone_matrix [pBone->parent] [2] [3] ), vBonePos2 ) )
			continue;

		RENDER::DrawLine( ( int ) vBonePos1.x, ( int ) vBonePos1.y, ( int ) vBonePos2.x, ( int ) vBonePos2.y, color );
	}
}

const char* clantaganimation[11] = {
	"           s",
	"     s     t",
	"s    t     a",
	"st    a    c",
	"sta   c    k",
	"stac  k    h",
	"stack  h   a",
	"stackh  a  c",
	"stackha  c k"
	"stackhac k  "
	"stackhack"
};

int kek = 0;
int autism = 0;
void CVisuals::apply_clantag()
{
	static size_t lastTime = 0;

	if (GetTickCount() > lastTime)
	{
		kek++;
		if (kek > 12) {
			autism = autism + 1;

			if (autism >= 11)
				autism = 0;

			char random[255];
			UTILS::SetClanTag(clantaganimation[autism], clantaganimation[autism]);

			lastTime = GetTickCount() + 500;
		}

		if (kek > 11)
			kek = 0;
	}
}

void CVisuals::DrawInaccuracy()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;
	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon)
		return;

	int W, H, cW, cH;
	INTERFACES::Engine->GetScreenSize(W, H);
	cW = W / 2;
	cH = H / 2;
	if (local_player->IsAlive())
	{
		auto accuracy = (weapon->GetInaccuracy() + weapon->GetSpreadCone()) * 500.f;

		if (!weapon->is_grenade() && !weapon->is_knife())
			RENDER::DrawFilledCircle(cW, cH, accuracy + 3, 30, CColor(0, 0, 0, 85));
	}
}

void CVisuals::DrawBulletBeams()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!INTERFACES::Engine->IsInGame() || !local_player)
	{
		Impacts.clear();
		return;
	}

	if (Impacts.size() > 30)
		Impacts.pop_back();

	for (int i = 0; i < Impacts.size(); i++)
	{
		auto current = Impacts.at(i);

		if (!current.pPlayer)
			continue;

		if (current.pPlayer->GetIsDormant())
			continue;

		bool is_local_player = current.pPlayer == local_player;
		bool is_teammate = local_player->GetTeam() == current.pPlayer->GetTeam() && !is_local_player;

		if (current.pPlayer == local_player)
		{
			//current.color = { 0, 255, 0 };
			continue;
		}
		else if (current.pPlayer != local_player && !is_teammate)
			current.color = { 255, 0, 0 };
		else if (current.pPlayer != local_player && is_teammate)
			current.color = { 0, 80, 255};

		SDK::BeamInfo_t beamInfo;
		beamInfo.m_nType = SDK::TE_BEAMPOINTS;
		beamInfo.m_pszModelName = "sprites/physbeam.vmt"; //purplelaser1
		beamInfo.m_nModelIndex = -1;
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = 3.0f;
		beamInfo.m_flWidth = 2.0f;
		beamInfo.m_flEndWidth = 2.0f;
		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = 2.0f;
		beamInfo.m_flBrightness = 255.f; 
		beamInfo.m_flSpeed = 0.2f;
		beamInfo.m_nStartFrame = 0;
		beamInfo.m_flFrameRate = 0.f;
		beamInfo.m_flRed = current.color.RGBA[0];
		beamInfo.m_flGreen = current.color.RGBA[1];
		beamInfo.m_flBlue = current.color.RGBA[2];
		beamInfo.m_nSegments = 2;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = SDK::FBEAM_ONLYNOISEONCE | SDK::FBEAM_NOTILE | SDK::FBEAM_HALOBEAM;

		beamInfo.m_vecStart = current.pPlayer->GetVecOrigin() + current.pPlayer->GetViewOffset();
		beamInfo.m_vecEnd = current.vecImpactPos;

		auto beam = INTERFACES::ViewRenderBeams->CreateBeamPoints(beamInfo);
		if (beam)
			INTERFACES::ViewRenderBeams->DrawBeam(beam);

		//g_pVDebugOverlay->AddBoxOverlay( current.vecImpactPos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), Vector( 0, 0, 0 ), current.color.r(), current.color.g(), current.color.b(), 125, 0.8f );

		Impacts.erase(Impacts.begin() + i);
	}
}

void CVisuals::DrawCrosshair()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex());

	if (!weapon)
		return;

	//--- Screen Positioning ---//
	static int screen_x;
	static int screen_y;
	if (screen_x == 0)
	{
		INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
		screen_x /= 2; screen_y /= 2;
	}

	//--- Calculating Recoil ---//
	Vector view_angles;
	INTERFACES::Engine->GetViewAngles(view_angles);
	view_angles += local_player->GetPunchAngles() * 2.f;
	Vector factor_vec;
	MATH::AngleVectors(view_angles, &factor_vec);
	factor_vec = factor_vec * 10000.f;
	Vector start = local_player->GetVecOrigin() + local_player->GetViewOffset();
	Vector end = start + factor_vec, recoil_to_screen;

	//--- Calculating Spread (unused) ---//
	//auto cone = weapon->GetSpread() + weapon->GetInaccuracy();// *500.f;

	//if (!cone)
		//return;

	//RENDER::DrawFilledCircle(recoil_to_screen.x, recoil_to_screen.y, spread, 50, CColor(0, 0, 0, 100));

	//--- Rendering Crosshairs ---//
	switch (SETTINGS::settings.xhair_type % 3)
	{
	case 0:
		break;
	case 1:
		RENDER::DrawF(screen_x, screen_y, FONTS::visuals_xhair_font, true, true, WHITE, "+");
		break;
	case 2:
		if (RENDER::WorldToScreen(end, recoil_to_screen) && local_player->GetHealth() > 0)
			RENDER::DrawF(recoil_to_screen.x, recoil_to_screen.y, FONTS::visuals_xhair_font, true, true, WHITE, "+");
		break;
	}
}

void CVisuals::DrawFovArrows(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (entity->GetIsDormant())
		return;

	Vector screenPos;
	if (UTILS::IsOnScreen(aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_BODY), screenPos)) return;

	auto client_viewangles = Vector();
	auto screen_width = 0, screen_height = 0;

	INTERFACES::Engine->GetViewAngles(client_viewangles);
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	auto radius = 450.f;
	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	const auto screen_center = Vector(screen_width / 2.f, screen_height / 2.f, 0);
	const auto rot = DEG2RAD(client_viewangles.y - UTILS::CalcAngle(local_position, aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_BODY)).y - 90);

	std::vector<SDK::Vertex_t> vertices;

	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(2)) * (radius - 16), screen_center.y + sinf(rot + DEG2RAD(2)) * (radius - 16))));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(2)) * (radius - 16), screen_center.y + sinf(rot - DEG2RAD(2)) * (radius - 16))));

	RENDER::TexturedPolygon(3, vertices, SETTINGS::settings.fov_col); //255, 40, 230
}

void CVisuals::DrawIndicator()
{
	//--- LBY Indication Stuff ---//
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	if (!SETTINGS::settings.aa_bool)
		return;

	float breaking_lby_fraction = fabs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw())) / 180.f;
	float lby_delta = abs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw()));

	int screen_width, screen_height;
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	/*Vector pre_position;
	Vector origin_delta;
	if (local_player->GetVecOrigin() != pre_position)
	{
		origin_delta = local_player->GetVecOrigin() - pre_position;
		pre_position = local_player->GetVecOrigin();
	}

	const auto breaking_lc = origin_delta.Length() > 64;
	if (local_player->GetVelocity().Length2D() >= 300)
	{
		if (breaking_lc)
			RENDER::DrawF(10, screen_height - 122, FONTS::visuals_lby_font, false, false, CColor(15, 255, 5), "LC");
		else
			RENDER::DrawF(10, screen_height - 122, FONTS::visuals_lby_font, false, false, CColor(255, 0, 0), "LC");
		RENDER::DrawF(10, screen_height - 88, FONTS::visuals_lby_font, false, false, CColor((1.f - breaking_lby_fraction) * 255.f, breaking_lby_fraction * 255.f, 0), "LBY");
	}
	else
	{
		RENDER::DrawF(10, screen_height - 88, FONTS::visuals_lby_font, false, false, CColor((1.f - breaking_lby_fraction) * 255.f, breaking_lby_fraction * 255.f, 0), "LBY");
	}*/


	//wtf are you kidding me this is retarded
	//you have to have more checks because real_angle doesnt work
	//try running and holding v down
	//it will say lby is broken
	//RENDER::DrawF( 10, screen_height - 88, FONTS::visuals_lby_font, false, false, lby_delta > 35 ? CColor( 133, 188, 22 ) : CColor( 255, 0, 0 ), "LBY" );

	static CColor lby;

	if (local_player->GetVelocity().Length2D() > 0.1f && !fake_walk && local_player->GetFlags() & FL_ONGROUND) //fixes moving delay with fast moving angles, thanks red
	{
		lby = CColor(255, 0, 0);
	}
	else
	{
		if (lby_delta > 35)
		{
			lby = CColor(110, 255, 0);
		}
		else
		{
			lby = CColor(255, 0, 0);
		}
	}

	RENDER::DrawF(10, screen_height - 88, FONTS::visuals_lby_font, false, false, lby, "LBY");

	if (!SETTINGS::settings.angle_bool)
		return;

	if (!SETTINGS::settings.tp_angle_bool)
	{
		if (in_tp)
			return;
	}

	//--- Anti-Aim Arrows ---//
	static const auto real_color = CColor(0, 255, 0, 255);
	static const auto fake_color = CColor(255, 0, 0, 255);
	static const auto lby_color = CColor(0, 0, 255, 255);

	auto client_viewangles = Vector();
	INTERFACES::Engine->GetViewAngles(client_viewangles);

	constexpr auto radius = 80.f;

	const auto screen_center = Vector2D(screen_width / 2.f, screen_height / 2.f);
	const auto real_rot = DEG2RAD(client_viewangles.y - GLOBAL::real_angles.y - 90);
	const auto fake_rot = DEG2RAD(client_viewangles.y - GLOBAL::fake_angles.y - 90);
	const auto lby_rot = DEG2RAD(client_viewangles.y - local_player->GetLowerBodyYaw() - 90);

	auto draw_arrow = [&](float rot, CColor color) -> void
	{
		std::vector<SDK::Vertex_t> vertices;
		vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
		vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(10)) * (radius - 15.f), screen_center.y + sinf(rot + DEG2RAD(10)) * (radius - 15.f)))); //25
		vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(10)) * (radius - 15.f),screen_center.y + sinf(rot - DEG2RAD(10)) * (radius - 15.f)))); //25
		RENDER::TexturedPolygon(3, vertices, color);
	};

	draw_arrow(real_rot, real_color);
	draw_arrow(fake_rot, fake_color);
	draw_arrow(lby_rot, lby_color);

	/*if (SETTINGS::settings.aa_type == 2 || SETTINGS::settings.aa_type == 4)
	{
		if (in_tp)
			return;

		if (SETTINGS::settings.flip_bool)
		{
			RENDER::DrawF((screen_width / 2) + 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(10, 145, 190, 255), ">"); //green
			RENDER::DrawF((screen_width / 2) - 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(255, 255, 255, 255), "<");
		}
		else
		{
			RENDER::DrawF((screen_width / 2) - 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(10, 145, 190, 255), "<"); //green
			RENDER::DrawF((screen_width / 2) + 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(255, 255, 255, 255), ">");
		}
	}*/
}

void CVisuals::LogEvents()
{
	static bool convar_performed = false, convar_lastsetting;

	/*if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (convar_performed)
			convar_performed = false;
		return;
	}*/

	if (convar_lastsetting != SETTINGS::settings.info_bool)
	{
		convar_lastsetting = SETTINGS::settings.info_bool;
		convar_performed = false;
	}

	if (!convar_performed)
	{
		//--- Log Events ---//
		static auto developer = INTERFACES::cvar->FindVar("developer");
		developer->SetValue(1);
		static auto con_filter_text_out = INTERFACES::cvar->FindVar("con_filter_text_out");
		static auto con_filter_enable = INTERFACES::cvar->FindVar("con_filter_enable");
		static auto con_filter_text = INTERFACES::cvar->FindVar("con_filter_text");

		con_filter_text->SetValue("    ");
		con_filter_text_out->SetValue("");
		con_filter_enable->SetValue(2);
		convar_performed = true;
	}
}

void CVisuals::ModulateWorld() //credits to my nigga monarch
{
	static bool nightmode_performed = false, nightmode_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame()) 
	{
		if (nightmode_performed)
			nightmode_performed = false;
		return;
	}

	if (nightmode_performed != SETTINGS::settings.night_bool)
	{
		nightmode_lastsetting = SETTINGS::settings.night_bool;
		nightmode_performed = false;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player) 
	{
		if (nightmode_performed)
			nightmode_performed = false;
		return;
	}

	if (nightmode_lastsetting != SETTINGS::settings.night_bool)
	{
		nightmode_lastsetting = SETTINGS::settings.night_bool;
		nightmode_performed = false;
	}

	if (!nightmode_performed) 
	{
		static auto r_DrawSpecificStaticProp = INTERFACES::cvar->FindVar("r_DrawSpecificStaticProp");
		r_DrawSpecificStaticProp->nFlags &= ~FCVAR_CHEAT;
		r_DrawSpecificStaticProp->SetValue(1);

		static auto sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
		sv_skyname->nFlags &= ~FCVAR_CHEAT;

		static auto mat_postprocess_enable = INTERFACES::cvar->FindVar("mat_postprocess_enable");
		mat_postprocess_enable->SetValue(0);

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i)) 
		{
			SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World")) 
			{
				if (SETTINGS::settings.night_bool)
					pMaterial->ColorModulate(0.1, 0.1, 0.1);
				else
					pMaterial->ColorModulate(1, 1, 1);

				if (SETTINGS::settings.night_bool)
				{
					sv_skyname->SetValue("sky_csgo_night02");
					//pMaterial->SetMaterialVarFlag(SDK::MATERIAL_VAR_TRANSLUCENT, false);
					pMaterial->ColorModulate(0.05, 0.05, 0.05);
				}
				else
				{
					sv_skyname->SetValue("vertigoblue_hdr");
					pMaterial->ColorModulate(1.00, 1.00, 1.00);
				}
			}
			else if (strstr(pMaterial->GetTextureGroupName(), "StaticProp")) 
			{
				if (SETTINGS::settings.night_bool)
				{
					pMaterial->ColorModulate(0.30, 0.30, 0.30);
					pMaterial->AlphaModulate(0.7);
				}
				else
				{
					pMaterial->ColorModulate(1, 1, 1);
					pMaterial->AlphaModulate(1);
				}
			}
		}
		nightmode_performed = true;
	}
}

void CVisuals::DrawHitmarker()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	static int lineSize = 6;

	static float alpha = 0;
	float step = 255.f / 0.3f * INTERFACES::Globals->frametime;


	if ( GLOBAL::flHurtTime + 0.4f >= INTERFACES::Globals->curtime )
		alpha = 255.f;
	else
		alpha -= step;

	if ( alpha > 0 ) {
		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		INTERFACES::Engine->GetScreenSize( screenSizeX, screenSizeY );

		screenCenterX = screenSizeX / 2;
		screenCenterY = screenSizeY / 2;
		CColor col = CColor( 255, 255, 255, alpha );
		RENDER::DrawLine( screenCenterX - lineSize * 2, screenCenterY - lineSize * 2, screenCenterX - ( lineSize ), screenCenterY - ( lineSize ), col );
		RENDER::DrawLine( screenCenterX - lineSize * 2, screenCenterY + lineSize * 2, screenCenterX - ( lineSize ), screenCenterY + ( lineSize ), col );
		RENDER::DrawLine( screenCenterX + lineSize * 2, screenCenterY + lineSize * 2, screenCenterX + ( lineSize ), screenCenterY + ( lineSize ), col );
		RENDER::DrawLine( screenCenterX + lineSize * 2, screenCenterY - lineSize * 2, screenCenterX + ( lineSize ), screenCenterY - ( lineSize ), col );
	}
}

void CVisuals::DrawPenetration(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector point;
	INTERFACES::Engine->GetViewAngles(point);

	int screen_x;
	int screen_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	screen_x /= 2; screen_y /= 2;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	if (autowall->CalculateDamage(local_position, point, local_player, entity).damage > SETTINGS::settings.damage_val)
		RENDER::DrawFilledRect(screen_x - 1, screen_y - 1, screen_x, screen_y, GREEN);
	else
		RENDER::DrawFilledRect(screen_x - 1, screen_y - 1, screen_x, screen_y, RED);
}

void CVisuals::DrawAntiAimSides()
{
	int screen_x;
	int screen_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	screen_x /= 2; screen_y /= 2;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	if (!SETTINGS::settings.aa_type == 2 || !SETTINGS::settings.aa_type == 4)
		return;

	if (SETTINGS::settings.aa_side == 0)
		RENDER::DrawWF(screen_x - 25, screen_y, FONTS::visuals_side_font, CColor(0, 255, 0, 255), L"\u25c4");
	if (SETTINGS::settings.aa_side == 1)
		RENDER::DrawWF(screen_x + 25, screen_y, FONTS::visuals_side_font, CColor(0, 255, 0, 255), L"\u25ba");
}

void CVisuals::DrawBorderLines()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex());

	if (!weapon)
		return;

	//--- Screen Positioning ---//
	int screen_x;
	int screen_y;
	int center_x;
	int center_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	INTERFACES::Engine->GetScreenSize(center_x, center_y);
	center_x /= 2; center_y /= 2;

	//--- Rendering Scope Lines ---//
	if (local_player->GetIsScoped())
	{
		RENDER::DrawLine(0, center_y, screen_x, center_y, CColor(0, 0, 0, 255));
		RENDER::DrawLine(center_x, 0, center_x, screen_y, CColor(0, 0, 0, 255));
	}
}

void CVisuals::DrawBarrel(SDK::CBaseEntity* entity)
{
	//uh fuck idk yet
}

void CVisuals::DrawBomb(SDK::CBaseEntity* entity)
{
	//:thinking:
}

CVisuals* visuals = new CVisuals();