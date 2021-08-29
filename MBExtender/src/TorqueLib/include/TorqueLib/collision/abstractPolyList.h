#pragma once
#include <TorqueLib/math/mMatrix.h>
#include <TorqueLib/math/mPoint3.h>
#include <TorqueLib/sim/sceneObject.h>

namespace TGE 
{
	class PlaneTransformer
	{
		MatrixF mTransform;
		Point3F mScale;

		MatrixF mTransposeInverse;
	};

	class AbstractPolyList
	{
	protected:
		// User set state
		SceneObject* mCurrObject;

		MatrixF  mBaseMatrix;               // Base transform
		MatrixF  mTransformMatrix;          // Current object transform
		MatrixF  mMatrix;                   // Base * current transform
		Point3F  mScale;

		PlaneTransformer mPlaneTransformer;

		bool     mInterestNormalRegistered;
		Point3F  mInterestNormal;

		virtual ~AbstractPolyList() {}
	};

	class ConcretePolyList : public AbstractPolyList
	{
	public:
		BRIDGE_CLASS(ConcretePolyList);

	public:
		struct Poly {
			PlaneF plane;
			TGE::SceneObject* object;
			U32 material;
			U32 vertexStart;
			U32 vertexCount;
			U32 surfaceKey;
		};

		typedef TGE::Vector<PlaneF> PlaneList;
		typedef TGE::Vector<Point3F> VertexList;
		typedef TGE::Vector<Poly>   PolyList;
		typedef TGE::Vector<U32>    IndexList;

		PolyList   mPolyList;
		VertexList mVertexList;
		IndexList  mIndexList;

		PlaneList  mPolyPlaneList;

		CONSTRUCTOR((), 0x408404_win, 0x24F200_mac);
	};
}