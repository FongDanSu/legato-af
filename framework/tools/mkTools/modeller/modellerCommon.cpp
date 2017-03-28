//--------------------------------------------------------------------------------------------------
/**
 * @file modellerCommon.cpp
 *
 * Functions shared by multiple modeller modules.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include <stdlib.h>
#include <climits>

#include "modellerCommon.h"

namespace modeller
{


//--------------------------------------------------------------------------------------------------
/**
 * Binds a client-side interface to a service provided by the root user.
 */
//--------------------------------------------------------------------------------------------------
static void BindToRootService
(
    model::App_t* appPtr,       ///< App the interface belongs to.
    model::ApiClientInterfaceInstance_t* ifInstancePtr,    ///< Interface to be bound.
    const std::string& serviceName                   ///< Name of server-side interface to bind to.
)
//--------------------------------------------------------------------------------------------------
{
    auto bindingPtr = new model::Binding_t(NULL);
    bindingPtr->clientType = model::Binding_t::INTERNAL;
    bindingPtr->clientAgentName = appPtr->name;
    bindingPtr->clientIfName = ifInstancePtr->name;
    bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
    bindingPtr->serverAgentName = "root";
    bindingPtr->serverIfName = serviceName;
    ifInstancePtr->bindingPtr = bindingPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the validity of a binding's target.
 *
 * @throw mk::Exception_t if the binding is definitely invalid.
 **/
//--------------------------------------------------------------------------------------------------
void CheckBindingTarget
(
    model::System_t* systemPtr,
    model::Binding_t* bindingPtr
)
//--------------------------------------------------------------------------------------------------
{
    // We can only check if it's a binding to an app.  We don't know what
    // non-app users are going to exist on the system.
    // Also, note that we don't have to check internal bindings, because
    // they will have been checked when the binding was created.
    if (bindingPtr->serverType == model::Binding_t::EXTERNAL_APP)
    {
        auto appMapIter = systemPtr->apps.find(bindingPtr->serverAgentName);
        if (appMapIter == systemPtr->apps.end())
        {
            bindingPtr->parseTreePtr->ThrowException("Binding to non-existent server app '"
                                                     + bindingPtr->serverAgentName + "'.");
        }
        auto appPtr = appMapIter->second;

        auto ifMapIter = appPtr->externServerInterfaces.find(bindingPtr->serverIfName);
        if (ifMapIter == appPtr->externServerInterfaces.end())
        {
            bindingPtr->parseTreePtr->ThrowException("Binding to non-existent server interface '"
                                                     + bindingPtr->serverIfName + "' on "
                                                     "app '" + bindingPtr->serverAgentName + "'.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Verifies that all client-side interfaces of all applications in a system have been bound
 * to something.  Will auto-bind any unbound le_cfg or le_wdog interfaces it finds.
 *
 * @throw mk::Exception_t if any client-side interface is unbound.
 */
//--------------------------------------------------------------------------------------------------
void EnsureClientInterfacesBound
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto appMapEntry : systemPtr->apps)
    {
        auto appPtr = appMapEntry.second;

        for (auto mapItem : appPtr->executables)
        {
            auto exePtr = mapItem.second;

            for (auto componentInstancePtr : exePtr->componentInstances)
            {
                for (auto ifInstancePtr : componentInstancePtr->clientApis)
                {
                    // If the client-side interface is unbound,
                    if (ifInstancePtr->bindingPtr == NULL)
                    {
                        // If the binding of this interface is not optional,
                        if ( ! (ifInstancePtr->ifPtr->optional))
                        {
                            // If le_cfg API, then bind it to the one served by the root user.
                            if (ifInstancePtr->ifPtr->internalName == "le_cfg")
                            {
                                BindToRootService(appPtr, ifInstancePtr, "le_cfg");
                            }
                            // If le_wdog API, then bind it to the one served by the root user.
                            else if (ifInstancePtr->ifPtr->internalName == "le_wdog")
                            {
                                BindToRootService(appPtr, ifInstancePtr, "le_wdog");
                            }
                            // At this point, we know it's an error, just need to figure out which
                            // type of error message to report (depending on whether the interface
                            // has been marked "extern" or not).
                            else if (ifInstancePtr->externMarkPtr != NULL)
                            {
                                throw mk::Exception_t("Client interface '"
                                                      + appPtr->name + "."
                                                      + ifInstancePtr->name + "' (aka '"
                                                      + appPtr->name + "."
                                                      + exePtr->name + "."
                                                      + componentInstancePtr->componentPtr->name
                                                      + "."
                                                      + ifInstancePtr->ifPtr->internalName
                                                      + "') is not bound to anything.");
                            }
                            else
                            {
                                throw mk::Exception_t("Client interface '"
                                                      + appPtr->name + "."
                                                      + ifInstancePtr->name
                                                      + "' is not bound to anything.");
                            }
                        }
                    }
                    else
                    {
                        // It has a binding, but is it a good binding?
                        CheckBindingTarget(systemPtr, ifInstancePtr->bindingPtr);
                    }
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Verifies that all client-side interfaces of an application have either been bound to something
 * or marked as an external interface to be bound at the system level.  Will auto-bind any unbound
 * le_cfg or le_wdog interfaces it finds.
 *
 * @throw mk::Exception_t if any client-side interface is found to be unsatisfied.
 */
//--------------------------------------------------------------------------------------------------
void EnsureClientInterfacesSatisfied
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto mapItem : appPtr->executables)
    {
        auto exePtr = mapItem.second;

        for (auto componentInstancePtr : exePtr->componentInstances)
        {
            for (auto ifInstancePtr : componentInstancePtr->clientApis)
            {
                if ((ifInstancePtr->bindingPtr == NULL) && (ifInstancePtr->externMarkPtr == NULL))
                {
                    // If the binding of this interface is not optional,
                    if ( ! (ifInstancePtr->ifPtr->optional))
                    {
                        // If this is an le_cfg API, bind it to the one offered by the root user.
                        if (ifInstancePtr->ifPtr->internalName == "le_cfg")
                        {
                            BindToRootService(appPtr, ifInstancePtr, "le_cfg");
                        }
                        // If this is an le_wdog API, bind it to the one offered by the root user.
                        else if (ifInstancePtr->ifPtr->internalName == "le_wdog")
                        {
                            BindToRootService(appPtr, ifInstancePtr, "le_wdog");
                        }
                        else
                        {
                            throw mk::Exception_t("Client interface '"
                                                  + ifInstancePtr->ifPtr->internalName
                                                  + "' of component '"
                                                  + componentInstancePtr->componentPtr->name
                                                  + "' in executable '" + exePtr->name
                                                  + "' is unsatisfied. It must either be declared"
                                                  " an external (inter-app) required interface"
                                                  " (in a \"requires: api:\" section in the .adef)"
                                                  " or be bound to a server side interface"
                                                  " (in the \"bindings:\" section of the .adef).");
                        }
                    }
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set permissions inside a Permissions_t object based on the contents of a FILE_PERMISSIONS token.
 */
//--------------------------------------------------------------------------------------------------
void GetPermissions
(
    model::Permissions_t& permissions,  ///< [OUT]
    const parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Permissions string always starts with '[' and ends with ']'.
    // Could contain 'r', 'w', or 'x'.
    const std::string& permissionsText = tokenPtr->text;
    for (int i = 1; permissionsText[i] != ']'; i++)
    {
        switch (permissionsText[i])
        {
            case 'r':
                permissions.SetReadable();
                break;
            case 'w':
                permissions.SetWriteable();
                break;
            case 'x':
                permissions.SetExecutable();
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given file or directory, that may optionally contain
 * permissions, in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
static model::FileSystemObject_t* GetPermissionItem
(
    const parseTree::TokenList_t* itemPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto fileSystemObjectPtr = new model::FileSystemObject_t(itemPtr);

    const std::string* srcPathPtr;
    const std::string* destPathPtr;

    // Optionally, there may be permissions.
    auto firstTokenPtr = itemPtr->Contents()[0];
    if (firstTokenPtr->type == parseTree::Token_t::FILE_PERMISSIONS)
    {
        srcPathPtr = &(itemPtr->Contents()[1]->text);
        destPathPtr = &(itemPtr->Contents()[2]->text);

        GetPermissions(fileSystemObjectPtr->permissions, firstTokenPtr);
    }
    // If no permissions, default to read-only.
    else
    {
        srcPathPtr = &(firstTokenPtr->text);
        destPathPtr = &(itemPtr->Contents()[1]->text);

        fileSystemObjectPtr->permissions.SetReadable();
    }

    fileSystemObjectPtr->srcPath = path::Unquote(envVars::DoSubstitution(*srcPathPtr));
    fileSystemObjectPtr->destPath = path::Unquote(envVars::DoSubstitution(*destPathPtr));

    // If the destination path ends in a slash, append the last path node from the source to it.
    if (fileSystemObjectPtr->destPath.back() == '/')
    {
        fileSystemObjectPtr->destPath += path::GetLastNode(fileSystemObjectPtr->srcPath);
    }

    return fileSystemObjectPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given bundled file or directory in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetBundledItem
(
    const parseTree::TokenList_t* itemPtr
)
//--------------------------------------------------------------------------------------------------
{
    return GetPermissionItem(itemPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given required file or directory in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetRequiredFileOrDir
(
    const parseTree::TokenList_t* itemPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto srcPathTokenPtr = itemPtr->Contents()[0];
    auto destPathTokenPtr = itemPtr->Contents()[1];

    std::string srcPath = path::Unquote(envVars::DoSubstitution(srcPathTokenPtr->text));
    std::string destPath = path::Unquote(envVars::DoSubstitution(destPathTokenPtr->text));

    // The source path must not end in a slash.
    if (srcPath.back() == '/')
    {
        srcPathTokenPtr->ThrowException("Required item's path must not end in a '/'.");
    }

    // If the destination path ends in a slash, append the last path node from the source to it.
    if (destPath.back() == '/')
    {
        destPath += path::GetLastNode(srcPath);
    }

    auto fileSystemObjPtr = new model::FileSystemObject_t(itemPtr);
    fileSystemObjPtr->srcPath = srcPath;
    fileSystemObjPtr->destPath = destPath;

    // Note: Items bind-mounted into the sandbox from outside have the permissions they
    //       have inside the target filesystem.  This cannot be changed by the app.

    return fileSystemObjPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a FileSystemObject_t instance for a given device in the parse tree.
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
model::FileSystemObject_t* GetRequiredDevice
(
    const parseTree::TokenList_t* itemPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto permPtr = GetPermissionItem(itemPtr);

    // Execute permissions are not allowed on devices.
    if (permPtr->permissions.IsExecutable())
    {
        throw std::invalid_argument("Execute permission is not allowed on devices: '"
                                                + permPtr->srcPath + "'");
    }

    return permPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the integer value from a simple (name: value) section and verifies that it is
 * non-negative.
 *
 * @return the value.
 *
 * @throw mkToolsException if out of range.
 */
//--------------------------------------------------------------------------------------------------
size_t GetNonNegativeInt
(
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto valueTokenPtr = sectionPtr->Contents()[0];
    char* endPtr;
    errno = 0;
    size_t result = strtoul(valueTokenPtr->text.c_str(), &endPtr, 0);
    if (errno != 0)
    {
        std::stringstream msg;
        msg << "Value must be an integer between 0 and " << ULONG_MAX << ", with an optional 'K'"
               " suffix.";
        valueTokenPtr->ThrowException(msg.str());
    }
    if (*endPtr == 'K')
    {
        result *= 1024;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the signed integer value from a simple (name: value) section.
 *
 * @return the value.
 *
 * @throw mkToolsException if out of range.
 */
//--------------------------------------------------------------------------------------------------
ssize_t GetInt
(
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto valueTokenPtr = sectionPtr->Contents()[0];
    char* endPtr;
    errno = 0;
    size_t result = strtol(valueTokenPtr->text.c_str(), &endPtr, 0);
    if (errno != 0)
    {
        std::stringstream msg;
        msg << "Value must be an integer between " << LONG_MIN << " and " << LONG_MAX
            << ", with an optional 'K' suffix.";
        valueTokenPtr->ThrowException(msg.str());
    }
    if (*endPtr == 'K')
    {
        result *= 1024;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts the integer value from a simple (name: value) section and verifies that it is
 * positive.
 *
 * @return the value.
 *
 * @throw mkToolsException if out of range.
 */
//--------------------------------------------------------------------------------------------------
size_t GetPositiveInt
(
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    size_t result = GetNonNegativeInt(sectionPtr);

    if (result == 0)
    {
        std::stringstream msg;
        msg << "Value must be an integer between 1 and " << ULONG_MAX << ", with an optional 'K'"
               " suffix.";
        sectionPtr->Contents()[0]->ThrowException(msg.str());
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print permissions to stdout.
 **/
//--------------------------------------------------------------------------------------------------
void PrintPermissions
(
    const model::Permissions_t& permissions
)
//--------------------------------------------------------------------------------------------------
{
    if (permissions.IsReadable())
    {
        std::cout << " read";
    }
    if (permissions.IsWriteable())
    {
        std::cout << " write";
    }
    if (permissions.IsExecutable())
    {
        std::cout << " execute";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to remove angle brackets from around a non-app user name specification in an IPC_AGENT
 * token's text.
 *
 * E.g., if the agentName is "<root>", then "root" will be returned.
 *
 * @return The user name, without the angle brackets.
 */
//--------------------------------------------------------------------------------------------------
std::string RemoveAngleBrackets
(
    const std::string& agentName    ///< The user name with angle brackets around it.
)
//--------------------------------------------------------------------------------------------------
{
    return agentName.substr(1, agentName.length() - 2);
}


//--------------------------------------------------------------------------------------------------
/**
 * Makes the application a member of groups listed in a given "groups" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
void AddGroups
(
    model::App_t* appPtr,
    const parseTree::TokenListSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto tokenPtr : sectionPtr->Contents())
    {
        appPtr->groups.insert(tokenPtr->text);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets whether the Supervisor will start the application automatically at system start-up,
 * or only when asked to do so, based on the contents of a "start:" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
void SetStart
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& mode = sectionPtr->Text();

    if (mode == "auto")
    {
        appPtr->startTrigger = model::App_t::AUTO;
    }
    else if (mode == "manual")
    {
        appPtr->startTrigger = model::App_t::MANUAL;
    }
    else
    {
        sectionPtr->Contents()[0]->ThrowException("Internal error: unexpected startup option.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the app-level watchdog action setting.
 */
//--------------------------------------------------------------------------------------------------
void SetWatchdogAction
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->watchdogAction.IsSet())
    {
        sectionPtr->ThrowException("Only one watchdogAction section allowed.");
    }
    appPtr->watchdogAction = sectionPtr->Text();
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the app-level watchdog timeout setting.
 */
//--------------------------------------------------------------------------------------------------
void SetWatchdogTimeout
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->watchdogTimeout.IsSet())
    {
        sectionPtr->ThrowException("Only one watchdogTimeout section allowed.");
    }

    auto tokenPtr = sectionPtr->Contents()[0];
    if (tokenPtr->type == parseTree::Token_t::NAME)
    {
        appPtr->watchdogTimeout = -1;   // Never timeout (watchdog disabled).
    }
    else
    {
        appPtr->watchdogTimeout = GetInt(sectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the API File object for a given .api file path.
 **/
//--------------------------------------------------------------------------------------------------
model::ApiFile_t* GetApiFilePtr
(
    const std::string& apiFile,
    const std::list<std::string>& searchList,   ///< List of dirs to search for .api files.
    const parseTree::Token_t* tokenPtr  ///< Token to use to throw error exceptions.
)
//--------------------------------------------------------------------------------------------------
{
    auto apiFilePtr = model::ApiFile_t::GetApiFile(apiFile);

    if (apiFilePtr == NULL)
    {
        apiFilePtr = model::ApiFile_t::CreateApiFile(apiFile);

        // Handler function that gets called for each USETYPES in the .api file.
        // Finds that .api file and adds it to this .api file's list of includes.
        auto handler = [&apiFilePtr, &searchList, &tokenPtr](std::string&& dependency)
        {
            // Check if there is api suffix and if not add .api, as suffixes are not
            // required in USETYPES
            if (!path::HasSuffix(dependency, ".api"))
            {
                dependency += ".api";
            }

            // First look in the same directory as the .api file that is doing the including.
            auto dir = path::GetContainingDir(apiFilePtr->path);
            std::string includedFilePath = file::FindFile(dependency, { dir });

            // If not found there, look through the search directory list.
            if (includedFilePath.empty())
            {
                includedFilePath = file::FindFile(dependency, searchList);
                if (includedFilePath.empty())
                {
                    tokenPtr->ThrowException("Can't find dependent .api file: "
                                             "'" + dependency + "'.");
                }
            }

            // Get the API File object for the included file.
            auto includedFilePtr = GetApiFilePtr(includedFilePath, searchList, tokenPtr);

            // Mark the included file "included".
            includedFilePtr->isIncluded = true;

            // Add the included file to the list of files included by the including file.
            apiFilePtr->includes.push_back(includedFilePtr);
        };

        // Parse the .api file to figure out what it depends on.  Call the handler function
        // for each .api file that is included.
        parser::api::GetDependencies(apiFile, handler);
    }

    return apiFilePtr;
}




} // namespace modeller
