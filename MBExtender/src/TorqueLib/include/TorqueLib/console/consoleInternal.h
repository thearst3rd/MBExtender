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

#include <TorqueLib/console/console.h>
#include <TorqueLib/console/consoleObject.h>
#include <TorqueLib/core/tVector.h>

namespace TGE
{
	enum CompiledInstructions
	{
	   OP_FUNC_DECL,
	   OP_CREATE_OBJECT,
	   OP_CREATE_DATABLOCK,
   
	   OP_NAME_OBJECT,
	   OP_ADD_OBJECT,
	   OP_END_OBJECT,
	   OP_JMPIFFNOT,
	   OP_JMPIFNOT,
	   OP_JMPIFF,
	   OP_JMPIF,
	   OP_JMPIFNOT_NP,
	   OP_JMPIF_NP,
	   OP_JMP,
	   OP_RETURN,
	   OP_CMPEQ,
	   OP_CMPGR,
	   OP_CMPGE,
	   OP_CMPLT,
	   OP_CMPLE,
	   OP_CMPNE,
	   OP_XOR,
	   OP_MOD,
	   OP_BITAND,
	   OP_BITOR,
	   OP_NOT,
	   OP_NOTF,
	   OP_ONESCOMPLEMENT,

	   OP_SHR,
	   OP_SHL,
	   OP_AND,
	   OP_OR,

	   OP_ADD,
	   OP_SUB,
	   OP_MUL,
	   OP_DIV,
	   OP_NEG,

	   OP_SETCURVAR,
	   OP_SETCURVAR_CREATE,
	   OP_SETCURVAR_ARRAY,
	   OP_SETCURVAR_ARRAY_CREATE,

	   OP_LOADVAR_UINT,
	   OP_LOADVAR_FLT,
	   OP_LOADVAR_STR,

	   OP_SAVEVAR_UINT,
	   OP_SAVEVAR_FLT,
	   OP_SAVEVAR_STR,

	   OP_SETCUROBJECT,
	   OP_SETCUROBJECT_NEW,

	   OP_SETCURFIELD,
	   OP_SETCURFIELD_ARRAY,

	   OP_LOADFIELD_UINT,
	   OP_LOADFIELD_FLT,
	   OP_LOADFIELD_STR,

	   OP_SAVEFIELD_UINT,
	   OP_SAVEFIELD_FLT,
	   OP_SAVEFIELD_STR,

	   OP_STR_TO_UINT,
	   OP_STR_TO_FLT,
	   OP_STR_TO_NONE,
	   OP_FLT_TO_UINT,
	   OP_FLT_TO_STR,
	   OP_FLT_TO_NONE,
	   OP_UINT_TO_FLT,
	   OP_UINT_TO_STR,
	   OP_UINT_TO_NONE,

	   OP_LOADIMMED_UINT,
	   OP_LOADIMMED_FLT,
	   OP_TAG_TO_STR,
	   OP_LOADIMMED_STR,
	   OP_LOADIMMED_IDENT,
   
	   OP_CALLFUNC_RESOLVE,
	   OP_CALLFUNC,
	   OP_PROCESS_ARGS,
   
	   OP_ADVANCE_STR,
	   OP_ADVANCE_STR_APPENDCHAR,
	   OP_ADVANCE_STR_COMMA,
	   OP_ADVANCE_STR_NUL,
	   OP_REWIND_STR,
	   OP_TERMINATE_REWIND_STR,
	   OP_COMPARE_STR,
   
	   OP_PUSH,
	   OP_PUSH_FRAME,
   
	   OP_BREAK,
   
	   OP_INVALID
	};



	class CodeBlock;
	class ExprEvalState;
	class SimObject;
	class NamespaceEntry;

	class Namespace
	{
		BRIDGE_CLASS(Namespace);
	public:
		const char* mName;
		const char* mPackage;

		Namespace* mParent;
		Namespace* mNext;
		AbstractClassRep* mClassRep;
		U32 mRefCountToParent;
		NamespaceEntry* mEntryList;

		STATICFN(void, init, (), 0x407CE3_win, 0x348D0_mac);
		STATICFN(Namespace *, find, (const char* name, const char* package), 0x403B93_win, 0x33710_mac);
		STATICFN(void, shutdown, (), 0x4026E9_win, 0x33960_mac);
		MEMBERFN(NamespaceEntry*, lookup, (const char* name), 0x408008_win, 0x335A0_mac);
	};

	class NamespaceEntry
	{
		BRIDGE_CLASS(NamespaceEntry);
	public:
		Namespace* mNamespace;
		NamespaceEntry* mNext;
		const char* mFunctionName;
		S32 mType;
		S32 mMinArgs;
		S32 mMaxArgs;
		const char* mUsage;
		const char* mPackage;

		MEMBERFN(const char*, execute, (S32 argc, const char** argv, void* state), 0x4073AB_win, 0x32BF0_mac);
	};

	class Dictionary
	{
		BRIDGE_CLASS(Dictionary);
	public:
		struct Entry
		{
			enum
			{
				TypeInternalInt = -3,
				TypeInternalFloat = -2,
				TypeInternalString = -1,
			};

			StringTableEntry name;
			Entry *nextEntry;
			S32 type;
			char *sval;
			U32 ival;  // doubles as strlen when type = -1
			F32 fval;
			U32 bufferLen;
			void *dataPtr;

			const char *getStringValue()
			{
				if (type == TypeInternalString)
					return sval;
				if (type == TypeInternalFloat)
					return Con::getData(5, &fval, 0, nullptr, nullptr);
				else if (type == TypeInternalInt)
					return Con::getData(1, &ival, 0, nullptr, nullptr);
				else
					return Con::getData(type, dataPtr, 0, nullptr, nullptr);
			}
		};

		S32 hashTableSize;
		S32 hashTableCount;
		Dictionary::Entry **hashTableData;
		ExprEvalState *exprState;
		StringTableEntry scopeName;
		Namespace *scopeNamespace;

		// Note: code and ip are only initialized if this isn't the top of the
		// stack!

		CodeBlock *code;
		U32 ip;

		MEMBERFN(Dictionary::Entry*, add, (StringTableEntry name), 0x4068E3_win, 0x33AF0_mac);
		MEMBERFN(Dictionary::Entry*, lookup, (StringTableEntry name), 0x4066CC_win, 0x33000_mac);
		MEMBERFN(const char*, getVariable, (StringTableEntry name, bool* entValid), 0x401924_win, 0x337F0_mac);
	};

	class ExprEvalState
	{
		BRIDGE_CLASS(ExprEvalState);
	public:
		SimObject *thisObject;
		Dictionary::Entry *currentVariable;
		bool traceOn;
		Dictionary globalVars;
		Vector<Dictionary *> stack;

		MEMBERFN(void, pushFrame, (StringTableEntry frameName, Namespace *ns), 0x40417E_win, 0x34520_mac);
		MEMBERFN(void, popFrame, (), 0x40417E_win, 0x34F40_mac);
	};

	GLOBALVAR(ExprEvalState, gEvalState, 0x694A70_win, 0x2FF2C0_mac);
	GLOBALVAR(StringTableEntry*, mActivePackages, 0x698518_win, 0x2FEA80_mac);
	GLOBALVAR(U32, mNumActivePackages, 0x6991D4_win, 0x2DA518_mac);
}
