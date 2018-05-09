#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/ConVar.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/Backtracking.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Extrapolation.h"

//--- Variable Initaliztion ---//
int bestHitbox = -1, mostDamage;
Vector multipoints[128];
int multipointCount = 0;
bool lag_comp;
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

bool CAimbot::spread_limit(SDK::CBaseWeapon* weapon)
{
	if (!weapon) return false;
	float inaccuracy = weapon->GetInaccuracy();
	return inaccuracy < (float)15 / 1000.f;
}

void CAimbot::rotate_movement(float yaw, SDK::CUserCmd* cmd)
{
	Vector viewangles;
	INTERFACES::Engine->GetViewAngles(viewangles);

	float rotation = DEG2RAD(viewangles.y - yaw);

	float cos_rot = cos(rotation);
	float sin_rot = sin(rotation);

	float new_forwardmove = (cos_rot * cmd->forwardmove) - (sin_rot * cmd->sidemove);
	float new_sidemove = (sin_rot * cmd->forwardmove) + (cos_rot * cmd->sidemove);

	cmd->forwardmove = new_forwardmove;
	cmd->sidemove = new_sidemove;
}

float CAimbot::seedchance(Vector Point)//what do you mean
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return 0;

	float SpreadCone = weapon->GetInaccuracy() * 256.0f / M_PI + weapon->get_full_info()->flInaccuracyMove * local_player->GetVelocity().Length() / 3000.0f;
	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float a = (Point - local_position).Length();
	float b = sqrt(tan(SpreadCone * M_PI / 180.0f) * a);
	if (2.2f > b) return 100.0f;
	return (2.2f / fmax(b, 2.2f)) * 100.0f;
}

int lerped_ticks()
{
	static const auto cl_interp_ratio = INTERFACES::cvar->FindVar("cl_interp_ratio");
	static const auto cl_updaterate = INTERFACES::cvar->FindVar("cl_updaterate");
	static const auto cl_interp = INTERFACES::cvar->FindVar("cl_interp");

	return TIME_TO_TICKS(max(cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat()));
}

static SDK::ConVar *big_ud_rate = nullptr;
static SDK::ConVar *min_ud_rate = nullptr;
static SDK::ConVar *max_ud_rate = nullptr;
static SDK::ConVar *interp_ratio = nullptr;
static SDK::ConVar *cl_interp = nullptr;
static SDK::ConVar *cl_min_interp = nullptr;
static SDK::ConVar *cl_max_interp = nullptr;

float LerpTime()
{
	static SDK::ConVar* updaterate = INTERFACES::cvar->FindVar("cl_updaterate");
	static SDK::ConVar* minupdate = INTERFACES::cvar->FindVar("sv_minupdaterate");
	static SDK::ConVar* maxupdate = INTERFACES::cvar->FindVar("sv_maxupdaterate");
	static SDK::ConVar* lerp = INTERFACES::cvar->FindVar("cl_interp");
	static SDK::ConVar* cmin = INTERFACES::cvar->FindVar("sv_client_min_interp_ratio");
	static SDK::ConVar* cmax = INTERFACES::cvar->FindVar("sv_client_max_interp_ratio");
	static SDK::ConVar* ratio = INTERFACES::cvar->FindVar("cl_interp_ratio");

	float lerpurmom = lerp->GetFloat();
	float maxupdateurmom = maxupdate->GetFloat();
	int updaterateurmom = updaterate->GetInt();
	float ratiourmom = ratio->GetFloat();
	int sv_maxupdaterate = maxupdate->GetInt();
	int sv_minupdaterate = minupdate->GetInt();
	float cminurmom = cmin->GetFloat();
	float cmaxurmom = cmax->GetFloat();

	if (sv_maxupdaterate && sv_minupdaterate)
		updaterateurmom = maxupdateurmom;

	if (ratiourmom == 0)
		ratiourmom = 1.0f;

	if (cmin && cmax && cmin->GetFloat() != 1)
		ratiourmom = clamp(ratiourmom, cminurmom, cmaxurmom);

	return max(lerpurmom, ratiourmom / updaterateurmom);
}

