#include <clang-c/Index.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

struct param
{
    CXType ctype;
    string edl_type;
};

struct function
{
    CXType return_type;
    string name;
    vector<param> params;
};

typedef struct _parse_data
{
    const char* header_name;
    FILE* enclave;
    FILE* host;
    FILE* edl;
    FILE* types;
    char* types_name;
    const char* prefix;
} parse_data;

#define EDL_PREAMBLE                   \
"enclave {\n"                          \
"    include \"%s\"\n"                 \
"    include \"%s\"\n\n"               \
"    trusted {\n"                      \
"        public void init_%s();\n"     \
"    };\n\n"                           \
"    untrusted {\n"
    
#define EDL_EPILOGUE        \
"    };\n"                  \
"};\n"

#define INDENT "    "
#define OCALL_RESULT "oe_%s_result_t"
#define OCALL_FUNC "oe_host_ocall_%s"

const char vars[] = "abcdefghijklmnopqrstuvwxyz";

void gen_enclave(CXCursor c, FILE* file, vector<bool> extra_params)
{
    // Format of function:
    // return_type func_name(arg_type1 a, arg_type2, b, ...)
    // {
    //     OCALL_RESULT retval;
    //     OCALL_FUNC(&retval, a, b, ...);
    //     errno = retval.errno;
    //     return retval.ret;
    // }
    //
    // If OCALL has an extra parameter (i.e FUNC(void* [size=b_] a, size_t b_, size_t* b)),
    // then we call the OCALL like FUNC(&retval, a, *b, b);
    CXType func_type;
    CXString return_type;
    const char* return_type_str;
    CXString func_name;
    const char* func_name_str;
    int num_args;

    func_type = clang_getCursorType(c);

    // First, get the return type.
    return_type = clang_getTypeSpelling(clang_getResultType(func_type));
    return_type_str = clang_getCString(return_type);
    fprintf(file, "%s", return_type_str);

    // Second, get the function name.
    func_name = clang_getCursorSpelling(c);
    func_name_str = clang_getCString(func_name);
    fprintf(file, " %s(", func_name_str);

    // Third, get the arguments.
    num_args = clang_Cursor_getNumArguments(c);
    for (int i = 0; i < num_args; i++)
    {
        CXString arg_type =
            clang_getTypeSpelling(clang_getArgType(func_type, i));

        fprintf(file, "%s %c", clang_getCString(arg_type), vars[i]);
        if (i != num_args - 1)
            fprintf(file, ", ");

        clang_disposeString(arg_type);
    }

    // Finish up the function declaration and start the implementation.
    fprintf(file, ")\n{\n");

    // Implement the function.
    fprintf(
        file,
        INDENT OCALL_RESULT " retval;\n",
        func_name_str);

    fprintf(
        file,
        INDENT OCALL_FUNC "(&retval, ",
        func_name_str);

    for (int i = 0; i < num_args; i++)
    {
        if (extra_params[i])
            fprintf(file, "*%c, ", vars[i]);

        fprintf(file, "%c", vars[i]);
        if (i != num_args - 1)
            fprintf(file, ", ");
    }

    fprintf(file, ");\n");
    fprintf(file, INDENT "errno = retval.errno;\n");
    fprintf(file, INDENT "return retval.ret;\n");

    // Finish the function implementation.
    fprintf(file, "}\n\n");
    clang_disposeString(func_name);
    clang_disposeString(return_type);
}

string filter_type(CXType type)
{
    CXString str = clang_getTypeSpelling(type);
    string cstr = clang_getCString(str);
    clang_disposeString(str);

    if (type.kind != CXType_Pointer)
        return cstr;

    // Remove volatile and restrict qualifiers from type
    if (clang_isRestrictQualifiedType(type))
        cstr.erase(cstr.find("restrict"), sizeof("restrict") - 1);

   if (clang_isVolatileQualifiedType(type))
      cstr.erase(cstr.find("volatile"), sizeof("volatile") - 1);

   return cstr;
}

