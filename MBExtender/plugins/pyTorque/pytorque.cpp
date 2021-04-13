#include <MBExtender/MBExtender.h>
#include <TorqueLib/console/console.h>
#include <TorqueLib/console/simBase.h>
#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/console/scriptObject.h>
#include <fstream>
#include <sstream> //std::stringstream

#include "../../external/python/include/Python.h"
#include <MathLib/MathLib.h>
#include <TorqueLib/console/consoleInternal.h>
#include <map>

static std::map<PyObject*, TGE::SimObject*> allocObjects;
static std::map<const char*, PyObject*> exportedFunctions;

typedef struct {
	PyObject_HEAD
	TGE::SimObject* simObject;
} TorqueObject;


static PyTypeObject TorqueObjectType = { 
	PyVarObject_HEAD_INIT(NULL, 0)					
	"pytorque.TorqueObject"   /* tp_name */,
};

typedef struct {
	PyObject_HEAD
	TGE::NamespaceEntry* functionEntry;
	TGE::SimObject* callerObj;
} TorqueFunction;

static PyTypeObject TorqueFunctionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pytorque.TorqueFunction"
};

static void TorqueFunction_dealloc(TorqueFunction* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* TorqueFunction_repr(TorqueFunction* self)
{
	return PyUnicode_FromString(self->functionEntry->mFunctionName);
}

std::string parseArg(PyObject* o)
{
	std::string arg;
	if (PyUnicode_Check(o))
		arg = std::string(PyUnicode_AsUTF8(o));
	else
	{
		if (o->ob_type->tp_iter != NULL)
		{
			PyObject* iterator = PyObject_GetIter(o);
			PyObject* item;

			std::string valarg = std::string("");

			if (iterator != NULL) {

				while ((item = PyIter_Next(iterator))) {

					if (valarg != "")
					{
						valarg += " ";
					}
					valarg += parseArg(item);
					Py_DECREF(item);
				}

				Py_DECREF(iterator);
			}

			arg = valarg;
		}
		else
		{
			PyObject* str = PyObject_Str(o);
			arg = std::string(PyUnicode_AsUTF8(str));
			Py_DECREF(str);
		}
	}
	return arg;
}

static PyObject* TorqueFunction_call(TorqueFunction* self, PyObject* args, PyObject* kw)
{
	const char* argv[32];
	S32 argc = 2;

	argv[0] = self->functionEntry->mFunctionName;
	argv[1] = self->callerObj == NULL ? "" :self->callerObj->getIdString();

	for (int i = 0; i < PyTuple_Size(args) && i < 32; i++)
	{
		argc++;
		if (self->functionEntry->mMaxArgs > 0 && argc > self->functionEntry->mMaxArgs)
		{
			if (self->callerObj != NULL)
			{
				PyErr_Format(PyExc_SyntaxError, "TorqueFunction %s of class %s max arguments exceeded", self->functionEntry->mFunctionName, self->callerObj->getClassRep()->getClassName());
			}
			else
			{
				PyErr_Format(PyExc_SyntaxError, "TorqueFunction %s max arguments exceeded", self->functionEntry->mFunctionName);
			}
			return NULL;
		}

		PyObject* o = PyTuple_GetItem(args, i);
		std::string arg = parseArg(o);
		argv[i + 2] = TGE::StringTable->insert(arg.c_str(), false);
	}

	if (argc < self->functionEntry->mMinArgs)
	{
		if (self->callerObj != NULL)
		{
			PyErr_Format(PyExc_SyntaxError, "TorqueFunction %s of class %s not enough arguments", self->functionEntry->mFunctionName, self->callerObj->getClassRep()->getClassName());
		}
		else
		{
			PyErr_Format(PyExc_SyntaxError, "TorqueFunction %s not enough arguments", self->functionEntry->mFunctionName);
		}
		return NULL;
	}

	TGE::SimObject* save = TGE::gEvalState.thisObject;
	TGE::gEvalState.thisObject = self->callerObj;
	const char* ret = self->functionEntry->execute(argc, argv, &TGE::gEvalState);
	TGE::gEvalState.thisObject = save;

	return PyUnicode_FromString(ret);
}

static PyObject* TorqueObject_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	TorqueObject* self;
	self = (TorqueObject*)TorqueObjectType.tp_alloc(type, 0);
	if (self != NULL)
	{
		TGE::ScriptObject* object = TGE::ScriptObject::create();
		object->mClassName = "PyTorqueObject";
		object->mFlags |= TGE::SimObject::ModDynamicFields | TGE::SimObject::ModStaticFields;
		object->registerObject();
		self->simObject = object;

		allocObjects.insert(std::pair<PyObject*, TGE::SimObject*>((PyObject*)self, (TGE::SimObject*)object));
	}
	return (PyObject*)self;
}

