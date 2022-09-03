#include <TorqueLib/core/stream.h>
#include <string>
#include <TorqueLib/math/mMath.h>
#include <vector>
#include <map>

class RREC {
public:
	S16 version;
	S16 gameVersion;
	std::string missionFile;
	std::string marbleSelection;
	bool hasMetadata;
	bool lb;
	bool mp;
	S32 sprngSeed;
	
	std::string author;
	std::string name;
	std::string desc;
	std::string filename;

	char* frames;
	U32 frameSize;

	RREC() {

	}
	~RREC() {
		if (frames != NULL) {
			delete[] frames;
		}
	}

	void read(TGE::Stream* rstream) {
		this->readHeader(rstream);
		U32 remaining = rstream->getStreamSize() - rstream->getPosition();
		this->frames = new char[remaining];
		rstream->_read(remaining, this->frames);
		this->frameSize = remaining;
	}

	void write(TGE::Stream* wstream) {
		this->writeHeader(wstream);
		wstream->_write(frameSize, frames);
	}

private:
	void readHeader(TGE::Stream* rstream) {
		rstream->_read(sizeof(U16), &this->version);
		rstream->_read(sizeof(U16), &this->gameVersion);

		U8 length;

		rstream->_read(sizeof(U8), &length);
		char* str = new char[length + 1];
		rstream->_read(length, str);
		str[length] = '\0';
		missionFile  = str;
		delete[] str;

		rstream->_read(sizeof(U8), &length);
		str = new char[length + 1];
		rstream->_read(length, str);
		str[length] = '\0';
		marbleSelection = str;
		delete[] str;

		U8 flags;

		rstream->_read(sizeof(U8), &flags);

		this->hasMetadata = (flags & 1) > 0;
		this->lb = (flags & (1 << 1)) == (1 << 1);
		this->mp = (flags & (1 << 2)) == (1 << 2);

		if (this->hasMetadata)
			readMetadata(rstream);

		rstream->_read(sizeof(U32), &this->sprngSeed);
	}

	void writeHeader(TGE::Stream* wstream) {
		wstream->_write(sizeof(U16), &this->version);
		wstream->_write(sizeof(U16), &this->gameVersion);

		U8 length = missionFile.size();
		wstream->_write(sizeof(U8), &length);
		wstream->_write(length, missionFile.c_str());
		length = marbleSelection.size();
		wstream->_write(sizeof(U8), &length);
		wstream->_write(length, marbleSelection.c_str());

		if (this->author.size() != 0 || this->name.size() != 0 || this->desc.size() != 0) {
			this->hasMetadata = true;
		}

		U8 flags = 0;
		if (this->hasMetadata)
			flags |= 1;
		if (this->lb)
			flags |= (1 << 1);
		if (this->mp)
			flags |= (1 << 2);

		wstream->_write(sizeof(U8), &flags);

		if (this->hasMetadata)
			this->writeMetadata(wstream);

		wstream->_write(sizeof(U32), &this->sprngSeed);
	}

	void readMetadata(TGE::Stream* rstream) {
		U8 length;

		rstream->_read(sizeof(U8), &length);
		char* str = new char[length + 1];
		rstream->_read(length, str);
		str[length] = '\0';
		author = str;
		delete[] str;

		rstream->_read(sizeof(U8), &length);
		str = new char[length + 1];
		rstream->_read(length, str);
		str[length] = '\0';
		name = str;
		delete[] str;

		U16 len16;

		rstream->_read(sizeof(U16), &len16);
		str = new char[len16 + 1];
		rstream->_read(len16, str);
		str[len16] = '\0';
		desc = str;
		delete[] str;
	}

	void writeMetadata(TGE::Stream* wstream) {
		U8 length = author.size();
		wstream->_write(sizeof(U8), &length);
		wstream->_write(length, author.c_str());
		length = name.size();
		wstream->_write(sizeof(U8), &length);
		wstream->_write(length, name.c_str());
		U16 len16 = desc.size();
		wstream->_write(sizeof(U16), &len16);
		wstream->_write(len16, desc.c_str());
	}
};

struct RRECFrame {
	U8 tag;
	virtual ~RRECFrame() {

	}
	virtual void read(TGE::Stream* rstream) {

	}
	virtual void write(TGE::Stream* wstream) {

	}
};

struct CollisionFrame : RRECFrame {
	std::string dataBlock;
	Point3F position;

	CollisionFrame() {
		this->tag = 8;
	}
};

struct GemFrame : RRECFrame {
	U16 count;
	U16 max;
	U16 quota;
	U8 green;

	GemFrame() {
		this->tag = 10;
	}
};

struct GravityFrame : RRECFrame {
	MatrixF dir;
	U8 instant;
	AngAxisF rot;

	GravityFrame() {
		this->tag = 9;
	}
};

struct SceneObjectFrame : RRECFrame {
	MatrixF transform;
};

struct ShapeBaseFrame : SceneObjectFrame {
	std::vector<std::string> mountImages;
};

struct MarbleFrame : ShapeBaseFrame {
	Point3F velocity;
	Point3F angularVelocity;
	float radius;
	float cameraYaw;
	float cameraPitch;
};

struct MovementFrame : RRECFrame {
	float left;
	float right;
	float forward;
	float backward;

	MovementFrame() {
		this->tag = 11;
	}
};

struct PhysicsFrame : RRECFrame {
	std::map<std::string, std::string> physicsFields;

	PhysicsFrame() {
		this->tag = 7;
	}
};

struct PickupFrame : RRECFrame {
	std::string dataBlock;
	Point3F position;

	PickupFrame() {
		this->tag = 6;
	}
};

struct RRECPlatformData {
	S32 pathTime;
	S32 targetTime;
};

struct PlatformFrame : RRECFrame {
	std::vector<RRECPlatformData> platforms;

	PlatformFrame() {
		this->tag = 3;
	}
};

struct RRecMove {
	bool powerup;
	bool jump;
	bool mousefire;
	bool blast;
};

struct PositionFrame : MarbleFrame {
	RRecMove input;

	PositionFrame() {
		this->tag = 2;
	}
};

struct ServerTimeFrame : RRECFrame {
	S32 total;
	S32 current;
	S32 elapsed;
	S32 bonus;
	S32 clientTotal;
	S32 clientCurrent;
	S32 clientBonus;
	U8 active;

	ServerTimeFrame() {
		this->tag = 12;
	}
};

struct SpawnFrame : RRECFrame {
	S32 gemCount;
	std::vector<Point3F> spawnPositions;

	SpawnFrame() {
		this->tag = 5;
	}
};

struct TimeFrame : RRECFrame {
	S32 total;
	S32 current;
	S32 bonus;
	U8 active;

	TimeFrame() {
		this->tag = 1;
	}
};