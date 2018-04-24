#include "kbscan.h"
#include "proto_pok3r.h"
#include "proto_cykb.h"
#include "updatepackage.h"

#include "zlog.h"
#include "zfile.h"
#include "zpointer.h"
#include "zmap.h"
#include "zoptions.h"
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

    { "pok3r-rgb",          DEV_POK3R_RGB },
    { "pok3r_rgb",          DEV_POK3R_RGB },

    { "core",               DEV_VORTEX_CORE },
    { "vortex-core",        DEV_VORTEX_CORE },
    { "vortex_core",        DEV_VORTEX_CORE },
    
    { "race3",              DEV_VORTEX_RACE3 },
    { "vortex-race3",       DEV_VORTEX_RACE3 },
    { "vortex_race3",       DEV_VORTEX_RACE3 },

    { "tester",             DEV_VORTEX_TESTER },
    { "vortex-tester",      DEV_VORTEX_TESTER },
    { "vortex_tester",      DEV_VORTEX_TESTER },

    { "vibe",               DEV_VORTEX_VIBE },
    { "vortex-vibe",        DEV_VORTEX_VIBE },
    { "vortex_vibe",        DEV_VORTEX_VIBE },

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

    { "qmk_pok3r",          DEV_QMK_POK3R },
    { "qmk_pok3r_rgb",      DEV_QMK_POK3R_RGB },
    { "qmk_vortex_core",    DEV_QMK_VORTEX_CORE },
};

// Functions
// ////////////////////////////////

ZPointer<KBProto> openDevice(DeviceType dev){
    KBScan scanner;
    if(!scanner.find(dev))
        return nullptr;

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
        ELOG("No device found, check connection and permissions");
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
        LOG(kb->getInfo());
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

    ZPath out1 = param->args[1];
    // Dump Flash
    ZPointer<KBProto> kb = openDevice(param->device);
    if(kb.get()){
        /*
        LOG("Dump Flash");
        ZBinary bin1 = kb->dumpFlash();
//        RLOG(bin.dumpBytes(4, 8));
        LOG("Out: " << out1 << ", " << bin1.size() << " bytes");
        ZFile::writeBinary(out1, bin1);
        */

        if(param->args.size() > 2){
            ZPath out2 = param->args[2];
            LOG("Dump EEPROM");
            ZBinary bin2 = kb->dumpEEPROM();
            LOG("Out: " << out2 << ", " << bin2.size() << " bytes");
            ZFile::writeBinary(out2, bin2);
        }
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

        if(param->args[1] == "test"){
            LOG("EEPROM Test");
            bool ret = kb->eepromTest();
            LOG(ret);
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

        if(param->args[1] == "dump"){
            LOG("Keymap Dump");
            kb->keymapDump();
        }
        return 0;
    }
    return -1;
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
    { "dump",       { cmd_dump,         1, 2, "dump <flash dump> [eeprom dump]" } },
    { "flash",      { cmd_flash,        2, 2, "flash <version> <firmware>" } },
    { "wipe",       { cmd_wipe,        	0, 0, "wipe" } },
    { "decode",     { cmd_decode,       2, 2, "decode <path to updater> <output file>" } },
    { "eeprom",     { cmd_eeprom,       1, 2, "eeprom <cmd> [arg]" } },
    { "keymap",     { cmd_keymap,       1, 2, "keymap <cmd> [arg]" } },
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
    ZLog::logLevelFile(ZLog::INFO, lgf, "[%clock%] N %log%");
    ZLog::logLevelFile(ZLog::DEBUG, lgf, "[%clock%] %thread% D [%function%|%file%:%line%] %log%");
    ZLog::logLevelFile(ZLog::ERRORS, lgf, "[%clock%] E [%function%|%file%:%line%] %log%");

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
    ZLog::logLevelStdOut(ZLog::INFO, "[%clock%] N %log%");
    ZLog::logLevelStdErr(ZLog::ERRORS, TERM_RED "[%clock%] E %log%" TERM_RESET);
    if(options.getOpts().contains(OPT_VERBOSE)){
        ZLog::logLevelStdOut(ZLog::DEBUG, TERM_PURPLE "[%clock%] D %log%" TERM_RESET);
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
                } catch(ZException e){
                    ELOG("ERROR: " << e.what());
                    ELOG("Trace: " << e.traceStr());
                }
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
