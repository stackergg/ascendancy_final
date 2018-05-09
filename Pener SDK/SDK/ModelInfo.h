#pragma once

#include "IMaterial.h"


namespace SDK
{
	struct model_t;
	struct mstudioanimdesc_t {};
	struct mstudioseqdesc_t {};

	/*enum HitboxList
	{
		HITBOX_PELVIS,
		HITBOX_L_THIGH,
		HITBOX_L_CALF,
		HITBOX_L_FOOT,
		HITBOX_R_THIGH,
		HITBOX_R_CALF,
		HITBOX_R_FOOT,
		HITBOX_SPINE1,
		HITBOX_SPINE2,
		HITBOX_SPINE3,
		HITBOX_NECK,
		HITBOX_HEAD,
		HITBOX_L_UPPERARM,
		HITBOX_L_FOREARM,
		HITBOX_L_HAND,
		HITBOX_R_UPPERARM,
		HITBOX_R_FOREARM,
		HITBOX_R_HAND,
		HITBOX_L_CLAVICLE,
		HITBOX_R_CLAVICLE,
		HITBOX_SPINE4,
		HITBOX_MAX,
	};*/

	enum HitboxList
	{
		HITBOX_HEAD,
		HITBOX_NECK,
		HITBOX_LOWER_NECK,
		HITBOX_PELVIS,
		HITBOX_BODY,
		HITBOX_THORAX,
		HITBOX_CHEST,
		HITBOX_UPPER_CHEST,
		HITBOX_RIGHT_THIGH,
		HITBOX_LEFT_THIGH,
		HITBOX_RIGHT_CALF,
		HITBOX_LEFT_CALF,
		HITBOX_RIGHT_FOOT,
		HITBOX_LEFT_FOOT,
		HITBOX_RIGHT_HAND,
		HITBOX_LEFT_HAND,
		HITBOX_RIGHT_UPPER_ARM,
		HITBOX_RIGHT_FOREARM,
		HITBOX_LEFT_UPPER_ARM,
		HITBOX_LEFT_FOREARM,
		HITBOX_MAX
	};

	/*struct mstudiobbox_t
	{
		int					bone;
		int					group;
		Vector				bbmin;
		Vector				bbmax;
		int					szhitboxnameindex;
		int					m_iPad01[3];
		float				m_flRadius;
		int					m_iPad02[4];
	};*/
	struct mstudiobbox_t
	{
		int					bone;
		int					group;				// intersection group
		Vector				bbmin;				// bounding box
		Vector				bbmax;
		int					szhitboxnameindex;	// offset to the name of the hitbox.
		int					unused[3];
		float				radius;
		int					unused2[4];

		const char* pszHitboxName()
		{
			if (szhitboxnameindex == 0)
				return "";

			return ((char*)this) + szhitboxnameindex;
		}

		mstudiobbox_t() {}

	private:
		mstudiobbox_t(const mstudiobbox_t& vOther);
	};
	/*struct mstudiobone_t
	{
		int					sznameindex;
		inline char * const pszName(void) const { return ((char *)this) + sznameindex; }
		int		 			parent;		// parent bone
		int					bonecontroller[6];	// bone controller index, -1 == none

												// default values
		Vector				pos;
		Quaternion			quat;
		Vector			rot;
		// compression scale
		Vector				posscale;
		Vector				rotscale;

		VMatrix			poseToBone;
		Quaternion			qAlignment;
		int					flags;
		int					proctype;
		int					procindex;		// procedural rule
		mutable int			physicsbone;	// index into physically simulated bone
		inline void *pProcedure() const { if (procindex == 0) return NULL; else return  (void *)(((byte *)this) + procindex); };
		int					surfacepropidx;	// index into string tablefor property name
		inline char * const pszSurfaceProp(void) const { return ((char *)this) + surfacepropidx; }
		int					contents;		// See BSPFlags.h for the contents flags

		int					unused[8];		// remove as appropriate

		mstudiobone_t() {}
	private:
		// No copy constructors allowed
		mstudiobone_t(const mstudiobone_t& vOther);
	};*/
	struct mstudiobone_t
	{
		int					sznameindex;
		inline char * const pszName(void) const { return ((char *)this) + sznameindex; }
		int		 			parent;		// parent bone
		int					bonecontroller[6];	// bone controller index, -1 == none

