#include "kbscan.h"
#include "proto_pok3r.h"
#include "proto_cykb.h"
#include "keymap.h"
#include "updatepackage.h"

#include "zlog.h"
#include "zfile.h"
#include "zpointer.h"
#include "zmap.h"
#include "zoptions.h"
#include "zjson.h"
using namespace LibChaos;

#include <iostream>

// Types
// ////////////////////////////////

struct Param {
    bool ok;
    ZArray<ZString> args;
    DeviceType device;
};

// Constants
// ////////////////////////////////

const ZMap<ZString, DeviceType> devnames = {
    { "pok3r",              DEV_POK3R },

    { "rgb",                DEV_POK3R_RGB },
    { "pok3r-rgb",          DEV_POK3R_RGB },
    { "pok3r_rgb",          DEV_POK3R_RGB },

    { "rgb2",               DEV_POK3R_RGB2 },
    { "pok3r-rgb2",         DEV_POK3R_RGB2 },
    { "pok3r_rgb2",         DEV_POK3R_RGB2 },

    { "core",               DEV_VORTEX_CORE },
    { "vortex-core",        DEV_VORTEX_CORE },
    { "vortex_core",        DEV_VORTEX_CORE },
    
    { "race3",              DEV_VORTEX_RACE3 },
    { "vortex-race3",       DEV_VORTEX_RACE3 },
    { "vortex_race3",       DEV_VORTEX_RACE3 },

    { "vibe",               DEV_VORTEX_VIBE },
    { "vortex-vibe",        DEV_VORTEX_VIBE },
    { "vortex_vibe",        DEV_VORTEX_VIBE },

    { "cypher",             DEV_VORTEX_CYPHER },

    { "tab60",              DEV_VORTEX_TAB60 },
    { "tab75",              DEV_VORTEX_TAB75 },
    { "tab90",              DEV_VORTEX_TAB90 },

    { "kbpv60",             DEV_KBP_V60 },
    { "kbp-v60",            DEV_KBP_V60 },
    { "kbp_v60",            DEV_KBP_V60 },

    { "kbpv80",             DEV_KBP_V80 },
    { "kbp-v80",            DEV_KBP_V80 },
    { "kbp_v80",            DEV_KBP_V80 },

    { "yoda2",              DEV_TEX_YODA_II },
    { "tex-yoda-2",         DEV_TEX_YODA_II },
    { "tex_yoda_2",         DEV_TEX_YODA_II },
    { "tex-yoda-ii",        DEV_TEX_YODA_II },
    { "tex_yoda_ii",        DEV_TEX_YODA_II },

    { "md600",              DEV_MISTEL_MD600 },
    { "barocco",            DEV_MISTEL_MD600 },

    { "md200",              DEV_MISTEL_MD200 },
    { "freeboard",          DEV_MISTEL_MD200 },
};

// Functions
// ////////////////////////////////

ZPointer<KBProto> openDevice(DeviceType dev){
    KBScan scanner;
    if(!scanner.find(dev)){
        LOG("No device found, check connection and permissions");
        return nullptr;
    }

    auto devs = scanner.open();
    if(devs.size() == 1){
        auto kb = devs.front();
        if(kb.iface->isOpen()){
            LOG("Opened " << kb.info.name <<
                (kb.iface->isBuiltin() ? " (bootloader)" : "") <<
                (kb.iface->isQMK() ? " [QMK]" : "")
            );
            return kb.iface;
        } else {
            ELOG("Device found but not opened: " << kb.info.name);
        }
    } else if(devs.size() > 1){
        ELOG("Multiple identical devices found, disconnect devices other than target");
    } else {
        ELOG("No device to open?");
    }
    return nullptr;
}

void warning(){
    ELOG("WARNING: THIS TOOL IS RELATIVELY UNTESTED, AND HAS A VERY REAL "
         "RISK OF CORRUPTING YOUR KEYBOARD, MAKING IT UNUSABLE WITHOUT "
         "EXPENSIVE DEVELOPMENT TOOLS. PROCEED AT YOUR OWN RISK.");
    ELOG("Type \"OK\" to continue:");
    std::string str;
    std::getline(std::cin, str);
    if(str != "OK"){
        LOG("Exiting...");
        exit(EXIT_FAILURE);
    } else {
        LOG("Proceeding...");
    }
}

// Commands
// ////////////////////////////////

int cmd_list(Param *param){
    LOG("List Devices...");

    KBScan scanner;
    scanner.scan();
    auto devs = scanner.open();
    for (auto it = devs.begin(); it.more(); ++it){
        LOG(it.get().info.name <<
            (it.get().iface->isBuiltin() ? " (bootloader)" : "") <<
            (it.get().iface->isQMK() ? " [QMK]" : "") <<
            ": " << it.get().iface->getVersion());
    }

    return 0;
}

