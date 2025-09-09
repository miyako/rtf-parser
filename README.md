![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/rtf-parser)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/rtf-parser/total)

### Dependencies and Licensing

* the source code of this CLI tool is licensed under the MIT license.
* see [librtf](https://librtf.sourceforge.net) for the licensing of **librtf** (LGPL-2.1).
 
# rtf-parser
CLI tool to extract text from RTF

**acknowledgements**: https://github.com/yanxijian/LibRtf

## usage

```
rtf-parser -i example.rtf -o example.json

 -i path    : document to parse
 -o path    : text output (default=stdout)
 -          : use stdin for input
 -r         : raw text output (default=json)
 -t         : basic html tags (default=no)
```

## output (JSON)

```
{
    "type: "rtf",
    "text": "body"
}
```
