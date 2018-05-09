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
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Backtracking.h"

//run in FRAME_NET_UPDATE_POSTDATAUPDATE_START
void CBacktrack::log_players( ) {
	for ( int index = 1; index <= INTERFACES::Globals->maxclients; index++ ) {
		auto* player = INTERFACES::ClientEntityList->GetClientEntity( index );

		if ( player )
			players [index].player = player;
		else
			players [index].player = nullptr;

		players [index].index = index;
		auto &track = players [index].storedTicks;

		if ( !valid( player ) ) {
			if ( !track.empty( ) ) {
				track.clear( );
			}
			continue;
		}

		if ( player->GetIsDormant( ) )
			continue;

		//if the track size is too ((large))
		while ( track.size( ) > 128 )
			track.pop_back( );

		// check if head has same simulation time
		if ( !track.empty( ) ) {
			auto &head = *track.begin( );
			// check if player changed simulation time since last time updated
			if ( head.lag_record.m_flSimulationTime == player->GetSimTime( ) && head.lag_record.m_vecOrigin == player->GetVecOrigin( ) )
				continue;
		}

		// check if lagcomp possible
		
		static auto sv_unlag = INTERFACES::cvar->FindVar( "sv_unlag" );
		if ( !sv_unlag->GetBool( ) )
			return;

		// add new record to player track
		track.push_front( resolver_info_t( ) );
		auto &record = *track.begin( );

		// resolve func here

		// add priority flags
		if ( player->GetFlags( ) & FL_ONGROUND )
			record.flags_i |= RFL_ONGROUND; //flags so we can filter shit inside the aimbot
		if ( player->GetVelocity( ).Length2D( ) > 0.1f )
			record.flags_i |= RFL_MOVING; //flags so we can filter shit inside the aimbot

		record.tick_count = player->GetTickBase( );

		player->SetAbsAngles( Vector( player->GetEyeAngles( ) ) );
		player->SetAbsOrigin( player->GetVecOrigin( ) ); //set uninterpolated position

		// add lagcomp data
		record.lag_record.m_flLowerBodyYawTarget = player->GetLowerBodyYaw( );
		record.lag_record.m_flSimulationTime = player->GetSimTime( );
		record.lag_record.m_vecAbsAngles = player->GetAbsAngles( );
		record.lag_record.m_vecEyeAngles = player->GetEyeAngles( );
		record.lag_record.m_vecOrigin = player->GetVecOrigin( );
		record.lag_record.m_fFlags = player->GetFlags( );
		record.lag_record.m_flPoseParameter = player->GetPoseParamaters( );

		for ( int animlayer = 0; animlayer < 13; animlayer++ ) {
			record.lag_record.m_layerRecords [animlayer].m_flCycle = player->GetAnimOverlay( animlayer ).m_flCycle;
			record.lag_record.m_layerRecords [animlayer].m_nOrder = player->GetAnimOverlay( animlayer ).m_nOrder;
			record.lag_record.m_layerRecords [animlayer].m_nSequence = player->GetAnimOverlay( animlayer ).m_nSequence;
			record.lag_record.m_layerRecords [animlayer].m_flWeight = player->GetAnimOverlay( animlayer ).m_flWeight;
		}

		//invalidate_bonecache( player );

		// save bonematrix
		if ( !player->SetupBones( record.lag_record.m_bone_matrix, 128, 0x100, 0.f ) )
			return;

	}
}

void CBacktrack::anim_fix(ClientFrameStage_t stage)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	static int userId[64];
	static SDK::CAnimationLayer backupLayersUpdate[64][15];
	static SDK::CAnimationLayer backupLayersInterp[64][15];

	for (int i = 1; i <= INTERFACES::Globals->maxclients; i++) {
		auto* player = INTERFACES::ClientEntityList->GetClientEntity(i);

		if (!player || player == local_player || player->GetIsDormant())
			continue;

		bool is_local_player = player == local_player;
		bool is_teammate = local_player->GetTeam() == player->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (is_teammate)
			continue;

		SDK::player_info_t player_info;
		if (!INTERFACES::Engine->GetPlayerInfo(player->GetIndex(), &player_info))
			return;

		switch (stage) {
		case FRAME_NET_UPDATE_START:
		{
			userId[i] = player_info.userid;
			memcpy(&backupLayersUpdate[i], player->GetAnimOverlaysModifiable(), (sizeof SDK::CAnimationLayer) * 15);
			break;
		}
		case FRAME_RENDER_START:
		{
			if (userId[i] != player_info.userid)
				continue;

			memcpy(&backupLayersInterp[i], player->GetAnimOverlaysModifiable(), (sizeof SDK::CAnimationLayer) * 15);
			memcpy(player->GetAnimOverlaysModifiable(), &backupLayersUpdate[i], (sizeof SDK::CAnimationLayer) * 15);
			break;
		}
		case FRAME_RENDER_END:
		{
			if (userId[i] != player_info.userid)
				continue;

			memcpy(player->GetAnimOverlaysModifiable(), &backupLayersInterp[i], (sizeof SDK::CAnimationLayer) * 15);
			break;
		}
		default:
			return;
		}
	}
}