bool CAimbot::good_backtrack_tick(int tick)
{
	auto nci = INTERFACES::Engine->GetNetChannelInfo();

	if (!nci) {
		return false;
	}

	//float unlag = INTERFACES::cvar->FindVar("sv_maxunlag")->GetFloat();

	float correct = clamp(nci->GetLatency(FLOW_OUTGOING) + LerpTime(), 0.f, 1.f /*sv_maxunlag*/);

	float delta_time = correct - (INTERFACES::Globals->curtime - TICKS_TO_TIME(tick));

	return fabsf(delta_time) < 0.2f;
}

void CAimbot::run_aimbot(SDK::CUserCmd* cmd) 
{
	Entities.clear();

	SelectTarget();
	shoot_enemy(cmd);
}

void CAimbot::SelectTarget() 
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	for (int index = 1; index <= INTERFACES::Globals->maxclients; index++) 
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(index);

		if (!entity)
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

		auto class_id = entity->GetClientClass()->m_ClassID;

		if (class_id != 35)
			continue;

		if (entity->GetVecOrigin() == Vector(0, 0, 0))
			continue;

		if (entity->GetImmunity())
			continue;

		if (entity->GetIsDormant())
			continue;

		AimbotData_t data = AimbotData_t(entity, index);

		Entities.push_back(data);
	}
}

void CAimbot::allow_zeus(SDK::CUserCmd* cmd) //needs to be fixed
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	for (int y = 0; y <= 360; y += 360.f / 6.f) 
	{
		for (int x = -89; x <= 89; x += 179.f / 6.f) 
		{
			Vector ang = Vector(x, y, 0);
			Vector dir;
			MATH::angle_vectors_cus(ang, dir);

			SDK::trace_t trace;
			autowall->UTIL_TraceLine(local_position, local_position + (dir * 64), MASK_SHOT, local_player, 0, &trace);

			if (trace.m_pEnt == nullptr)
				continue;

			if (trace.m_pEnt == local_player)
				continue;

			if (!trace.m_pEnt->IsAlive())
				continue;

			if (trace.m_pEnt->GetHealth() <= 0)
				continue;

			if (trace.m_pEnt->GetTeam() == local_player->GetTeam())
				continue;

			if (trace.m_pEnt->GetIsDormant())
				continue;

			if (trace.m_pEnt->GetImmunity())
				continue;

			SDK::player_info_t info;
			if (!(INTERFACES::Engine->GetPlayerInfo(trace.m_pEnt->GetIndex(), &info)))
				continue;

			cmd->viewangles = Vector(x, y, 0);
			cmd->buttons |= IN_ATTACK;
			return;
		}
	}
}

void CAimbot::lby_backtrack(SDK::CUserCmd *pCmd, SDK::CBaseEntity* pLocal, SDK::CBaseEntity* pEntity)
{
	int index = pEntity->GetIndex();
	float PlayerVel = abs(pEntity->GetVelocity().Length2D());

	bool playermoving;

	if (PlayerVel > 0.f)
		playermoving = true;
	else
		playermoving = false;

	float lby = pEntity->GetLowerBodyYaw();
	static float lby_timer[65];
	static float lby_proxy[65];

	if (lby_proxy[index] != pEntity->GetLowerBodyYaw() && playermoving == false)
	{
		lby_timer[index] = 0;
		lby_proxy[index] = pEntity->GetLowerBodyYaw();
	}

	if (playermoving == false)
	{
		if (pEntity->GetSimTime() >= lby_timer[index])
		{
			tick_to_back[index] = pEntity->GetSimTime();
			lby_to_back[index] = pEntity->GetLowerBodyYaw();
			lby_timer[index] = pEntity->GetSimTime() + INTERFACES::Globals->interval_per_tick + 1.1;
		}
	}
	else
	{
		tick_to_back[index] = 0;
		lby_timer[index] = 0;
	}

	if (good_backtrack_tick(TIME_TO_TICKS(tick_to_back[index])))
		backtrack_tick[index] = true;
	else
		backtrack_tick[index] = false;
}

