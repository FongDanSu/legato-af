//--------------------------------------------------------------------------------------------------
/**
 * @file lexer.h Lexical Analyzer (Lexer) for the mk* tools.
 *
 * The Parsers use the Lexer_t to get lexical tokens from the .Xdef input file.
 *
 * As a side-effect, the Lexer_t builds a list of tokens in a given DefFile_t object.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_LEXER_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_LEXER_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Lexical analyzer.
 */
//--------------------------------------------------------------------------------------------------
class Lexer_t
{
    public:

        // Constructor.  Takes a def file object to be populated as the root of the parse tree.
        Lexer_t(parseTree::DefFile_t* fileObjPtr);

        // Check if the next sequence of text in the file could match a given type of token.
        bool IsMatch(parseTree::Token_t::Type_t type);

        // Pull a token from the file being parsed. Advances past the token in the file.
        parseTree::Token_t* Pull(parseTree::Token_t::Type_t type);

        // Attempt to convert a token one type to another.
        void ConvertToName(parseTree::Token_t* tokenPtr);

        // Attempt to convert a given token to a DOTTED_NAME token.
        void ConvertToDottedName(parseTree::Token_t* tokenPtr, size_t& dotCount);

        // true = print progress messages to the standard output stream.
        bool beVerbose;

        // Throw an exception with the file, line and column at the front.
        void ThrowException(const std::string& message) __attribute__ ((noreturn));
        void UnexpectedChar(const std::string& message) __attribute__ ((noreturn));

        parseTree::DefFile_t* filePtr;  ///< Pointer to the File object for the file being parsed.

    private:

        std::ifstream inputStream;      ///< File input stream from which tokens will be matched.
        char nextChar;                  ///< Next character to match.
        size_t line;                    ///< File line number.
        size_t column;                  ///< Char index on line (treat tab & return same as space).

        bool IsMatchBoolean();
        void PullConstString(parseTree::Token_t* tokenPtr, const char* tokenString);
        void PullWhitespace(parseTree::Token_t* tokenPtr);
        void PullComment(parseTree::Token_t* tokenPtr);
        void PullInteger(parseTree::Token_t* tokenPtr);
        void PullSignedInteger(parseTree::Token_t* tokenPtr);
        void PullBoolean(parseTree::Token_t* tokenPtr);
        void PullFloat(parseTree::Token_t* tokenPtr);
        void PullString(parseTree::Token_t* tokenPtr);
        void PullFilePermissions(parseTree::Token_t* tokenPtr);
        void PullServerIpcOption(parseTree::Token_t* tokenPtr);
        void PullClientIpcOption(parseTree::Token_t* tokenPtr);
        void PullIpcOption(parseTree::Token_t* tokenPtr);
        void PullArg(parseTree::Token_t* tokenPtr);
        void PullFilePath(parseTree::Token_t* tokenPtr);
        void PullFileName(parseTree::Token_t* tokenPtr);
        void PullName(parseTree::Token_t* tokenPtr);
        void PullDottedName(parseTree::Token_t* tokenPtr);
        void PullGroupName(parseTree::Token_t* tokenPtr);
        void PullIpcAgentName(parseTree::Token_t* tokenPtr);
        void PullQuoted(parseTree::Token_t* tokenPtr, char quoteChar);
        void PullEnvVar(parseTree::Token_t* tokenPtr);
        void PullMd5(parseTree::Token_t* tokenPtr);
        size_t Lookahead(char* buffPtr, size_t n);
        void AdvanceOneCharacter(parseTree::Token_t* tokenPtr);
        void AdvanceOneCharacter(std::string& string);
        std::string UnexpectedCharErrorMsg(char unexpectedChar,
                                           size_t lineNum,
                                           size_t columnNum,
                                           const std::string& message);
};


#endif // LEGATO_MKTOOLS_LEXER_H_INCLUDE_GUARD
