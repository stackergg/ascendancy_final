#include "..\includes.h"

#include "hooks.h"
#include "../UTILS/interfaces.h"
#include "../UTILS/offsets.h"
#include "../UTILS/NetvarHookManager.h"
#include "../UTILS/render.h"

#include "../SDK/CInput.h"
#include "../SDK/IClient.h"
#include "../SDK/CPanel.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlowObjectManager.h"
#include "../SDK/IEngine.h"
#include "../SDK/CTrace.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/RecvData.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/ModelRender.h"
#include "../SDK/RenderView.h"
#include "../SDK/CTrace.h"
#include "../SDK/CViewSetup.h"
#include "../SDK/CGlobalVars.h"

#include "../FEATURES/Movement.h"
#include "../FEATURES/Visuals.h"
#include "../FEATURES/Chams.h"
#include "../FEATURES/AntiAim.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Resolver.h"
#include "../FEATURES/Backtracking.h"
#include "../FEATURES/FakeWalk.h"
#include "../FEATURES/FakeLag.h"
#include "../FEATURES/EnginePred.h"
#include "../FEATURES/Extrapolation.h"
#include "../FEATURES/EventListener.h"

#include "../MENU/menu_framework.h"

#include <intrin.h>

//--- Other Globally Used Variables ---///
static bool tick = false;
static int ground_tick;
Vector vecAimPunch, vecViewPunch;
Vector* pAimPunch = nullptr;
Vector* pViewPunch = nullptr;

//--- Declare Signatures and Patterns Here ---///
static auto CAM_THINK = UTILS::FindSignature("client.dll", "85 C0 75 30 38 86");
static auto linegoesthrusmoke = UTILS::FindPattern("client.dll", (PBYTE)"\x55\x8B\xEC\x83\xEC\x08\x8B\x15\x00\x00\x00\x00\x0F\x57\xC0", "xxxxxxxx????xxx");
static auto clientstate = *reinterpret_cast<uintptr_t*>(uintptr_t(GetModuleHandle("engine.dll")) + 0x57D894);

//--- Tick Counting ---//
void ground_ticks()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetFlags() & FL_ONGROUND)
		ground_tick++;
	else
		ground_tick = 0;
}

namespace HOOKS
{
	CreateMoveFn original_create_move;
	PaintTraverseFn original_paint_traverse;
	PaintFn original_paint;
	FrameStageNotifyFn original_frame_stage_notify;
	DrawModelExecuteFn original_draw_model_execute;
	SceneEndFn original_scene_end;
	TraceRayFn original_trace_ray;
	SendDatagramFn original_send_datagram;
	OverrideViewFn original_override_view;
	RenderViewFn original_render_view;
	//ToFirstPersonFn original_to_firstperson;
	SvCheatsGetBoolFn original_get_bool;

	VMT::VMTHookManager iclient_hook_manager;
	VMT::VMTHookManager panel_hook_manager;
	VMT::VMTHookManager paint_hook_manager;
	VMT::VMTHookManager model_render_hook_manager;
	VMT::VMTHookManager scene_end_hook_manager;
	VMT::VMTHookManager render_view_hook_manager;
	VMT::VMTHookManager trace_hook_manager;
	VMT::VMTHookManager net_channel_hook_manager;
	VMT::VMTHookManager override_view_hook_manager;
	VMT::VMTHookManager input_table_manager;
	VMT::VMTHookManager get_bool_manager;

