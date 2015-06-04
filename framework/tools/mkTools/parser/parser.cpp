//--------------------------------------------------------------------------------------------------
/**
 * @file parser.cpp  Functions used by multiple parsers.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Pulls whitespace and comment tokens and throws them away (although, they still get added to
 * the file's token list).
 */
//--------------------------------------------------------------------------------------------------
void SkipWhitespaceAndComments
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        if (lexer.IsMatch(parseTree::Token_t::WHITESPACE))
        {
            (void)lexer.Pull(parseTree::Token_t::WHITESPACE);
        }
        else if (lexer.IsMatch(parseTree::Token_t::COMMENT))
        {
            (void)lexer.Pull(parseTree::Token_t::COMMENT);
        }
        else
        {
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a simple section.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::SimpleSection_t* ParseSimpleSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,    ///< The token containing the section name.
    parseTree::Token_t::Type_t tokenType        ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::SimpleSection_t(sectionNameTokenPtr);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect the content token next.
    sectionPtr->AddContent(lexer.Pull(tokenType));

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a simple named item.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseSimpleNamedItem
(
    Lexer_t& lexer,
    parseTree::Token_t* nameTokenPtr,           ///< The token containing the name of the item.
    parseTree::Content_t::Type_t contentType,   ///< The named item type.
    parseTree::Token_t::Type_t tokenType        ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto itemPtr = parseTree::CreateTokenList(contentType, nameTokenPtr);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect an '=' next.
    (void)lexer.Pull(parseTree::Token_t::EQUALS);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect the content token next.
    itemPtr->AddContent(lexer.Pull(tokenType));

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a section containing a list of tokens of the same type inside curly braces.
 *
 * This includes "cflags:", "cxxflags:", "ldflags:", "sources:", "groups", and more.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseTokenListSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    parseTree::Token_t::Type_t tokenType    ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::TokenListSection_t(sectionNameTokenPtr);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a '{' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Until we find a closing '}', keep calling the provided content parser function to parse
    // the next content item.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            std::stringstream msg;
            msg << "Unexpected end-of-file before end of " << sectionNameTokenPtr->text
                << " section starting at line " << sectionNameTokenPtr->line
                << " character " << sectionNameTokenPtr->column << ".";
            lexer.ThrowException(msg.str());
        }

        sectionPtr->AddContent(lexer.Pull(tokenType));

        SkipWhitespaceAndComments(lexer);
    }

    // Pull out the '}' and make that the last token in the section.
    sectionPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a compound named item containing a list of tokens of the same type.
 *
 * This includes executables inside the "executables:" section.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseTokenListNamedItem
