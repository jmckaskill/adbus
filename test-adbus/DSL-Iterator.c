#if 0
#include "DSL-Iterator.h"



enum TokenType
{
    InvalidToken,
    CommentToken,
    StringToken,
    NumberToken,
    NewlineToken,
};

struct Token
{
    enum TokenType  type;
    uint            negative;
    uint64_t        number;
    str_t        string;
};

static int NextChar(FILE* file, str_t* log)
{
    int ch = getc(file);
    if (0 <= ch && ch < 0x100)
        str_append_char(log, ch);
    return ch;
}

static void PutbackChar(FILE* file, int ch, str_t* log)
{
    ungetc(ch, file);
    str_remove_end(log, 1);
}

static uint8_t tonum(int ch)
{
    if ('0' <= ch && ch <= '9')
        return ch - '0';
    else if ('a' <= ch && ch <= 'f')
        return ch - 'a' + 10;
    else if ('A' <= ch && ch <= 'F')
        return ch - 'A' + 10;
    else
        assert(0);
    return 0;
}



#define GETC() NextChar(file, log)
#define UNGET() PutbackChar(file, ch, log)

// We have 4 types of tokens:
// 1. Newline / EOF
// 2. Strings (word seperated) - only found if number is false
// 3. Quoted strings (between "s) - always looked for
// 4. Numbers in hex (optionally prefixed with '-') - only found if number is true

static void NextToken(FILE* file, struct Token* token, str_t* log, uint number)
{
    token->type = InvalidToken;
    str_clear(&token->string);
    token->number = 0;
    token->negative = 0;
    while (!feof(file))
    {
        int ch = GETC();
        if (feof(file) || ch == '\n') {
            token->type = NewlineToken;
            break;

        } else if (ch == '#') {
            // Comments are to the end of the line and are ignored
            while (!feof(file) && ch != '\n') {
                ch = GETC();
            }
            UNGET();

        } else if (number && ch == '-') {
            token->negative = 1;
            // Continue on - we will hit the number entry next time around

        } else if (number && (isxdigit(ch) || ch == '-')) {
            token->type = NumberToken;
            while (!feof(file) && isxdigit(ch)) {
                token->number = (token->number << 4) | tonum(ch);
                ch = GETC();
            }
            UNGET();
            break;

        } else if (ch == '"') {
            token->type = StringToken;
            ch = GETC(); // skip over '"'
            while (!feof(file) && ch != '"') {
                str_append_char(&token->string, ch);
                ch = GETC();
            }
            break;

        } else if (!number && !isspace(ch)) {
            token->type = StringToken;
            while (!feof(file) && !isspace(ch)) {
                str_append_char(&token->string, ch);
                ch = GETC();
            }
            UNGET();
            break;
        }

        // Ignore everthing else

    }
}

#define ZERO(p) memset(p, 0, sizeof(*p))

static void GetMemoryBlock(FILE* f, VECTOR_T(uint8_t)* data, str_t* log)
{
    uint8_t* dest;
    struct Token tok;
    ZERO(&tok);
    while (NextToken(f, &tok, log, 1), tok.type != NewlineToken) {
        switch (tok.type) {
        case StringToken:
            dest = VECTOR_INSERT_END(data, str_size(&tok.string));
            memcpy(dest, tok.string, str_size(&tok.string));
            break;

        case NumberToken:
            if (tok.number >= 0x100 || tok.negative)
                TestError(log, "Expecting byte");
            dest = VECTOR_INSERT_END(data, 1);
            *dest = tok.number;
            break;

        default:
            TestError(log, "Expecting memory block");
            break;
        }
    }
    str_free(&tok.string);
}

static void ConsumeNewline(FILE* f, str_t* log)
{
    struct Token tok;
    ZERO(&tok);
    while (NextToken(f, &tok, log, 1), tok.type != NewlineToken)
    {}
    str_free(&tok.string);
}

static void GetString(FILE* f, str_t* data, str_t* log)
{
    struct Token tok;
    ZERO(&tok);
    NextToken(f, &tok, log, 0);
    if (tok.type != StringToken)
        TestError(log, "Expecting string");
    str_clear(data);
    str_append(data, tok.string);

    ConsumeNewline(f, log);
    str_free(&tok.string);
}

static void GetINumber(FILE* f, int64_t* num, str_t* log)
{
    struct Token tok;
    ZERO(&tok);
    NextToken(f, &tok, log, 1);
    if (tok.type != NumberToken)
        TestError(log, "Expecting number");
    *num = tok.number;
    if (tok.negative)
        *num *= -1;

    ConsumeNewline(f, log);
    str_free(&tok.string);
}

static void GetUNumber(FILE* f, uint64_t* num, str_t* log)
{
    struct Token tok;
    ZERO(&tok);
    NextToken(f, &tok, log, 1);
    if (tok.type != NumberToken || tok.negative)
        TestError(log, "Expecting positive number");
    *num = tok.number;

    ConsumeNewline(f, log);
    str_free(&tok.string);
}