	bool __stdcall HookedCreateMove(float sample_input_frametime, SDK::CUserCmd* cmd)
	{ 
		if (!cmd || cmd->command_number == 0)
			return false;

		uintptr_t* FPointer; __asm { MOV FPointer, EBP }
		byte* SendPacket = (byte*)(*FPointer - 0x1C);

		if (!SendPacket)
			return false;

		GLOBAL::should_send_packet = *SendPacket;
		// S T A R T

		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			GLOBAL::randomnumber = rand() % 2;

			if (SETTINGS::settings.bhop_bool)
				movement->bunnyhop(cmd);

			if (SETTINGS::settings.strafe_bool)
				movement->autostrafer(cmd);

			//if (ground_tick < 32)
				//cmd->buttons &= ~FL_DUCKING;

			for (int i = 1; i <= 65; i++)
			{
				auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

				if (!entity)
					continue;

				auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

				if (!local_player)
					return;

				bool is_local_player = entity == local_player;
				bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

				if (is_local_player)
					continue;

				if (is_teammate)
					continue;

				if (entity->GetHealth() <= 0)
					continue;

				if (entity->GetIsDormant())
					continue;

				if (SETTINGS::settings.stop_bool)
					movement->quick_stop(entity, cmd);
			}

			if (SETTINGS::settings.aim_type == 0 && SETTINGS::settings.aim_bool)
				slidebitch->do_fakewalk(cmd);

			prediction->run_prediction(cmd); //start prediction

			if (SETTINGS::settings.lag_bool)
				fakelag->do_fakelag(cmd);

			//the aimbool stuff is correct, it just disables these features when aimbot is turned off. chill alpine
			if (SETTINGS::settings.aim_type == 0 && SETTINGS::settings.aim_bool)
			{
				aimbot->run_aimbot(cmd);
				aimbot->fix_recoil(cmd);
				//aimbot->auto_revolver(cmd);
			}

			if (SETTINGS::settings.aim_type == 1 && SETTINGS::settings.aim_bool && SETTINGS::settings.back_bool)
				backtracking->run_legit(cmd);

			if (SETTINGS::settings.aa_bool || SETTINGS::settings.aa_type > 0 || SETTINGS::settings.aim_type == 0)
			{
				antiaim->do_antiaim(cmd);
				antiaim->fix_movement(cmd);

				ground_ticks();
			}

			if (SETTINGS::settings.aa_bool && SETTINGS::settings.aa_type > 0 && SETTINGS::settings.aa_type != 6)
			{
				if (!GLOBAL::should_send_packet)
					GLOBAL::real_angles = cmd->viewangles;
				else
					GLOBAL::fake_angles = cmd->viewangles;
			}
			else
			{
				INTERFACES::Engine->GetViewAngles( GLOBAL::real_angles );
				INTERFACES::Engine->GetViewAngles( GLOBAL::fake_angles );
			}
			prediction->end_prediction(cmd); //end prediction
		}

		// E N D
		*SendPacket = GLOBAL::should_send_packet;

		if (SETTINGS::settings.aa_pitch < 2)
			UTILS::ClampLemon(cmd->viewangles);

