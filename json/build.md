Prerequisites to build this plugin
----------------------------------

* Visual Studio 2010 or later
* Five FAR installations for the supported platforms: 1.75, 2.0/32, 2.0/64, 3.0/32, 3.0/64.
* Cygwin tools: `awk`, `iconv` and `sed`
   (edit buildall.bat to specify Cygwin installation path)
* Environment variables:
  * `VS100COMNTOOLS` (substitute here your version of Visual Studio instead of 100)
  * `FAR1`, `FAR2`, `FAR264`, `FAR3`, `FAR364` pointing to each FAR installation respectively
* RapidJSON's [include/rapidjson](https://github.com/miloyip/rapidjson/tree/master/include/rapidjson) directory which should be
located under `rapidjson/include/rapidjson` subdir.
* Optionally, `lng.generator.exe` from FAR build tools. It's supposed to be in the directory 
      `..\..\..\FAR\fardev\unicode_far\tools\` relatively to this project directory.  
   If it's not found, the language file `lang.templ` is not compiled but language files are still present and used in the build.
   
---
Inside Visual Studio, you may build or debug this plugin under any FAR platform except for FAR1.75/x64 which is not supported.

---
Michael Yutsis  
[yutsis@gmail.com](mailto:yutsis@gmail.com)  
[http://michael-y.org/FAR](http://michael-y.org/FAR)  
