//--------------------------------------------------------------------------------------------------
/**
 * @file appBuildScript.cpp
 *
 * Implementation of the build script generator for applications.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "exeBuildScript.h"
#include "appBuildScript.h"
#include "componentBuildScript.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>


namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for an app build script.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateCommentHeader
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for application '" << appPtr->name << "'\n"
            "\n"
            "# == Auto-generated file.  Do not edit. ==\n"
            "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate app build rules.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateAppBuildRules
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    script <<
        // Add a bundled file into the app's staging area.
        "rule BundleFile\n"
        "  description = Bundling file\n"
        "  command = legato-install -m $modeFlags $in $out\n"
        "\n"

        // Generate a rule for creating an info.properties file.
        "rule MakeAppInfoProperties\n"
        "  description = Creating info.properties\n"
        // Delete the old info.properties file, if there is one.
        "  command = rm -f $out && $\n"
        // Compute the MD5 checksum of the staging area.
        // Don't follow symlinks (-P), and include the directory structure and the contents of
        // symlinks as part of the MD5 hash.
        "            md5=$$( ( cd $workingDir/staging && $\n"
        "                      find -P -print0 |LC_ALL=C sort -z && $\n"
        "                      find -P -type f -print0 |LC_ALL=C sort -z |xargs -0 md5sum && $\n"
        "                      find -P -type l -print0 |LC_ALL=C sort -z"
                             " |xargs -0 -r -n 1 readlink $\n"
        "                    ) | md5sum) && $\n"
        "            md5=$${md5%% *} && $\n"
        // Generate the app's info.properties file.
        "            ( echo \"app.name=$name\" && $\n"
        "              echo \"app.md5=$$md5\" && $\n"
        "              echo \"app.version=$version\" && $\n"
        "              echo \"legato.version=`cat $$LEGATO_ROOT/version`\" $\n"
        "            ) > $out\n"
        "\n"

        // Create an update pack file for an app.
        "rule PackApp\n"
        "  description = Packaging app\n"
        // Pack the staging area into a tarball.
        "  command = (cd $workingDir/staging && find . -print0 | LC_ALL=C sort -z"
                     " |tar --no-recursion --null -T -"
                        " -cjf - --mtime=$adefPath) > $workingDir/$name.$target && $\n"
        // Get the size of the tarball.
        "            tarballSize=`stat -c '%s' $workingDir/$name.$target` && $\n"
        // Get the app's MD5 hash from its info.properties file.
        "            md5=`grep '^app.md5=' $in | sed 's/^app.md5=//'` && $\n"
        // Generate a JSON header and concatenate the tarball to it to create the update pack.
        "            ( printf '{\\n' && $\n"
        "              printf '\"command\":\"updateApp\",\\n' && $\n"
        "              printf '\"name\":\"$name\",\\n' && $\n"
        "              printf '\"version\":\"$version\",\\n' && $\n"
        "              printf '\"md5\":\"%s\",\\n' \"$$md5\" && $\n"
        "              printf '\"size\":%s\\n' \"$$tarballSize\" && $\n"
        "              printf '}' && $\n"
        "              cat $workingDir/$name.$target $\n"
        "            ) > $out\n"
        "\n"

        "rule BinPackApp\n"
        "  description = Packaging app for distribution.\n"
        "  command = cp -r $stagingDir/* $workingDir/ && $\n"
        "            rm $workingDir/info.properties $workingDir/root.cfg && $\n"
        "            (cd $workingDir/ && find . -print0 |LC_ALL=C sort -z"
                     " |tar --no-recursion --null -T - -cjf - --mtime=$adefPath) > $out\n"
        "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates build statements for all the executables in a given app.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateExeBuildStatements
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto mapItem : appPtr->executables)
    {
        exeGeneratorPtr->GenerateBuildStatements(mapItem.second);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a permission string for chmod based on the permissions we want to set on the target
 * file.
 **/
