#pragma once

struct AnimationInfo_t
{
	void*                m_ulEntHandle;
	float                    m_flSpawnTime;
	SDK::CBaseAnimState*    m_pAnimState;
};

/*class CAnimationCorrect
{
public:
	void run();
	SDK::CBaseAnimState*	m_serverAnimState;

	AnimationInfo_t			Players[MAX_PLAYERS];
private:
	void CreateAnimationState(SDK::CBaseAnimState* state, SDK::CBaseEntity* player);
	void UpdateAnimationState(SDK::CBaseAnimState* state, Vector ang);
	void ResetAnimationState(SDK::CBaseAnimState* state);
	void UpdatePlayerAnimations(int index);


	void*				m_ulEntHandle;
	float					m_flSpawnTime;
	Vector					m_angRealAngle = Vector(0, 0, 0);
	SDK::CBaseEntity*			m_pLocalPlayer;
	float					m_flNextBodyUpdate;
};*/