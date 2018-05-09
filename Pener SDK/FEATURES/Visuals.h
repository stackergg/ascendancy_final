#pragma once

namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
}

class CVisuals
{
public:
	void Draw();
	void ClientDraw();
	void apply_clantag();
	void DrawInaccuracy();
	void DrawBulletBeams();
	void ModulateWorld();
	void set_hitmarker_time( float time );
	void LogEvents();
	void LagCompHitbox(SDK::CBaseEntity * entity);
private:
	void DrawBox(SDK::CBaseEntity* entity, CColor color);
	void DrawCorners(SDK::CBaseEntity * entity, CColor color);
	void DrawName(SDK::CBaseEntity* entity, CColor color, int index);
	void DrawWeapon(SDK::CBaseEntity * entity, CColor color, int index);
	void DrawHealth(SDK::CBaseEntity * entity, CColor color, CColor dormant);
	void BombPlanted(SDK::CBaseEntity * entity);
	void DrawDropped(SDK::CBaseEntity * entity);
	void DrawAmmo(SDK::CBaseEntity * entity, CColor color, CColor dormant);
	float resolve_distance(Vector src, Vector dest);
	void DrawDistance(SDK::CBaseEntity * entity, CColor color);
	void DrawInfo(SDK::CBaseEntity * entity, CColor color, CColor alt);
	void RenderHitbox(SDK::CBaseEntity * entity);
	void HitboxESP(matrix3x4_t * matrix, Vector bbmin, Vector bbmax, int bone, CColor color);
	void DrawSkeleton( SDK::CBaseEntity * entity, CColor color, VMatrix bone_matrix [128] );
	void DrawBomb(SDK::CBaseEntity* entity);
	void DrawFovArrows(SDK::CBaseEntity* entity);
	void DrawCrosshair();
	void DrawIndicator();
	void DrawHitmarker();
	void DrawPenetration(SDK::CBaseEntity * entity);
	void DrawAntiAimSides();
	void DrawBorderLines();
	void DrawBarrel(SDK::CBaseEntity * entity);
public:
	std::vector<std::pair<int, float>>				Entities;
	std::deque<UTILS::BulletImpact_t>				Impacts;
};

extern CVisuals* visuals;