												// default values
		Vector				pos;
		Quaternion			quat;
		RadianEuler			rot;
		// compression scale
		Vector				posscale;
		Vector				rotscale;

		matrix3x4_t			poseToBone;
		Quaternion			qAlignment;
		int					flags;
		int					proctype;
		int					procindex;		// procedural rule
		mutable int			physicsbone;	// index into physically simulated bone
		inline void *pProcedure() const { if (procindex == 0) return NULL; else return  (void *)(((byte *)this) + procindex); };
		int					surfacepropidx;	// index into string tablefor property name
		inline char * const pszSurfaceProp(void) const { return ((char *)this) + surfacepropidx; }
		int					contents;		// See BSPFlags.h for the contents flags

		int					unused[8];		// remove as appropriate

		mstudiobone_t() {}
	private:
		// No copy constructors allowed
		mstudiobone_t(const mstudiobone_t& vOther);
	};
	/*struct mstudiohitboxset_t
	{
		int                                             sznameindex;
		inline char* const              GetName(void) const { return ((char*)this) + sznameindex; }
		int                                             numhitboxes;
		int                                             hitboxindex;
		inline mstudiobbox_t *GetHitbox(int i) const { return (mstudiobbox_t*)(((unsigned char*)this) + hitboxindex) + i; };
	};*/
	struct mstudiohitboxset_t
	{
		int	sznameindex;
		int	numhitboxes;
		int	hitboxindex;

		inline mstudiobbox_t* GetHitbox(int i) const
		{
			return (mstudiobbox_t*)(((DWORD)this) + hitboxindex) + i;
		};
	};

