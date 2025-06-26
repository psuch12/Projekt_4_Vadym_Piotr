# TP Projekt 4 – Zadanie 4

## Ścieżki do plików EXE

- **x64 EXE**: `/Projekt_4_Vadym_Piotr/x64/Debug/Projekt_4_Vadym_Piotr.exe`  
- **ARM64 EXE**: `/Projekt_4_Vadym_Piotr/ARM64/Debug/Projekt_4_Vadym_Piotr.exe`

---

## Konfiguracja w Visual Studio Code

1. **Dodanie biblioteki GDI+ do linkera**  
   Przejdź do: `Project → Properties → Linker → Input → Additional Dependencies`  
   Dodaj: `gdiplus.lib`

2. **Ustawienie znaków Unicode** (domyślnie powinno być poprawnie)  
   Przejdź do: `Project → Properties → Advanced → Character Set`  
   Ustaw na: `Use Unicode Character Set`