(
    Lexer_t& lexer,
    parseTree::Token_t* nameTokenPtr,           ///< The token containing the name of the item.
    parseTree::Content_t::Type_t contentType,   ///< The named item type.
    parseTree::Token_t::Type_t tokenType    ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto itemPtr = parseTree::CreateTokenList(contentType, nameTokenPtr);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect an '=' next.
    (void)lexer.Pull(parseTree::Token_t::EQUALS);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a '(' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_PARENTHESIS);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Until we find a closing ')', keep pulling out content tokens and skipping whitespace and
    // comments after each.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_PARENTHESIS))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            std::stringstream msg;
            msg << "Unexpected end-of-file before end of " << itemPtr->TypeName()
                << " named '" << nameTokenPtr->text
                << "' starting at line " << nameTokenPtr->line
                << " character " << nameTokenPtr->column << ".";
            lexer.ThrowException(msg.str());
        }

        itemPtr->AddContent(lexer.Pull(tokenType));

        SkipWhitespaceAndComments(lexer);
    }

    // Pull out the ')' and make that the last token in the section.
    itemPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_PARENTHESIS);

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a complex section (i.e., a section whose content contains compound items, not just tokens).
 *
 * Takes a pointer to a function that gets called to parse each item found in the section.
 * This item parser function returns a pointer to a parsed item to be added to the section's
 * content list, or throws an exception on error.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseComplexSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    std::function<parseTree::CompoundItem_t* (Lexer_t& lexer)> contentParserFunc
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::ComplexSection_t(sectionNameTokenPtr);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a '{' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Until we find a closing '}', keep calling the provided content parser function to parse
    // the next content item.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            std::stringstream msg;
            msg << "Unexpected end-of-file before end of " << sectionNameTokenPtr->text
                << " section starting at line " << sectionNameTokenPtr->line
                << " character " << sectionNameTokenPtr->column << ".";
            lexer.ThrowException(msg.str());
        }

        sectionPtr->AddContent(contentParserFunc(lexer));

        SkipWhitespaceAndComments(lexer);
    }

    // Pull out the '}' and make that the last token in the section.
    sectionPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a compound section containing a list of simple named items whose content are all the same
 * type of token.
 *
 * This includes pools inside a "pools:" section.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseSimpleNamedItemListSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    parseTree::Content_t::Type_t namedItemType, ///< Type of named items to expect.
    parseTree::Token_t::Type_t tokenType        ///< Type of token to expect in the named items.
)
//--------------------------------------------------------------------------------------------------
{
    auto namedItemParser = [namedItemType, tokenType](Lexer_t& lexer)
        {
            return ParseSimpleNamedItem(lexer,
                                        lexer.Pull(parseTree::Token_t::NAME),
                                        namedItemType,
                                        tokenType);
        };

    return ParseComplexSection(lexer, sectionNameTokenPtr, namedItemParser);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a file.  Calls a provided section parser function for each section found in the file.
 *
 * The section parser function must return a pointer to a section (CompoundItem_t, which will be
 * added to the list of sections in the DefFile_t), or throw an exception on error.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
void ParseFile
(
    parseTree::DefFile_t* defFilePtr,   ///< Pointer to the definition file object to populate.
    bool beVerbose,                 ///< true if progress messages should be printed.
    parseTree::CompoundItem_t* (*sectionParserFunc)(Lexer_t& lexer) ///< Section parser function.
)
//--------------------------------------------------------------------------------------------------
{
    if (beVerbose)
    {
        std::cout << "Parsing file: '" << defFilePtr->path << "'." << std::endl;
    }

    // Create a Lexer for this file.
    Lexer_t lexer(defFilePtr);
    lexer.beVerbose = beVerbose;

    // Expect a list of any combination of zero or more whitespace, comment, or sections.
    while (!lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
    {
        if (lexer.IsMatch(parseTree::Token_t::WHITESPACE))
        {
            (void)lexer.Pull(parseTree::Token_t::WHITESPACE);
        }
        else if (lexer.IsMatch(parseTree::Token_t::COMMENT))
        {
            (void)lexer.Pull(parseTree::Token_t::COMMENT);
        }
        else if (lexer.IsMatch(parseTree::Token_t::NAME))
        {
            defFilePtr->sections.push_back(sectionParserFunc(lexer));
        }
        else
        {
            lexer.UnexpectedChar("");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a bundled file or directory item from inside a "bundles:" section's "file" or "dir"
 * subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseBundledItem
(
    Lexer_t& lexer,
    parseTree::Content_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    // Accept an optional set of permissions.
    parseTree::Token_t* permissionsPtr = NULL;
    if (lexer.IsMatch(parseTree::Token_t::FILE_PERMISSIONS))
    {
        permissionsPtr = lexer.Pull(parseTree::Token_t::FILE_PERMISSIONS);
        SkipWhitespaceAndComments(lexer);
    }

    // Expect a build host file system path followed by a target host file system path.
    parseTree::Token_t* buildHostPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    SkipWhitespaceAndComments(lexer);
    parseTree::Token_t* targetPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    // Create a new bundled item.
    parseTree::Token_t* firstPtr = (permissionsPtr != NULL ? permissionsPtr : buildHostPathPtr);
    auto bundledItemPtr = parseTree::CreateTokenList(type, firstPtr);

    // Add its contents.
    if (permissionsPtr != NULL)
    {
        bundledItemPtr->AddContent(permissionsPtr);
    }
    bundledItemPtr->AddContent(buildHostPathPtr);
    bundledItemPtr->AddContent(targetPathPtr);

    return bundledItemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "bundles:" section.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseBundlesSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Expect the subsection name as the first token.
    parseTree::Token_t* nameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    // Figure out which type of content item to parse depending on what subsection it is.
    parseTree::Content_t::Type_t itemType;
    if (nameTokenPtr->text == "file")
    {
        itemType = parseTree::Content_t::BUNDLED_FILE;
    }
    else if (nameTokenPtr->text == "dir")
    {
        itemType = parseTree::Content_t::BUNDLED_DIR;
    }
    else
    {
        lexer.ThrowException("Unexpected subsection name '" + nameTokenPtr->text
                             + "' in 'bundles' section.");
    }

    // Create a closure that knows which type of item should be parsed and how to parse it.
    auto itemParser = [itemType](Lexer_t& lexer)
        {
            return ParseBundledItem(lexer, itemType);
        };

    // Parse the subsection.
    return ParseComplexSection(lexer, nameTokenPtr, itemParser);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a required file or directory item from inside a "requires:" section's "file" or "dir"
 * subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseRequiredFileOrDir
(
    Lexer_t& lexer,
    parseTree::Content_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    // Expect a source file system path followed by a destination file system path.
    parseTree::Token_t* srcPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    SkipWhitespaceAndComments(lexer);
    parseTree::Token_t* destPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    SkipWhitespaceAndComments(lexer);

    // Create a new item.
    auto itemPtr = parseTree::CreateTokenList(type, srcPathPtr);

    // Add its contents.
    itemPtr->AddContent(srcPathPtr);
    itemPtr->AddContent(destPathPtr);

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a single item from inside a "file:" subsection inside a "requires" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseRequiredFile
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    return ParseRequiredFileOrDir(lexer, parseTree::Content_t::REQUIRED_FILE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a single item from inside a "dir:" subsection inside a "requires" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseRequiredDir
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    return ParseRequiredFileOrDir(lexer, parseTree::Content_t::REQUIRED_DIR);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a 'faultAction:' subsection.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseFaultAction
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);

    // Double check that the name contains valid content.
    // TODO: Consider moving this to the lexer and creating a new FAULT_ACTION token.
    const std::string& content = sectionPtr->lastTokenPtr->text;

    if (   (content != "ignore")
        && (content != "restart")
        && (content != "restartApp")
        && (content != "stopApp")
        && (content != "reboot") )
    {
        lexer.ThrowException("Invalid fault action '" + content + "'. Must be one of 'ignore',"
                             " 'restart', 'restartApp', 'stopApp', or 'reboot'.");
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a section containing a scheduling priority.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParsePriority
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);

    // Make sure the priority name is valid.
    // TODO: Consider creating a new THREAD_PRIORITY token for this.
    const std::string& content = sectionPtr->lastTokenPtr->text;

    if (   (content != "idle")
        && (content != "low")
        && (content != "medium")
        && (content != "high") )
    {
        if (   (content[0] == 'r')
            && (content[1] == 't')
            && isdigit(content[2])
            && (   (content[3] == '\0')
                || (   isdigit(content[3])
                    && (content[4] == '\0')
                    )
                )
            )
        {
            auto numberString = content.substr(2);

            std::stringstream numberStream(numberString);

            unsigned int number;

            numberStream >> number;

            if ((number < 1) || (number > 32))
            {
                lexer.ThrowException("Real-time priority level " + content + " out of range."
                                     "  Must be in the range 'rt1' through 'rt32'.");
            }
        }
        else
        {
            lexer.ThrowException("Invalid priority '" + content + "'.");
        }
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a 'watchdogAction:' subsection.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseWatchdogAction
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);

    // Make sure the watchdog action is valid.
    // TODO: Consider creating a WATCHDOG_ACTION token for this.
    const std::string& content = sectionPtr->lastTokenPtr->text;

    if (   (content != "ignore")
        && (content != "restart")
        && (content != "restartApp")
        && (content != "stop")
        && (content != "stopApp")
        && (content != "reboot") )
    {
        lexer.ThrowException("Invalid watchdog action '" + content + "'. Must be one of 'ignore',"
                             " 'restart', 'restartApp', 'stop', 'stopApp', or 'reboot'.");
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a 'watchdogTimeout:' subsection.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseWatchdogTimeout
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: This simple section is different from others that always contain the same type
    //       of token because it can contain either an INTEGER or the NAME "never".

    auto sectionPtr = new parseTree::SimpleSection_t(sectionNameTokenPtr);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Skip over any whitespace or comments.
    SkipWhitespaceAndComments(lexer);

    // Expect the content token next.  It could be the word "never" or an integer (number of ms).
    parseTree::Token_t* tokenPtr;
    if (lexer.IsMatch(parseTree::Token_t::NAME))
    {
        tokenPtr = lexer.Pull(parseTree::Token_t::NAME);

        if (tokenPtr->text != "never")
        {
            lexer.ThrowException("Invalid watchdog timeout value '" + tokenPtr->text + "'. Must be"
                                 " an integer or the word 'never'.");
        }
    }
    else if (lexer.IsMatch(parseTree::Token_t::INTEGER))
    {
        tokenPtr = lexer.Pull(parseTree::Token_t::INTEGER);
    }
    else
    {
        lexer.ThrowException("Invalid watchdog timeout. Must be an integer or the word 'never'.");
    }

    sectionPtr->AddContent(tokenPtr);

    return sectionPtr;
}



} // namespace parser