int cmd_version(Param *param){
    // Read Version
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG("Version: " << kb->getVersion());
        return 0;
    }
    return -1;
}

int cmd_setversion(Param *param){
    if(!param->ok)
        warning();

    ZString version = param->args[1];
    // Set Version
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG("Old Version: " << kb->getVersion());
        LOG(kb->setVersion(version));
        LOG(kb->rebootFirmware());
        return 0;
    }
    return -1;
}

int cmd_info(Param *param){
    if(!param->ok)
        warning();

    // Get Info
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        if(kb->isQMK()){
            ProtoQMK *qmk = dynamic_cast<ProtoQMK *>(kb.get());
            LOG(qmk->qmkInfo());
        } else {
            LOG(kb->getInfo());
        }
        return 0;
    }
    return -1;
}

int cmd_reboot(Param *param){
    // Reset to Firmware
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG(kb->rebootFirmware(false));
        // Read version
//        LOG("Version: " << kb->getVersion());
        return 0;
    }
    return -1;
}

int cmd_bootloader(Param *param){
    // Reset to Bootloader
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG(kb->rebootBootloader(false));
        // Read version
//        LOG("Version: " << kb->getVersion());
        return 0;
    }
    return -1;
}

int cmd_dump(Param *param){
    if(!param->ok)
        warning();

    ZPath out = param->args[1];
    // Dump Flash
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG("Dump Flash");
        ZBinary bin = kb->dumpFlash();
//        RLOG(bin.dumpBytes(4, 8));
        LOG("Out: " << out << ", " << bin.size() << " bytes");
        ZFile::writeBinary(out, bin);
        return 0;
    }
    return -1;
}

int cmd_flash(Param *param){
    if(!param->ok)
        warning();

    ZString version = param->args[1];
    ZPath firmware = param->args[2];
    // Update Firmware
    if(param->device == 0){
        LOG("Please specifiy a device");
        return 2;
    }
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG("Update Firmware: " << firmware);
        ZBinary fwbin;
        ZFile file;
        if(!file.open(firmware)){
            ELOG("Failed to open file");
            return -3;
        }
        if(file.read(fwbin, file.fileSize()) != file.fileSize()){
            ELOG("Failed to read file");
            return -4;
        }
        LOG(kb->update(version, fwbin));
        return 0;
    }
    return -1;
}

int cmd_wipe(Param *param){
    if(!param->ok)
        warning();

    // Erase Firmware
    if(param->device == 0){
        LOG("Please specifiy a device");
        return 2;
    }
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        LOG("Erase Firmware");
        bool ret = kb->eraseAndCheck();
        LOG(ret);
        return 0;
    }
    return -1;
}

int cmd_decode(Param *param){
    UpdatePackage package;
    if(!package.loadFromExe(param->args[1], 0)){
        ELOG("Load Error: " << param->args[1]);
        return 1;
    }
    ZPath out = param->args[2];
    LOG("Write " << out);
    ZBinary fw = package.getFirmware();
    if(!ZFile::writeBinary(out, fw)){
        ELOG("Write Error");
        return 2;
    }
    LOG("Done");
    return 0;
}

int cmd_eeprom(Param *param){
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        if(!kb->isQMK()){
            ELOG("Not a QMK keyboard!");
            return -2;
        }
        ProtoQMK *qmk = dynamic_cast<ProtoQMK *>(kb.get());

        if(param->args[1] == "dump"){
            if(param->args.size() != 3){
                ELOG("Usage: pok3rtool eeprom dump <out file>");
                return -2;
            }
            ZPath out = param->args[2];
            LOG("Dump EEPROM");
            ZBinary bin = qmk->dumpEEPROM();
            LOG("Out: " << out << ", " << bin.size() << " bytes");
            ZFile::writeBinary(out, bin);

        } else if(param->args[1] == "erase"){
            if(param->args.size() != 3){
                ELOG("Usage: pok3rtool eeprom erase <addr>");
                return -2;
            }
            zu32 addr = param->args[2].toUint(16);
            LOG("Erase EEPROM Page 0x" << ZString::ItoS((zu64)addr, 16));
            LOG(qmk->eraseEEPROM(addr));

        } else if(param->args[1] == "wipe"){
            LOG("Wipe EEPROM");
            for(zu32 i = 0; i < 128; ++i){
                LOG(qmk->eraseEEPROM(i << 12));
                ZThread::msleep(500);
            }

        } else if(param->args[1] == "keymap"){
            if(param->args.size() != 2){
                ELOG("Usage: pok3rtool eeprom keymap");
                return -2;
            }
            LOG("EEPROM Keymap");
            ZBinary eebin;
            for(zu32 addr = QMK_EE_KEYM_PAGE; addr < QMK_EE_KEYM_PAGE + QMK_EE_PAGE_SIZE; addr += 60){
                if(!qmk->readEEPROM(addr, eebin))
                    break;
            }
            zassert(eebin.size() > QMK_EE_PAGE_SIZE);
            eebin.resize(QMK_EE_PAGE_SIZE);
            RLOG(eebin.dumpBytes(4, 8, QMK_EE_KEYM_PAGE));

        } else if(param->args[1] == "test"){
            LOG("EEPROM Test");
            bool ret = qmk->eepromTest();
            LOG(ret);

        } else {
            LOG("Usage: pok3rtool eeprom");
        }
        return 0;
    }
    return -1;
}