//--------------------------------------------------------------------------------------------------
static std::string PermissionsToModeFlags
(
    model::Permissions_t permissions  ///< The permission flags to set on the given file(s).
)
//--------------------------------------------------------------------------------------------------
{
    std::string flags;
    std::string executableFlag = (permissions.IsExecutable()?"+x":"-x");

    flags = "u+rw" + executableFlag +
            ",g+r" + executableFlag +
            ",o" + executableFlag;

    if (permissions.IsReadable())
    {
        flags += "+r";
    }
    else
    {
        flags += "-r";
    }

    if (permissions.IsWriteable())
    {
        flags += "+w";
    }
    else
    {
        flags += "-w";
    }

    return flags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 *
 * Adds the absolute destination file path to the bundledFiles set.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateFileBundleBuildStatement
(
    const model::FileSystemObject_t& fileObject,  ///< File object to generate
    model::FileSystemObjectSet_t& bundledFiles    ///< Set to fill with bundled file paths.
)
//--------------------------------------------------------------------------------------------------
{
    std::string containingDir = path::GetContainingDir(fileObject.destPath);
    auto bundledFileIter = bundledFiles.find(fileObject);

    if (bundledFileIter == bundledFiles.end())
    {
        script << "build " << fileObject.destPath << " : BundleFile " << fileObject.srcPath << "\n"
               << "  modeFlags = " << PermissionsToModeFlags(fileObject.permissions) << "\n";

        bundledFiles.insert(fileObject);
    }
    else
    {
        if (fileObject.srcPath != bundledFileIter->srcPath)
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("error: Cannot bundle file '%s' with destination '%s' since it"
                                   " conflicts with existing bundled file '%s'."),
                           fileObject.srcPath, fileObject.destPath, bundledFileIter->srcPath)
            );
        }
        else if (fileObject.permissions != bundledFileIter->permissions)
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("error: Cannot bundle file '%s'.  It is already bundled with"
                                   " different permissions."),
                           fileObject.srcPath)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling files from a directory into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateDirBundleBuildStatements
(
    const model::FileSystemObject_t& fileObject, ///< File system object to bundle
    model::FileSystemObjectSet_t& bundledFiles   ///< Set to fill with bundled file paths.
)
//--------------------------------------------------------------------------------------------------
{
    // Attempt to open the source as a directory stream.
    DIR* dir = opendir(fileObject.srcPath.c_str());
    if (dir == NULL)
    {
        // If failed for some reason other than this just not being a directory,
        if (errno != ENOTDIR)
        {
            int err = errno;
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("Can't access file or directory '%s' (%s)"),
                           fileObject.srcPath,
                           strerror(err))
            );
        }
        // If the source is not a directory,
        else
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("Not a directory: '%s'."), fileObject.srcPath)
            );
        }
    }

    // Loop over list of directory contents.
    for (;;)
    {
        // Setting errno so as to be able to detect errors from end of directory
        // (as recommended in the documentation).
        errno = 0;

        // Read an entry from the directory.
        struct dirent* entryPtr = readdir(dir);

        if (entryPtr == NULL)
        {
            if (errno != 0)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: readdir() failed.  Errno = %s"),
                               strerror(errno))
                );
            }
            else
            {
                // Hit end of the directory.  Nothing more to do.
                break;
            }
        }
        // Skip "." and ".."
        else if ((strcmp(entryPtr->d_name, ".") != 0) && (strcmp(entryPtr->d_name, "..") != 0))
        {
            auto entrySrcPath = path::Combine(fileObject.srcPath, entryPtr->d_name);
            auto entryDestPath = path::Combine(fileObject.destPath, entryPtr->d_name);

            // If this is a directory, then recursively descend into it.
            if (file::DirectoryExists(entrySrcPath))
            {
                GenerateDirBundleBuildStatements(model::FileSystemObject_t(entrySrcPath,
                                                                           entryDestPath,
                                                                           fileObject.permissions,
                                                                           &fileObject),
                                                 bundledFiles);
            }
            // If this is a file, create a build statement for it.
            else if (file::FileExists(entrySrcPath))
            {
                GenerateFileBundleBuildStatement(model::FileSystemObject_t(entrySrcPath,
                                                                           entryDestPath,
                                                                           fileObject.permissions,
                                                                           &fileObject),
                                                 bundledFiles);
            }
            // If this is anything else, we don't support it.
            else
            {
                fileObject.parseTreePtr->ThrowException(
                    mk::format(LE_I18N("File system object is not a directory or a file: '%s'."),
                               entrySrcPath)
                );
            }
        }
    }

    closedir(dir);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateFileBundleBuildStatement
(
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    model::App_t* appPtr, ///< App to bundle the file into.
    const model::FileSystemObject_t* fileSystemObjPtr  ///< File bundling info.
)
//--------------------------------------------------------------------------------------------------
{
    // The file will be put in the app's staging area.
    path::Path_t destPath = "$builddir";
    destPath += appPtr->workingDir;
    destPath += "staging";

    // Put in different place depending on whether it should be read-only or writeable on target.
    if (fileSystemObjPtr->permissions.IsWriteable())
    {
        destPath += "writeable";
    }
    else
    {
        destPath += "read-only";
    }

    destPath += fileSystemObjPtr->destPath;

    GenerateFileBundleBuildStatement(model::FileSystemObject_t(fileSystemObjPtr->srcPath,
                                                               destPath.str,
                                                               fileSystemObjPtr->permissions,
                                                               fileSystemObjPtr),
                                     bundledFiles);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling files from a directory into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateDirBundleBuildStatements
(
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    model::App_t* appPtr, ///< App to bundle the directory into.
    const model::FileSystemObject_t* fileSystemObjPtr  ///< Directory bundling info.
)
//--------------------------------------------------------------------------------------------------
{
    // The files will be put in the app's staging area.
    path::Path_t destPath = "$builddir";
    destPath += appPtr->workingDir;
    destPath += "staging";

    // Put in different place depending on whether the directory contents should be read-only
    // or writeable on target.
    if (fileSystemObjPtr->permissions.IsWriteable())
    {
        destPath += "writeable";
    }
    else
    {
        destPath += "read-only";
    }

    destPath += fileSystemObjPtr->destPath;

    GenerateDirBundleBuildStatements(model::FileSystemObject_t(fileSystemObjPtr->srcPath,
                                                               destPath.str,
                                                               fileSystemObjPtr->permissions,
                                                               fileSystemObjPtr),
                                     bundledFiles);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling a given app's files into the
 * app's staging area.
 *
 * @note Uses a set to track the bundled objects (destination paths) that have been included so far.
 *       This allows us to avoid bundling two files into the same location in the staging area.
 *       The set can also be used later by the calling function to add these staged files to the
 *       bundle's dependency list.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateStagingBundleBuildStatements
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& allBundledFiles = appPtr->getTargetInfo<target::FileSystemAppInfo_t>()->allBundledFiles;

    // Start with the application's list of bundled items first, so they override any items
    // bundled by components.
    // NOTE: Source paths for bundled items are always absolute.
    for (auto fileSystemObjPtr : appPtr->bundledFiles)
    {
        GenerateFileBundleBuildStatement(allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr.get());
    }
    for (auto fileSystemObjPtr : appPtr->bundledDirs)
    {
        GenerateDirBundleBuildStatements(allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr.get());
    }
    for (auto fileSystemObjPtr : appPtr->bundledBinaries)
    {
        GenerateFileBundleBuildStatement(allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr.get());
    }

    // Now do the same for each component in the app, and also generate statements for bundling
    // the component libraries into the app.
    for (auto componentPtr : appPtr->components)
    {
        for (auto fileSystemObjPtr : componentPtr->bundledFiles)
        {
            GenerateFileBundleBuildStatement(allBundledFiles,
                                             appPtr,
                                             fileSystemObjPtr.get());
        }
        for (auto fileSystemObjPtr : componentPtr->bundledDirs)
        {
            GenerateDirBundleBuildStatements(allBundledFiles,
                                             appPtr,
                                             fileSystemObjPtr.get());
        }

        // Generate a statement for bundling a component library into an application, if it has
        // a component library (which will only be the case if the component has sources).
        if ((componentPtr->HasCOrCppCode()) || (componentPtr->HasJavaCode()))
        {
            auto destPath = "$builddir/" + appPtr->workingDir
                          + "/staging/read-only/lib/" +
                path::GetLastNode(componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib);
            auto lib = componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib;

            // Copy the component library into the app's lib directory.
            // Cannot use hard link as this will cause builds to fail occasionally (LE-7383)
            script << "build " << destPath << " : BundleFile " << lib << "\n"
                   << "  modeFlags = " << PermissionsToModeFlags(model::Permissions_t(true,
                                                                                      false,
                                                                                      true))
                   << "\n\n";

            // Add the component library to the set of bundled files.
            allBundledFiles.insert(model::FileSystemObject_t(
                                       lib,
                                       destPath,
                                       model::Permissions_t(true,
                                                            false,
                                                            componentPtr->HasCOrCppCode())));
        }
    }

    // Finally bundle all executables into the app
    for (auto& exeMapPtr : appPtr->executables)
    {
        auto exePtr = exeMapPtr.second;
        auto destPath = "$builddir/" + appPtr->workingDir
            + "/staging/read-only/bin/" + exePtr->name;
        auto exePath = "$builddir/" + exePtr->path;

        // Copy the component library into the app's lib directory.
        // Cannot use hard link as this will cause builds to fail occasionally (LE-7383)
        script << "build " << destPath << " : BundleFile " << exePath << "\n"
               << "  modeFlags = " << PermissionsToModeFlags(model::Permissions_t(true,
                                                                                  false,
                                                                                  true))
               << "\n\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into an application
 * bundle.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateAppBundleBuildStatement
(
    model::App_t* appPtr,
    const std::string& outputDir    ///< Path to the directory into which the built app will be put.
)
//--------------------------------------------------------------------------------------------------
{
    // Give this a FS target info
    appPtr->setTargetInfo(new target::FileSystemAppInfo_t());

    // Generate build statements for bundling files into the staging area.
    GenerateStagingBundleBuildStatements(appPtr);

    // Compute the staging directory path.
    auto stagingDir = "$builddir/" + path::Combine(appPtr->workingDir, "staging");

    // Compute the info.properties file path.
    auto infoPropertiesPath = stagingDir + "/info.properties";

    // Generate build statement for generating the info.properties file.
    script << "build " << infoPropertiesPath << " : MakeAppInfoProperties |";

    // This depends on all the bundled files and executables in the app.
    for (auto filePath : appPtr->getTargetInfo<target::FileSystemAppInfo_t>()->allBundledFiles)
    {
        script << " " << filePath.destPath;
    }
    for (auto mapItem : appPtr->executables)
    {
        script << " $builddir/" << appPtr->workingDir
               << "/staging/read-only/bin/" << mapItem.second->name;
    }

    // It also depends on the generated config file.
    script << " $builddir/" << appPtr->ConfigFilePath();

    // End of dependency list.
    script << "\n";

    // Tell the build rule what the app's name and version are and where its working directory is.
    script << "  name = " << appPtr->name << "\n"
              "  version = " << appPtr->version << "\n"
              "  workingDir = $builddir/" + appPtr->workingDir << "\n"
              "\n";

    // Generate build statement for zipping up the staging area into an update pack file.
    // This depends on the info.properties file, which is the last thing to be added to the
    // app's staging area.
    auto outputFile = path::Combine(outputDir, appPtr->name) + ".$target.update";
    script << "build " << outputFile << ": PackApp " << infoPropertiesPath << "\n";

    // Tell the build rule what the app's name and version are and where its working directory
    // is.
    script << "  name = " << appPtr->name << "\n"
        "  adefPath = " << appPtr->defFilePtr->path << "\n"
        "  version = " << appPtr->version << "\n"
        "  workingDir = $builddir/" + appPtr->workingDir << "\n"
        "\n";

    // Are we building a binary app package as well?
    if (buildParams.binPack)
    {
        const std::string appPackDir = "$builddir/" + appPtr->name;
        const std::string interfacesDir = appPackDir + "/interfaces";

        // We need to copy all the included .api files into the pack directory, so generate rules to
        // do this.
        for (auto apiFile : model::ApiFile_t::GetApiFileMap())
        {
            script << "build " << interfacesDir << "/" << path::GetLastNode(apiFile.second->path)
                   << ": CopyFile " << apiFile.second->path << "\n"
                      "\n";
        }

        // Now, copy all of the app files into the pack directory, and get it packed up as our final
        // output.
        auto outputFile = path::Combine(outputDir, appPtr->name) + ".$target.app";
        script << "build " << outputFile << ": BinPackApp " << infoPropertiesPath;

        if (!model::ApiFile_t::GetApiFileMap().empty())
        {
            script << " ||";

            for (auto apiFile : model::ApiFile_t::GetApiFileMap())
            {
                script << " " << interfacesDir << "/" << path::GetLastNode(apiFile.second->path);
            }
        }

        script << "\n"
                  "  adefPath = " << appPtr->defFilePtr->path << "\n"
                  "  stagingDir = $builddir/" << appPtr->workingDir << "/staging" << "\n"
                  "  workingDir = " << appPackDir << "\n"
                  "\n";


    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // In addition to the .adef file, the build.ninja depends on the .cdef files of all components
    // and all the .api files they use.
    // Create a set of dependencies.
    std::set<std::string> dependencies;
    for (auto componentPtr : appPtr->components)
    {
        dependencies.insert(componentPtr->defFilePtr->path);

        for (auto ifPtr : componentPtr->typesOnlyApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }

        for (auto ifPtr : componentPtr->serverApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }

        for (auto ifPtr : componentPtr->clientApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }

        for (auto apiFilePtr : componentPtr->clientUsetypesApis)
        {
            dependencies.insert(apiFilePtr->path);
        }

        for (auto apiFilePtr : componentPtr->serverUsetypesApis)
        {
            dependencies.insert(apiFilePtr->path);
        }
    }

    baseGeneratorPtr->GenerateNinjaScriptBuildStatement(dependencies);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate all build rules required for building an application.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateBuildRules
(
    void
)
{
    exeGeneratorPtr->GenerateBuildRules();
    GenerateAppBuildRules();
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an application.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::Generate
(
    model::App_t* appPtr
)
{
    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(appPtr);
    std::string includes;
    includes = " -I " + buildParams.workingDir;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }
    script << "builddir =" << buildParams.workingDir << "\n\n";
    script << "cFlags =" << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags =" << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags =" << buildParams.ldFlags << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    GenerateBuildRules();

    // If we are not just generating code,
    if (!buildParams.codeGenOnly)
    {
        // For each component included in executables in this application.
        for (auto componentPtr : appPtr->components)
        {
            componentGeneratorPtr->GenerateBuildStatementsRecursive(componentPtr);
            componentGeneratorPtr->GenerateIpcBuildStatements(componentPtr);
        }

        // For each executable built by the mk tools for this application,
        GenerateExeBuildStatements(appPtr);

        // Generate build statement for packing everything into an application bundle.
        GenerateAppBundleBuildStatement(appPtr, buildParams.outputDir);
    }

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(appPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an application.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    AppBuildScriptGenerator_t appGenerator(filePath, buildParams);

    appGenerator.Generate(appPtr);
}


} // namespace ninja
