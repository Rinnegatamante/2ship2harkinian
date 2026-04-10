#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

static std::unordered_map<const char*, int32_t>     gCacheInt;
static std::unordered_map<const char*, float>       gCacheFloat;
static std::unordered_map<const char*, const char*> gCacheString;
static std::unordered_map<const char*, Color_RGBA8> gCacheColor;
static std::unordered_map<const char*, Color_RGB8>  gCacheColor24;

std::shared_ptr<Ship::CVar> CVarGet(const char* name) {
    return Ship::Context::GetInstance()->GetConsoleVariables()->Get(name);
}

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    auto it = gCacheInt.find(name);
    if (it != gCacheInt.end()) [[likely]] return it->second;
    int32_t val = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(name, defaultValue);
    gCacheInt.emplace(name, val);
    return val;
}

float CVarGetFloat(const char* name, float defaultValue) {
    auto it = gCacheFloat.find(name);
    if (it != gCacheFloat.end()) [[likely]] return it->second;
    float val = Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(name, defaultValue);
    gCacheFloat.emplace(name, val);
    return val;
}

const char* CVarGetString(const char* name, const char* defaultValue) {
    auto it = gCacheString.find(name);
    if (it != gCacheString.end()) [[likely]] return it->second;
    const char* val = Ship::Context::GetInstance()->GetConsoleVariables()->GetString(name, defaultValue);
    gCacheString.emplace(name, val);
    return val;
}

Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue) {
    auto it = gCacheColor.find(name);
    if (it != gCacheColor.end()) [[likely]] return it->second;
    Color_RGBA8 val = Ship::Context::GetInstance()->GetConsoleVariables()->GetColor(name, defaultValue);
    gCacheColor.emplace(name, val);
    return val;
}

Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue) {
    auto it = gCacheColor24.find(name);
    if (it != gCacheColor24.end()) [[likely]] return it->second;
    Color_RGB8 val = Ship::Context::GetInstance()->GetConsoleVariables()->GetColor24(name, defaultValue);
    gCacheColor24.emplace(name, val);
    return val;
}

void CVarSetInteger(const char* name, int32_t value) {
    gCacheInt[name] = value;
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(name, value);
}

void CVarSetFloat(const char* name, float value) {
    gCacheFloat[name] = value;
    Ship::Context::GetInstance()->GetConsoleVariables()->SetFloat(name, value);
}

void CVarSetString(const char* name, const char* value) {
    gCacheString[name] = value;
    Ship::Context::GetInstance()->GetConsoleVariables()->SetString(name, value);
}

void CVarSetColor(const char* name, Color_RGBA8 value) {
    gCacheColor[name] = value;
    Ship::Context::GetInstance()->GetConsoleVariables()->SetColor(name, value);
}

void CVarSetColor24(const char* name, Color_RGB8 value) {
    gCacheColor24[name] = value;
    Ship::Context::GetInstance()->GetConsoleVariables()->SetColor24(name, value);
}

void CVarRegisterInteger(const char* name, int32_t defaultValue) {
    Ship::Context::GetInstance()->GetConsoleVariables()->RegisterInteger(name, defaultValue);
    int32_t val = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(name, defaultValue);
    gCacheInt.emplace(name, val);
}

void CVarRegisterFloat(const char* name, float defaultValue) {
    Ship::Context::GetInstance()->GetConsoleVariables()->RegisterFloat(name, defaultValue);
    float val = Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(name, defaultValue);
    gCacheFloat.emplace(name, val);
}

void CVarRegisterString(const char* name, const char* defaultValue) {
    Ship::Context::GetInstance()->GetConsoleVariables()->RegisterString(name, defaultValue);
    const char* val = Ship::Context::GetInstance()->GetConsoleVariables()->GetString(name, defaultValue);
    gCacheString.emplace(name, val);
}

void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue) {
    Ship::Context::GetInstance()->GetConsoleVariables()->RegisterColor(name, defaultValue);
    Color_RGBA8 val = Ship::Context::GetInstance()->GetConsoleVariables()->GetColor(name, defaultValue);
    gCacheColor.emplace(name, val);
}

void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue) {
    Ship::Context::GetInstance()->GetConsoleVariables()->RegisterColor24(name, defaultValue);
    Color_RGB8 val = Ship::Context::GetInstance()->GetConsoleVariables()->GetColor24(name, defaultValue);
    gCacheColor24.emplace(name, val);
}

void CVarClear(const char* name) {
    gCacheInt.erase(name);
    gCacheFloat.erase(name);
    gCacheString.erase(name);
    gCacheColor.erase(name);
    gCacheColor24.erase(name);
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(name);
}

void CVarClearBlock(const char* name) {
    gCacheInt.clear();
    gCacheFloat.clear();
    gCacheString.clear();
    gCacheColor.clear();
    gCacheColor24.clear();
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearBlock(name);
}

void CVarCopy(const char* from, const char* to) {
    gCacheInt.erase(to);
    gCacheFloat.erase(to);
    gCacheString.erase(to);
    gCacheColor.erase(to);
    gCacheColor24.erase(to);
    Ship::Context::GetInstance()->GetConsoleVariables()->CopyVariable(from, to);
}

void CVarLoad() {
    gCacheInt.clear();
    gCacheFloat.clear();
    gCacheString.clear();
    gCacheColor.clear();
    gCacheColor24.clear();
    Ship::Context::GetInstance()->GetConsoleVariables()->Load();
}

void CVarSave() {
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}
}
