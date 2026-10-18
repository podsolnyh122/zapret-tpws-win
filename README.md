# tpws for Windows (Zapret Port)

Нативный порт утилиты **tpws** (Transparent / SOCKS5 Proxy) из состава проекта [zapret (bol-van)](https://github.com/bol-van/zapret) для операционных систем Windows (x86_64).

В отличие от `winws.exe`, данный бинарник работает полностью в пространстве пользователя (User Space) через протокол SOCKS5, **не требует прав администратора, не устанавливает драйверы ядра (WinDivert)** и не вызывает конфликтов с античитами (Vanguard, Easy Anti-Cheat и др.).

---

##  Быстрый старт (Готовый бинарник)

Если вы скачали готовый архив из раздела [Releases](https://github.com/podsolnyh122/zapret-tpws-win/releases):

1. Распакуйте архив в любую удобную папку.
2. Убедитесь, что рядом с `tpws.exe` находятся библиотеки `cygwin1.dll` и `cygz.dll`.
3. Запустите прокси (например, через командную строку или `.bat`-файл):
   ```cmd
   tpws.exe --socks --bind-addr=127.0.0.1 --port=1080 --split-pos=sniext+4
