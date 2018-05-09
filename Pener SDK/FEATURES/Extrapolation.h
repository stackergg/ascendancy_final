#pragma once

namespace SDK
{
	class CBaseEntity;
}

struct ExtrapolationInfo_t
{
	Vector pre_position;
	Vector post_position;
	float pre_simtime;
	int choked_ticks;
	Vector vec_position;
	Vector origin_delta;
};

class CExtrapolate
{
public:
	void run_extrapolation(SDK::CBaseEntity* entity);
	bool breaking_lc;

	ExtrapolationInfo_t	Players[MAX_PLAYERS];
};

extern CExtrapolate* extrapolation;