#pragma once

namespace SDK
{
	/*struct CViewSetup
	{
		char _0x0000[16];
		__int32 x;
		__int32 x_old;
		__int32 y;
		__int32 y_old;
		__int32 width;
		__int32    width_old;
		__int32 height;
		__int32    height_old;
		char _0x0030[128];
		float fov;
		float fovViewmodel;
		Vector origin;
		Vector angles;
		float zNear;
		float zFar;
		float zNearViewmodel;
		float zFarViewmodel;
		float m_flAspectRatio;
		float m_flNearBlurDepth;
		float m_flNearFocusDepth;
		float m_flFarFocusDepth;
		float m_flFarBlurDepth;
		float m_flNearBlurRadius;
		float m_flFarBlurRadius;
		float m_nDoFQuality;
		__int32 m_nMotionBlurMode;
		char _0x0104[68];
		__int32 m_EdgeBlur;
	};*/
	class CViewSetup
	{
	public:
		int			x, x_old;
		int			y, y_old;
		int			width, width_old;
		int			height, height_old;
		bool		m_bOrtho;
		float		m_OrthoLeft;
		float		m_OrthoTop;
		float		m_OrthoRight;
		float		m_OrthoBottom;
		bool		m_bCustomViewMatrix;
		matrix3x4_t	m_matCustomViewMatrix;
		char		pad_0x68[0x48];
		float		fov;
		float		fovViewmodel;
		Vector		origin;
		Vector		angles;
		float		zNear;
		float		zFar;
		float		zNearViewmodel;
		float		zFarViewmodel;
		float		m_flAspectRatio;
		float		m_flNearBlurDepth;
		float		m_flNearFocusDepth;
		float		m_flFarFocusDepth;
		float		m_flFarBlurDepth;
		float		m_flNearBlurRadius;
		float		m_flFarBlurRadius;
		int			m_nDoFQuality;
		int			m_nMotionBlurMode;
		float		m_flShutterTime;
		Vector		m_vShutterOpenPosition;
		Vector		m_shutterOpenAngles;
		Vector		m_vShutterClosePosition;
		Vector		m_shutterCloseAngles;
		float		m_flOffCenterTop;
		float		m_flOffCenterBottom;
		float		m_flOffCenterLeft;
		float		m_flOffCenterRight;
		int			m_EdgeBlur;
	};

}