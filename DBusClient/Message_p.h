#pragma once

int _DBusProcessField(Message* m, Field* f);

int _DBusProcess8Bit(Message* m, Field* f, uint8_t* fieldData);
int _DBusProcess16Bit(Message* m, Field* f, uint16_t* fieldData);
int _DBusProcess32Bit(Message* m, Field* f, uint32_t* fieldData);
int _DBusProcess64Bit(Message* m, Field* f, uint64_t* fieldData);
int _DBusProcessBoolean(Message* m, Field* f);

int _DBusProcessStringData(Message* m, Field* f);
int _DBusProcessObjectPath(Message* m, Field* f);
int _DBusProcessString(Message* m, Field* f);
int _DBusProcessSignature(Message* m, Field* f);

int _DBusNextRootField(Message* m, Field* f);
bool _DBusIsRootAtEnd(Message* m);

int _DBusProcessStruct(Message* m, Field* f);
int _DBusNextStructField(Message* m, Field* f);
bool _DBusIsStructAtEnd(Message* m);

int _DBusProcessDictEntry(Message* m, Field* f);
int _DBusNextDictEntryField(Message* m, Field* f);
bool _DBusIsDictEntryAtEnd(Message* m);

int _DBusProcessArray(Message* m, Field* f);
int _DBusNextArrayField(Message* m, Field* f);
bool _DBusIsArrayAtEnd(Message* m);

int _DBusProcessVariant(Message* m, Field* f);
int _DBusNextVariantField(Message* m, Field* f);
bool _DBusIsVariantAtEnd(Message* m);

int _DBusNextField(Message* m, Field* f);
