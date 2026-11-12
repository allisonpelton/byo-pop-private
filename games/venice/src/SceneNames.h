// The names the scene files use for attributes, filters, triggers and actions.

#ifndef VENICE_SCENENAMES_H
#define VENICE_SCENENAMES_H

int AttributeFromName(const char* theName);
int FilterTypeFromName(const char* theName);
int TriggerFromName(const char* theName);
int ActionFromName(const char* theName);

// 0x42ba20: a number in a data file; "button" stands for the z order of the buttons
float ParseNumber(const char* theText);

#endif