	/*struct studiohdr_t
	{
		int                                     id;
		int                                     version;

		int                                     checksum;

		char                            name[64];
		int                                     length;


		Vector                          eyeposition;

		Vector                          illumposition;

		Vector                          hull_min;
		Vector                          hull_max;

		Vector                          view_bbmin;
		Vector                          view_bbmax;

		int                                     flags;

		int                                     numbones;
		int                                     boneindex;

		inline mstudiobone_t *GetBone(int i) const { return (mstudiobone_t *)(((byte *)this) + boneindex) + i; };
		//	inline mstudiobone_t *pBone(int i) const { Assert(i >= 0 && i < numbones); return (mstudiobone_t *)(((byte *)this) + boneindex) + i; };

		int                                     numbonecontrollers;
		int                                     bonecontrollerindex;

		int                                     numhitboxsets;
		int                                     hitboxsetindex;

		/*mstudiohitboxset_t* GetHitboxSet(int i) const
		{
			return (mstudiohitboxset_t*)(((byte*)this) + hitboxsetindex) + i;
		}
		// Look up hitbox set by index
		mstudiohitboxset_t	*pHitboxSet(int i) const
		{
			Assert(i >= 0 && i < numhitboxsets);
			return (mstudiohitboxset_t *)(((byte *)this) + hitboxsetindex) + i;
		};
		inline mstudiobbox_t* GetHitbox(int i, int set) const
		{
			mstudiohitboxset_t const* s = GetHitboxSet(set);

			if (!s)
				return NULL;

			return s->GetHitbox(i);
		}

		inline int GetHitboxCount(int set) const
		{
			mstudiohitboxset_t const* s = GetHitboxSet(set);

			if (!s)
				return 0;

			return s->numhitboxes;
		}*//*
		mstudiohitboxset_t	*GetHitboxSet(int i) const //phitboxset
		{
			Assert(i >= 0 && i < numhitboxsets);
			return (mstudiohitboxset_t *)(((byte *)this) + hitboxsetindex) + i;
		};

		// Calls through to hitbox to determine size of specified set
		inline mstudiobbox_t *GetHitbox(int i, int set) const
		{
			mstudiohitboxset_t const *s = GetHitboxSet(set);
			if (!s)
				return NULL;

			return s->GetHitbox(i);
		};

		// Calls through to set to get hitbox count for set
		inline int			iHitboxCount(int set) const
		{
			mstudiohitboxset_t const *s = GetHitboxSet(set);
			if (!s)
				return 0;

			return s->numhitboxes;
		};

		int                                     numlocalanim;
		int                                     localanimindex;

		int                                     numlocalseq;
		int                                     localseqindex;

		mutable int                     activitylistversion;
		mutable int                     eventsindexed;

		int                                     numtextures;
		int                                     textureindex;

		int                                     numcdtextures;
		int                                     cdtextureindex;

		int                                     numskinref;
		int                                     numskinfamilies;
		int                                     skinindex;

		int                                     numbodyparts;
		int                                     bodypartindex;

		int                                     numlocalattachments;
		int                                     localattachmentindex;

		int                                     numlocalnodes;
		int                                     localnodeindex;
		int                                     localnodenameindex;

		int                                     numflexdesc;
		int                                     flexdescindex;

		int                                     numflexcontrollers;
		int                                     flexcontrollerindex;

		int                                     numflexrules;
		int                                     flexruleindex;

		int                                     numikchains;
		int                                     ikchainindex;

		int                                     nummouths;
		int                                     mouthindex;

		int                                     numlocalposeparameters;
		int                                     localposeparamindex;

		int                                     surfacepropindex;

		int                                     keyvalueindex;
		int                                     keyvaluesize;


		int                                     numlocalikautoplaylocks;
		int                                     localikautoplaylockindex;

		float                           mass;
		int                                     contents;

		int                                     numincludemodels;
		int                                     includemodelindex;

		mutable void            *virtualModel;

		int                                     szanimblocknameindex;
		int                                     numanimblocks;
		int                                     animblockindex;

		mutable void            *animblockModel;

		int                                     bonetablebynameindex;

		void                            *pVertexBase;
		void                            *pIndexBase;

		byte                            constdirectionallightdot;

		byte                            rootLOD;

		byte                            numAllowedRootLODs;

		byte                            unused[1];

		int                                     unused4;

		int                                     numflexcontrollerui;
		int                                     flexcontrolleruiindex;
		float                           flVertAnimFixedPointScale;
		int                                     unused3[1];
		int                                     studiohdr2index;
		int                                     unused2[1];
	};*/

/*struct studiohdr_t
{
	int                                     id;
	int                                     version;

	int                                     checksum;

	char                            name[64];
	int                                     length;


	Vector                          eyeposition;

	Vector                          illumposition;

	Vector                          hull_min;
	Vector                          hull_max;

	Vector                          view_bbmin;
	Vector                          view_bbmax;

	int                                     flags;

	int                                     numbones;
	int                                     boneindex;

	inline mstudiobone_t *GetBone(int i) const { return (mstudiobone_t *)(((BYTE *)this) + boneindex) + i; };
	//	inline mstudiobone_t *pBone(int i) const { Assert(i >= 0 && i < numbones); return (mstudiobone_t *)(((byte *)this) + boneindex) + i; };

	int                                     numbonecontrollers;
	int                                     bonecontrollerindex;

	int                                     numhitboxsets;
	int                                     hitboxsetindex;

	mstudiohitboxset_t* GetHitboxSet(int i) const
	{
		return (mstudiohitboxset_t*)(((BYTE*)this) + hitboxsetindex) + i;
	}

	inline mstudiobbox_t* GetHitbox(int i, int set) const
	{
		mstudiohitboxset_t const* s = GetHitboxSet(set);

		if (!s)
			return NULL;

		return s->GetHitbox(i);
	}

	inline int GetHitboxCount(int set) const
	{
		mstudiohitboxset_t const* s = GetHitboxSet(set);

		if (!s)
			return 0;

		return s->numhitboxes;
	}

	int                                     numlocalanim;
	int                                     localanimindex;

	int                                     numlocalseq;
	int                                     localseqindex;

	mutable int                     activitylistversion;
	mutable int                     eventsindexed;

	int                                     numtextures;
	int                                     textureindex;

	int                                     numcdtextures;
	int                                     cdtextureindex;

	int                                     numskinref;
	int                                     numskinfamilies;
	int                                     skinindex;

	int                                     numbodyparts;
	int                                     bodypartindex;

	int                                     numlocalattachments;
	int                                     localattachmentindex;

	int                                     numlocalnodes;
	int                                     localnodeindex;
	int                                     localnodenameindex;

	int                                     numflexdesc;
	int                                     flexdescindex;

	int                                     numflexcontrollers;
	int                                     flexcontrollerindex;

	int                                     numflexrules;
	int                                     flexruleindex;

	int                                     numikchains;
	int                                     ikchainindex;

	int                                     nummouths;
	int                                     mouthindex;

	int                                     numlocalposeparameters;
	int                                     localposeparamindex;

	int                                     surfacepropindex;

	int                                     keyvalueindex;
	int                                     keyvaluesize;


	int                                     numlocalikautoplaylocks;
	int                                     localikautoplaylockindex;

	float                           mass;
	int                                     contents;

	int                                     numincludemodels;
	int                                     includemodelindex;

	mutable void            *virtualModel;

	int                                     szanimblocknameindex;
	int                                     numanimblocks;
	int                                     animblockindex;

	mutable void            *animblockModel;

	int                                     bonetablebynameindex;

	void                            *pVertexBase;
	void                            *pIndexBase;

	BYTE                            constdirectionallightdot;

	BYTE                            rootLOD;

	BYTE                            numAllowedRootLODs;

	BYTE                            unused[1];

	int                                     unused4;

	int                                     numflexcontrollerui;
	int                                     flexcontrolleruiindex;
	float                           flVertAnimFixedPointScale;
	int                                     unused3[1];
	int                                     studiohdr2index;
	int                                     unused2[1];
};*/
struct studiohdr_t
{
	int	id;
	int	version;