		return false;
	}
	void __fastcall HookedPaint(void *ecx, void *edx, int mode) 
	{
		original_paint(ecx, edx, mode);

		//static auto StartDrawing = reinterpret_cast<void(__thiscall*)(void*)>(UTILS::FindSignature("vguimatsurface.dll", "55 8B EC 64 A1 ? ? ? ? 6A FF 68 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 14")); //hmm lol
		//static auto StopDrawing = reinterpret_cast<void(__thiscall*)(void*)>(UTILS::FindSignature("vguimatsurface.dll", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 56 6A 00"));

		if (mode & 1) 
		{
			//StartDrawing(INTERFACES::Surface);
			MENU::PPGUI_PP_GUI::Begin();
			MENU::Do();
			MENU::PPGUI_PP_GUI::End();

			UTILS::INPUT::input_handler.Update();

			RENDER::DrawSomething();
			//StopDrawing(INTERFACES::Surface);
		}
	}
	void __stdcall HookedPaintTraverse(int VGUIPanel, bool ForceRepaint, bool AllowForce)
	{
		std::string panel_name = INTERFACES::Panel->GetName(VGUIPanel);

		if (panel_name == "HudZoom" && SETTINGS::settings.scope_bool)
			return;

		if (panel_name == "FocusOverlayPanel") //MatSystemTopPanel
		{
			if (FONTS::ShouldReloadFonts())
				FONTS::InitFonts();

			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				if (SETTINGS::settings.esp_bool)
				{
					visuals->Draw();
					visuals->ClientDraw();
				}
			}

			MENU::PPGUI_PP_GUI::Begin();
			MENU::Do();
			MENU::PPGUI_PP_GUI::End();

			UTILS::INPUT::input_handler.Update();

			RENDER::DrawSomething();

			visuals->LogEvents();
		}

		original_paint_traverse(INTERFACES::Panel, VGUIPanel, ForceRepaint, AllowForce);
	}
	void __fastcall HookedFrameStageNotify(void* ecx, void* edx, int stage)
	{
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!local_player)
			return;

		//--- Night Mode ---//
		visuals->ModulateWorld();

		switch (stage)
		{
		case FRAME_NET_UPDATE_POSTDATAUPDATE_START:

			//--- Backtrack Log Players ---//
			//backtracking->log_players( );

			//--- Angle Resolving and Such ---//
			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				for (int i = 1; i <= 65; i++)
				{
					auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

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

					if (entity->GetIsDormant())
						continue;

					if (SETTINGS::settings.aim_type == 0)
						resolver->resolve(entity);

					entity->UpdateClientSideAnimation();
				}
			}
			break;

		case FRAME_NET_UPDATE_POSTDATAUPDATE_END:

			break;

		case FRAME_RENDER_START:

			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				//--- Thirdperson Deadflag Stuff ---//
				if (in_tp)
				{
					SDK::CBaseAnimState* animstate = local_player->GetAnimState();

					if (!animstate)
						return;

					if (animstate->m_bInHitGroundAnimation && ground_tick > 1)
						*(Vector*)((DWORD)local_player + 0x31C8) = Vector(0.0f, GLOBAL::real_angles.y, 0.f);
					else
						*(Vector*)((DWORD)local_player + 0x31C8) = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, 0.f);
				}

				//--- "PVS Fix" ---//
				for (int i = 1; i <= INTERFACES::Globals->maxclients; i++)
				{
					auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

					if (!entity)
						continue;

					if (entity == local_player)
						continue;

					*(int*)((uintptr_t)entity + 0xA30) = INTERFACES::Globals->framecount;
					*(int*)((uintptr_t)entity + 0xA28) = 0;
				}

				//--- No Visual Recoil ---//
				if (local_player && local_player->IsAlive() && SETTINGS::settings.novis_bool)
				{
					pAimPunch = (Vector*)((DWORD)local_player + OFFSETS::m_aimPunchAngle);
					pViewPunch = (Vector*)((DWORD)local_player + OFFSETS::m_viewPunchAngle);

					vecAimPunch = *pAimPunch;
					vecViewPunch = *pViewPunch;

					*pAimPunch = Vector(0, 0, 0);
					*pViewPunch = Vector(0, 0, 0);
				}

				//--- fire_delay ei = 0 ---//
				/*intptr_t events = (uintptr_t)clientstate + 0x4DE0;
				uintptr_t ei = *(uintptr_t*)(events + 0xC), next = NULL; // cl.events.Head()

				if (ei)
				{
					do
					{
						next = *(uintptr_t*)(ei + 0x38);
						uint16_t classID = *(uint16_t*)ei - 1;

						if (classID == 168)
						{
							*(float*)(ei + 0x4) = 0.0f;
						}
						ei = next;

					} while (next != NULL);
				}*/

				//--- Clantag Changer ---//
				visuals->apply_clantag();
			}
			break;

		case FRAME_NET_UPDATE_START:
			//--- Bullet Beams ---//
			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				if (SETTINGS::settings.beam_bool)
					visuals->DrawBulletBeams();
			}

			break;

		case FRAME_NET_UPDATE_END:
			break;
		}

		original_frame_stage_notify(ecx, stage);

		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame() && local_player->IsAlive() && pAimPunch && pViewPunch && SETTINGS::settings.novis_bool)
		{
			*pAimPunch = vecAimPunch;
			*pViewPunch = vecViewPunch;
		}
	}
	void __fastcall HookedDrawModelExecute(void* ecx, void* edx, SDK::IMatRenderContext* context, const SDK::DrawModelState_t& state, const SDK::ModelRenderInfo_t& render_info, matrix3x4_t* matrix)
	{
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

			if (!local_player)
				return;

			if (render_info.entity_index == INTERFACES::Engine->GetLocalPlayer() && local_player && in_tp && SETTINGS::settings.esp_bool && local_player->GetIsScoped())
				INTERFACES::RenderView->SetBlend(0.4f);

			/*if (render_info.pModel) //ghetto shadow remove
			{
				std::string modelName = INTERFACES::ModelInfo->GetModelName(render_info.pModel);
				static SDK::IMaterial* notignorez = chams->CreateMaterial(false, true, false);

				if (modelName.find("models/player") != std::string::npos && SETTINGS::settings.esp_bool)
				{
					SDK::CBaseEntity* pModelEntity = (SDK::CBaseEntity*)INTERFACES::ClientEntityList->GetClientEntity(render_info.entity_index);
					if (!pModelEntity)
						return;

					if (pModelEntity->GetIndex() == INTERFACES::Engine->GetLocalPlayer() && pModelEntity->IsAlive() && !pModelEntity->GetIsDormant())
					{
						INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
						original_draw_model_execute(ecx, context, state, render_info, matrix);
					}
				}
			}*/
		}
		original_draw_model_execute(ecx, context, state, render_info, matrix);
		INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
	}
