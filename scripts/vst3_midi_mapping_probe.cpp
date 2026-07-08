/*
 * Minimal VST3 host probing the IMidiMapping behavior of the built JS80P.vst3
 * (see the "IMidiMapping" section of BUILD.md for why this matters).
 *
 * Reproduces two host lifecycles:
 *   REAPER-style: connect component<->controller DIRECTLY, then query IMidiMapping.
 *   Bitwig-style: query IMidiMapping EARLY (right after controller init), CACHE the
 *                 interface pointer, and connect through host-side connection
 *                 proxies. Hosts query COM interfaces once, so whatever the early
 *                 query returns is what such a host uses forever.
 *
 * Expected output (JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=1, both modes):
 *   - POST-connect fresh query    -> the legacy mapping: CC1 ch0 = 0x00000001,
 *     aftertouch = 0x00000080, pitch bend = 0x00000081, ch15 aftertouch = 0x0f80.
 *   - POST-connect CACHED ptr     -> MAPPED via JUCE's CC emulation
 *     (0x6d636d00 + channel*130 + controller); this line is the Bitwig fix — if it
 *     says UNMAPPED, early-querying hosts get NO CC / pitch bend / aftertouch.
 *
 * Build & run (Linux):
 *   g++ -std=c++17 scripts/vst3_midi_mapping_probe.cpp -o /tmp/midimap_probe -ldl \
 *     -I build-juce/_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK
 *   /tmp/midimap_probe \
 *     build-juce/JS80P_artefacts/Release/VST3/JS80P.vst3/Contents/x86_64-linux/JS80P.so direct
 *   ... and the same with "proxy" as the second argument.
 */

#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ipluginbase.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>

using namespace Steinberg;

#include <map>
#include <string>

/* Minimal IAttributeList/IMessage/IHostApplication so the plugin can allocate
 * and exchange its component<->controller handshake messages, as any real host
 * (REAPER, Bitwig) allows. */
class AttributeList : public Vst::IAttributeList
{
public:
    std::map<std::string, int64> ints;
    std::map<std::string, double> floats;

    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual(iid, Vst::IAttributeList_iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown_iid))
        { *obj = this; return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return 100; }
    uint32 PLUGIN_API release() override { return 100; }

    tresult PLUGIN_API setInt(AttrID id, int64 v) override { ints[id] = v; return kResultOk; }
    tresult PLUGIN_API getInt(AttrID id, int64& v) override
    {
        auto it = ints.find(id);
        if (it == ints.end()) return kResultFalse;
        v = it->second; return kResultOk;
    }
    tresult PLUGIN_API setFloat(AttrID id, double v) override { floats[id] = v; return kResultOk; }
    tresult PLUGIN_API getFloat(AttrID id, double& v) override
    {
        auto it = floats.find(id);
        if (it == floats.end()) return kResultFalse;
        v = it->second; return kResultOk;
    }
    tresult PLUGIN_API setString(AttrID, const Vst::TChar*) override { return kResultOk; }
    tresult PLUGIN_API getString(AttrID, Vst::TChar*, uint32) override { return kResultFalse; }
    tresult PLUGIN_API setBinary(AttrID, const void*, uint32) override { return kResultOk; }
    tresult PLUGIN_API getBinary(AttrID, const void*&, uint32&) override { return kResultFalse; }
};

class Message : public Vst::IMessage
{
public:
    char messageId[128] = {0};
    AttributeList attrs;
    int refCount = 1;

    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual(iid, Vst::IMessage_iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown_iid))
        { *obj = this; addRef(); return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return (uint32)++refCount; }
    uint32 PLUGIN_API release() override
    {
        int rc = --refCount;
        if (rc == 0) { delete this; return 0; }
        return (uint32)rc;
    }

    FIDString PLUGIN_API getMessageID() override { return messageId; }
    void PLUGIN_API setMessageID(FIDString id) override
    {
        strncpy(messageId, id, sizeof(messageId) - 1);
    }
    Vst::IAttributeList* PLUGIN_API getAttributes() override { return &attrs; }
};