int cmd_keymap(Param *param){
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        if(!kb->isQMK()){
            ELOG("Not a QMK keyboard!");
            return -2;
        }
        ProtoQMK *qmk = dynamic_cast<ProtoQMK *>(kb.get());

        if(param->args[1] == "dump"){
            qmk->keymapDump();

        } else if(param->args[1] == "knownlayouts"){
            for(auto it = Keymap::getKnownLayouts().cbegin(); it.more(); ++it){
                LOG(it.get());
            }

        } else if(param->args[1] == "set"){
            if(param->args.size() != 5 && param->args.size() != 6){
                ELOG("Usage: pok3rtool keymap set <layer> [<key_row> <key_col> | <key>] <keycode>");
                return -2;
            }

            auto keymap = qmk->loadKeymap();
            if(!keymap.get()){
                ELOG("Unable to load keymap");
                return -3;
            }

            zu8 layer;
            zu16 kp;
            ZString keycode;
            if(param->args.size() == 5){
                layer = param->args[2].toUint();
                kp = param->args[3].toUint();
                keycode = param->args[4];
            } else if(param->args.size() == 6){
                layer = param->args[2].toUint();
                kp = keymap->layoutRC2K(param->args[3].toUint(), param->args[4].toUint());
                keycode = param->args[5];
            } else {
                return -4;
            }

            Keymap::keycode kc = keymap->toKeycode(keycode);

            LOG("Keymap Set: layer " << layer << ", key " << kp << " -> " << keymap->keycodeName(kc));

            keymap->set(layer, kp, kc);
            //keymap->printLayers();
            LOG(qmk->uploadKeymap(keymap));

        } else if(param->args[1] == "commit"){
            if(param->args.size() != 2){
                ELOG("Usage: pok3rtool keymap commit");
                return -2;
            }
            LOG("Commit Keymap");
            LOG(qmk->commitKeymap());

        } else if(param->args[1] == "reload"){
            if(param->args.size() != 2){
                ELOG("Usage: pok3rtool keymap reload");
                return -2;
            }
            LOG("Reload Keymap");
            LOG(qmk->reloadKeymap());

        } else if(param->args[1] == "reset"){
            if(param->args.size() != 2){
                ELOG("Usage: pok3rtool keymap reset");
                return -2;
            }
            LOG("Reset Keymap");
            LOG(qmk->resetKeymap());

        } else if(param->args[1] == "layouts"){
            LOG("Layouts");
            ZArray<ZString> layouts;
            qmk->getLayouts(layouts);
            for(zu8 i = 0; i < layouts.size(); ++i){
                LOG(i << ": " << layouts[i]);
            }

        } else if(param->args[1] == "setlayout"){
            if(param->args.size() != 3){
                ELOG("Usage: pok3rtool keymap setlayout <layout>");
                return -2;
            }
            ZString layout = param->args[2];
            ZArray<ZString> layouts;
            qmk->getLayouts(layouts);
            for(zu8 i = 0; i < layouts.size(); ++i){
                if(layouts[i] == layout){
                    LOG("Set Layout " << layout);
                    LOG(qmk->setLayout(i));
                    return 0;
                }
            }
            LOG("No Such Layout " << layout);
            return -3;

        } else {
            LOG("Usage: pok3rtool keymap");
        }
        return 0;
    }
    return -1;
}

int cmd_console(Param *param){
    while(true){
        ZPointer<HIDDevice> con = KBScan::openConsole(param->device);
        if(con.get()){
            LOG("Opened console");

            ZBinary bin;
            while(true){
                if(con->recv(bin)){
                    RLOG(bin.asChar());
                } else {
                    ELOG("Error");
                    break;
                }
            }

            return 0;
        }
    }
}

// Main
// ////////////////////////////////

#define OPT_OK      "ok"
#define OPT_VERBOSE "verbose"
#define OPT_TYPE    "device"

