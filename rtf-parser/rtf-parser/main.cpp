//
//  main.cpp
//  rtf-parser
//
//  Created by miyako on 2025/09/09.
//

#include "rtf-parser.h"

static void usage(void)
{
    fprintf(stderr, "Usage:  rtf-parser -r -i in -o out -\n\n");
    fprintf(stderr, "text extractor for rtf documents\n\n");
    fprintf(stderr, " -%c path    : %s\n", 'i' , "document to parse");
    fprintf(stderr, " -%c path    : %s\n", 'o' , "text output (default=stdout)");
    fprintf(stderr, " %c          :\n", '-' , "use stdin for input");
    fprintf(stderr, " -%c         : %s\n", 'r' , "raw text output (default=json)");
    fprintf(stderr, " %c          :\n", 't' , "basic html tags (default=no)");
    exit(1);
}

extern OPTARG_T optarg;
extern int optind, opterr, optopt;

#ifdef WIN32
OPTARG_T optarg = 0;
int opterr = 1;
int optind = 1;
int optopt = 0;
int getopt(int argc, OPTARG_T *argv, OPTARG_T opts) {

    static int sp = 1;
    register int c;
    register OPTARG_T cp;
    
    if(sp == 1)
        if(optind >= argc ||
             argv[optind][0] != '-' || argv[optind][1] == '\0')
            return(EOF);
        else if(wcscmp(argv[optind], L"--") == NULL) {
            optind++;
            return(EOF);
        }
    optopt = c = argv[optind][sp];
    if(c == ':' || (cp=wcschr(opts, c)) == NULL) {
        ERR(L": illegal option -- ", c);
        if(argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return('?');
    }
    if(*++cp == ':') {
        if(argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            ERR(L": option requires an argument -- ", c);
            sp = 1;
            return('?');
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if(argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return(c);
}
#define ARGS (OPTARG_T)L"i:o:-rht"
#else
#define ARGS "i:o:-rht"
#endif

struct Document {
    std::string type;
    std::string text;
};

static void document_to_json(Document& document, std::string& text, bool rawText) {
    
    if(rawText){
        text = document.text;
    }else{
        Json::Value documentNode(Json::objectValue);
        documentNode["type"] = document.type;
        documentNode["text"] = document.text;
        
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        text = Json::writeString(writer, documentNode);
    }
}

#ifdef _WIN32
static std::string wchar_to_utf8(const wchar_t* wstr) {
    if (!wstr) return std::string();

    // Get required buffer size in bytes
    int size_needed = WideCharToMultiByte(
        CP_UTF8,            // convert to UTF-8
        0,                  // default flags
        wstr,               // source wide string
        -1,                 // null-terminated
        nullptr, 0,         // no output buffer yet
        nullptr, nullptr
    );

    if (size_needed <= 0) return std::string();

    // Allocate buffer
    std::string utf8str(size_needed, 0);

    // Perform conversion
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr,
        -1,
        &utf8str[0],
        size_needed,
        nullptr,
        nullptr
    );

    // Remove the extra null terminator added by WideCharToMultiByte
    if (!utf8str.empty() && utf8str.back() == '\0') {
        utf8str.pop_back();
    }

    return utf8str;
}
#endif

int main(int argc, OPTARG_T argv[]) {
        
    const OPTARG_T input_path  = NULL;
    const OPTARG_T output_path = NULL;
    
    std::vector<unsigned char>rtf_data(0);

    int ch;
    std::string text;
    bool rawText = false;
    OPTARG_T password = NULL;
    bool basicTags = false;
    
    while ((ch = getopt(argc, argv, ARGS)) != -1){
        switch (ch){
            case 'i':
                input_path  = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case '-':
            {
                _fseek(stdin, 0, SEEK_END);
                size_t len = (size_t)_ftell(stdin);
                _fseek(stdin, 0, SEEK_SET);
                rtf_data.resize(len);
                fread(rtf_data.data(), 1, rtf_data.size(), stdin);
            }
                break;
            case 'r':
                rawText = true;
                break;
            case 't':
                basicTags = true;
                break;
            case 'h':
            default:
                usage();
                break;
        }
    }
        
    if((!rtf_data.size()) && (input_path != NULL)) {
        FILE *f = _fopen(input_path, _rb);
        if(f) {
            _fseek(f, 0, SEEK_END);
            size_t len = (size_t)_ftell(f);
            _fseek(f, 0, SEEK_SET);
            rtf_data.resize(len);
            fread(rtf_data.data(), 1, rtf_data.size(), f);
            fclose(f);
        }
    }
    
    if(!rtf_data.size()) {
        usage();
    }
    
    std::string instring((const char *)rtf_data.data(), rtf_data.size());
    std::string outstring;
    
//    std::streambuf* old_buf = std::cerr.rdbuf();
//    std::ostringstream dummy;
//    std::cerr.rdbuf(dummy.rdbuf());
    
    if(basicTags){
        RtfReader::RtfString2HtmlString(outstring, instring);
    }else{
        RtfReader::RtfString2TextString(outstring, instring);
    }
    
//    std::cerr.rdbuf(old_buf);

    Document document;
    document.type = "rtf";
    document.text = outstring;
    
    document_to_json(document, text, rawText);
    
    if(!output_path) {
        std::cout << text << std::endl;
    }else{
        FILE *f = _fopen(output_path, _wb);
        if(f) {
            fwrite(text.c_str(), 1, text.length(), f);
            fclose(f);
        }
    }

    end:
    
    return 0;
}
