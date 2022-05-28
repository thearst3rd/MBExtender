//-----------------------------------------------------------------------------
// Copyright (c) 2021 The Platinum Team
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

#include <TorqueLib/math/mMatrix.h>
#include <TorqueLib/math/mPoint3.h>
#include <TorqueLib/core/resManager.h>

namespace TGE
{

	namespace Audio {
		struct Description
		{
			F32  mVolume;    // 0-1    1=loudest volume
			bool mIsLooping;
			bool mIsStreaming;
			bool mIs3D;

			F32  mReferenceDistance;
			F32  mMaxDistance;
			U32  mConeInsideAngle;
			U32  mConeOutsideAngle;
			F32  mConeOutsideVolume;
			Point3F mConeVector;

			// environment info
			F32 mEnvironmentLevel;

			// used by 'AudioEmitter' class
			S32  mLoopCount;
			S32  mMinLoopGap;
			S32  mMaxLoopGap;

			// each 'type' can have its own volume
			S32  mType;
		};
	}

	class AudioBuffer : ResourceInstance {
		StringTableEntry mFilename;
		bool mLoading;
		int malBuffer;
	};

	struct LoopingImage
	{
	   int             mHandle;
	   Resource<AudioBuffer>   mBuffer;
	   Audio::Description      mDescription;
	   void* mEnvironment;

	   Point3F                 mPosition;
	   Point3F                 mDirection;
	   F32                     mPitch;
	   F32                     mScore;
	   U32                     mCullTime;
	};

	struct LoopingList : VectorPtr<LoopingImage*> {

	};

	class AudioStreamSource {
	public:
		int             mHandle;
		int				    mSource;

		Audio::Description      mDescription;
		void* mEnvironment;

		Point3F                 mPosition;
		Point3F                 mDirection;
		F32                     mPitch;
		F32                     mScore;
		U32                     mCullTime;

		bool					bFinishedPlaying;
		bool					bIsValid;
		const char* mFilename;
	};

	struct StreamingList : VectorPtr<AudioStreamSource*> {

	};

	FN(int, alxPlay, (void *profile, MatrixF *mat, Point3F *point), 0x408733_win, 0xD130_mac);
	FN(bool, cullSource, (int* index, float volume), 0x5EA490_win, 0x85B0_mac);

#if defined(_WIN32)
	GLOBALVAR(LoopingList, mLoopingList, 0x6E3BA0_win);
	GLOBALVAR(StreamingList, mStreamingList, 0x6E3E68_win);
#endif
	GLOBALVAR(U32, mAudioNumSources, 0x6E3E90_win, 0x3134C0_mac);
}