static void RunIteratorTest(FILE* f, enum ADBusEndianness endianness)
{
    struct ADBusIterator* iter = ADBusCreateIterator();
    ADBusSetIteratorEndianness(iter, endianness);
    str_t log = NULL;
    uint8_t* data = NULL;
    str_t string = NULL;
    str_t signature = NULL;
    uint64_t unum;
    int64_t inum;
    struct Token tok;
    ZERO(&tok);
    while (NextToken(f, &tok, &log, 0), tok.type != NewlineToken) {
        switch (tok.type) {
        case StringToken:
            if (strcasecmp(tok.string, "DATA") == 0) {
                GetMemoryBlock(f, &data, &log);

            } else if (strcasecmp(tok.string, "TEST") == 0) {
                GetString(f, &signature, &log);
                ADBusResetIterator(iter, signature, -1, data, VECTOR_SIZE(&data));

            } else if (strcasecmp(tok.string, "U8") == 0) {
                GetUNumber(f, &unum, &log);
                TestUInt8(iter, unum, &log);

            } else if (strcasecmp(tok.string, "U16") == 0) {
                GetUNumber(f, &unum, &log);
                TestUInt16(iter, unum, &log);

            } else if (strcasecmp(tok.string, "I16") == 0) {
                GetINumber(f, &inum, &log);
                TestInt16(iter, inum, &log);

            } else if (strcasecmp(tok.string, "U32") == 0) {
                GetUNumber(f, &unum, &log);
                TestUInt32(iter, unum, &log);

            } else if (strcasecmp(tok.string, "I32") == 0) {
                GetINumber(f, &inum, &log);
                TestInt32(iter, inum, &log);

            } else if (strcasecmp(tok.string, "U64") == 0) {
                GetUNumber(f, &unum, &log);
                TestUInt64(iter, unum, &log);

            } else if (strcasecmp(tok.string, "I64") == 0) {
                GetINumber(f, &inum, &log);
                TestInt64(iter, inum, &log);

            } else if (strcasecmp(tok.string, "DOUBLE") == 0) {
                GetUNumber(f, &unum, &log);
                TestDouble(iter, *(double*)&unum, &log);

            } else if (strcasecmp(tok.string, "BOOL") == 0) {
                GetUNumber(f, &unum, &log);
                TestBoolean(iter, unum, &log);

            } else if (strcasecmp(tok.string, "STR") == 0) {
                GetString(f, &string, &log);
                TestString(iter, string, &log);

            } else if (strcasecmp(tok.string, "OBJECT_PATH") == 0) {
                GetString(f, &string, &log);
                TestObjectPath(iter, string, &log);

            } else if (strcasecmp(tok.string, "SIGNATURE") == 0) {
                GetString(f, &string, &log);
                TestSignature(iter, string, &log);

            } else if (strcasecmp(tok.string, "VARIANT_BEGIN") == 0) {
                GetString(f, &string, &log);
                TestVariantBegin(iter, string, &log);

            } else if (strcasecmp(tok.string, "VARIANT_END") == 0) {
                ConsumeNewline(f, &log);
                TestVariantEnd(iter, &log);

            } else if (strcasecmp(tok.string, "STRUCT_BEGIN") == 0) {
                ConsumeNewline(f, &log);
                TestStructBegin(iter, &log);

            } else if (strcasecmp(tok.string, "STRUCT_END") == 0) {
                ConsumeNewline(f, &log);
                TestStructEnd(iter, &log);

            } else if (strcasecmp(tok.string, "ARRAY_BEGIN") == 0) {
                ConsumeNewline(f, &log);
                TestArrayBegin(iter, &log);

            } else if (strcasecmp(tok.string, "ARRAY_END") == 0) {
                ConsumeNewline(f, &log);
                TestArrayEnd(iter, &log);

            } else if (strcasecmp(tok.string, "DICT_BEGIN") == 0) {
                ConsumeNewline(f, &log);
                TestDictEntryBegin(iter, &log);

            } else if (strcasecmp(tok.string, "DICT_END") == 0) {
                ConsumeNewline(f, &log);
                TestDictEntryEnd(iter, &log);

            } else if (strcasecmp(tok.string, "ERR") == 0) {
                ConsumeNewline(f, &log);
                TestInvalidData(iter, &log);

            } else if (strcasecmp(tok.string, "END") == 0) {
                ConsumeNewline(f, &log);
                TestEnd(iter, &log);

            } else {
                TestError(&log, "Invalid token");
            }
            break;

        default:
            TestError(&log, "Invalid token");
            break;

        }

    }

    ADBusFreeIterator(iter);
    str_free(&string);
    str_free(&signature);
    str_free(&tok.string);
    VECTOR_FREE(&data);
}

static void TestIterator()
{
    FILE* file = fopen("iterator-little-endian.txt", "r");
    if (file) {
        while (!feof(file)) {
            RunIteratorTest(file, ADBusLittleEndian);
        }
        fclose(file);
    }

    file = fopen("iterator-big-endian.txt", "r");
    if (file) {
        while (!feof(file)) {
            RunIteratorTest(file, ADBusBigEndian);
        }
        fclose(file);
    }
}
#endif