void CAimbot::auto_revolver(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return;

	if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_REVOLVER)
	{
		cmd->buttons |= IN_ATTACK;

		float flPostponeFireReady = weapon->GetPostponeFireReadyTime();
		if (flPostponeFireReady > 0 && flPostponeFireReady < INTERFACES::Globals->curtime)
		{
			cmd->buttons &= ~IN_ATTACK;
		}
	}
}

void CAimbot::shoot_enemy(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetFlags() & FL_ATCONTROLS)
		return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return;

	if (!can_shoot(cmd))
	{
		cmd->buttons &= ~IN_ATTACK;	
		return;
	}

	if (weapon->GetLoadedAmmo() == 0)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	Vector aim_angles;

	for (auto players : Entities)
	{
		auto entity = players.pPlayer;

		if (!entity)
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

		auto class_id = entity->GetClientClass()->m_ClassID;

		if (class_id != 35)
			continue;

		if (entity->GetVecOrigin() == Vector(0, 0, 0))
			continue;

		if (entity->GetImmunity())
			continue;

		if (entity->GetIsDormant())
			continue;

		Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();
		Vector shoot_here;
		
		if (SETTINGS::settings.multi_bool) //selection for aim point
		{
			switch (SETTINGS::settings.acc_type % 3)
			{
			case 0:
				shoot_here = head_multipoint(entity);
				break;
			case 1:
				shoot_here = body_multipoint(entity);
				break;
			case 2:
				shoot_here = full_multipoint(entity);
				break;
			}
		}
		else
		{
			switch (SETTINGS::settings.acc_type % 3)
			{
			case 0:
				shoot_here = head_point(entity);
				break;
			case 1:
				shoot_here = body_point(entity);
				break;
			case 2:
				shoot_here = full_point(entity);
				break;
			}
		}

		if (shoot_here == Vector(0, 0, 0)) //better fucking hope not
			continue;

		aim_angles = MATH::NormalizeAngle(UTILS::CalcAngle(local_position, shoot_here));
		//cmd->viewangles = MATH::NormalizeAngle(UTILS::CalcAngle(local_position, shoot_here)); //silent aim
		//INTERFACES::Engine->SetViewAngles(MATH::NormalizeAngle(UTILS::CalcAngle(local_position, shoot_here))); //lock aim

		Vector vec_position[65];
		Vector origin_delta[65];
		if (entity->GetVecOrigin() != vec_position[entity->GetIndex()]) //calculate if breaking lag comp
		{
			origin_delta[entity->GetIndex()] = entity->GetVecOrigin() - vec_position[entity->GetIndex()];
			vec_position[entity->GetIndex()] = entity->GetVecOrigin();

			lag_comp = fabs(origin_delta[entity->GetIndex()].Length()) > 64;
		}

		if (lag_comp && entity->GetVelocity().Length2D() > 300 && SETTINGS::settings.delay_shot == 1) //don't shoot if breaking lag comp (for testing)
			return;

		if (SETTINGS::settings.fakefix_bool)
			extrapolation->run_extrapolation(entity);

		if (weapon->GetItemDefenitionIndex() == SDK::WEAPON_TASER) //if we have a taser men!1!!1
		{
			if (can_shoot(cmd))
			{
				int bone = zeus_hitbox(entity); //you can change this but keep in mind this has range stuff. it only has pelvis as a bone but why do other stuff really it will make it inaccurate shooting at arms and legs if they arent resolved right

				if (bone != 1)
				{
					Vector fucknigga = get_hitbox_pos(entity, bone);
					Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

					if (fucknigga != Vector(0, 0, 0))
					{
						SDK::trace_t trace;

						autowall->UTIL_TraceLine(local_position, fucknigga, MASK_SOLID, local_player, 0, &trace);

						SDK::player_info_t info;

						if (!(INTERFACES::Engine->GetPlayerInfo(trace.m_pEnt->GetIndex(), &info)))
							continue;

						if (fucknigga != Vector(0, 0, 0))
						{
							cmd->viewangles = MATH::NormalizeAngle(UTILS::CalcAngle(local_position, fucknigga));
							GLOBAL::should_send_packet = true;
							cmd->buttons |= IN_ATTACK;
						}
					}
				}
			}
			continue;
		}

		if (weapon->get_full_info()->WeaponType == 9 || weapon->get_full_info()->WeaponType == 0) //idk why i still do this check, checking itemdefenitionindex would work too
			continue;

		if (accepted_inaccuracy(weapon) < SETTINGS::settings.chance_val) //done as late as possible to reduce stress on aimbot having to re-lock once this returns true
			continue;

		if (good_backtrack_tick(TIME_TO_TICKS(entity->GetSimTime() + LerpTime())))
			cmd->tick_count = TIME_TO_TICKS(entity->GetSimTime() + LerpTime()); //lolololol CS:S meme

		cmd->buttons |= IN_ATTACK;

		target = entity->GetIndex();
		break;
	}

	if (cmd->buttons & IN_ATTACK) 
	{
		GLOBAL::should_send_packet = true;
		cmd->viewangles = aim_angles;
	}
}

