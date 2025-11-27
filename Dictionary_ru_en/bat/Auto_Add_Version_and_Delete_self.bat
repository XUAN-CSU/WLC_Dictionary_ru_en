@echo off
setlocal enabledelayedexpansion

:: ---------------------------------------------------------
:: Always work inside the bat folder (safe for Qt build)
:: ---------------------------------------------------------
pushd "%~dp0"

:: ---------------------------------------------------------
:: Clean unwanted files/folders
:: ---------------------------------------------------------
echo Deleting build cache files...

if exist ".qmake.stash" (
    del ".qmake.stash" /f /q
    echo Deleted .qmake.stash
)

if exist "Makefile" (
    del "Makefile" /f /q
    echo Deleted Makefile
)

if exist "russian_word_history.txt" (
    del "russian_word_history.txt" /f /q
    echo Deleted russian_word_history.txt
)

if exist "word_audio" (
    rmdir /s /q "word_audio"
    echo Deleted folder: word_audio
)

echo Cleanup done.

:: ---------------------------------------------------------
:: Work in the same folder as the bat (Test folder)
:: ---------------------------------------------------------
set "CURRENT_DIR=%~dp0"
pushd "%CURRENT_DIR%"

:: ---------------------------------------------------------
:: Find max version WLC_release_Vx.x.x
:: ---------------------------------------------------------
set max_major=0
set max_minor=0
set max_patch=0

for /d %%D in (Dictionary_RU_EN_release_V*.*.*) do (
    for /f "tokens=2-4 delims=V." %%a in ("%%D") do (
        set major=%%a
        set minor=%%b
        set patch=%%c

        if !major! gtr !max_major! (
            set max_major=!major!
            set max_minor=!minor!
            set max_patch=!patch!
        ) else if !major! equ !max_major! if !minor! gtr !max_minor! (
            set max_minor=!minor!
            set max_patch=!patch!
        ) else if !major! equ !max_major! if !minor! equ !max_minor! if !patch! gtr !max_patch! (
            set max_patch=!patch!
        )
    )
)

:: Next version patch
set /a next_patch=max_patch+1
set "newname=Dictionary_RU_EN_release_V!max_major!.!max_minor!.!next_patch!"

echo Next version: !newname!

:: ---------------------------------------------------------
:: COPY WLC_release → new version folder
:: ---------------------------------------------------------
if not exist "Dictionary_RU_EN_release" (
    echo ERROR: Folder "Dictionary_RU_EN_release" not found!
    goto :end
)

echo Copying Dictionary_RU_EN_release → !newname!
xcopy "Dictionary_RU_EN_release" "!newname!" /E /I /H /K /Y >nul

echo DONE.

:end
popd

:: Self-delete
start "" /b cmd /c del "%~f0"
exit /b
