# godeye---Basic-Remote-Access-Trojan

- REMEMBER TO CHANGE HOST_ID OF YOUR HOST MACHINE AND PORT YOU WANT TO CONNECT.
  
- TO COMPILE FILE CLIENT.CPP, PLEASE USE THIS COMMAND:

```
g++ .\client.cpp -o client.exe -lws2_32 -lgdiplus -mwindows -static-libgcc -static-libstdc++
```

- TO RUN FILE SERVER.PY, PLEASE USE THIS COMMAND:

```
python3 .\server.py
```

#                   ADD STATIC LIBRARY VISUAL STUDIO 2022
- Add path of .h file to [Right click to your project -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories]. Example "C:\Program Files (x86)\Windows Kits\10\Include\10.0.20348.0\um"
- Add path of .lib file to [Right click to your project -> Properties -> Configuration Properties -> Linker -> General -> Additional Library Directories]. Example "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.20348.0\um\x86"
- Add your .lib file you added in code to [Right click to your project -> Properties -> Configuration Properties -> Linker -> Input -> Additional Dependencies]. Example you import gdilplus.lib and urlmon.lib, edit Additional Dependencies to gdiplus.lib;urlmon.lib
- In Linker -> General -> Link Library Dependencies, turn No to Yes

- If you want to compile code to file .exe and .pdb:
	+ In C/C++ -> General -> Debug Information, choose Program Database (/Zi)
	+ In C/C++ -> Output Files -> Program Databse File Name, edit to name you want (eg. client.pdb, godeye.pdb)