void gen_host(CXCursor c, FILE* file, vector<bool> extra_params)
{
    // Format for OCALL functions:
    // OCALL_RESULT OCALL_NAME(arg_type1 a, extra_param b_, arg_type2 b, ...)
    // {
    //     OCALL_RESULT result;
    //     result.ret = func_name(a, b, ...);
    //     result.errno = errno;
    //     return result;
    // } 
    CXType func_type;
    CXString func_name;
    const char* func_name_str;
    int num_args;

    func_type = clang_getCursorType(c);
    func_name = clang_getCursorSpelling(c);
    func_name_str = clang_getCString(func_name);

    // Write the function header.
    fprintf(file, OCALL_RESULT " " OCALL_FUNC "(", func_name_str, func_name_str);
    num_args = clang_Cursor_getNumArguments(c);
    for (int i = 0; i < num_args; i++)
    {
        string arg_type = filter_type(clang_getArgType(func_type, i));

        if (extra_params[i])
        {
            CXString arg_type2 = clang_getTypeSpelling(clang_getPointeeType(clang_getArgType(func_type, i)));
            fprintf(file, "%s %c_, ", clang_getCString(arg_type2), vars[i]);
            clang_disposeString(arg_type2);
        }

        fprintf(file, "%s %c", arg_type.c_str(), vars[i]);
        if (i != num_args - 1)
            fprintf(file, ", ");
    }
    fprintf(file, ")\n{\n");

    // Execute the function and put the return code + errno in the struct.
    fprintf(file, INDENT OCALL_RESULT " result;\n", func_name_str);
    fprintf(file, INDENT "result.ret = %s(", func_name_str);
    for (int i = 0; i < num_args; i++)
    {
        fprintf(file, "%c", vars[i]);
        if (i != num_args - 1)
            fprintf(file, ", ");
    }
    fprintf(file, ");\n");
    fprintf(file, INDENT "result.errno = errno;\n");
    fprintf(file, INDENT "return result;\n");

    // Finish the implementation.
    fprintf(file, "}\n\n");
    clang_disposeString(func_name);
}

bool gen_edl_type(CXType c_type, FILE* file, char varname, CXType next)
{
    // For param_types 
    //  - If it's a array (i.e. int[2]), convert to edl syntax
    //  - If it's a value, then it can be directly copied,
    //  - If it's a pointer then:
    //      - char* -> [string]
    //      - Pointer: If the next parameter is an unsigned 
    //        number, assume that is the size parameter. Otherwise,
    //        assume it's a pointer to a single type.
    string typestr;

    if (c_type.kind == CXType_ConstantArray)
    {
        // Array case, we just need to mark it in/out.
        // Convert TYPE[N] -> [in/out] TYPE VAR[N]
        if (clang_isConstQualifiedType(c_type))
            fprintf(file, "[in] ");
        else
            fprintf(file, "[in, out] ");
        
        CXString str = clang_getTypeSpelling(clang_getElementType(c_type));
        typestr = clang_getCString(str);
        clang_disposeString(str);

        fprintf(
            file,
            "%s %c[%lld]",
            typestr.c_str(),
            varname,
            clang_getNumElements(c_type));

        return false;
    }    

    if (c_type.kind != CXType_Pointer)
    {
        // Regular struct / type case
        CXString str = clang_getTypeSpelling(c_type);
        typestr = clang_getCString(str);
        clang_disposeString(str);
        fprintf(file, "%s %c", typestr.c_str(), varname);
        return false;
    }

    // Pointer case. 
    typestr = filter_type(c_type);
    if (clang_isConstQualifiedType(c_type) || clang_isConstQualifiedType(clang_getPointeeType(c_type)))
        fprintf(file, "[in");
    else
        fprintf(file, "[in, out");

    // const char* pointer case (string)
    if ((clang_getPointeeType(c_type).kind == CXType_Char_U ||
        clang_getPointeeType(c_type).kind == CXType_Char_S) &&
        clang_isConstQualifiedType(clang_getPointeeType(c_type)))
    {
        fprintf(file, ", string] %s %c", typestr.c_str(), varname);
        return false;
    }

    // Non string pointer case:
    // If there is no next parameter, assume that it's a single element.
    if (next.kind == CXType_Invalid)
    {
        fprintf(file, "] %s %c", typestr.c_str(), varname);
        return false;
    }

    // If the next type is a pointer, it's a bit more complicated,
    // EDL needs integers for the size, so we need to add an extra
    // parameter for the size.
    CXType canon = clang_getCanonicalType(next);
    enum CXTypeKind t = canon.kind;
    if (canon.kind == CXType_Pointer)
    {
        t = clang_getPointeeType(canon).kind;
    }        

    // For now, don't check for unsigned char and uint128_t...
    if (t != CXType_UShort && t != CXType_UInt && t != CXType_ULong && t != CXType_ULongLong)
    {
        // No size parameter, so we just handle the normal case.
        fprintf(file, "] %s %c", typestr.c_str(), varname);
        return false;
    }
    
    // Size parameter isn't a pointer, so just use it.
    if (canon.kind != CXType_Pointer)
    { 
        fprintf(file, ", size=%c] %s %c", (char) (varname + 1), typestr.c_str(), varname);
        return false;
    }

    // Size parameter is a pointer, so add the extra parameter
    char nextv = (char) (varname + 1);
    CXString nextstr = clang_getTypeSpelling(clang_getPointeeType(next));
    fprintf(file, ", size=%c_] %s %c, %s %c_", nextv, typestr.c_str(), varname, clang_getCString(nextstr), nextv);
    clang_disposeString(nextstr);
    return true;
}

