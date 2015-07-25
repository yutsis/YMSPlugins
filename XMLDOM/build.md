Prerequisites to build this plugin
----------------------------------

* Visual Studio 2010 or later
* Five FAR installations for the supported platforms: 1.75, 2.0/32, 2.0/64, 3.0/32, 3.0/64.
* Cygwin tools: `awk`, `iconv` and `sed`
   (edit buildall.bat to specify Cygwin installation path)
* Environment variables:
  * `VS100COMNTOOLS` (substitute here your version of Visual Studio instead of 100)
  * `FAR1`, `FAR2`, `FAR264`, `FAR3`, `FAR364` pointing to each FAR installation respectively
   
---
Inside Visual Studio, you may build or debug this plugin under any FAR platform except for FAR1.75/x64 which is not supported.

---
Michael Yutsis  
[yutsis@gmail.com](mailto:yutsis@gmail.com)  
http://michael-y.org/FAR  
http://plugring.farmanager.com/plugin.php?pid=536