/*	void __fastcall Hooked_DrawModel(void* thisptr, void* edx, SDK::DrawModelResults_t *pResults, const SDK::ModelRenderInfo_t& info, matrix3x4_t *pBoneToWorld, float *pFlexWeights, float *pFlexDelayedWeights, const Vector &modelOrigin, int flags) 
	{
		std::string modelName = INTERFACES::ModelInfo->GetModelName(info.pModel);

		if (modelName.find("shadow") != std::string::npos)
		{
			original_draw_model(thisptr, pResults, info, pBoneToWorld, pFlexWeights, pFlexDelayedWeights, modelOrigin, flags);
			return;
		}

		original_draw_model(thisptr, pResults, info, pBoneToWorld, pFlexWeights, pFlexDelayedWeights, modelOrigin, flags);
		INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
	}*/
	void __fastcall HookedSceneEnd(void* ecx, void* edx)
	{
		original_scene_end(ecx);
		//Crash not coming from SE
		static SDK::IMaterial* ignorez = chams->CreateMaterial(true, true, false);
		static SDK::IMaterial* notignorez = chams->CreateMaterial(false, true, false);
		
		CColor color;
		color = SETTINGS::settings.glow_col;

		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			//--- Entity Glow ---//
			for (auto i = 0; i < INTERFACES::GlowObjManager->GetSize(); i++)
			{
				auto &glowObject = INTERFACES::GlowObjManager->m_GlowObjectDefinitions[i];
				auto entity = reinterpret_cast<SDK::CBaseEntity*>(glowObject.m_pEntity);
				auto m_pLocalPlayer = reinterpret_cast<SDK::CBaseEntity*>(INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer()));

				if (!entity)
					continue;

				if (!m_pLocalPlayer)
					continue;

				if (glowObject.IsUnused())
					continue;

				bool is_local_player = entity == m_pLocalPlayer;
				bool is_teammate = m_pLocalPlayer->GetTeam() == entity->GetTeam() && !is_local_player;

				if (is_local_player)
					continue;

				if (is_local_player && in_tp && SETTINGS::settings.localglow_bool) //unused
				{
					glowObject.m_nGlowStyle = 1;
					glowObject.m_flAlpha = 1.f;
					glowObject.m_flRed = color.RGBA[0] / 255.0f;
					glowObject.m_flGreen = color.RGBA[1] / 255.0f;
					glowObject.m_flBlue = color.RGBA[2] / 255.0f;
					glowObject.m_bRenderWhenOccluded = true;
					glowObject.m_bRenderWhenUnoccluded = false;
					glowObject.m_bFullBloomRender = true;
					continue;
				}

				if (!SETTINGS::settings.glow_bool)
					continue;

				if (is_teammate)
					continue;

				auto class_id = entity->GetClientClass()->m_ClassID;


				switch (class_id)
				{
				default:
					glowObject.m_flAlpha = 0.0f;
					break;
				case 35:
					glowObject.m_nGlowStyle = 0;
					glowObject.m_flAlpha = 0.7f;
					break;
				}

				glowObject.m_flRed = color.RGBA[0] / 255.0f;
				glowObject.m_flGreen = color.RGBA[1] / 255.0f;
				glowObject.m_flBlue = color.RGBA[2] / 255.0f;
				glowObject.m_bRenderWhenOccluded = true;
				glowObject.m_bRenderWhenUnoccluded = false;
			}

			//--- Entity Chams ---//
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

				if (!SETTINGS::settings.cham_bool && is_local_player)
					continue;

				if (is_teammate)
					continue;

				//--- Colored Models ---//

				if (!is_local_player && entity && SETTINGS::settings.chams_type == 2)
				{
					ignorez->ColorModulate(SETTINGS::settings.imodel_col); //255, 40, 200
					INTERFACES::ModelRender->ForcedMaterialOverride(ignorez);
					entity->DrawModel(0x1, 255);
					notignorez->ColorModulate(SETTINGS::settings.vmodel_col); //0, 125, 255
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
					entity->DrawModel(0x1, 255);
					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}
				else if (!is_local_player && entity && SETTINGS::settings.chams_type == 1)
				{
					notignorez->ColorModulate(SETTINGS::settings.vmodel_col); //255, 40, 200
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
					entity->DrawModel(0x1, 255);
					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}

				if (is_local_player && local_player && SETTINGS::settings.cham_bool)
				{
					notignorez->ColorModulate(SETTINGS::settings.lvmodel_col); //255, 40, 200
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
					entity->DrawModel(0x1, 255);
					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}
			}

			//--- Wireframe Smoke ---//
			if (SETTINGS::settings.smoke_bool)
			{
				std::vector<const char*> vistasmoke_wireframe =
				{
					"particle/vistasmokev1/vistasmokev1_smokegrenade",
				};

				std::vector<const char*> vistasmoke_nodraw =
				{
					"particle/vistasmokev1/vistasmokev1_fire",
					"particle/vistasmokev1/vistasmokev1_emods",
					"particle/vistasmokev1/vistasmokev1_emods_impactdust",
				};

				for (auto mat_s : vistasmoke_wireframe)
				{
					SDK::IMaterial* mat = INTERFACES::MaterialSystem->FindMaterial(mat_s, TEXTURE_GROUP_OTHER);
					mat->SetMaterialVarFlag(SDK::MATERIAL_VAR_WIREFRAME, true); //wireframe
				}

				for (auto mat_n : vistasmoke_nodraw)
				{
					SDK::IMaterial* mat = INTERFACES::MaterialSystem->FindMaterial(mat_n, TEXTURE_GROUP_OTHER);
					mat->SetMaterialVarFlag(SDK::MATERIAL_VAR_NO_DRAW, true);
				}

				static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x8);
				*(int*)(smokecout) = 0;
			}
		}
	}
	void __fastcall HookedOverrideView(void* ecx, void* edx, SDK::CViewSetup* pSetup)
	{
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!local_player)
			return;

		auto animstate = local_player->GetAnimState();

		if (!animstate)
			return;

		if (GetAsyncKeyState(VK_MBUTTON) & 1)
			in_tp = !in_tp;

		//--- Actual Thirdperson Stuff ---//
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			auto GetCorrectDistance = [&local_player](float ideal_distance) -> float //lambda
			{
				Vector inverse_angles;
				INTERFACES::Engine->GetViewAngles(inverse_angles);

				inverse_angles.x *= -1.f, inverse_angles.y += 180.f;

				Vector direction;
				MATH::AngleVectors(inverse_angles, &direction);

				SDK::CTraceWorldOnly filter;
				SDK::trace_t trace;
				SDK::Ray_t ray;

				ray.Init(local_player->GetVecOrigin() + local_player->GetViewOffset(), (local_player->GetVecOrigin() + local_player->GetViewOffset()) + (direction * (ideal_distance + 5.f)));
				INTERFACES::Trace->TraceRay(ray, MASK_ALL, &filter, &trace);

				return ideal_distance * trace.flFraction;
			};

			if (SETTINGS::settings.tp_bool && in_tp)
			{
				if (local_player->GetHealth() <= 0)
					local_player->SetObserverMode(5);

				if (!INTERFACES::Input->m_fCameraInThirdPerson)
				{
					INTERFACES::Input->m_fCameraInThirdPerson = true;
					if (animstate->m_bInHitGroundAnimation && ground_tick > 1)
						INTERFACES::Input->m_vecCameraOffset = Vector(0.0f, GLOBAL::real_angles.y, GetCorrectDistance(100)); //100, GLOBAL::real_angles.x
					else
						INTERFACES::Input->m_vecCameraOffset = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, GetCorrectDistance(100)); //100
					Vector camForward;

					MATH::AngleVectors(Vector(INTERFACES::Input->m_vecCameraOffset.x, INTERFACES::Input->m_vecCameraOffset.y, 0), &camForward);
				}
			}
			else
			{
				INTERFACES::Input->m_fCameraInThirdPerson = false;
				INTERFACES::Input->m_vecCameraOffset = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, 0);
			}

			/*if (!local_player->GetIsScoped())
			{
				if (SETTINGS::settings.vfov_val == 0)
					pSetup->fov = 90; 
				else
					pSetup->fov = SETTINGS::settings.vfov_val; //110, 90 default
			}*/
		}
		original_override_view(ecx, pSetup);
	}
	void __fastcall HookedRenderView(void* thisptr, void*, SDK::CViewSetup& setup, SDK::CViewSetup& hudViewSetup, unsigned int nClearFlags, int whatToDraw)
	{
		if (INTERFACES::Engine->IsInGame())
		{
			auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

			if (!local_player)
				return;

			if (!local_player->IsAlive())
				return;

			if (!local_player->GetIsScoped())
				setup.fov += SETTINGS::settings.fov_val;

			setup.fovViewmodel += SETTINGS::settings.vfov_val;
		}
		original_render_view(thisptr, setup, hudViewSetup, nClearFlags, whatToDraw);
	}
	void __fastcall HookedTraceRay(void *thisptr, void*, const SDK::Ray_t &ray, unsigned int fMask, SDK::ITraceFilter *pTraceFilter, SDK::trace_t *pTrace)
	{
		original_trace_ray(thisptr, ray, fMask, pTraceFilter, pTrace);
		pTrace->surface.flags |= SURF_SKY;
	}
	void __fastcall HookedSendDatagram(void* ecx, void* data)
	{
		original_send_datagram(ecx, data);
	}
	bool __fastcall HookedGetBool(void* pConVar, void* edx)
	{
		if ((uintptr_t)_ReturnAddress() == CAM_THINK)
			return true;

		return original_get_bool(pConVar);
	}
	void InitHooks()
	{
		iclient_hook_manager.Init(INTERFACES::Client);
		original_frame_stage_notify = reinterpret_cast<FrameStageNotifyFn>(
			iclient_hook_manager.HookFunction<FrameStageNotifyFn>(36, HookedFrameStageNotify));
		//iclient_hook_manager.HookTable(true);

		panel_hook_manager.Init(INTERFACES::Panel);
		original_paint_traverse = reinterpret_cast<PaintTraverseFn>(
			panel_hook_manager.HookFunction<PaintTraverseFn>(41, HookedPaintTraverse));
		//panel_hook_manager.HookTable(true);

		/*paint_hook_manager.Init(INTERFACES::EngineVGui);
		original_paint = reinterpret_cast<PaintFn>(
			paint_hook_manager.HookFunction<PaintFn>(14, HookedPaint));*/

		model_render_hook_manager.Init(INTERFACES::ModelRender);
		original_draw_model_execute = reinterpret_cast<DrawModelExecuteFn>(model_render_hook_manager.HookFunction<DrawModelExecuteFn>(21, HookedDrawModelExecute));
		//model_render_hook_manager.HookTable(true);

		scene_end_hook_manager.Init(INTERFACES::RenderView);
		original_scene_end = reinterpret_cast<SceneEndFn>(scene_end_hook_manager.HookFunction<SceneEndFn>(9, HookedSceneEnd));
		//render_view_hook_manager.HookTable(true);

		render_view_hook_manager.Init(INTERFACES::ClientMode);
		original_render_view = reinterpret_cast<RenderViewFn>(render_view_hook_manager.HookFunction<RenderViewFn>(6, HookedRenderView));

		trace_hook_manager.Init(INTERFACES::Trace);
		original_trace_ray = reinterpret_cast<TraceRayFn>(trace_hook_manager.HookFunction<TraceRayFn>(5, HookedTraceRay));
		//trace_hook_manager.HookTable(true);

		override_view_hook_manager.Init(INTERFACES::ClientMode);
		original_override_view = reinterpret_cast<OverrideViewFn>(override_view_hook_manager.HookFunction<OverrideViewFn>(18, HookedOverrideView));
		original_create_move = reinterpret_cast<CreateMoveFn>(override_view_hook_manager.HookFunction<CreateMoveFn>(24, HookedCreateMove));
		//override_view_hook_manager.HookTable(true);

		/*input_table_manager = VMT::CVMTHookManager(INTERFACES::Input);
		original_to_firstperson = reinterpret_cast<ToFirstPersonFn>(input_table_manager.HookFunction(36, HookedToFirstPerson));
		input_table_manager.HookTable(true);*/

		auto sv_cheats = INTERFACES::cvar->FindVar("sv_cheats");
		get_bool_manager = VMT::VMTHookManager(reinterpret_cast<DWORD**>(sv_cheats));
		original_get_bool = reinterpret_cast<SvCheatsGetBoolFn>(get_bool_manager.HookFunction<SvCheatsGetBoolFn>(13, HookedGetBool));
		}

	void EyeAnglesPitchHook(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
	{
		*reinterpret_cast<float*>(pOut) = pData->m_Value.m_Float;

		auto entity = reinterpret_cast<SDK::CBaseEntity*>(pStruct);
		if (!entity)
			return;

	}
	void EyeAnglesYawHook(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
	{
		*reinterpret_cast<float*>(pOut) = pData->m_Value.m_Float;

		auto entity = reinterpret_cast<SDK::CBaseEntity*>(pStruct);
		if (!entity)
			return;

		//resolver->record(entity, pData->m_Value.m_Float);
	}

	void InitNetvarHooks()
	{
		UTILS::netvar_hook_manager.Hook("DT_CSPlayer", "m_angEyeAngles[0]", EyeAnglesPitchHook);
		UTILS::netvar_hook_manager.Hook("DT_CSPlayer", "m_angEyeAngles[1]", EyeAnglesYawHook);
	}
}
