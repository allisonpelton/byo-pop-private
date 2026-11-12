// The cloud: a background object that drifts to the left, goes round to the right again when it is out of the picture, and
// starts at a height of the data file's choosing (or a random one).

#ifndef VENICE_CLOUD_H
#define VENICE_CLOUD_H

#include "SceneObject.h"

// 0x148 bytes in the original
class Cloud : public SceneObject
{
public:
	Cloud(); // 0x409f40
	void OnObjectClosed() override; // 0x405090
};

#endif