/*bool CAimbot::ValidTick(LagRecord* targetRecord, LagRecord* prevRecord)
{
	if (targetRecord == nullptr)
		return false;

	int tick = TIME_TO_TICKS(targetRecord->m_flSimulationTime);

	SDK::INetChannelInfo* nci = INTERFACES::Engine->GetNetChannelInfo();
	float correct = 0.0f;

	correct += nci->GetLatency(FLOW_OUTGOING);

	float m_fLerpTime = LerpTime();
	int lerpTicks = TIME_TO_TICKS(m_fLerpTime);

	correct += TICKS_TO_TIME(lerpTicks);

	int targettick = tick - lerpTicks;
	int predictTickcount = INTERFACES::Globals->tickcount + 1 + TIME_TO_TICKS(nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING));
	float deltaTime = correct - TICKS_TO_TIME(predictTickcount - targettick);

	if (fabs(deltaTime) > 0.2f)
		// too much difference, can't backtrack here
		return false;
	else if (prevRecord == nullptr)
		return true;

	return (prevRecord->m_vecOrigin - targetRecord->m_vecOrigin).LengthSqr() < 4096;
}*/

struct AimbotTargetIter {
	SDK::CBaseEntity* target;
	int hp;
	float fov;
	float distance;
	float threat;
	int lagticks;
	int predictTicks;
	size_t id;

	AimbotTargetIter() : target(nullptr), hp(1000), fov(10000.0f), distance(1000000.0f), threat(1.0f), id(0), lagticks(1), predictTicks(0) {}

	AimbotTargetIter(SDK::CBaseEntity* pl, int health, float FOV, float dist, float b1gThreat, size_t plID, int lTicks, int pTicks)
	{
		target = pl;
		hp = health;
		fov = FOV;
		distance = dist;
		threat = b1gThreat;
		id = plID;
		lagticks = lTicks;
		predictTicks = pTicks;
	}
};

float CAimbot::accepted_inaccuracy(SDK::CBaseWeapon* weapon) //ayyyyyywareeee
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	if (!weapon)
		return 0;

	if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_TASER)
		return 0;

	float hitchance = 101; //lol idk why, its pasted anyway so w/e
	float inaccuracy = weapon->GetInaccuracy();

	if (inaccuracy == 0) 
		inaccuracy = 0.0000001;

	inaccuracy = 1 / inaccuracy;
	hitchance = inaccuracy;

	return hitchance;
}

bool CAimbot::meets_requirements(SDK::CBaseEntity* entity)
{
	//is the aimbot targeted on this enemy
	return true;
}

int CAimbot::select_target()
{
	//finds a target based on distance
	return 0;
}

