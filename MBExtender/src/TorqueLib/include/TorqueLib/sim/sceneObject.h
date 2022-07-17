//-----------------------------------------------------------------------------
// Copyright (c) 2021 The Platinum Team
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#pragma once

#include <MBExtender/InteropMacros.h>
#include <TorqueLib/platform/platform.h>

#include <TorqueLib/collision/collision.h>
#include <TorqueLib/math/mMatrix.h>
#include <TorqueLib/sim/netObject.h>
#include <TorqueLib/core/tVector.h>
#include <TorqueLib/core/color.h>
#include <TorqueLib/dgl/materialList.h>

namespace TGE
{
	class SceneRenderImage;
	class SceneState;
	class SceneGraph;

	struct RayInfo : public Collision
	{
		// The collision struct has object, point, normal & material.

		/// Distance along ray to contact point.
		F32 t;

		/// Set the point of intersection according to t and the given ray.
		///
		/// Several pieces of code will not use ray information but rather rely
		/// on contact points directly, so it is a good thing to always set
		/// this in castRay functions.
		void setContactPoint(const Point3F& start, const Point3F& end)
		{
			Point3F startToEnd = end - start;
			startToEnd *= t;
			point = startToEnd + start;
		}

		RayInfo() : t(0.0f) {}
	};

	class Container
	{
		BRIDGE_CLASS(Container);
		 typedef void (*FindCallback)(SceneObject*, void* key);
	public:
		MEMBERFN(bool, castRay, (const Point3F &start, const Point3F &end, U32 mask, RayInfo *info), 0x403652_win, 0x192B80_mac);
		MEMBERFN(void, findObjects, (const Box3F& box, U32 mask, FindCallback, void* key), 0x402DFB_win, 0x18F760_mac);
		struct Link
		{
			Link* next;
			Link* prev;
		};
	};

	class SceneObjectRef
	{
	public:
		class SceneObject *object;
		SceneObjectRef *nextInBin;
		SceneObjectRef *prevInBin;
		SceneObjectRef *nextInObj;
		U32 zone;
	};

	class SceneObject : public NetObject
	{
		BRIDGE_CLASS(SceneObject);
	public:

		enum SceneObjectMasks {
			ScaleMask = 1 << 0,
			NextFreeMask = ScaleMask << 1
		};

		struct LightingInfo
		{
			bool                       mUseInfo;
			bool                       mDirty;
			ColorF                     mDefaultColor;
			ColorF                     mAlarmColor;

			SimObjectPtr<SceneObject>  mInterior;

			bool                       mHasLastColor;
			ColorF                     mLastColor;
			U32                        mLastTime;

			enum {
				Interior = 0,
				Terrain,
			};
			U32                        mLightSource;
		};
		enum TraversalState {
			Pending = 0,
			Working = 1,
			Done = 2
		};

		Container::Link link; // 4c
		LightingInfo mLightingInfo; // 54
		Container *mContainer; //98
		MatrixF mObjToWorld; // 9c
		MatrixF mWorldToObj; // dc
		Point3F mObjScale; // 11c

		Box3F mObjBox; // 128
		Box3F mWorldBox; // 140
		SphereF mWorldSphere; // 158

		MatrixF mRenderObjToWorld; // 168
		MatrixF mRenderWorldToObj; // 1a8
		Box3F mRenderWorldBox; // 1e8
		SphereF mRenderWorldSphere; // 200

		SceneObjectRef *mZoneRefHead; // 210
		SceneObjectRef *mBinRefHead; // 214

		U32 mBinMinX; // 218
		U32 mBinMaxX; // 21c
		U32 mBinMinY; // 220
		U32 mBinMaxY; // 224

		U32 mContainerSeqKey; // 228
		S32 mCollisionCount; // 22c
		SceneGraph *mSceneManager; // 230
		U32 mZoneRangeStart; // 234
		U32 mNumCurrZones; // 238
		TraversalState mTraversalState; // 23c
		SceneState *mLastState; // 240
		U32 mLastStateKey; // 244

		// Full size: 248

		virtual void disableCollision() = 0;
		virtual void enableCollision() = 0;
		UNDEFVIRT(isDisplacable);
		UNDEFVIRT(getMomentum);
		UNDEFVIRT(setMomentum);
		UNDEFVIRT(getMass);
		UNDEFVIRT(displaceObject);
		virtual void setTransformVirt(const MatrixF &transform) = 0;
		UNDEFVIRT(setScale);
		UNDEFVIRT(setRenderTransform);
		UNDEFVIRT(buildConvex);
		virtual bool buildPolyList(void*, const Box3F&, const SphereF&) = 0;
		UNDEFVIRT(buildCollisionBSP);
		virtual bool castRay(const Point3F &start, const Point3F &end, RayInfo* info) = 0;
		UNDEFVIRT(collideBox);
		UNDEFVIRT(getOverlappingZones);
		UNDEFVIRT(getPointZone);
		UNDEFVIRT(renderShadowVolumes);
		virtual void renderObject(SceneState *state, SceneRenderImage *renderImage) = 0;
		UNDEFVIRT(prepRenderImage);
		UNDEFVIRT(scopeObject);
		virtual Material* getMaterialProperty(U32 which) = 0;
		MEMBERFN(Material*, getMaterialPropertyUnvirt, (U32 which), 0x18dc20_mac, 0x403288_win);
		UNDEFVIRT(onSceneAdd);
		UNDEFVIRT(onSceneRemove);
		UNDEFVIRT(transformModelview);
		UNDEFVIRT(transformPosition);
		UNDEFVIRT(computeNewFrustum);
		UNDEFVIRT(openPortal);
		UNDEFVIRT(closePortal);
		UNDEFVIRT(getWSPortalPlane);
		UNDEFVIRT(installLights);
		UNDEFVIRT(uninstallLights);
		UNDEFVIRT(getLightingAmbientColor);

		MEMBERFN(void, setTransform, (const MatrixF &transform), 0x401226_win, 0x1909A0_mac);
		MEMBERFN(void, setRenderTransform, (const MatrixF &transform), 0x402040_win, 0x190E20_mac);

		//		MEMBERFN(void, renderObject, (void *sceneState, void *sceneRenderImage), (sceneState, sceneRenderImage), 0x4E5CD0_win, 0xA5F00_mac);

		MEMBERFN(void, setScale, (const VectorF &scale), 0x4091CE_win, 0x18DD10_mac);


		/// Returns the transform which can be used to convert object space
		/// to world space
		const MatrixF& getTransform() const      { return mObjToWorld; }

		/// Returns the transform which can be used to convert world space
		/// into object space
		const MatrixF& getWorldTransform() const { return mWorldToObj; }

		/// Returns the scale of the object
		const VectorF& getScale() const          { return mObjScale;   }

		/// Returns the bounding box for this object in local coordinates
		const Box3F&   getObjBox() const        { return mObjBox;      }

		/// Returns the bounding box for this object in world coordinates
		const Box3F&   getWorldBox() const      { return mWorldBox;    }
	};

	FN(void, cSetTransform, (TGE::SimObject *obj, int argc, const char **argv), 0x402CB1_win, 0x18EE70_mac);

	GLOBALVAR(Container, gClientContainer, 0x6E1838_win, 0x310560_mac);
	GLOBALVAR(Container, gServerContainer, 0x6E1760_win, 0x3105C0_mac);
}