void CBacktrack::invalidate_bonecache( SDK::CBaseEntity *ent ) {
	static DWORD InvalidateBoneCache = UTILS::FindPattern( "client.dll", ( PBYTE )"\x80\x3D\x00\x00\x00\x00\x00\x74\x16\xA1", "xx????xxxx" );
	static unsigned long g_iModelBoneCounter = **( unsigned long** ) ( InvalidateBoneCache + 10 ); //    Offsets::InvalidateBoneCache = FindPatternIDA("client.dll", "80 3D ? ? ? ? 00 74 16 A1");
	*( int* ) ( ( DWORD ) ent + OFFSETS::m_nForceBone + 0x20 ) = 0; //m_nForceBone + 0x20
	*( unsigned int* ) ( ( DWORD ) ent + 0x2914 ) = -FLT_MAX; // m_flLastBoneSetupTime = -FLT_MAX;
	*( unsigned int* ) ( ( DWORD ) ent + 0x2680 ) = ( g_iModelBoneCounter - 1 ); // m_iMostRecentModelBoneCounter = g_iModelBoneCounter - 1;
}

resolver_player_t * CBacktrack::get_records( int index ) {
	if ( index < 0 )
		return nullptr;

	return &players [index];
}

inline Vector CBacktrack::angle_vector(Vector meme)
{
	auto sy = sin(meme.y / 180.f * static_cast<float>(M_PI));
	auto cy = cos(meme.y / 180.f * static_cast<float>(M_PI));

	auto sp = sin(meme.x / 180.f * static_cast<float>(M_PI));
	auto cp = cos(meme.x / 180.f* static_cast<float>(M_PI));

	return Vector(cp*cy, cp*sy, -sp);
}

inline float CBacktrack::point_to_line(Vector Point, Vector LineOrigin, Vector Dir)
{
	auto PointDir = Point - LineOrigin;

	auto TempOffset = PointDir.Dot(Dir) / (Dir.x*Dir.x + Dir.y*Dir.y + Dir.z*Dir.z);
	if (TempOffset < 0.000001f)
		return FLT_MAX;

	auto PerpendicularPoint = LineOrigin + (Dir * TempOffset);

	return (Point - PerpendicularPoint).Length();
}

void CBacktrack::run_legit(SDK::CUserCmd* cmd)
{
	int bestTargetIndex = -1;
	float bestFov = FLT_MAX;
	SDK::player_info_t info;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	for (int i = 1; i < 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

		if (!entity)
			continue;

		if (entity == local_player)
			continue;

		if (!INTERFACES::Engine->GetPlayerInfo(i, &info))
			continue;

		if (entity->GetIsDormant())
			continue;

		if (entity->GetTeam() == local_player->GetTeam())
			continue;

		if (!local_player->GetHealth() > 0)
			return;

		if (entity->GetHealth() > 0)
		{
			float simtime = entity->GetSimTime();
			Vector hitboxPos = aimbot->get_hitbox_pos(entity, 0);

			headPositions[i][cmd->command_number % 12] = legit_backtrackdata{ simtime, hitboxPos };
			Vector ViewDir = angle_vector(cmd->viewangles + (local_player->GetPunchAngles() * 2.f));
			Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();
			float FOVDistance = point_to_line(hitboxPos, local_position, ViewDir);

			if (bestFov > FOVDistance)
			{
				bestFov = FOVDistance;
				bestTargetIndex = i;
			}
		}
	}

	float bestTargetSimTime;
	if (bestTargetIndex != -1)
	{
		float tempFloat = FLT_MAX;
		Vector ViewDir = angle_vector(cmd->viewangles + (local_player->GetPunchAngles() * 2.f));
		Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

		for (int t = 0; t < 12; ++t)
		{
			float tempFOVDistance = point_to_line(headPositions[bestTargetIndex][t].hitboxPos, local_position, ViewDir);
			if (tempFloat > tempFOVDistance && headPositions[bestTargetIndex][t].simtime > local_player->GetSimTime() - 1)
			{
				tempFloat = tempFOVDistance;
				bestTargetSimTime = headPositions[bestTargetIndex][t].simtime;
			}
		}
		if (cmd->buttons & IN_ATTACK)
		{
			cmd->tick_count = TIME_TO_TICKS(bestTargetSimTime);
		}
	}
}

bool CBacktrack::valid(SDK::CBaseEntity * player)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return false;

	if (!player)
		return false;

	if (player->GetIsDormant() || player->GetHealth() == 0 || player->GetFlags() & FL_ATCONTROLS)
		return false;

	if (player->GetTeam() == local_player->GetTeam())
		return false;

	if (player->GetClientClass()->m_ClassID != 35)
		return false;

	if (player == local_player)
		return false;

	if (player->GetImmunity())
		return false;

	return true;
}

CBacktrack* backtracking = new CBacktrack();
legit_backtrackdata headPositions[64][12];