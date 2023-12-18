#include "pch.h"

#include "xvmc_script.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/resource_manager.h"

namespace jcmr::game::format
{
#pragma pack(push, 8)
struct SXvmFormatStructInstance {
    uint64_t m_Instance;
    uint64_t m_Type;
};

struct SXvmFormatFunction {
    uint32_t                 m_NameHash;
    uint16_t                 m_LocalsCount;
    uint16_t                 m_ArgCount;
    ava::SAdfArray<uint16_t> m_Instructions;
    uint16_t                 m_MaxStackDepth;
    uint64_t                 m_Module;
    uint64_t                 m_LinenoPtr;
    uint64_t                 m_ColnoPtr;
    ava::SAdfArray<char>     m_Name;
};

struct SXvmFormatConstant {
    uint64_t m_Type;
    uint64_t m_Value;
};

struct SXvmFormatModule {
    uint32_t                           m_NameHash;
    uint32_t                           m_SrcCRC;
    uint32_t                           m_Properties;
    uint32_t                           m_ModuleSize;
    uint64_t                           m_DebugInfoArray;
    SXvmFormatStructInstance           m_ThisInstance;
    ava::SAdfArray<SXvmFormatFunction> m_Functions;
    ava::SAdfArray<uint32_t>           m_ImportHashes;
    ava::SAdfArray<SXvmFormatConstant> m_Constants;
    ava::SAdfArray<uint32_t>           m_StringHashes;
    ava::SAdfArray<char>               m_StringBuffer;
    uint64_t                           m_DebugStringPointer;
    uint64_t                           m_DebugStrings;
    ava::SAdfArray<char>               m_Name;
};

static_assert(sizeof(SXvmFormatStructInstance) == 0x10, "SXvmFormatStructInstance alignment is wrong!");
static_assert(sizeof(SXvmFormatFunction) == 0x48, "SXvmFormatFunction alignment is wrong!");
static_assert(sizeof(SXvmFormatConstant) == 0x10, "SXvmFormatConstant alignment is wrong!");
static_assert(sizeof(SXvmFormatModule) == 0x98, "SXvmFormatModule alignment is wrong!");
#pragma pack(pop)

struct XVMCScriptInstance {
    XVMCScriptInstance(App& app, ava::AvalancheDataFormat::ADF* script_adf)
        : m_app(app)
        , m_script_adf(script_adf)
    {
        load_xvmc();
    }

    ~XVMCScriptInstance()
    {
        LOG_INFO("~XVMCScriptInstance");
        if (m_script_adf) delete m_script_adf;
        if (m_module) std::free(m_module);
    }

  private:
    void load_xvmc()
    {
        ASSERT(m_script_adf != nullptr);
        m_script_adf->ReadInstance(0, (void**)&m_module);

        static auto make_function_arguments = [](const SXvmFormatFunction& func) {
            std::string result = "(";
            for (u32 i = 0; i < func.m_ArgCount; ++i) {
                if (i > 0) result += ", ";
                result += "arg" + std::to_string(i + 1);
            }

            result += ")";
            return result;
        };

        static std::string opcodes[255];
        for (u32 i = 0; i < lengthOf(opcodes); ++i)
            opcodes[i] = fmt::format("op[{}]", i);

        opcodes[9]  = "call_c_function";
        opcodes[18] = "xvm_get_attr";
        opcodes[21] = "load_global";
        opcodes[25] = "debug_log";
        opcodes[26] = "return";
        opcodes[27] = "xvm_set_attr";

        for (auto& func : m_module->m_Functions) {
            // if (strcmp(func.m_Name.c_str(), "TestReturnMatrix4")) continue;

            LOG_INFO("function {}{} ({})", func.m_Name.c_str(), make_function_arguments(func), func.m_NameHash);

            // std::string _instructions = "\t";
            for (auto& instruction : func.m_Instructions) {
                // LOG_INFO("\t{} ({:x}, {})", opcodes[instruction & 0x1f], instruction, instruction >> 5);
                LOG_INFO("\t{}", opcodes[instruction & 0x1f]);
            }

            // LOG_INFO("{}", _instructions);
            // LOG_INFO("}");
        }
    }

  private:
    App&                           m_app;
    ava::AvalancheDataFormat::ADF* m_script_adf;
    SXvmFormatModule*              m_module = nullptr;
};

struct XVMCScriptImpl final : XVMCScript {
    XVMCScriptImpl(App& app)
        : m_app(app)
    {
        //
    }

    bool load(const std::string& filename) override
    {
        if (is_loaded(filename)) {
            return true;
        }

        LOG_INFO("XVMCScript : loading \"{}\"...", filename);

        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            LOG_ERROR("XVMCScript : failed to load file.");
            return false;
        }

        auto* adf = new ava::AvalancheDataFormat::ADF(buffer);
        m_scripts.insert({filename, std::make_unique<XVMCScriptInstance>(m_app, adf)});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_scripts.find(filename);
        if (iter == m_scripts.end()) return;
        m_scripts.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        //
        return false;
    }

    bool is_loaded(const std::string& filename) const override { return m_scripts.find(filename) != m_scripts.end(); }

  private:
    App& m_app;

    std::unordered_map<std::string, std::unique_ptr<XVMCScriptInstance>> m_scripts;
};

XVMCScript* XVMCScript::create(App& app)
{
    return new XVMCScriptImpl(app);
}

void XVMCScript::destroy(XVMCScript* instance)
{
    delete instance;
}
} // namespace jcmr::game::format