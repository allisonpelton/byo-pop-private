// Makes the scene object for the type name a data file gives (the "type" of an object tag).

#ifndef VENICE_OBJECTFACTORY_H
#define VENICE_OBJECTFACTORY_H

class SceneObject;

// 0x4381a0: a new object of the class the name stands for; a plain scene object when the name is none of the classes'
SceneObject* NewObjectByTypeName(const char* theTypeName);

#endif
