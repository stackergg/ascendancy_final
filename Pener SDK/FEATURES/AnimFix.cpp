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
#include "../FEATURES/AnimFix.h"

/*void CAnimationCorrect::run() {
	for (int index = 1; index <= INTERFACES::Globals->maxclients; index++) {
		UpdatePlayerAnimations(index);
	}
}

void CAnimationCorrect::UpdatePlayerAnimations(int index) {
	auto pPlayer = reinterpret_cast< SDK::CBaseEntity* >(INTERFACES::ClientEntityList->GetClientEntity(index));
	if (!pPlayer || pPlayer->GetClientClass()->m_ClassID != 35 || index == INTERFACES::Engine->GetLocalPlayer()) {
		Players[index].m_pAnimState = nullptr;
		return;
	}

	bool allocate = (Players[index].m_pAnimState == nullptr);
	bool change = (!allocate) && (pPlayer->GetRefEHandle() != Players[index].m_ulEntHandle);
	bool reset = (!allocate && !change) && (pPlayer->GetSpawnTime() != m_flSpawnTime);

	if (change) {
		g_pMemAlloc->Free(m_serverAnimState);
	}

	if (reset) {
		if (m_serverAnimState)
			ResetAnimationState(m_serverAnimState);

		Players[index].m_flSpawnTime = pPlayer->GetSpawnTime();
	}

	if (allocate || change) {
		auto state = reinterpret_cast< SDK::CBaseAnimState* > (g_pMemAlloc->Alloc(sizeof(CCSGOPlayerAnimState)));

		if (state)
			CreateAnimationState(state, pPlayer);

		Players[index].m_ulEntHandle = pPlayer->GetRefEHandle();
		Players[index].m_flSpawnTime = pPlayer->GetSpawnTime();

		Players[index].m_pAnimState = state;
		return;
	}

	// backup ------------------------------------------------------------------------------------------------------------------

	Vector backup_absorigin = pPlayer->GetAbsOrigin();
	Vector backup_absangles = pPlayer->GetAbsAngles();

	float backup_poses[24];
	for (int i = 0; i < 24; i++)
		backup_poses[i] = pPlayer->GetPoseParamaters()[i];

	SDK::CAnimationLayer backup_layers[16];
	for (int i = 0; i < 13; i++) {
		backup_layers[i].m_flCycle = pPlayer->GetAnimOverlay[i].m_flCycle;
		backup_layers[i].m_flPlaybackRate = pPlayer->GetAnimOverlay[i].m_flPlaybackRate;
		backup_layers[i].m_flPrevCycle = pPlayer->GetAnimOverlay[i].m_flPrevCycle;
		backup_layers[i].m_flWeight = pPlayer->GetAnimOverlay[i].m_flWeight;
		backup_layers[i].m_flWeightDeltaRate = pPlayer->GetAnimOverlay[i].m_flWeightDeltaRate;
		backup_layers[i].m_nOrder = pPlayer->GetAnimOverlay[i].m_nOrder;
		backup_layers[i].m_nSequence = pPlayer->GetAnimOverlay[i].m_nSequence;
	}

	UpdateAnimationState(Players[index].m_pAnimState, pPlayer->GetEyeAngles());

	// store -------------------------------------------------------------------------------------------------------------------


	for (int i = 0; i < 24; i++)
		c_bonesetup::get().m_flPoseParams[index][i] = pPlayer->GetPoseParamaters()[i];

	for (int i = 0; i < 13; i++) {
		c_bonesetup::get().m_vAnimLayers[index][i].m_flCycle = pPlayer->GetAnimOverlay[i].m_flCycle;
		c_bonesetup::get().m_vAnimLayers[index][i].m_flPlaybackRate = pPlayer->GetAnimOverlay[i].m_flPlaybackRate;
		c_bonesetup::get().m_vAnimLayers[index][i].m_flPrevCycle = pPlayer->GetAnimOverlay[i].m_flPrevCycle;
		c_bonesetup::get().m_vAnimLayers[index][i].m_flWeight = pPlayer->GetAnimOverlay[i].m_flWeight;
		c_bonesetup::get().m_vAnimLayers[index][i].m_flWeightDeltaRate = pPlayer->GetAnimOverlay[i].m_flWeightDeltaRate;
		c_bonesetup::get().m_vAnimLayers[index][i].m_nOrder = pPlayer->GetAnimOverlay[i].m_nOrder;
		c_bonesetup::get().m_vAnimLayers[index][i].m_nSequence = pPlayer->GetAnimOverlay[i].m_nSequence;
	}

	// restore -----------------------------------------------------------------------------------------------------------------

	pPlayer->SetAbsOrigin(backup_absorigin);
	pPlayer->SetAbsAngles(backup_absangles);

	for (int i = 0; i < 24; i++)
		pPlayer->GetPoseParamaters()[i] = backup_poses[i];

	for (int i = 0; i < 13; i++) {
		pPlayer->GetAnimOverlay[i].m_flCycle = backup_layers[i].m_flCycle;
		pPlayer->GetAnimOverlay[i].m_flPlaybackRate = backup_layers[i].m_flPlaybackRate;
		pPlayer->GetAnimOverlay[i].m_flPrevCycle = backup_layers[i].m_flPrevCycle;
		pPlayer->GetAnimOverlay[i].m_flWeight = backup_layers[i].m_flWeight;
		pPlayer->GetAnimOverlay[i].m_flWeightDeltaRate = backup_layers[i].m_flWeightDeltaRate;
		pPlayer->GetAnimOverlay[i].m_nOrder = backup_layers[i].m_nOrder;
		pPlayer->GetAnimOverlay[i].m_nSequence = backup_layers[i].m_nSequence;
	}
}*/