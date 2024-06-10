
Malloc Scaled for UE5.4

This is my version malloc for UE5.4


If you wish to launch Unreal Engine using my memory allocator You need to copy files in current directory into your UE5.4 source engine directories:

1. Copy header files into ..Engine\Source\Runtime\Core\Public\HAL
2. Copy cpp files into ..Engine\Source\Runtime\Core\Private\HAL
3. Copy UE5 file "WindowsPlatformMemory.cpp" into directory ..Engine\Source\Runtime\Core\Private\Windows. Click replace.
4. Open UE5 solution in Visual Studio and add this files to UE5 project. 
5. Run Build UE5