class HostApp : public Vst::IHostApplication
{
public:
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual(iid, Vst::IHostApplication_iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown_iid))
        { *obj = this; return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return 100; }
    uint32 PLUGIN_API release() override { return 100; }

    tresult PLUGIN_API getName(Vst::String128 name) override
    {
        const char16_t* n = u"MiniProbeHost";
        for (int i = 0; i < 14; ++i) name[i] = (Vst::TChar)n[i];
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID cid, TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual(cid, Vst::IMessage_iid)
            && FUnknownPrivate::iidEqual(iid, Vst::IMessage_iid))
        { *obj = new Message(); return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
};

static HostApp host_app;

typedef bool (*ModuleEntryFunc)(void*);
typedef bool (*ModuleExitFunc)();
typedef IPluginFactory* (PLUGIN_API *GetFactoryProc)();

/* Forwards connect/disconnect/notify to the real other end, like Bitwig's
 * host-side connection proxies: each plugin object is connected to a host
 * object, never directly to its peer. */
class ConnectionProxy : public Vst::IConnectionPoint
{
public:
    Vst::IConnectionPoint* target = nullptr;

    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual(iid, Vst::IConnectionPoint_iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown_iid))
        {
            *obj = this;
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return 100; }
    uint32 PLUGIN_API release() override { return 100; }

    tresult PLUGIN_API connect(Vst::IConnectionPoint*) override { return kResultOk; }
    tresult PLUGIN_API disconnect(Vst::IConnectionPoint*) override { return kResultOk; }
    tresult PLUGIN_API notify(Vst::IMessage* message) override
    {
        return target != nullptr ? target->notify(message) : kResultFalse;
    }
};

static void test_mapping(const char* label, Vst::IEditController* controller)
{
    void* obj = nullptr;
    if (controller->queryInterface(Vst::IMidiMapping_iid, &obj) != kResultOk || obj == nullptr)
    {
        printf("%-28s: no IMidiMapping interface\n", label);
        return;
    }
    Vst::IMidiMapping* mapping = (Vst::IMidiMapping*)obj;

    struct { const char* name; int16 ch; Vst::CtrlNumber num; } probes[] = {
        { "modwheel CC1  ch0",  0, 1 },
        { "aftertouch    ch0",  0, Vst::kAfterTouch },
        { "pitchbend     ch0",  0, Vst::kPitchBend },
        { "aftertouch   ch15", 15, Vst::kAfterTouch },
    };

    printf("%-28s: obj=%p\n", label, (void*)mapping);
    for (auto& p : probes)
    {
        Vst::ParamID id = 0xffffffff;
        tresult r = mapping->getMidiControllerAssignment(0, p.ch, p.num, id);
        printf("    %-20s -> %s  paramID=0x%08x\n",
               p.name, r == kResultTrue ? "MAPPED  " : "UNMAPPED", (unsigned)id);
    }
    mapping->release();
}

static void dump_params(Vst::IEditController* controller)
{
    int32 n = controller->getParameterCount();
    printf("parameter count: %d\n", (int)n);
    for (int32 i = 0; i < n && i < 4; ++i)
    {
        Vst::ParameterInfo info{};
        controller->getParameterInfo(i, info);
        char name[129] = {0};
        for (int k = 0; k < 128 && info.title[k]; ++k) name[k] = (char)info.title[k];
        printf("    [%d] id=0x%08x flags=0x%x name=\"%s\"\n",
               (int)i, (unsigned)info.id, (unsigned)info.flags, name);
    }
    if (n > 4)
    {
        Vst::ParameterInfo info{};
        controller->getParameterInfo(n - 1, info);
        char name[129] = {0};
        for (int k = 0; k < 128 && info.title[k]; ++k) name[k] = (char)info.title[k];
        printf("    [%d] id=0x%08x flags=0x%x name=\"%s\" (last)\n",
               (int)(n - 1), (unsigned)info.id, (unsigned)info.flags, name);
    }
}

