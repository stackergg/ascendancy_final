#pragma once

namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
	class CBaseWeapon;
	struct mstudiobbox_t;
}

struct AimbotData_t
{
	AimbotData_t(SDK::CBaseEntity* player, const int& idx)
	{
		this->pPlayer = player;
		this->index = idx;
	}
	SDK::CBaseEntity*	pPlayer;
	int				index;
};

class CAimbot
{
public:
	void shoot_enemy(SDK::CUserCmd* cmd);
	bool good_backtrack_tick(int tick);
	void run_aimbot(SDK::CUserCmd * cmd);
	void SelectTarget();
	void allow_zeus(SDK::CUserCmd * cmd);
	void lby_backtrack(SDK::CUserCmd * pCmd, SDK::CBaseEntity * pLocal, SDK::CBaseEntity * pEntity);
	void auto_revolver(SDK::CUserCmd * cmd);
	float accepted_inaccuracy(SDK::CBaseWeapon * weapon);
	bool can_shoot(SDK::CUserCmd * cmd);
	void fix_recoil(SDK::CUserCmd* cmd);
	void rotate_movement(float yaw, SDK::CUserCmd * cmd);
	Vector get_hitbox_pos(SDK::CBaseEntity* entity, int hitbox_id);
	SDK::mstudiobbox_t* get_hitbox(SDK::CBaseEntity* entity, int hitbox_index);
	Vector head_multipoint(SDK::CBaseEntity * entity);
	Vector body_multipoint(SDK::CBaseEntity * entity);
	Vector full_multipoint(SDK::CBaseEntity * entity);
	Vector head_point(SDK::CBaseEntity * entity);
	Vector body_point(SDK::CBaseEntity * entity);
	Vector full_point(SDK::CBaseEntity * entity);

	std::vector<AimbotData_t>	Entities;
private:
	int select_target();
	std::vector<Vector> GetMultiplePointsForHitbox(SDK::CBaseEntity * local, SDK::CBaseEntity * entity, int iHitbox, VMatrix BoneMatrix[128]);
	int zeus_hitbox(SDK::CBaseEntity * entity); //int
	bool meets_requirements(SDK::CBaseEntity* entity);
	int get_damage(Vector position);
	bool spread_limit(SDK::CBaseWeapon* weapon);
	float seedchance(Vector Point);
};

extern CAimbot* aimbot;