	long checksum;	// this has to be the same in the phy and vtx files to load!

	char name[64];
	int	length;

	Vector	eyeposition;	// ideal eye position

	Vector	illumposition;	// illumination center

	Vector	hull_min;	// ideal movement hull size
	Vector	hull_max;

	Vector	view_bbmin;	// clipping bounding box
	Vector	view_bbmax;

	int	flags;

	int	numbones;	// bones
	int	boneindex;
	inline mstudiobone_t *pBone(int i) const { return (mstudiobone_t *)(((byte *)this) + boneindex) + i; };

	int	numbonecontrollers;	// bone controllers
	int	bonecontrollerindex;
	inline void *pBonecontroller(int i) const { return (((byte *)this) + bonecontrollerindex) + i; };

	int	numhitboxsets;
	int	hitboxsetindex;

	// Look up hitbox set by index
	mstudiohitboxset_t  *pHitboxSet(int i) const
	{
		return (mstudiohitboxset_t *)(((byte *)this) + hitboxsetindex) + i;
	};

	// Calls through to hitbox to determine size of specified set
	inline mstudiobbox_t *pHitbox(int i, int set) const
	{
		mstudiohitboxset_t *s = pHitboxSet(set);

		if (!s)
			return NULL;

		return s->GetHitbox(i);
	};

	// Calls through to set to get hitbox count for set
	inline int  iHitboxCount(int set) const
	{
		mstudiohitboxset_t const *s = pHitboxSet(set);
		if (!s)
			return 0;

		return s->numhitboxes;
	};

	int	numanim;	// animations/poses
	int	animdescindex;	// animation descriptions
	inline void *pAnimdesc(int i) const { return (((byte *)this) + animdescindex) + i; };

	int 	numanimgroup;
	int 	animgroupindex;
	inline  void *pAnimGroup(int i) const { return (((byte *)this) + animgroupindex) + i; };

