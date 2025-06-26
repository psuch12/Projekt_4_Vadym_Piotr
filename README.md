x64 EXE: /Projekt_4_Vadym_Piotr/x64/Debug/Projekt_4_Vadym_Piotr.exe
ARM64 EXE: /Projekt_4_Vadym_Piotr/ARM64/Debug/Projekt_4_Vadym_Piotr.exe

Setup w Visual Studio Code:
Dodanie biblioteki do Linkera: Project -> Properties -> Linker -> Input -> Additional Dependencies, dodaj gdiplus.lib.
Ustawienie Unicode (domyslnie powinno byc poprawnie): Project -> Properties -> Advanced -> Character Set, wybierz Use Unicode Character Set
