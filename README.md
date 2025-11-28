![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/rtf-parser)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/rtf-parser/total)

### Dependencies and Licensing

* the source code of this CLI tool is licensed under the MIT license.
* see [librtf](https://librtf.sourceforge.net) for the licensing of **librtf** (LGPL-2.1).
* **note**: [yanxijian/LibRtf](https://github.com/yanxijian/LibRtf) is no longer used. 
 
# rtf-parser
CLI tool to extract text from RTF

```
text extractor for rtf documents

 -i path    : document to parse
 -o path    : text output (default=stdout)
 -          : use stdin for input
 -r         : raw text output (default=json)
```

## JSON

|Property|Level|Type|Description|
|-|-|-|-|
|document|0|||
|document.type|0|Text||
|document.pages|0|Array||
|document.pages[].paragraphs|1|Array||
|document.pages[].paragraphs[].text|2|Text||
