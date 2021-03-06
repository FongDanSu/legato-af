//--------------------------------------------------------------------------------------------------
/**
 * @file buildParams.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_BUILD_PARAMS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_BUILD_PARAMS_H_INCLUDE_GUARD

namespace mk
{


//--------------------------------------------------------------------------------------------------
/**
 * Object that holds build parameters gathered from the command line.
 */
//--------------------------------------------------------------------------------------------------
struct BuildParams_t
{
    bool                    beVerbose;          ///< true = output progress msgs to stdout.
    std::string             target;             ///< (e.g., "localhost" or "ar7")
    std::list<std::string>  interfaceDirs;      ///< Interface search directory paths.
    std::list<std::string>  sourceDirs;         ///< Source search directory paths.
    std::string             libOutputDir;       ///< Dir path for built libraries ("" if not set).
    std::string             outputDir;          ///< Dir path for built products ("" if not set).
    std::string             workingDir;         ///< Dir path for intermediate build products.
    std::string             debugDir;           ///< Dir path for debug symbol files.
                                                ///< If unset no debug symbols are generated.
    std::string             cFlags;             ///< Flags to be passed to the C compiler.
    std::string             cxxFlags;           ///< Flags to be passed to the C++ compiler.
    std::string             ldFlags;            ///< Flags to be passed to the linker.
    bool                    codeGenOnly;        ///< true = only generate code, don't compile, etc.
    bool                    isStandAloneComp;   ///< true = generate stand-alone component
    bool                    binPack;            ///< true = generate a binary package for redist.

    int                     argc;               ///< Number of arguments (argc to main)
    const char**            argv;               ///< Argument list (argv to main)

    // File system paths to tool chain executables
    std::string             cCompilerPath;      ///< C compiler
    std::string             cxxCompilerPath;    ///< C++ compiler
    std::string             sysrootPath;        ///< Root directory of compiler's system files
    std::string             linkerPath;         ///< Linker
    std::string             archiverPath;       ///< Static library archiver
    std::string             assemblerPath;      ///< Assembler
    std::string             stripPath;          ///< Debug symbol stripper (needed for -d option)
    std::string             objcopyPath;        ///< Object file copier (needed for -d option)
    std::string             readelfPath;        ///< ELF file reader (needed for -d option)

    /// Constructor
    BuildParams_t();
};


} // namespace mk

#endif // LEGATO_MKTOOLS_BUILD_PARAMS_H_INCLUDE_GUARD