std::vector<Vector> CAimbot::GetMultiplePointsForHitbox(SDK::CBaseEntity* local, SDK::CBaseEntity* entity, int iHitbox, VMatrix BoneMatrix[128])
{
	auto VectorTransform_Wrapper = [](const Vector& in1, const VMatrix &in2, Vector &out)
	{
		auto VectorTransform = [](const float *in1, const VMatrix& in2, float *out)
		{
			auto DotProducts = [](const float *v1, const float *v2)
			{
				return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
			};
			out[0] = DotProducts(in1, in2[0]) + in2[0][3];
			out[1] = DotProducts(in1, in2[1]) + in2[1][3];
			out[2] = DotProducts(in1, in2[2]) + in2[2][3];
		};
		VectorTransform(&in1.x, in2, &out.x);
	};

	SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());
	SDK::mstudiohitboxset_t* set = pStudioModel->pHitboxSet(0);
	SDK::mstudiobbox_t *hitbox = set->GetHitbox(iHitbox);

	std::vector<Vector> vecArray;

	Vector max;
	Vector min;
	VectorTransform_Wrapper(hitbox->bbmax, BoneMatrix[hitbox->bone], max);
	VectorTransform_Wrapper(hitbox->bbmin, BoneMatrix[hitbox->bone], min);

	auto center = (min + max) * 0.5f;

	Vector CurrentAngles = UTILS::CalcAngle(center, local->GetEyePosition());

	Vector Forward;
	MATH::AngleVectors(CurrentAngles, &Forward);

	Vector Right = Forward.Cross(Vector(0, 0, 1));
	Vector Left = Vector(-Right.x, -Right.y, Right.z);

	Vector Top = Vector(0, 0, 1);
	Vector Bot = Vector(0, 0, -1);

	switch (iHitbox) {
	case 0:
		for (auto i = 0; i < 4; ++i)
		{
			vecArray.emplace_back(center);
		}
		vecArray[1] += Top * (hitbox->radius * SETTINGS::settings.point_val);
		vecArray[2] += Right * (hitbox->radius * SETTINGS::settings.point_val);
		vecArray[3] += Left * (hitbox->radius * SETTINGS::settings.point_val);
		break;

	default:

		for (auto i = 0; i < 3; ++i)
		{
			vecArray.emplace_back(center);
		}
		vecArray[1] += Right * (hitbox->radius * SETTINGS::settings.body_val);
		vecArray[2] += Left * (hitbox->radius * SETTINGS::settings.body_val);
		break;
	}
	return vecArray;
}

int CAimbot::zeus_hitbox(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float closest = 180.f;

	bestHitbox = -1;

	Vector point = get_hitbox_pos(entity, SDK::HitboxList::HITBOX_PELVIS);

	if (point != Vector(0, 0, 0))
	{
		float distance = fabs((point - local_position).Length());

		if (distance <= closest)
		{
			bestHitbox = SDK::HitboxList::HITBOX_PELVIS;
			closest = distance;
		}
	}

	return bestHitbox;
}

Vector CAimbot::head_multipoint(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return Vector(0,0,0);

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector vector_best_point = Vector(0, 0, 0);

	mostDamage = SETTINGS::settings.damage_val;

	VMatrix matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	int hitboxes[] =
	{
		SDK::HitboxList::HITBOX_HEAD
	};

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
		{
			SDK::trace_t trace;
			int damage = autowall->CalculateDamage(local_position, point, local_player, entity).damage;

			if (damage > mostDamage)
			{
				bestHitbox = hitboxes[i];
				mostDamage = damage;
				vector_best_point = point;

				if (mostDamage >= entity->GetHealth())
					return vector_best_point;
			}

		}
	}
	return vector_best_point;
}

Vector CAimbot::body_multipoint(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector vector_best_point = Vector(0, 0, 0);

	mostDamage = SETTINGS::settings.damage_val;

	VMatrix matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	int hitboxes[] =
	{
		SDK::HitboxList::HITBOX_BODY,
		SDK::HitboxList::HITBOX_PELVIS,
		SDK::HitboxList::HITBOX_UPPER_CHEST,
		SDK::HitboxList::HITBOX_THORAX,
		SDK::HitboxList::HITBOX_MAX
	};

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
		{
			SDK::trace_t trace;
			int damage = autowall->CalculateDamage(local_position, point, local_player, entity).damage;

			if (damage > mostDamage)
			{
				bestHitbox = hitboxes[i];
				mostDamage = damage;
				vector_best_point = point;

				if (mostDamage >= entity->GetHealth())
					return vector_best_point;
			}

		}
	}
	return vector_best_point;
}