const ZArray<ZOptions::OptDef> optdef = {
    { OPT_OK,       0,   ZOptions::NONE },
    { OPT_VERBOSE,  'v', ZOptions::NONE},
    { OPT_TYPE,     't', ZOptions::STRING },
};

typedef int (*cmd_func)(Param *);
struct CmdEntry {
    cmd_func func;
    unsigned argmin;
    unsigned argmax;
    ZString usage;
};

const ZMap<ZString, CmdEntry> cmds = {
    { "list",       { cmd_list,         0, 0, "list" } },
    { "version",    { cmd_version,      0, 0, "version" } },
    { "setversion", { cmd_setversion,   1, 1, "setversion <version>" } },
    { "info",       { cmd_info,         0, 0, "info" } },
    { "reboot",     { cmd_reboot,       0, 0, "reboot" } },
    { "bootloader", { cmd_bootloader,   0, 0, "bootloader" } },
    { "dump",       { cmd_dump,         1, 1, "dump <flash dump>" } },
    { "flash",      { cmd_flash,        2, 2, "flash <version> <firmware>" } },
    { "wipe",       { cmd_wipe,        	0, 0, "wipe" } },
    { "decode",     { cmd_decode,       2, 2, "decode <path to updater> <output file>" } },
    { "eeprom",     { cmd_eeprom,       1, 2, "eeprom <cmd> [arg]" } },
    { "keymap",     { cmd_keymap,       1, 5, "keymap <cmd> [arg]" } },
    { "console",    { cmd_console,      0, 0, "console" } },
};

void printUsage(){
    LOG("Pok3rTool Usage:");
    for(auto it = cmds.begin(); it.more(); ++it){
        LOG("    pok3rtool " << cmds[it.get()].usage);
    }
}

#define TERM_RESET  "\x1b[m"
#define TERM_RED    "\x1b[31m"
#define TERM_PURPLE "\x1b[35m"

int main(int argc, char **argv){
    // Log files
    ZPath lgf = ZPath("logs") + ZLog::genLogFileName("pok3rtool_");
    ZLog::defaultWorker()->logLevelFile(ZLog::INFO, lgf, "[%clock%] %thread% N %log%");
    ZLog::defaultWorker()->logLevelFile(ZLog::DEBUG, lgf, "[%clock%] %thread% D [%function%|%file%:%line%] %log%");
    ZLog::defaultWorker()->logLevelFile(ZLog::ERRORS, lgf, "[%clock%] %thread% E [%function%|%file%:%line%] %log%");

    ZString optbuf;
    for(int i = 0; i < argc; ++i){
        optbuf += argv[i];
        optbuf += " ";
    }
    DLOG("Command Line: " << optbuf);

    ZOptions options(optdef);
    if(!options.parse(argc, argv))
        return -2;

    // Console log
    if(options.getOpts().contains(OPT_VERBOSE)){
        ZLog::defaultWorker()->logLevelStdOut(ZLog::INFO, "[%clock%] N %log%");
        ZLog::defaultWorker()->logLevelStdOut(ZLog::DEBUG, TERM_PURPLE "[%clock%] D %log%" TERM_RESET);
        ZLog::defaultWorker()->logLevelStdErr(ZLog::ERRORS, TERM_RED "[%clock%] E %log%" TERM_RESET);
    } else {
        ZLog::defaultWorker()->logLevelStdOut(ZLog::INFO, "%log%");
        ZLog::defaultWorker()->logLevelStdErr(ZLog::ERRORS, TERM_RED "%log%" TERM_RESET);
    }

    Param param;
    param.device = DEV_NONE;
    param.ok = options.getOpts().contains(OPT_OK);
    param.args = options.getArgs();

    if(options.getOpts().contains(OPT_TYPE)){
        ZString type = options.getOpts()[OPT_TYPE];
        if(devnames.contains(type))
            param.device = devnames[type];
    }

    if(param.args.size()){
        ZString cmstr = param.args[0];
        if(cmds.contains(cmstr)){
            CmdEntry cmd = cmds[cmstr];
            if((param.args.size() >= cmd.argmin + 1) && (param.args.size() <= cmd.argmax + 1)){
                try {
                    return cmd.func(&param);
                } catch(const ZException &e){
                    ELOG("ERROR: " << e.what());
                    ELOG("Trace: " << e.traceStr());
#if 1
                } catch(const zexception &e){
                    ELOG("FATAL: " << e.what);
#endif
                }
                return -2;
            } else {
                LOG("Usage: pok3rtool " << cmd.usage);
                return -1;
            }
        } else {
            LOG("Unknown Command \"" << cmstr << "\"");
            printUsage();
            return -1;
        }
    } else {
        LOG("No Command");
        printUsage();
        return -1;
    }
}
