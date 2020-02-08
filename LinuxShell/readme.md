# Shelly
## Introducere
Shelly este un shell pentru Linux.

## Funcționalități
Shelly are implementate următoarele funcționalități:
- Parantezare și acceptă argumente în ghilimele
- Suportă operatori logici `&&` și `||`
- Suportă operatorii pipe `|` și redirecționare `>`, `>>`, `<`
- Gestionează transmiterea semnalelor `SIGINT` și `SIGTSTP`
- E ca un tanc

## Structura
Shellul preia inputul de la utilizator utilizând funcțiile din fișierul `console_input.c`.
Pe urmă, stringul introdus este spart în token-uri și este construit un arbore de acțiuni
de executat, cu ajutorul funcțiilor din `input_parser.c`.

Parcurgerea efectivă a arborelui și execuția proceselor are loc în `action.c`.

Shellul suportă așa-zisele meta-acțiuni, anume diferite comenzi:
- `cd` pentru schimbarea directorului curent
- `exit` pentru terminarea execuției
- `help` pentru afișarea unui mic dialog de ajutor
- [`tank`](https://www.youtube.com/watch?v=qZAIfEp0AoQ)

## Compilare
Proiectul prezintă suport pentru CMake, prin care pot fi create fișiere Makefile, Ninja, etc.
A fost utilizat mediul de programare CLion.