int main(int argc, char** argv)
{
    const char* path = argv[1];
    const char* mode = argc > 2 ? argv[2] : "direct";

    void* lib = dlopen(path, RTLD_NOW);
    if (!lib) { printf("dlopen failed: %s\n", dlerror()); return 1; }

    auto moduleEntry = (ModuleEntryFunc)dlsym(lib, "ModuleEntry");
    auto getFactory = (GetFactoryProc)dlsym(lib, "GetPluginFactory");
    if (!getFactory) { printf("no GetPluginFactory\n"); return 1; }
    if (moduleEntry) moduleEntry(lib);

    IPluginFactory* factory = getFactory();

    /* find the audio-module (component) class */
    TUID componentCid{};
    bool found = false;
    for (int32 i = 0; i < factory->countClasses(); ++i)
    {
        PClassInfo ci{};
        factory->getClassInfo(i, &ci);
        printf("class %d: category=%s name=%s\n", (int)i, ci.category, ci.name);
        if (strcmp(ci.category, kVstAudioEffectClass) == 0)
        {
            memcpy(componentCid, ci.cid, sizeof(TUID));
            found = true;
        }
    }
    if (!found) { printf("no component class\n"); return 1; }

    Vst::IComponent* component = nullptr;
    if (factory->createInstance(componentCid, Vst::IComponent_iid, (void**)&component) != kResultOk)
    { printf("createInstance(component) failed\n"); return 1; }
    component->initialize((FUnknown*)&host_app);

    TUID controllerCid{};
    if (component->getControllerClassId(controllerCid) != kResultTrue)
    { printf("getControllerClassId failed\n"); return 1; }

    Vst::IEditController* controller = nullptr;
    if (factory->createInstance(controllerCid, Vst::IEditController_iid, (void**)&controller) != kResultOk)
    { printf("createInstance(controller) failed\n"); return 1; }
    controller->initialize((FUnknown*)&host_app);

    printf("\n=== mode: %s ===\n", mode);

    /* 1. Early query, before any connection (what an eager host does). */
    test_mapping("PRE-connect query", controller);

    /* Hold on to the pre-connect interface to test whether an early-caching
     * host would ever start working. */
    void* earlyObj = nullptr;
    controller->queryInterface(Vst::IMidiMapping_iid, &earlyObj);
    Vst::IMidiMapping* earlyMapping = (Vst::IMidiMapping*)earlyObj;

    /* 2. Connect. */
    Vst::IConnectionPoint* compCP = nullptr;
    Vst::IConnectionPoint* ctrlCP = nullptr;
    component->queryInterface(Vst::IConnectionPoint_iid, (void**)&compCP);
    controller->queryInterface(Vst::IConnectionPoint_iid, (void**)&ctrlCP);
    printf("connection points: comp=%p ctrl=%p\n", (void*)compCP, (void*)ctrlCP);

    ConnectionProxy toComp, toCtrl;
    if (compCP && ctrlCP)
    {
        if (strcmp(mode, "proxy") == 0)
        {
            /* Bitwig-style: each side is connected to a host proxy. */
            toComp.target = compCP;
            toCtrl.target = ctrlCP;
            compCP->connect(&toCtrl);
            ctrlCP->connect(&toComp);
        }
        else
        {
            /* REAPER-style: direct connection. */
            compCP->connect(ctrlCP);
            ctrlCP->connect(compCP);
        }
    }

    /* 3. Query again after connect (fresh queryInterface). */
    test_mapping("POST-connect fresh query", controller);

    /* 4. And through the interface pointer cached before connect. */
    if (earlyMapping)
    {
        Vst::ParamID id = 0xffffffff;
        tresult r = earlyMapping->getMidiControllerAssignment(0, 0, Vst::kAfterTouch, id);
        printf("%-28s: aftertouch ch0 -> %s  paramID=0x%08x\n",
               "POST-connect CACHED ptr", r == kResultTrue ? "MAPPED  " : "UNMAPPED", (unsigned)id);
        earlyMapping->release();
    }

    printf("\n");
    dump_params(controller);

    return 0;
}