static void TorqueObject_dealloc(TorqueObject* self)
{
	for (auto& it : allocObjects)
	{
		if (it.first == (PyObject*)self)
		{
			self->simObject->deleteObject();
			allocObjects.erase((PyObject*)self);
		}
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}

TGE::NamespaceEntry* lookupFunction(TGE::Namespace* nmspace, StringTableEntry func)
{
	TGE::NamespaceEntry* curEntry = nmspace->mEntryList;
	while (curEntry != NULL)
	{
		if (strcmp(curEntry->mFunctionName,func)==0)
		{
			return curEntry;
		}
		curEntry = curEntry->mNext;
	}
	// Couldnt find, try parents
	if (nmspace->mParent != NULL)
	{
		return lookupFunction(nmspace->mParent, func);
	}
	return NULL;
}

static PyObject* TorqueObject_getattr(TorqueObject* self, char* attr)
{
	// Copied from getFieldValue

	//What field are we searching for?
	const char* fieldName = attr;
	bool hasIndex = false;
	U32 index = 0;
	char tempName[2048];

	if (strstr(fieldName, "[") != NULL) {
		//It's an array. Get a value at an array index
		//Crazy sscanf extracts the index from the braces and the name from before
		sscanf(fieldName, "%[^[][%d]", tempName, &index);
		fieldName = tempName;
		hasIndex = true;
	}

	//Iterate until we find it
	TGE::AbstractClassRep::FieldList list = self->simObject->getClassRep()->getFieldList();
	for (S32 i = 0; i < list.size(); i++) {
		const TGE::AbstractClassRep::Field field = list[i];
		//Does it match (case insensitive)
		if (_stricmp(field.pFieldname, fieldName) == 0) {
			const char* buf = TGE::Con::getData(field.type, (void*)(((const char*)self->simObject) + field.offset), index, field.table, &field.flag);
			return PyUnicode_FromString(buf);
		}
	}

	//Try to get script fields...
	if (hasIndex) {
		const char* field = self->simObject->getDataField(TGE::StringTable->insert(fieldName, false), TGE::StringTable->insert(StringMath::print(index), false));
		if (strcmp(field, "") != 0) {
			return PyUnicode_FromString(field);
		}
		//Fallback try using the non-index field
		fieldName = attr;
	}

	TGE::Namespace* gFunctionNameSpace = self->simObject->mNamespace;
	AssertFatal(gFunctionNameSpace, "SimObject with no namespace");

	const char* gFunctionString = fieldName;

	TGE::NamespaceEntry* gFunctionEntry = lookupFunction(gFunctionNameSpace,gFunctionString);

	if (gFunctionEntry)
	{

		TorqueFunction* tf;
		tf = (TorqueFunction*)TorqueFunctionType.tp_alloc(&TorqueFunctionType, 0);
		if (tf != NULL)
		{
			tf->callerObj = self->simObject;
			tf->functionEntry = gFunctionEntry;
			Py_INCREF(tf);
			return (PyObject*)tf;
		}
		return Py_None;
	}

	return PyUnicode_FromString(self->simObject->getDataField(TGE::StringTable->insert(fieldName, false), NULL));
}

static int TorqueObject_setattr(TorqueObject* self, char* attr, PyObject* pyValue)
{
	// Once again taken from setFieldValue

	const char* fieldName = attr;
	U32 index = 0;
	bool hasIndex = false;
	char tempName[2048];

	if (strstr(fieldName, "[") != NULL) {
		//It's an array. Get a value at an array index
		//Crazy sscanf extracts the index from the braces and the name from before
		sscanf(fieldName, "%[^[][%d]", tempName, &index);
		fieldName = tempName;
		hasIndex = true;
	}

	const char* value = PyUnicode_AsUTF8(pyValue);

	//Iterate until we find it
	TGE::AbstractClassRep::FieldList list = self->simObject->getClassRep()->getFieldList();
	for (S32 i = 0; i < list.size(); i++) {
		const TGE::AbstractClassRep::Field field = list[i];
		//Does it match (case insensitive)
		if (strcasecmp(field.pFieldname, fieldName) == 0) {
			const char* argv[1];
			argv[0] = &value[0];

			TGE::Con::setData(field.type, (void*)(((const char*)self->simObject) + field.offset), index, 1, argv, field.table, &field.flag);
			return 0;
		}
	}

	if (hasIndex)
		self->simObject->setDataField(TGE::StringTable->insert(fieldName, false), TGE::StringTable->insert(StringMath::print(index), false), TGE::StringTable->insert(value, true));
	else
		self->simObject->setDataField(TGE::StringTable->insert(fieldName, false), NULL, TGE::StringTable->insert(value, true));

	return 0;
}

static PyObject* TorqueObject_repr(TorqueObject* self)
{
	return PyUnicode_FromString(self->simObject->getIdString());
}

// pytorque.echo(string);
static PyObject* pyTorque_echo(PyObject* self, PyObject* args)
{
	const char* text;
	PyArg_ParseTuple(args, "s", &text);
	TGE::Con::printf(text);
	return Py_None;
}

// pytorque.findObject(string) -> TorqueObject;
static PyObject* pyTorque_findObject(PyObject* self, PyObject* args)
{
	const char* name;
	PyArg_ParseTuple(args, "s", &name);
	TGE::SimObject* obj = TGE::Sim::findObject(name);
	if (obj != NULL)
	{
		TorqueObject* self;
		self = (TorqueObject*)TorqueObjectType.tp_alloc(&TorqueObjectType, 0);
		if (self != NULL)
		{
			self->simObject = obj;
			return (PyObject*) self;
		}
		return Py_None;
	}
	else
	{
		return Py_None;
	}
}

// pytorque.findFunction(string) -> TorqueFunction;
static PyObject* pyTorque_findFunction(PyObject* self, PyObject* args)
{
	const char* name;
	PyArg_ParseTuple(args, "s", &name);
	TGE::NamespaceEntry* func = lookupFunction(TGE::Con::lookupNamespace(NULL), TGE::StringTable->insert(name, false));
	TorqueFunction* tf;
	tf = (TorqueFunction*)TorqueFunctionType.tp_alloc(&TorqueFunctionType, 0);
	if (tf != NULL)
	{
		tf->callerObj = NULL;
		tf->functionEntry = func;
		Py_INCREF(tf);
		return (PyObject*)tf;
	}
	return Py_None;
}

// pytorque.getGlobal(string) -> string;
static PyObject* pyTorque_getGlobal(PyObject* self, PyObject* args)
{
	const char* name;
	PyArg_ParseTuple(args, "s", &name);
	return PyUnicode_FromString(TGE::Con::getVariable(name));
}

// pytorque.setGlobal(global, value);
static PyObject* pyTorque_setGlobal(PyObject* self, PyObject* args)
{
	const char* name;
	const char* value;
	PyArg_ParseTuple(args, "ss", &name, &value);
	TGE::Con::setVariable(name, TGE::StringTable->insert(value, true));
	return Py_None;
}

// pytorque.evaluate(string);
static PyObject* pyTorque_evaluate(PyObject* self, PyObject* args)
{
	const char* str;
	PyArg_ParseTuple(args, "s", &str);
	const char* ret = TGE::Con::evaluatef(str);
	if (ret != NULL)
	{
		return PyUnicode_FromString(ret);
	}
	return Py_None;
}

const char* pythonDefinedFunctionCb(TGE::SimObject* thisObj, int argc, const char** argv)
{
	// Once again, from the OG pyTorque

	PyObject* callable = NULL;
	const char* look = NULL;
	char buffer[512];

	PyObject* args = NULL;

	if (argc > 1)
	{
		sprintf(buffer, "%s::%s", argv[1], argv[0]);
		look = TGE::StringTable->insert(buffer, false);
		if (!exportedFunctions.count(look))
			look = NULL;
		else
		{
			args = PyTuple_New(argc - 2);
			for (int i = 2; i < argc; i++) PyTuple_SetItem(args, i - 2, PyUnicode_FromString(argv[i]));
		}
	}
	else
	{
		//next look in objects namespace
		if (!look && thisObj)
		{
			sprintf(buffer, "%s::%s", thisObj->mName, argv[0]);
			look = TGE::StringTable->insert(buffer, false);
			if (!exportedFunctions.count(look))
				look = NULL;
		}

		//finally look for a global export
		if (!look)
		{
			look = argv[0]; //assumes argv[0,] which is the function name, is a stringtable entry
			if (!exportedFunctions.count(look))
				look = NULL;
		}

		if (!look)
		{
			TGE::Con::errorf("pythonDefinedFunctionCb - missing export %s", argv[0]);
			return "";
		}

		args = PyTuple_New(argc - 1);
		for (int i = 1; i < argc; i++) PyTuple_SetItem(args, i - 1, PyUnicode_FromString(argv[i]));
	
	}

	if (!look)
	{
		look = TGE::StringTable->insert(argv[0], false);
		args = PyTuple_New(argc - 1);
		for (int i = 1; i < argc; i++) PyTuple_SetItem(args, i - 1, PyUnicode_FromString(argv[i]));
	}

	auto iter = exportedFunctions.find(look);

	if (iter == exportedFunctions.end())
	{
		TGE::Con::errorf("pythonDefinedFunctionCb - missing export %s", argv[0]);
		return "";
	}

	auto pair = *iter;
	callable = pair.second;

	PyObject* result = PyObject_CallObject(callable, args);
	if (PyErr_Occurred())
		PyErr_Print();

	const char* r = "";

	if (result)
	{
		PyObject* str = PyObject_Str(result);
		r = PyUnicode_AsUTF8(str);
		Py_DECREF(str);
	}

	Py_XDECREF(args);
	Py_XDECREF(result);

	char* retbuf = TGE::Con::getReturnBuffer(1024);

	strcpy(retbuf, r);

	return retbuf;
}

// pytorque.export(callback,[namespace],functionname,usage,minargs,maxargs);
static PyObject* pyTorque_export(PyObject* self, PyObject* args)
{
	// Taken from the OG pytorque from more than a decade ago

	int size = PyTuple_Size(args);
	if (size != 5 && size != 6)
	{
		PyErr_SetString(PyExc_SyntaxError, "pytorque.export(callback,[namespace],functionname,usagedoc,minargs,maxargs)");
		return NULL;
	}

	int index = 0;
	PyObject* o, * callable;
	const char* ns, * fname, * usage;
	S32 minargs, maxargs;
	StringTableEntry lookup;

	o = PyTuple_GetItem(args, index++);
	if (!PyCallable_Check(o))
	{
		PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - non-callable callback");
		return NULL;
	}

	callable = o;

	if (size == 6) //with namespace
	{
		o = PyTuple_GetItem(args, index++);
		if (!PyUnicode_Check(o))
		{
			PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - non-string namespace");
			return NULL;
		}

		ns = TGE::StringTable->insert(PyUnicode_AsUTF8(o), false);
	}

	//function name
	o = PyTuple_GetItem(args, index++);
	if (!PyUnicode_Check(o))
	{
		PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - non-string function name");
		return NULL;
	}

	fname = TGE::StringTable->insert(PyUnicode_AsUTF8(o), false);

	//usage
	o = PyTuple_GetItem(args, index++);
	if (!PyUnicode_Check(o))
	{
		PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - non-string usage");
		return NULL;
	}

	usage = TGE::StringTable->insert(PyUnicode_AsUTF8(o), false);

	//min args
	o = PyTuple_GetItem(args, index++);
	if (!PyNumber_Check(o))
	{
		PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - non integer min args");
		return NULL;
	}

	minargs = PyLong_AsLong(o) + 1; //adjust for function name argument

	//max args
	o = PyTuple_GetItem(args, index++);
	if (!PyNumber_Check(o))
	{
		PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - non integer max args");
		return NULL;
	}

	maxargs = PyLong_AsLong(o) + 1; //adjust for function name argumen

	if (size == 6)
	{
		//with namespace
		char catname[512];
		sprintf(catname, "%s::%s", ns, fname);
		lookup = TGE::StringTable->insert(catname, false);

		if (exportedFunctions.count(lookup))
		{
			PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - function already exported will not overwrite.  Consider using a namespace");
			return NULL;
		}
		
		exportedFunctions.insert(std::pair<StringTableEntry, PyObject*>(lookup, callable));

		TGE::Con::addCommand(ns, fname, pythonDefinedFunctionCb, usage, minargs + 1, maxargs + 1);
	}
	else
	{
		lookup = TGE::StringTable->insert(fname, false);
		if (exportedFunctions.count(lookup))
		{
			PyErr_SetString(PyExc_SyntaxError, "pytorque.export() - function already exported will not overwrite.  Consider using a namespace");
			return NULL;
		}

		exportedFunctions.insert(std::pair<StringTableEntry, PyObject*>(lookup, callable));

		TGE::Con::addCommand(fname, pythonDefinedFunctionCb, usage, minargs, maxargs);
	}

	return Py_None;

}

static PyMethodDef pytorquemethods[] = {
	{"echo", pyTorque_echo, METH_VARARGS, NULL},
	{"findObject", pyTorque_findObject, METH_VARARGS, NULL},
	{"getGlobal", pyTorque_getGlobal, METH_VARARGS, NULL},
	{"setGlobal", pyTorque_setGlobal, METH_VARARGS, NULL},
	{"evaluate", pyTorque_evaluate, METH_VARARGS, NULL},
	{"findFunction", pyTorque_findFunction, METH_VARARGS, NULL},
	{"export", pyTorque_export, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL}
};

static PyModuleDef pytorqueModule = {
	PyModuleDef_HEAD_INIT
};

PyMODINIT_FUNC initPyTorque()
{
	pytorqueModule.m_name = "pytorque";
	pytorqueModule.m_doc = "Python + Torque";
	pytorqueModule.m_size = -1;
	pytorqueModule.m_methods = pytorquemethods;

	TorqueObjectType.tp_doc = "Torque SimObject";
	TorqueObjectType.tp_basicsize = sizeof(TorqueObject);
	TorqueObjectType.tp_itemsize = 0;
	TorqueObjectType.tp_flags = Py_TPFLAGS_DEFAULT;
	TorqueObjectType.tp_new = TorqueObject_new;
	TorqueObjectType.tp_dealloc = (destructor)TorqueObject_dealloc;
	TorqueObjectType.tp_getattr = (getattrfunc)TorqueObject_getattr;
	TorqueObjectType.tp_setattr = (setattrfunc)TorqueObject_setattr;
	TorqueObjectType.tp_repr = (reprfunc)TorqueObject_repr;

	TorqueFunctionType.tp_doc = "Torque Function";
	TorqueFunctionType.tp_basicsize = sizeof(TorqueFunction);
	TorqueFunctionType.tp_itemsize = 0;
	TorqueFunctionType.tp_flags = Py_TPFLAGS_DEFAULT;
	TorqueFunctionType.tp_dealloc = (destructor)TorqueFunction_dealloc;
	TorqueFunctionType.tp_call = (ternaryfunc)TorqueFunction_call;
	TorqueFunctionType.tp_repr = (reprfunc)TorqueFunction_repr;


	PyObject* m;
	if (PyType_Ready(&TorqueObjectType) < 0)
		return NULL;

	if (PyType_Ready(&TorqueFunctionType) < 0)
		return NULL;


	m = PyModule_Create(&pytorqueModule);
	if (m == NULL)
		return NULL;

	Py_INCREF(&TorqueObjectType);
	PyModule_AddObject(m, "TorqueObject", (PyObject*)&TorqueObjectType);

	Py_INCREF(&TorqueFunctionType);
	PyModule_AddObject(m, "TorqueFunction", (PyObject*)&TorqueFunctionType);
	return m;

}

MBX_MODULE(pyTorque)

PyObject* pyTorqueModule;

MBX_CONSOLE_FUNCTION(initPython, void, 1, 1, "initPython();")
{
	if (PyImport_AppendInittab("pytorque", initPyTorque) == -1) {
		TGE::Con::printf("Error: could not extend in-built modules table");
	}

	Py_Initialize();
	PyObject* pyTorqueModule = PyImport_ImportModule("pytorque");
	if (!pyTorqueModule) {
		PyErr_Print();
		TGE::Con::printf("Error: could not import module 'pytorque'");
	}
}

MBX_CONSOLE_FUNCTION(execPython, int, 2, 2, "execPython(str);")
{
	int ret = PyRun_SimpleString(argv[1]);
	return ret;
}

MBX_CONSOLE_FUNCTION(execPythonModule, int, 2, 2, "execPythonModule(file)")
{

	int ret = 0;
	PyObject* sys = PyImport_ImportModule("sys");
	PyObject* path = PyObject_GetAttrString(sys, "path");
	PyList_Append(path, PyUnicode_FromString("."));

	PyObject* myModuleString = PyUnicode_FromString(argv[1]);
	PyObject* myModule = PyImport_Import(myModuleString);

	PyObject* err = PyErr_Occurred();
	if (err != NULL)
	{
		PyObject* ptype, * pvalue, * ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);
		if (ptype != NULL)
		{
			if (ptype->ob_type->tp_str != NULL)
			{
				const char* ptypestr = PyUnicode_AsUTF8(PyObject_Str(ptype));
				TGE::Con::printf(TGE::StringTable->insert(ptypestr, false));
			}
		}
		if (pvalue != NULL)
		{
			if (pvalue->ob_type->tp_str != NULL)
			{
				const char* pvaluestr = PyUnicode_AsUTF8(PyObject_Str(pvalue));
				TGE::Con::printf(TGE::StringTable->insert(pvaluestr, false));
			}
		}
		if (ptraceback != NULL)
		{
			if (ptraceback->ob_type->tp_str != NULL)
			{
				const char* ptracebackstr = PyUnicode_AsUTF8(PyObject_Str(ptraceback));
				TGE::Con::printf(TGE::StringTable->insert(ptracebackstr, false));
			}
		}
	}

	//Get error message
	return ret;
}

bool initPlugin(MBX::Plugin& plugin)
{

	MBX_INSTALL(plugin, pyTorque);
	// initPyTorque();
	return true;
}