	int 	numbonedesc;
	int 	bonedescindex;
	inline  void *pBoneDesc(int i) const { return (((byte *)this) + bonedescindex) + i; };

	int	numseq;		// sequences
	int	seqindex;
	inline mstudioseqdesc_t *pSeqdesc(int i) const { if (i < 0 || i >= numseq) i = 0; return (mstudioseqdesc_t *)(((byte *)this) + seqindex) + i; };
	int	sequencesindexed;	// initialization flag - have the sequences been indexed?

	int	numseqgroups;		// demand loaded sequences
	int	seqgroupindex;
	inline  void *pSeqgroup(int i) const { return (((byte *)this) + seqgroupindex) + i; };

	int	numtextures;		// raw textures
	int	textureindex;
	inline void *pTexture(int i) const { return (((byte *)this) + textureindex) + i; };

	int	numcdtextures;		// raw textures search paths
	int	cdtextureindex;
	inline char			*pCdtexture(int i) const { return (((char *)this) + *((int *)(((byte *)this) + cdtextureindex) + i)); };

	int	numskinref;		// replaceable textures tables
	int	numskinfamilies;
	int	skinindex;
	inline short		*pSkinref(int i) const { return (short *)(((byte *)this) + skinindex) + i; };

	int	numbodyparts;
	int	bodypartindex;
	inline void	*pBodypart(int i) const { return (((byte *)this) + bodypartindex) + i; };

	int	numattachments;		// queryable attachable points
	int	attachmentindex;
	inline void	*pAttachment(int i) const { return (((byte *)this) + attachmentindex) + i; };

	int	numtransitions;		// animation node to animation node transition graph
	int	transitionindex;
	inline byte	*pTransition(int i) const { return (byte *)(((byte *)this) + transitionindex) + i; };

	int	numflexdesc;
	int	flexdescindex;
	inline void *pFlexdesc(int i) const { return (((byte *)this) + flexdescindex) + i; };

	int	numflexcontrollers;
	int	flexcontrollerindex;
	inline void *pFlexcontroller(int i) const { return (((byte *)this) + flexcontrollerindex) + i; };

	int	numflexrules;
	int	flexruleindex;
	inline void *pFlexRule(int i) const { return (((byte *)this) + flexruleindex) + i; };

	int	numikchains;
	int	ikchainindex;
	inline void *pIKChain(int i) const { return (((byte *)this) + ikchainindex) + i; };

	int	nummouths;
	int	mouthindex;
	inline void *pMouth(int i) const { return (((byte *)this) + mouthindex) + i; };

	int	numposeparameters;
	int	poseparamindex;
	inline void *pPoseParameter(int i) const { return (((byte *)this) + poseparamindex) + i; };

	int	surfacepropindex;
	inline char * const pszSurfaceProp(void) const { return ((char *)this) + surfacepropindex; }

	// Key values
	int	keyvalueindex;
	int	keyvaluesize;
	inline const char * KeyValueText(void) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

	int	numikautoplaylocks;
	int	ikautoplaylockindex;
	inline void *pIKAutoplayLock(int i) const { return (((byte *)this) + ikautoplaylockindex) + i; };

	float mass;		// The collision model mass that jay wanted
	int	contents;
	int	unused[9];	// remove as appropriate
};
	//0x3F00FB33
	class CModelInfo
	{
	public:
		studiohdr_t* GetStudioModel(const model_t* Model)
		{
			typedef studiohdr_t*(__thiscall* Fn)(void*, const model_t*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 32)(this, Model);
		}

		char* GetModelName(const model_t *Model)
		{
			typedef char*(__thiscall* Fn)(void*, const model_t*);
			return VMT::VMTHookManager::GetFunction<Fn>(this, 3)(this, Model);
		}

		void GetModelMaterials(const model_t *model, int count, IMaterial** ppMaterial)
		{
			typedef char*(__thiscall* Fn)(void*, const model_t*, int, IMaterial**);
			VMT::VMTHookManager::GetFunction<Fn>(this, 19)(this, model, count, ppMaterial);
		}
	};
}