/*
HITBOX_HEAD;
HITBOX_LEFT_UPPER_ARM;
HITBOX_LEFT_FOREARM;
HITBOX_RIGHT_UPPER_ARM;
HITBOX_RIGHT_FOREARM;
HITBOX_UPPER_CHEST;
HITBOX_CHEST;
HITBOX_PELVIS;
HITBOX_BODY;
HITBOX_LEFT_CALF;
HITBOX_RIGHT_CALF;
HITBOX_LEFT_FOOT;
HITBOX_RIGHT_FOOT;*/

Vector CAimbot::full_multipoint(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector vector_best_point = Vector(0, 0, 0);

	mostDamage = SETTINGS::settings.damage_val;

	VMatrix matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	int hitboxes[] =
	{
		SDK::HitboxList::HITBOX_HEAD,
		SDK::HitboxList::HITBOX_BODY,
		SDK::HitboxList::HITBOX_PELVIS,
		SDK::HitboxList::HITBOX_LEFT_UPPER_ARM,
		SDK::HitboxList::HITBOX_LEFT_FOREARM,
		SDK::HitboxList::HITBOX_RIGHT_UPPER_ARM,
		SDK::HitboxList::HITBOX_RIGHT_FOREARM,
		SDK::HitboxList::HITBOX_UPPER_CHEST,
		SDK::HitboxList::HITBOX_CHEST,
		SDK::HitboxList::HITBOX_LEFT_CALF,
		SDK::HitboxList::HITBOX_RIGHT_CALF,
		SDK::HitboxList::HITBOX_LEFT_FOOT,
		SDK::HitboxList::HITBOX_RIGHT_FOOT
	};

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
		{
			SDK::trace_t trace;
			int damage = autowall->CalculateDamage(local_position, point, local_player, entity).damage;

			if (damage > mostDamage)
			{
				bestHitbox = hitboxes[i];
				mostDamage = damage;
				vector_best_point = point;

				if (mostDamage >= entity->GetHealth())
					return vector_best_point;
			}

		}
	}
	return vector_best_point;
}

Vector CAimbot::head_point(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector vector_best_point = Vector(0, 0, 0);

	int hitboxes[] =
	{
		SDK::HitboxList::HITBOX_HEAD
	};

	mostDamage = SETTINGS::settings.damage_val;

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		int damage = autowall->CalculateDamage(local_position, entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;

		if (damage > mostDamage)
		{
			bestHitbox = hitboxes[i];
			mostDamage = damage;
			vector_best_point = get_hitbox_pos(entity, bestHitbox);

			if (mostDamage >= entity->GetHealth())
				return vector_best_point;
		}
	}

	return vector_best_point;
}

Vector CAimbot::body_point(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector vector_best_point = Vector(0, 0, 0);

	int hitboxes[] =
	{
		SDK::HitboxList::HITBOX_BODY,
		SDK::HitboxList::HITBOX_PELVIS,
		SDK::HitboxList::HITBOX_UPPER_CHEST,
		SDK::HitboxList::HITBOX_THORAX,
		SDK::HitboxList::HITBOX_MAX
	};

	mostDamage = SETTINGS::settings.damage_val;

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		int damage = autowall->CalculateDamage(local_position, entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;

		if (damage > mostDamage)
		{
			bestHitbox = hitboxes[i];
			mostDamage = damage;
			vector_best_point = get_hitbox_pos(entity, bestHitbox);

			if (mostDamage >= entity->GetHealth())
				return vector_best_point;
		}
	}

	return vector_best_point;
}

