# TP Projekt 4 – Zadanie 4

## Instrukcja Obsługi - Symulator Dźwigu

Program symuluje pracę dźwigu wieżowego. Twoim celem jest podnoszenie i układanie klocków w wieże.

#### Sterowanie Dźwigiem (Klawiatura):

* **Strzałka w lewo:** Przesuwa wózek dźwigu w lewo.
* **Strzałka w prawo:** Przesuwa wózek dźwigu w prawo.
* **Strzałka w górę:** Podnosi hak.
* **Strzałka w dół:** Opuszcza hak.
* **Spacja:**
    * **Naciśnij raz, aby podnieść klocek:** Ustaw hak nad klockiem, który chcesz podnieść i naciśnij spację.
    * **Naciśnij ponownie, aby upuścić klocek:** Przesuń dźwig z klockiem w wybrane miejsce i naciśnij spację, by go postawić.

#### Panel Sterowania (Mysz):

* **Wybierz tryb gry:**
    * Możesz wybrać, którym typem klocków chcesz operować w trybie manualnym (Kwadraty, Trójkąty, Koła). Zmiana trybu resetuje planszę.

* **Maks. udźwig:**
    * W tym polu możesz wpisać maksymalną masę, jaką dźwig może podnieść.
    * Zatwierdź nową wartość klawiszem **Enter**. Próba podniesienia cięższego klocka zakończy się błędem.

* **Automatyczne Budowanie Wieży:**
    * **"Buduj wieżę (KW, T, K)":** Kliknij, aby dźwig automatycznie zbudował wieżę w kolejności: kwadrat, trójkąt, koło.
    * **"Buduj wieżę (K, KW, T)":** Kliknij, aby dźwig automatycznie zbudował wieżę w kolejności: koło, kwadrat, trójkąt.

#### Zasady i Komunikaty o Błędach:

* Możesz podnosić tylko klocki odpowiadające wybranemu trybowi gry.
* Możesz podnosić tylko klocek znajdujący się na samej górze stosu.
* Wieża może mieć maksymalnie 3 klocki wysokości.
* Klocki muszą być układane na klockach tego samego kształtu.
* Wszelkie błędy będą wyświetlane w formie komunikatu w panelu po prawej stronie. Aby go zamknąć i kontynuować, naciśnij dowolny klawisz sterujący.

---

## Ścieżki do plików EXE

-   **x64 EXE**: `/Projekt_4_Vadym_Piotr/x64/Debug/Projekt_4_Vadym_Piotr.exe`
-   **ARM64 EXE**: `/Projekt_4_Vadym_Piotr/ARM64/Debug/Projekt_4_Vadym_Piotr.exe`

---

## Konfiguracja w Visual Studio

1.  **Dodanie biblioteki GDI+ do linkera** Przejdź do: `Project → Properties → Linker → Input → Additional Dependencies`  
    Dodaj: `gdiplus.lib`

2.  **Ustawienie znaków Unicode** (domyślnie powinno być poprawnie)  
    Przejdź do: `Project → Properties → Advanced → Character Set`  
    Ustaw na: `Use Unicode Character Set`