vector<bool> gen_edl(CXCursor c, FILE* file)
{
    // Format for EDL defs:
    // OCALL_RESULT OCALL_FUNC(param_type a, ...);
    CXString func_name; 
    const char* func_name_str;
    CXType func_type;
    int num_args;
    int found_ptr = 0;
    vector<bool> extra_params;

    // Get the function name.
    func_name = clang_getCursorSpelling(c);
    func_name_str = clang_getCString(func_name);
    fprintf(file, INDENT INDENT OCALL_RESULT " " OCALL_FUNC "(", func_name_str, func_name_str);

    // Handle the params now.
    func_type = clang_getCursorType(c);
    num_args = clang_Cursor_getNumArguments(c);

    extra_params.push_back(false);
    for (int i = 0; i < num_args; i++)
    {
        CXType type = clang_getArgType(func_type, i);
        CXType next = { CXType_Invalid, { NULL, NULL }};

        if (i != num_args - 1)
           next = clang_getArgType(func_type, i + 1);
         
        bool extra = gen_edl_type(type, file, vars[i], next);
 
        if (i != num_args - 1)
        {
            fprintf(file, ", ");
            extra_params.push_back(extra);
        }

        found_ptr |= (type.kind == CXType_Pointer);
    }

    if (found_ptr)
        printf("Found pointer type for %s. Please check the edl file.\n", clang_getCString(func_name));

    clang_disposeString(func_name);
    fprintf(file, ");\n\n");

    return extra_params;
}

void gen_types(CXCursor c, FILE* file)
{
    // Format of the result types:
    // typedef struct _OCALL_RESULT
    // {
    //     return_type ret;
    //     int errno;
    // } OCALL_RESULT;

    /* First, get the return type. */
    CXType type = clang_getCursorType(c);
    CXString ret = clang_getTypeSpelling(clang_getResultType(type));
    CXString name = clang_getCursorSpelling(c);

    fprintf(file, "typedef struct _" OCALL_RESULT "\n", clang_getCString(name));
    fprintf(file, "{\n");
    fprintf(file, INDENT "%s ret;\n", clang_getCString(ret));
    fprintf(file, INDENT "int errno;\n");
    fprintf(file, "} " OCALL_RESULT ";\n\n", clang_getCString(name));

    clang_disposeString(ret);
    clang_disposeString(name);
}

enum CXChildVisitResult gen_functions(CXCursor cursor, CXCursor parent, CXClientData data)
{
    parse_data* files = (parse_data*) data;

