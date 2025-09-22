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
    //fprintf(stderr, " %c          :\n", 't' , "basic html tags (default=no)");
//    fprintf(stderr, " -%c         : %s\n", 'c' , "ansi codepage (default=1252)");
//    fprintf(stderr, " -%c         : %s\n", 'l' , "use librtf (default=platform)");
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
#define ARGS (OPTARG_T)L"i:o:-rhtcl"
#define _atoi _wtoi
#else
#define HWND char*
#define ARGS "i:o:-rhtcl"
#define _atoi atoi
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

static void utf16_to_utf8(const uint8_t* u16data, size_t u16size, std::string& u8) {

#ifdef __APPLE__
    CFStringRef str = CFStringCreateWithCharacters(kCFAllocatorDefault, (const UniChar*)u16data, u16size);
    if (str) {
        size_t size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8) + sizeof(uint8_t);
        std::vector<uint8_t> buf(size + 1);
        CFIndex len = 0;
        CFStringGetBytes(str, CFRangeMake(0, CFStringGetLength(str)), kCFStringEncodingUTF8, 0, true, (UInt8*)buf.data(), buf.size(), &len);
        u8 = (const char*)buf.data();
        CFRelease(str);
    }
    else {
        u8 = "";
    }
#else
    int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)u16data, u16size, NULL, 0, NULL, NULL);
    if (len) {
        std::vector<uint8_t> buf(len + 1);
        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)u16data, u16size, (LPSTR)buf.data(), buf.size(), NULL, NULL);
        u8 = (const char*)buf.data();
    }
    else {
        u8 = "";
    }
#endif
}
#if WITH_NATIVE_RTF_CONVERT
static void rtf_to_text_platform(HWND hwnd, std::string& rtf, std::string& text) {
#ifdef _WIN32
    
    SETTEXTEX st = {};
    st.flags = ST_DEFAULT;
    st.codepage = CP_ACP;
    SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)rtf.c_str());
    
    int len = GetWindowTextLength(hwnd);
    //int len = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
    
    std::vector<uint16_t> buf((len + 1));
    GetWindowText(hwnd,(LPWSTR)buf.data(), buf.size());
    
    utf16_to_utf8((const uint8_t *)buf.data(), buf.size(), text);
    /*
     GETTEXTEX gt = {};
     gt.cb = buf.size();
     gt.flags = GT_USECRLF;
     gt.codepage = CP_UTF8;
     SendMessage(hwnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buf.data());
     */
    
    // text = (const char *)buf.data();
#else
    
    NSData *src = [[NSData alloc]initWithBytes:rtf.c_str() length:rtf.length()];
    if(src) {
        NSError *error = nil;
        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithData:src
                                                                       options:@{NSDocumentTypeDocumentOption: NSRTFTextDocumentType}
                                                            documentAttributes:nil
                                                                         error:&error];
        if (!error) {
            NSString *u8 = [attrStr string];
            text = (const char *)[u8 UTF8String];
        }
    }
#endif
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
    bool usePlatform = true;
    
    int codepage = 1252;
    
#if WITH_NATIVE_RTF_CONVERT
    HWND hwnd = NULL;

#ifdef _WIN32

    //HMODULE hmodule = LoadLibrary(L"msftedit.dll");
    HMODULE hmodule = LoadLibrary(L"Riched20.dll");

    if (hmodule)
    {
        /*
        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_WIN95_CLASSES;
        InitCommonControlsEx(&icex);
        */
       
        hwnd = CreateWindowExW(
            0, L"RichEdit20W", nullptr,
            WS_CHILD | ES_MULTILINE,
            0, 0, 0, 0,
            HWND_MESSAGE, // message-only parent works with RichEdit 2.0
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);
        /*
        if (hwnd)
        {
            DWORD err = GetLastError();
            std::cerr << "CreateWindowEx" << err << std::endl;
        }
        */
    }
#endif
#endif
    
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
                std::vector<uint8_t> buf(BUFLEN);
                size_t n;
                
                while ((n = fread(buf.data(), 1, buf.size(), stdin)) > 0) {
                    rtf_data.insert(rtf_data.end(), buf.begin(), buf.begin() + n);
                }
            }
                break;
            case 'r':
                rawText = true;
                break;
            case 't':
                basicTags = true;
                break;
            case 'l':
                usePlatform = false;
                break;
            case 'c':
                codepage = _atoi(optarg);
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
    
    if(usePlatform){
        rtf_to_text_platform(hwnd, instring, outstring);
    }else{
        if(basicTags){
            RtfReader::RtfString2HtmlString(outstring, instring);
        }else{
            RtfReader::RtfString2TextString(outstring, instring);
        }
    }
    
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
    
#if WITH_NATIVE_RTF_CONVERT
#if defined(_WIN32)
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
    if (hmodule) {
        FreeLibrary(hmodule);
        hmodule = NULL;
    }
#endif
#endif
    
    return 0;
}