Vector CAimbot::full_point(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	Vector vector_best_point = Vector(0, 0, 0);

	int hitboxes[] =
	{
		SDK::HitboxList::HITBOX_HEAD,
		SDK::HitboxList::HITBOX_BODY,
		SDK::HitboxList::HITBOX_PELVIS,
		SDK::HitboxList::HITBOX_UPPER_CHEST,
		SDK::HitboxList::HITBOX_CHEST,
		SDK::HitboxList::HITBOX_LEFT_FOOT,
		SDK::HitboxList::HITBOX_RIGHT_FOOT
	};

	mostDamage = SETTINGS::settings.damage_val;

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		int damage = autowall->CalculateDamage(local_position, entity->GetBonePosition(hitboxes[i]), local_player, entity).damage;

		if (damage > mostDamage)
		{
			bestHitbox = hitboxes[i];
			mostDamage = damage;
			vector_best_point = get_hitbox_pos(entity, bestHitbox);

			if (mostDamage >= entity->GetHealth())
				return vector_best_point;
		}
	}

	return vector_best_point;
}

bool CAimbot::can_shoot(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player || local_player->GetHealth() <= 0)
		return false;

	if (local_player->GetFlags() & FL_ATCONTROLS)
		return false;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return false;

	if (weapon->GetLoadedAmmo() == 0)
	{
		return false;
	}

	if (weapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_REVOLVER)
	{
		if (fabs(weapon->GetPostponeFireReadyTime() - UTILS::GetCurtime()) <= 0.05f)
		{
			if (cmd->buttons & IN_ATTACK)
				return true;
		}
	}

	return (weapon->GetNextPrimaryAttack() < UTILS::GetCurtime()) && (local_player->GetNextAttack() < UTILS::GetCurtime());
}

SDK::mstudiobbox_t* CAimbot::get_hitbox(SDK::CBaseEntity* entity, int hitbox_index)
{
	if (entity->GetIsDormant() || entity->GetHealth() <= 0)
		return NULL;

	const auto pModel = entity->GetModel();
	if (!pModel)
		return NULL;

	auto pStudioHdr = INTERFACES::ModelInfo->GetStudioModel(pModel);
	if (!pStudioHdr)
		return NULL;

	auto pSet = pStudioHdr->pHitboxSet(0);
	if (!pSet)
		return NULL;

	if (hitbox_index >= pSet->numhitboxes || hitbox_index < 0)
		return NULL;

	return pSet->GetHitbox(hitbox_index);
}

Vector CAimbot::get_hitbox_pos(SDK::CBaseEntity* entity, int hitbox_id)
{
	auto hitbox = get_hitbox(entity, hitbox_id);
	if (!hitbox)
		return Vector(0, 0, 0);

	auto bone_matrix = entity->GetBoneMatrix(hitbox->bone);

	Vector bbmin, bbmax;
	MATH::VectorTransform(hitbox->bbmin, bone_matrix, bbmin);
	MATH::VectorTransform(hitbox->bbmax, bone_matrix, bbmax);

	return (bbmin + bbmax) * 0.5f;
}

void CAimbot::fix_recoil(SDK::CUserCmd* cmd)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	float recoil_scale = INTERFACES::cvar->FindVar("weapon_recoil_scale")->GetFloat(); //huhuuhhuuh
	if (cmd->buttons & IN_ATTACK)
	{
		if (recoil_scale)
		{
			Vector PunchAngles = local_player->GetPunchAngles();
			Vector AimAngles = cmd->viewangles;
			AimAngles -= PunchAngles * recoil_scale;
			cmd->viewangles = AimAngles;
		}
	}
	//cmd->viewangles -= local_player->GetPunchAngles() * 2.f;
}

int CAimbot::get_damage(Vector position) //not needed, thanks autowall!
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));

	if (!weapon)
		return 0;

	SDK::trace_t trace;
	SDK::Ray_t ray;
	SDK::CTraceWorldOnly filter;
	ray.Init(local_player->GetVecOrigin() + local_player->GetViewOffset(), position);

	INTERFACES::Trace->TraceRay(ray, MASK_ALL, (SDK::ITraceFilter*)&filter, &trace);

	if (trace.flFraction == 1.f)
	{
		auto weapon_info = weapon->get_full_info();
		if (!weapon_info)
			return -1;

		return weapon_info->iDamage;
		return 1;
	}
	else
		return 0;
}

CAimbot* aimbot = new CAimbot();