    // We only care about the main file, so ignore the others
    if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0)
        return CXChildVisit_Continue;

    // We only care about function declarations, so we can generate the
    // enclave, host and edl files. The rest should get added automatically
    // when the oeedger8r includes the given header files.
    
    if (cursor.kind != CXCursor_FunctionDecl)
        return CXChildVisit_Continue;

    // Generate the function signatures. Once we are done, we can just skip
    // to the next node, since we don't care about any other data.
    vector<bool> extra_params = gen_edl(cursor, files->edl);
    gen_types(cursor, files->types);
    gen_host(cursor, files->host, extra_params);
    gen_enclave(cursor, files->enclave, extra_params);
    return CXChildVisit_Continue;
}

int gen_files(CXCursor cursor, parse_data* data)
{

    printf("%s %p %p %p %p %s\n", data->header_name, data->enclave, data->host, data->edl, data->types, data->types_name);


    // Print the preamble for each file before we begin parsing
    fprintf(data->enclave, "#include <%s>\n", data->header_name);
    fprintf(data->enclave, "#include \"%s\"\n\n", data->types_name);
    fprintf(data->host, "#include <%s>\n", data->header_name);
    fprintf(data->host, "#include \"%s\"\n\n", data->types_name);
    fprintf(data->edl, EDL_PREAMBLE, data->header_name, data->types_name, data->prefix);
    fprintf(data->types, "#include <%s>\n", data->header_name);

    // Traverse through the AST to fill out the files
    clang_visitChildren(cursor, gen_functions, data);

    // Print the epilogue after we are done parsing. Only need edl for now.
    fprintf(data->edl, EDL_EPILOGUE);
    return 0;
}

void init_params(char** argv, parse_data* data)
{
    const char* prefix = argv[2];
    size_t size;
    char* tmp = NULL;

    size = strlen(prefix) + 1;
    tmp = (char*) malloc(size + 16);
    memcpy(tmp, prefix, size);

    data->header_name = argv[1];
    printf("%s\n", data->header_name);

    memcpy(tmp + size - 1, "_enclave.c", sizeof("_enclave.c"));
    printf("%s\n", tmp);
    data->enclave = fopen(tmp, "w");

    memcpy(tmp + size - 1, "_host.c", sizeof("_host.c"));
    printf("%s\n", tmp);
    data->host = fopen(tmp, "w");

    memcpy(tmp + size - 1, ".edl", sizeof(".edl"));
    printf("%s\n", tmp);
    data->edl = fopen(tmp, "w");

    memcpy(tmp + size - 1, "_types.h", sizeof("_types.h"));
    printf("%s\n", tmp);
    data->types = fopen(tmp, "w");
    data->types_name = tmp;
    data->prefix = prefix;

    printf("%s %p %p %p %p %s\n", data->header_name, data->enclave, data->host, data->edl, data->types, data->types_name);
}


void free_params(parse_data* data)
{
    fclose(data->enclave);
    fclose(data->host);
    fclose(data->edl);
    fclose(data->types);
    free(data->types_name);
}

int main(int argc, char** argv)
{
    parse_data data;
    CXIndex idx = NULL;
    CXTranslationUnit tu = NULL;
    CXCursor cursor;
    int rc = 1;

    if (argc < 4)
    {
        printf("Usage: ./h2edl "
            "<HEADER_NAME> <PREFIX> [COMPILER_ARGS] <HEADER_PATHi\n");
        return 1;
    }

    // Load params.
    init_params(argv, &data);

    // Create clang index.
    idx = clang_createIndex(0, 1);
    if (!idx)
        goto done;
    
    // Create clang translation unit.
    tu = clang_createTranslationUnitFromSourceFile(
        idx,
        argv[argc - 1],  // Header is last arg.
        argc - 4,        // # of optional compiler args.
        argv + 3,        // Start of optional compiler args.
        0,
        NULL);

    if (!tu)
        goto done;

    // Travel through the translation unit generating the requested files.
    cursor = clang_getTranslationUnitCursor(tu);
    rc = gen_files(cursor, &data);

done:
    if (tu)
        clang_disposeTranslationUnit(tu);
    
    if (idx)
        clang_disposeIndex(idx);

    free_params(&data);

    return rc;
}
